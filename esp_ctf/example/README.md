# ESP-CTF Example

This example demonstrates how to use the ESP-CTF (Common Trace Format) component to capture and analyze traces from ESP32 devices.

## Overview

The ESP-CTF component provides Common Trace Format (CTF) support for ESP-IDF, allowing you to capture runtime traces and analyze them using standard CTF tools like babeltrace2 and trace-compass

## Features

- **CTF Trace Capture**: Capture runtime traces (FreeRTOS events) from ESP32 devices via UART
- **Multiple Analysis Tools**: Parse traces using babeltrace2 or export to Perfetto format (not fully ready yet)
- **Real-time Monitoring**: Monitor system behavior and performance in real-time
- **Standard CTF Format**: Compatible with industry-standard trace analysis tools

## Prerequisites

### Hardware
- Espressif development board

### Software
- ESP-IDF v6.0 or later
- Required python packages:
  ```bash
  pip install babeltrace2 pyserial
  ```

### Optional Tools
- **babeltrace2**: For advanced trace analysis
- **Perfetto**: For web-based trace visualization
- **trace-compass**: For comprehensive trace analysis with graphical UI 
cd ex
## Quick Start

### 1. Build and Flash

```bash
# Configure the project
idf.py set-target esp32

# Build the project
idf.py build

# Flash to device
idf.py flash
```

### 2. Capture Traces

The example is configured to output traces via UART. You can capture these traces using the provided tool:

```bash
cd example
mkdir trace_data
# Capture traces from UART and save to file
python ../tools/uart_data_capture.py -c -p /dev/ttyUSB0 -b 1000000 -o trace_data/trace.dat
```

**Note**: Replace `/dev/ttyUSB0` with your actual serial port:
- Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`
- macOS: `/dev/tty.usbserial-*`, `/dev/tty.usbmodem*`
- Windows: `COM3`, `COM4`, etc.

### 3. Parse and Analyze Traces

```bash
# Copy metadata from tsdl directory
cp ../tsdl/metadata trace_data/

# Parse traces using babeltrace2 (basic analysis)
python ../tools/uart_data_capture.py -i trace_data -l 50

# Export to Perfetto format for web visualization
python ../tools/uart_data_capture.py -i trace_data --perfetto -o perfetto_trace.json
```

## Configuration

The example is pre-configured with the following settings in `sdkconfig.defaults`:

```ini
CONFIG_ESP_TRACE_ENABLE=y
CONFIG_ESP_TRACE_LIB_EXTERNAL=y
CONFIG_ESP_TRACE_TRANSPORT_APPTRACE=y
CONFIG_APPTRACE_DEST_UART=y
CONFIG_APPTRACE_DEST_UART_NUM=0
CONFIG_ESP_CONSOLE_NONE=y
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
```

### Key Configuration Options

- **CONFIG_ESP_TRACE_ENABLE**: Enables the ESP tracing system
- **CONFIG_ESP_TRACE_LIB_EXTERNAL**: Uses external trace library (CTF)
- **CONFIG_APPTRACE_DEST_UART**: Routes traces to UART
- **CONFIG_APPTRACE_DEST_UART_NUM**: UART number for trace output (0 = UART0)

## Advanced Usage

### Custom Trace Events (In progress)

You can add custom trace events in your application:

```c
#include "esp_trace.h"

// Custom trace event
ESP_TRACE_EVENT("custom_event", "My custom event with value: %d", value);
```

## Resources

- [ESP-IDF Tracing Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-monitor.html#idf-monitor-tracing)
- [Common Trace Format Specification](https://diamond.org/ctf/)
- [Perfetto Documentation](https://perfetto.dev/docs/)
- [babeltrace2 Documentation](https://babeltrace.org/docs/v2.0/)
- [Trace Compass Documentation](https://www.eclipse.org/tracecompass/)
- [Trace Compass User Guide](https://archive.eclipse.org/tracecompass/doc/stable/org.eclipse.tracecompass.doc.user/User-Guide.html)

## License

This example is part of the ESP-IDF Extra Components project and is licensed under the Apache License 2.0.