/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_trace_ctf.h"
#include "ctf_map.h"
#include "esp_trace_types.h"
#include "esp_trace_registry.h"
#include "esp_trace_port_encoder.h"
#include "esp_trace_port_transport.h"

static const char *TAG = "adapter_encoder_ctf";

static esp_trace_encoder_t *s_enc;

static esp_err_t init(esp_trace_encoder_t *enc, const void *enc_cfg)
{
    (void)enc_cfg;

    if (!enc || !enc->tp->vt->write) {
        return ESP_ERR_INVALID_STATE;
    }

    s_enc = enc;

    ESP_LOGI(TAG, "Initialized encoder ctf");

    return ESP_OK;
}

static esp_err_t write(esp_trace_encoder_t *enc, const void *data, size_t size, uint32_t tmo)
{
    if (!enc) {
        return ESP_ERR_INVALID_ARG;
    }

    return enc->tp->vt->write(enc->tp, data, size, tmo);
}

static esp_err_t print_event(esp_trace_encoder_t *enc, const char *event_name, const char *formatted_str)
{
    (void)enc;

    if (!event_name || !formatted_str) {
        return ESP_ERR_INVALID_ARG;
    }

    char combined_msg[256]; //TODO: size can be defined at compiled time
    snprintf(combined_msg, sizeof(combined_msg), "[%s] %s", event_name, formatted_str);

    return esp_trace_ctf_print_event(combined_msg);
}

static const esp_trace_encoder_vtable_t s_ctf_vt = {
    .init                  = init,
    .write                 = write,
    .print_event           = print_event,
};

#define ESP_CTF_FIELD_SIZE(x) + sizeof(x)

#define ESP_CTF_FIELD_APPEND(x) \
    { \
        memcpy(edata_ptr, &(x), sizeof(x)); \
        edata_ptr += sizeof(x); \
    }

#define ESP_CTF_GATHER_FIELDS(...) \
    { \
        uint8_t edata[0 MAP(ESP_CTF_FIELD_SIZE, ##__VA_ARGS__)]; \
        uint8_t *edata_ptr = &edata[0]; \
        MAP(ESP_CTF_FIELD_APPEND, ##__VA_ARGS__) \
        return write(s_enc, edata, sizeof(edata), ESP_TRACE_TMO_INFINITE); \
    }

#define ESP_CTF_EVENT(event_id, ...) \
    { \
        const uint32_t timestamp = esp_cpu_get_cycle_count(); \
        const uint8_t core_id = esp_cpu_get_core_id(); \
        ESP_CTF_GATHER_FIELDS(timestamp, event_id, core_id, ##__VA_ARGS__) \
    }

#define ESP_CTF_LITERAL(type, value) ((type) { (type)(value) })

esp_err_t esp_trace_ctf_task_delay(uint32_t xTicksToDelay)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_DELAY), xTicksToDelay);
}

esp_err_t esp_trace_ctf_task_notify_take(uint32_t xClearCountOnExit, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_NOTIFY_TAKE), xClearCountOnExit, xTicksToWait);
}

esp_err_t esp_trace_ctf_task_delay_until(void)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_DELAY_UNTIL));
}

esp_err_t esp_trace_ctf_task_notify_give_from_isr(uint32_t pxTCB, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_NOTIFY_GIVE_FROM_ISR),
                  pxTCB, pxHigherPriorityTaskWoken);
}

esp_err_t esp_trace_ctf_task_priority_inherit(uint32_t pxMutexHolder)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_PRIORITY_INHERIT), pxMutexHolder);
}

esp_err_t esp_trace_ctf_task_resume(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_RESUME), pxTCB);
}

esp_err_t esp_trace_ctf_increase_tick_count(uint32_t xTicksToJump)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_INCREASE_TICK_COUNT), xTicksToJump);
}

esp_err_t esp_trace_ctf_task_suspend(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_SUSPEND), pxTCB);
}

esp_err_t esp_trace_ctf_task_priority_disinherit(uint32_t pxMutexHolder)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_PRIORITY_DISINHERIT), pxMutexHolder);
}

esp_err_t esp_trace_ctf_task_resume_from_isr(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_RESUME_FROM_ISR), pxTCB);
}

esp_err_t esp_trace_ctf_task_notify(uint32_t pxTCB, uint32_t ulValue, uint32_t eAction,
                                    uint32_t pulPreviousNotificationValue)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_NOTIFY),
                  pxTCB, ulValue, eAction, pulPreviousNotificationValue);
}

esp_err_t esp_trace_ctf_task_notify_from_isr(uint32_t pxTCB, uint32_t ulValue, uint32_t eAction,
        uint32_t pulPreviousNotificationValue, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_NOTIFY_FROM_ISR),
                  pxTCB, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken);
}

esp_err_t esp_trace_ctf_task_notify_wait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
        uint32_t pulNotificationValue, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_NOTIFY_WAIT),
                  ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait);
}

esp_err_t esp_trace_ctf_queue_create(uint32_t uxQueueLength, uint32_t uxItemSize, uint32_t ucQueueType)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_CREATE),
                  uxQueueLength, uxItemSize, ucQueueType);
}

esp_err_t esp_trace_ctf_queue_delete(uint32_t pxQueue)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_DELETE), pxQueue);
}

esp_err_t esp_trace_ctf_queue_peek(uint32_t pxQueue, uint32_t pvBuffer,
                                   uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_PEEK),
                  pxQueue, pvBuffer, xTicksToWait, param);
}

esp_err_t esp_trace_ctf_queue_peek_from_isr(uint32_t pxQueue, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_PEEK_FROM_ISR),
                  pxQueue, xTicksToWait);
}

esp_err_t esp_trace_ctf_queue_peek_from_isr_failed(uint32_t pxQueue, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_PEEK_FROM_ISR_FAILED),
                  pxQueue, xTicksToWait);
}

esp_err_t esp_trace_ctf_queue_receive(uint32_t pxQueue, uint32_t pvBuffer,
                                      uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_RECEIVE),
                  pxQueue, pvBuffer, xTicksToWait, param);
}

esp_err_t esp_trace_ctf_queue_receive_failed(uint32_t pxQueue, uint32_t pvBuffer,
        uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FAILED),
                  pxQueue, pvBuffer, xTicksToWait, param);
}

esp_err_t esp_trace_ctf_queue_semaphore_receive(uint32_t pxQueue, uint32_t pvBuffer,
        uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_SEMAPHORE_RECEIVE),
                  pxQueue, pvBuffer, xTicksToWait, param);
}

esp_err_t esp_trace_ctf_queue_receive_from_isr(uint32_t pxQueue, uint32_t pvBuffer,
        uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FROM_ISR),
                  pxQueue, pvBuffer, pxHigherPriorityTaskWoken);
}

esp_err_t esp_trace_ctf_queue_receive_from_isr_failed(uint32_t pxQueue, uint32_t pvBuffer,
        uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FROM_ISR_FAILED),
                  pxQueue, pvBuffer, pxHigherPriorityTaskWoken);
}

esp_err_t esp_trace_ctf_queue_registry_add(uint32_t xQueue, uint32_t pcQueueName)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_REGISTRY_ADD),
                  xQueue, pcQueueName);
}

esp_err_t esp_trace_ctf_queue_send_failed(uint32_t pxQueue, uint32_t pvItemToQueue,
        uint32_t xTicksToWait, uint32_t xCopyPosition)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_SEND_FAILED),
                  pxQueue, pvItemToQueue, xTicksToWait, xCopyPosition);
}

esp_err_t esp_trace_ctf_queue_send_from_isr(uint32_t pxQueue, uint32_t pvItemToQueue,
        uint32_t pxHigherPriorityTaskWoken, uint32_t xCopyPosition)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_SEND_FROM_ISR),
                  pxQueue, pvItemToQueue, pxHigherPriorityTaskWoken, xCopyPosition);
}

esp_err_t esp_trace_ctf_queue_send_from_isr_failed(uint32_t pxQueue, uint32_t pvItemToQueue,
        uint32_t xTicksToWait, uint32_t xCopyPosition)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_SEND_FROM_ISR_FAILED),
                  pxQueue, pvItemToQueue, xTicksToWait, xCopyPosition);
}

esp_err_t esp_trace_ctf_queue_give_from_isr(uint32_t pxQueue, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_GIVE_FROM_ISR),
                  pxQueue, pxHigherPriorityTaskWoken);
}

esp_err_t esp_trace_ctf_queue_give_from_isr_failed(uint32_t pxQueue, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_QUEUE_GIVE_FROM_ISR_FAILED),
                  pxQueue, xTicksToWait);
}

esp_err_t esp_trace_ctf_stream_buffer_create(uint32_t xIsMessageBuffer, uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_CREATE),
                  xIsMessageBuffer, pxStreamBuffer);
}

esp_err_t esp_trace_ctf_stream_buffer_create_failed(uint32_t xIsMessageBuffer, uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_CREATE_FAILED),
                  xIsMessageBuffer, pxStreamBuffer);
}

esp_err_t esp_trace_ctf_stream_buffer_delete(uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_DELETE), pxStreamBuffer);
}

esp_err_t esp_trace_ctf_stream_buffer_reset(uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_RESET), pxStreamBuffer);
}

esp_err_t esp_trace_ctf_stream_buffer_send(uint32_t pxStreamBuffer, uint32_t xBytesSent)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND), pxStreamBuffer, xBytesSent);
}

esp_err_t esp_trace_ctf_stream_buffer_send_failed(uint32_t pxStreamBuffer, uint32_t xBytesSent)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND_FAILED), pxStreamBuffer, xBytesSent);
}

esp_err_t esp_trace_ctf_stream_buffer_send_from_isr(uint32_t pxStreamBuffer, uint32_t xBytesSent)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND_FROM_ISR), pxStreamBuffer, xBytesSent);
}

esp_err_t esp_trace_ctf_stream_buffer_receive(uint32_t pxStreamBuffer, uint32_t xReceivedLength)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE), pxStreamBuffer, xReceivedLength);
}

esp_err_t esp_trace_ctf_stream_buffer_receive_failed(uint32_t pxStreamBuffer, uint32_t xReceivedLength)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE_FAILED),
                  pxStreamBuffer, xReceivedLength);
}

esp_err_t esp_trace_ctf_stream_buffer_receive_from_isr(uint32_t pxStreamBuffer, uint32_t xReceivedLength)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE_FROM_ISR),
                  pxStreamBuffer, xReceivedLength);
}

esp_err_t esp_trace_ctf_task_delete(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_DELETE), pxTCB);
}

esp_err_t esp_trace_ctf_task_create(uint32_t pxNewTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_CREATE), pxNewTCB);
}

esp_err_t esp_trace_ctf_task_priority_set(uint32_t pxTask, uint32_t uxNewPriority)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_PRIORITY_SET), pxTask, uxNewPriority);
}

esp_err_t esp_trace_ctf_idle(void)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_IDLE));
}

esp_err_t esp_trace_ctf_task_switched_in(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_SWITCHED_IN), pxTCB);
}

esp_err_t esp_trace_ctf_task_to_ready_state(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_TO_READY_STATE), pxTCB);
}

esp_err_t esp_trace_ctf_task_to_delayed_list(uint32_t pxTCB, uint32_t cause)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_TO_DELAYED_LIST), pxTCB, cause);
}

esp_err_t esp_trace_ctf_task_to_overflow_delayed_list(uint32_t pxTCB, uint32_t cause)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_TO_OVERFLOW_DELAYED_LIST), pxTCB, cause);
}

esp_err_t esp_trace_ctf_task_to_suspended_list(uint32_t pxTCB, uint32_t cause)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_TASK_TO_SUSPENDED_LIST), pxTCB, cause);
}

esp_err_t esp_trace_ctf_print_event(const char *formatted_str)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_PRINT_EVENT), formatted_str);
}

esp_err_t esp_trace_iser_exit_to_scheduler(void)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_ISR_EXIT_TO_SCHEDULER));
}

esp_err_t esp_trace_ctf_isr_exit(void)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_ISR_EXIT));
}

esp_err_t esp_trace_ctf_isr_enter(uint32_t n)
{
    ESP_CTF_EVENT(ESP_CTF_LITERAL(uint8_t, ESP_TRACE_CTF_EVT_ISR_ENTER), n);
}

ESP_TRACE_REGISTER_ENCODER("ctf", &s_ctf_vt);
