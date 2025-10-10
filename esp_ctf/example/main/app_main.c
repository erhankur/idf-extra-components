/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "soc/uart_pins.h"
#include "esp_app_trace.h"
#include "esp_trace.h"
#include "esp_trace_ctf.h"

#define QUEUE_LEN       8
#define ITERATIONS      100

esp_trace_open_params_t esp_trace_get_user_params(void)
{
    esp_trace_open_params_t trace_params = {
        .core_cfg = NULL,
        .encoder_name = "ctf",
        .encoder_cfg = NULL,
        .transport_name = CONFIG_ESP_TRACE_TRANSPORT_NAME,
        .transport_cfg = NULL,
    };

#if CONFIG_ESP_TRACE_TRANSPORT_APPTRACE
    static esp_apptrace_config_t app_trace_config = APPTRACE_CONFIG_DEFAULT();
#if CONFIG_APPTRACE_DEST_UART
    /* Override default values to use console pins as a uart channel */
    app_trace_config.dest_cfg.uart.tx_pin_num = U0TXD_GPIO_NUM;
    app_trace_config.dest_cfg.uart.rx_pin_num = U0RXD_GPIO_NUM;
#endif
    trace_params.transport_cfg = &app_trace_config;
#endif

    return trace_params;
}

static void producer_task(void *arg)
{
    QueueHandle_t queue = arg;

    for (uint32_t i = 0; i < ITERATIONS; i++) {
        xQueueSend(queue, &i, portMAX_DELAY);
        if (i % 10 == 0) {
            esp_trace_ctf_print_event("producer checkpoint");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

static void consumer_task(void *arg)
{
    QueueHandle_t queue = arg;
    uint32_t value;

    while (1) {
        xQueueReceive(queue, &value, portMAX_DELAY);
    }
}

void app_main(void)
{
    QueueHandle_t queue = xQueueCreate(QUEUE_LEN, sizeof(uint32_t));
    configASSERT(queue);

    xTaskCreate(consumer_task, "consumer", 2048, queue, 5, NULL);
    xTaskCreate(producer_task, "producer", 2048, queue, 4, NULL);
}
