#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
#
# Captures CTF trace data from a serial port (UART or USB Serial JTAG), or parses it with babeltrace2.
# The trace directory must contain the metadata file and the trace data file.
#   python uart_data_capture.py -c -p /dev/tty.usbserial-210 -o trace_data_dir/trace.dat
#   python uart_data_capture.py -i trace_data_dir [-l 50]
#   python uart_data_capture.py -i trace_data_dir --perfetto [-o perfetto_trace.json]

import argparse
import json
import sys

import bt2
import serial


def parse_args() -> argparse.Namespace:
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description='Capture UART data and save to file, or parse existing trace data',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument('-c', '--capture', action='store_true', help='Capture mode: capture data from UART')
    mode_group.add_argument(
        '-i',
        '--input',
        help='Parse mode: parse existing trace data file. \
                           Directory containing trace data and metadata files is required.',
    )

    parser.add_argument(
        '-b', '--baudrate', type=int, default=1000000, help='Serial baudrate (default: 1000000, ignored by USB Serial JTAG)'
    )
    parser.add_argument('-p', '--port', help='Serial port (e.g., /dev/ttyUSB0, COM3) - required for capture mode')
    parser.add_argument('-o', '--output', help='Output file path - required for capture mode, optional for parse mode')

    parser.add_argument('-l', '--limit', type=int, default=10, help='Limit number of events to display (default: 10)')
    parser.add_argument('--perfetto', action='store_true', help='Export to Perfetto Trace Event Format (JSON)')

    return parser.parse_args()


def parse_trace_data(input_path: str, limit: int = 10) -> None:
    """Parse existing trace data using babeltrace2"""
    print(f'Parsing trace data from: {input_path}')

    try:
        trace_collection = bt2.TraceCollectionMessageIterator(input_path)

        event_count = 0
        for msg in trace_collection:
            if isinstance(msg, bt2._EventMessageConst):
                event_count += 1
                event = msg.event

                print(f'Event {event_count}:')
                print(f'  Name: {event.name}')
                print(f'  Event ID: 0x{event.id:02X}')
                print(f'  Timestamp: {msg.default_clock_snapshot.ns_from_origin}')

                if hasattr(event, 'common_context_field') and event.common_context_field:
                    print('  Context:')
                    for field_name, field_value in event.common_context_field.items():
                        print(f'    {field_name}: {field_value}')

                if hasattr(event, 'payload_field') and event.payload_field:
                    print('  Payload:')
                    for field_name, field_value in event.payload_field.items():
                        print(f'    {field_name}: {field_value}')

                print()

                if event_count >= limit:
                    print(f'... (showing first {limit} events, total events may be more)')
                    break

        print(f'Parsed {event_count} events from trace data')

    except bt2._Error as e:
        print(f'Babeltrace2 error: {e}')
        sys.exit(1)
    except Exception as e:
        print(f'Error parsing trace data: {e}')
        sys.exit(1)


def export_to_perfetto(input_path: str, output_filename: str) -> None:
    """Export ESP-IDF CTF trace data to Perfetto Trace Event Format (JSON)"""
    print(f'Exporting CTF trace data from: {input_path}')
    print(f'Output file: {output_filename}')

    try:
        trace_collection = bt2.TraceCollectionMessageIterator(input_path)

        trace_events = []
        named_threads = set()
        event_count = 0
        task_names: dict[int, str] = {}
        isr_stack: dict[int, list[str]] = {}
        running_task: dict[int, str] = {}

        for msg in trace_collection:
            if isinstance(msg, bt2._EventMessageConst):
                event_count += 1
                event = msg.event

                core_id = 0
                if hasattr(event, 'common_context_field') and event.common_context_field:
                    core_id_raw = event.common_context_field.get('core_id')
                    if core_id_raw is not None:
                        core_id = int(core_id_raw)

                timestamp_us = msg.default_clock_snapshot.ns_from_origin / 1000.0

                thread_name = f'Core_{core_id}'
                thread_id = core_id + 1

                if thread_name not in named_threads:
                    named_threads.add(thread_name)
                    trace_events.append(
                        {
                            'args': {'name': thread_name},
                            'cat': '__metadata',
                            'name': 'thread_name',
                            'ph': 'M',
                            'pid': thread_id,
                            'tid': thread_id,
                            'ts': 0,
                        }
                    )

                event_name = event.name
                event_args: dict[str, str | int] = {}

                if hasattr(event, 'payload_field') and event.payload_field:
                    for key, value in event.payload_field.items():
                        try:
                            event_args[key] = int(value)
                        except (ValueError, TypeError):
                            event_args[key] = str(value)

                event_args['core_id'] = core_id

                def add_event(ph: str, name: str) -> None:
                    trace_events.append(
                        {
                            'ts': timestamp_us,
                            'pid': thread_id,
                            'tid': thread_id,
                            'ph': ph,
                            'name': name,
                            'cat': 'freertos',
                            'args': event_args,
                        }
                    )

                if event_name == 'task_create':
                    task_names[event_args['pxTCB']] = str(event_args['name'])

                # Show ISRs and running tasks as nested slices per core
                if event_name == 'isr_enter':
                    name = f'ISR_{event_args["isr_number"]}'
                    isr_stack.setdefault(core_id, []).append(name)
                    add_event('B', name)
                elif event_name in ('isr_exit', 'isr_exit_to_scheduler'):
                    if isr_stack.get(core_id):
                        add_event('E', isr_stack[core_id].pop())
                elif event_name in ('task_switched_in', 'idle'):
                    if core_id in running_task:
                        add_event('E', running_task[core_id])
                    if event_name == 'idle':
                        name = 'IDLE'
                    else:
                        tcb = event_args['pxTCB']
                        name = task_names.get(tcb, f'Task_0x{tcb:08x}')
                    running_task[core_id] = name
                    add_event('B', name)
                else:
                    add_event('i', event_name)

        output = {'traceEvents': trace_events}
        with open(output_filename, 'w') as f:
            json.dump(output, f, indent=2)

        print(f'Exported {len(trace_events)} events to {output_filename}')
        print(f'Total events processed: {event_count}')
        print(f'Named threads: {len(named_threads)}')

    except bt2._Error as e:
        print(f'Babeltrace2 error: {e}')
        sys.exit(1)
    except Exception as e:
        print(f'Error exporting to Perfetto: {e}')
        sys.exit(1)


def main() -> None:
    """Main function to capture UART data or parse existing trace data"""
    args = parse_args()

    if args.input:
        if args.perfetto:
            output_file = args.output if args.output else 'perfetto_trace.json'
            export_to_perfetto(args.input, output_file)
        else:
            parse_trace_data(args.input, args.limit)
        return

    if not args.capture:
        print('Error: Must specify either -c/--capture or -i/--input')
        sys.exit(1)

    if not args.port:
        print('Error: -p/--port is required for capture mode')
        sys.exit(1)

    if not args.output:
        print('Error: -o/--output is required for capture mode')
        sys.exit(1)

    serial_port = args.port
    serial_baudrate = args.baudrate
    output_file = args.output

    print(f'Opening serial port: {serial_port} at {serial_baudrate} baud')
    print(f'Output file: {output_file}')

    try:
        ser = serial.Serial(serial_port, serial_baudrate, timeout=1)
        print('Serial port opened successfully')

        with open(output_file, 'wb') as file_desc:
            print('Started capturing data... Press Ctrl+C to stop')

            total_data = 0
            while True:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    file_desc.write(data)
                    file_desc.flush()

                    total_data += len(data)
                    print(f'Captured {total_data} bytes', end='\r')

    except serial.SerialException as e:
        print(f'Serial port error: {e}')
        sys.exit(1)
    except FileNotFoundError as e:
        print(f'File error: {e}')
        sys.exit(1)
    except PermissionError as e:
        print(f'Permission error: {e}')
        sys.exit(1)
    except KeyboardInterrupt:
        print(f'\nData capture interrupted. Data saved to: {output_file}')
        sys.exit(0)
    except Exception as e:
        print(f'Unexpected error: {e}')
        sys.exit(1)
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print('Serial port closed')


if __name__ == '__main__':
    main()
