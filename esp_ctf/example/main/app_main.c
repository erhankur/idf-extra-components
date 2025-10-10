/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/uart_pins.h"
#include "esp_log.h"
#include "esp_app_trace.h"
#include "esp_trace.h"
#include "esp_timer.h"

static const char *TAG = "main";

esp_trace_open_params_t esp_trace_get_user_params(void)
{
    static esp_apptrace_config_t app_trace_config = APPTRACE_CONFIG_DEFAULT();
#if CONFIG_APPTRACE_DEST_UART
    /* Override default values to use console pins as a uart channel */
    app_trace_config.dest_cfg.uart.tx_pin_num = U0TXD_GPIO_NUM;
    app_trace_config.dest_cfg.uart.rx_pin_num = U0RXD_GPIO_NUM;
#endif

    esp_trace_open_params_t trace_params = {
        .core_cfg = NULL,
        .encoder_name = "ctf",
        .encoder_cfg = NULL,
        .transport_name = "apptrace",
        .transport_cfg = &app_trace_config,
    };
    return trace_params;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Start of main...");

    // Concept: Not tested yet!
    ESP_TRACE_EVENT("custom_event", "My custom event with value: %d", 147);
    ESP_TRACE_EVENT("user_action", "Button pressed at timestamp: %llu", esp_timer_get_time());
    ESP_TRACE_EVENT("sensor_reading", "Temperature: %.2f°C", 25.6f);
    ESP_TRACE_EVENT("system_state", "System initialized successfully");

    ESP_LOGI(TAG, "End of main...");
}
