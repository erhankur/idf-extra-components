#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
#
# UART Data Capture and Trace Parser Tool
# Captures data from UART and saves it to a file, or parses existing trace data
# To parse the existing trace data, you need to have the babeltrace2 tool installed
# and the trace data directory must contain the metadata file and the trace data file
# Usage:
#   Capture: python uart_data_capture.py -c -p /dev/tty.usbserial-210 -b 1000000 -o trace_data_dir/trace.dat
#   Parse:   python uart_data_capture.py -i trace_data_dir [-l 50]
#   Perfetto: python uart_data_capture.py -i trace_data_dir --perfetto [-o perfetto_trace.json]

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

    # Create mutually exclusive group for capture vs parse modes
    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument('-c', '--capture', action='store_true', help='Capture mode: capture data from UART')
    mode_group.add_argument(
        '-i',
        '--input',
        help='Parse mode: parse existing trace data file. \
                           Directory containing trace data and metadata files is required.',
    )

    # Arguments for capture mode
    parser.add_argument('-b', '--baudrate', type=int, help='Serial baudrate (e.g., 115200) - required for capture mode')
    parser.add_argument('-p', '--port', help='Serial port (e.g., /dev/ttyUSB0, COM3) - required for capture mode')
    parser.add_argument('-o', '--output', help='Output file path - required for capture mode, optional for parse mode')

    # Arguments for parse mode
    parser.add_argument('-l', '--limit', type=int, default=10, help='Limit number of events to display (default: 10)')
    parser.add_argument('--perfetto', action='store_true', help='Export to Perfetto Trace Event Format (JSON)')

    return parser.parse_args()


def parse_trace_data(input_path: str, limit: int = 10) -> None:
    """Parse existing trace data using babeltrace2"""
    print(f'Parsing trace data from: {input_path}')

    try:
        # Create a trace collection from the input path (can be file or directory)
        trace_collection = bt2.TraceCollectionMessageIterator(input_path)

        event_count = 0
        for msg in trace_collection:
            if isinstance(msg, bt2._EventMessageConst):
                event_count += 1
                event = msg.event

                # Print event information
                print(f'Event {event_count}:')
                print(f'  Name: {event.name}')
                print(f'  Event ID: 0x{event.id:02X}')
                print(f'  Timestamp: {msg.default_clock_snapshot.ns_from_origin}')

                # Print event context (core_id)
                if hasattr(event, 'common_context_field') and event.common_context_field:
                    print('  Context:')
                    for field_name, field_value in event.common_context_field.items():
                        print(f'    {field_name}: {field_value}')

                # Print event payload if available
                if hasattr(event, 'payload_field') and event.payload_field:
                    print('  Payload:')
                    for field_name, field_value in event.payload_field.items():
                        print(f'    {field_name}: {field_value}')

                print()  # Empty line for readability

                # Limit output for large traces
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
        # Create a trace collection from the input path
        trace_collection = bt2.TraceCollectionMessageIterator(input_path)

        trace_events = []
        named_threads = set()
        event_count = 0

        for msg in trace_collection:
            if isinstance(msg, bt2._EventMessageConst):
                event_count += 1
                event = msg.event

                # Get core_id from event context
                core_id = 0  # Default to core 0
                if hasattr(event, 'common_context_field') and event.common_context_field:
                    core_id_raw = event.common_context_field.get('core_id')
                    if core_id_raw is not None:
                        core_id = int(core_id_raw)

                # Get timestamp from event header
                timestamp = msg.default_clock_snapshot.ns_from_origin
                if hasattr(event, 'header_field') and event.header_field:
                    if 'timestamp' in event.header_field:
                        timestamp = int(event.header_field['timestamp'])

                # Convert timestamp to microseconds for Perfetto
                timestamp_us = timestamp / 1000.0

                # Create thread name based on core
                thread_name = f'Core_{core_id}'
                thread_id = core_id + 1  # Use core_id + 1 as thread ID

                # Name thread if not already named
                if thread_name not in named_threads:
                    named_threads.add(thread_name)
                    trace_event = {
                        'args': {'name': thread_name},
                        'cat': '__metadata',
                        'name': 'thread_name',
                        'ph': 'M',
                        'pid': thread_id,
                        'tid': thread_id,
                        'ts': 0,
                    }
                    trace_events.append(trace_event)

                # Map ESP-IDF events to Perfetto events
                event_name = event.name
                event_args: dict[str, str | int] = {}

                # Add payload fields as arguments
                if hasattr(event, 'payload_field') and event.payload_field:
                    for key, value in event.payload_field.items():
                        try:
                            if hasattr(value, '__int__'):
                                event_args[key] = int(value)
                            elif hasattr(value, '__str__'):
                                event_args[key] = str(value)
                            else:
                                event_args[key] = str(value)
                        except (ValueError, TypeError):
                            event_args[key] = str(value)

                # Add core_id to arguments
                event_args['core_id'] = core_id

                # Create Perfetto trace event
                trace_event = {
                    'ts': timestamp_us,
                    'pid': thread_id,
                    'tid': thread_id,
                    'ph': 'i',  # Instant event
                    'name': event_name,
                    'cat': 'freertos',
                    'args': event_args,
                }
                trace_events.append(trace_event)

                # Handle special events that could be converted to duration events
                if event_name in ['isr_enter', 'isr_exit']:
                    # Convert ISR enter/exit to duration events
                    if event_name == 'isr_enter':
                        trace_event['ph'] = 'B'  # Begin
                        trace_event['name'] = f'ISR_{event_args.get("isr_number", "unknown")}'
                    else:
                        trace_event['ph'] = 'E'  # End
                        trace_event['name'] = f'ISR_{event_args.get("isr_number", "unknown")}'

                elif event_name in ['task_switched_in', 'task_switched_out']:
                    # Convert task switching to duration events
                    if event_name == 'task_switched_in':
                        trace_event['ph'] = 'B'  # Begin
                        trace_event['name'] = 'Task_Switch'
                    else:
                        trace_event['ph'] = 'E'  # End
                        trace_event['name'] = 'Task_Switch'

                elif event_name in ['queue_send', 'queue_receive']:
                    # Convert queue operations to duration events
                    trace_event['ph'] = 'B'  # Begin
                    trace_event['name'] = f'Queue_{event_name.split("_")[1].title()}'

                elif event_name in ['vTaskDelay', 'task_delay_until']:
                    # Convert delay events to duration events
                    trace_event['ph'] = 'B'  # Begin
                    trace_event['name'] = 'Task_Delay'

        # Write to JSON file
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

    # Check if we're in parse mode
    if args.input:
        if args.perfetto:
            output_file = args.output if args.output else 'perfetto_trace.json'
            export_to_perfetto(args.input, output_file)
        else:
            parse_trace_data(args.input, args.limit)
        return

    # Capture mode - validate required arguments
    if not args.capture:
        print('Error: Must specify either -c/--capture or -i/--input')
        sys.exit(1)

    if not args.port:
        print('Error: -p/--port is required for capture mode')
        sys.exit(1)

    if not args.baudrate:
        print('Error: -b/--baudrate is required for capture mode')
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
        # Open serial connection
        ser = serial.Serial(serial_port, serial_baudrate, timeout=1)
        print('Serial port opened successfully')

        # Open output file for writing
        with open(output_file, 'wb') as file_desc:
            print('Started capturing data... Press Ctrl+C to stop')

            total_data = 0
            while True:
                # Check if data is available
                if ser.in_waiting > 0:
                    # Read available data
                    data = ser.read(ser.in_waiting)
                    file_desc.write(data)
                    file_desc.flush()  # Ensure data is written immediately

                    # Print progress indicator
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
        # Ensure serial port is closed
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print('Serial port closed')


if __name__ == '__main__':
    main()
