# CTF (Common Trace Format) for ESP-IDF

[![Component Registry](https://components.espressif.com/components/espressif/esp_ctf/badge.svg)](https://components.espressif.com/components/espressif/esp_ctf)

This component is an `esp_trace` encoder that records FreeRTOS events in [Common Trace Format](https://github.com/efficios/ctf) (CTF 1.8). Traces can be read with standard tools such as babeltrace2 and Trace Compass, or exported to Perfetto.

## Install (managed component)

Add a dependency in your project's `idf_component.yml`:

```yaml
dependencies:
  espressif/esp_ctf: ^1
```

Configure tracing in `idf.py menuconfig`:
- Select trace library: Component config > ESP Trace Configuration > Trace library > External library from component registry (`CONFIG_ESP_TRACE_LIB_EXTERNAL`)
- Select transport: Component config > ESP Trace Configuration > Trace transport (apptrace over UART/JTAG, or USB Serial JTAG)
- Select timestamp source: Component config > ESP Trace Configuration > Trace timestamp source
- Tune the encoder: Component config > CTF (Common Trace Format) Configuration

Select the encoder by name in `esp_trace_get_user_params()`:

```c
esp_trace_open_params_t esp_trace_get_user_params(void)
{
    esp_trace_open_params_t params = {
        .encoder_name = "ctf",
        .transport_name = CONFIG_ESP_TRACE_TRANSPORT_NAME,
    };
    return params;
}
```

## Trace format

The stream description is in `tsdl/metadata`. Copy it next to the captured trace file to decode it. Each event is a packed record:

| Field       | Size | Description                                   |
|-------------|------|-----------------------------------------------|
| `timestamp` | 4    | Raw ticks of the esp_trace timestamp source   |
| `id`        | 1    | Event ID (`ESP_TRACE_CTF_EVT_*`)              |
| `core_id`   | 1    | CPU core that recorded the event              |
| payload     | var  | Event fields, see `tsdl/metadata`             |

- The first event of a trace is `trace_info`. It carries the timestamp rate (`ts_freq`) and the CPU frequency. The metadata clock defaults to 1 MHz, which matches the `esp_timer` timestamp source. For other sources, set `freq` in the metadata to `ts_freq`.
- If an event cannot be written within `CONFIG_ESP_CTF_BUF_WAIT_TMO`, it is dropped. The number of dropped events is reported in an `events_lost` event once writing succeeds again.
- `esp_trace_stop()` and `esp_trace_start()` pause and resume recording.
- Applications can add their own strings with `esp_trace_ctf_print_event()`.

## Function tracing

When `CONFIG_ESP_TRACE_FUNCTION_TRACE` is enabled, this component implements the `esp_trace` function-trace callbacks and records `func_enter`/`func_exit` events with the raw function and call-site addresses. Map them to names offline against the ELF file (for example with `addr2line`).

## Example

See [example](example/) for capturing and analyzing a trace, and [tools/uart_data_capture.py](tools/uart_data_capture.py) for the capture and export tool.

## License

Apache 2.0. See `LICENSE` file.
