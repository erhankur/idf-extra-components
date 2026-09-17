/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_trace_ctf.h"
#include "ctf_map.h"
#include "esp_trace_types.h"
#include "esp_trace_registry.h"
#include "esp_trace_port_encoder.h"
#include "esp_trace_port_transport.h"
#include "esp_trace_util.h"

/* Record layout must match the stream definition in tsdl/metadata */

#define CTF_FLUSH_TMO_US        (1000 * 1000)  /* 1 second */
#define CTF_FLUSH_THRESH        0
#define CTF_BUF_WAIT_TMO        ((uint32_t)CONFIG_ESP_CTF_BUF_WAIT_TMO)
#define CTF_MAX_PAYLOAD_SIZE    (2 * sizeof(uint32_t) + ESP_TRACE_CTF_MAX_STR_LEN)

typedef struct __attribute__((packed))
{
    uint32_t timestamp;
    uint8_t  id;
    uint8_t  core_id;
} ctf_event_header_t;

static const char *TAG = "adapter_encoder_ctf";

static esp_trace_encoder_t *s_enc;
static esp_trace_lock_t s_lock;
static uint32_t s_ts_freq;
static volatile bool s_recording;
/* Sent before the first event so the host knows the timestamp rate */
static bool s_info_sent;
static uint32_t s_events_lost;

static esp_err_t write_event_nolock(uint8_t id, const void *payload, size_t size)
{
    uint8_t buf[sizeof(ctf_event_header_t) + CTF_MAX_PAYLOAD_SIZE];
    ctf_event_header_t hdr = {
        .timestamp = esp_trace_timestamp_get(),
        .id = id,
        .core_id = (uint8_t)esp_cpu_get_core_id(),
    };

    memcpy(buf, &hdr, sizeof(hdr));
    if (size) {
        memcpy(buf + sizeof(hdr), payload, size);
    }

    return s_enc->tp->vt->write(s_enc->tp, buf, sizeof(hdr) + size, CTF_BUF_WAIT_TMO);
}

static void ctf_record(uint8_t id, const void *payload, size_t size)
{
    if (!s_recording) {
        return;
    }

    if (esp_trace_lock_take(&s_lock, CTF_BUF_WAIT_TMO) != ESP_OK) {
        s_events_lost++;
        return;
    }

    if (!s_info_sent) {
        const uint32_t info[] = { s_ts_freq, esp_trace_cpu_freq_get() };
        s_info_sent = write_event_nolock(ESP_TRACE_CTF_EVT_TRACE_INFO, info, sizeof(info)) == ESP_OK;
    }

    if (s_events_lost && write_event_nolock(ESP_TRACE_CTF_EVT_EVENTS_LOST, &s_events_lost, sizeof(s_events_lost)) == ESP_OK) {
        s_events_lost = 0;
    }

    if (write_event_nolock(id, payload, size) != ESP_OK) {
        s_events_lost++;
    }

    esp_trace_lock_give(&s_lock);
}

static void ctf_record_with_str(uint8_t id, const void *fixed, size_t fixed_size, const char *str)
{
    uint8_t payload[CTF_MAX_PAYLOAD_SIZE];
    size_t len = str ? strnlen(str, ESP_TRACE_CTF_MAX_STR_LEN - 1) : 0;

    if (fixed_size) {
        memcpy(payload, fixed, fixed_size);
    }
    if (len) {
        memcpy(payload + fixed_size, str, len);
    }
    payload[fixed_size + len] = '\0';

    ctf_record(id, payload, fixed_size + len + 1);
}

/* Called once per core */
static esp_err_t init(esp_trace_encoder_t *enc, const void *enc_cfg)
{
    (void)enc_cfg;

    if (!enc || !enc->tp || !enc->tp->vt || !enc->tp->vt->write) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_enc) {
        return ESP_OK;
    }

    esp_trace_lock_init(&s_lock);
    s_ts_freq = esp_trace_timestamp_init();

    if (enc->tp->vt->set_config) {
        uint32_t flush_tmo = CTF_FLUSH_TMO_US;
        uint32_t flush_thresh = CTF_FLUSH_THRESH;
        enc->tp->vt->set_config(enc->tp, ESP_TRACE_TRANSPORT_CFG_FLUSH_TMO, &flush_tmo);
        enc->tp->vt->set_config(enc->tp, ESP_TRACE_TRANSPORT_CFG_FLUSH_THRESH, &flush_thresh);
    }

    s_enc = enc;
    s_recording = true;
    esp_trace_notify_recording_state(true);

    ESP_EARLY_LOGI(TAG, "Initialized CTF encoder, timestamp rate %" PRIu32 " Hz", s_ts_freq);

    return ESP_OK;
}

static esp_err_t write(esp_trace_encoder_t *enc, const void *data, size_t size, uint32_t tmo)
{
    if (!enc || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    if (esp_trace_lock_take(&s_lock, tmo) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = enc->tp->vt->write(enc->tp, data, size, tmo);
    esp_trace_lock_give(&s_lock);

    return err;
}

static esp_err_t start(esp_trace_encoder_t *enc)
{
    (void)enc;
    s_recording = true;
    return ESP_OK;
}

static esp_err_t stop(esp_trace_encoder_t *enc)
{
    (void)enc;
    s_recording = false;
    return ESP_OK;
}

static esp_err_t flush(esp_trace_encoder_t *enc)
{
    if (!enc->tp->vt->flush_nolock) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_trace_lock_take(&s_lock, ESP_TRACE_TMO_INFINITE);
    esp_err_t err = enc->tp->vt->flush_nolock(enc->tp);
    esp_trace_lock_give(&s_lock);

    return err;
}

static void panic_handler(esp_trace_encoder_t *enc, const void *info)
{
    (void)info;

    /* The lock may be held by the code that panicked, so flush without it */
    if (enc->tp->vt->flush_nolock) {
        enc->tp->vt->flush_nolock(enc->tp);
    }
}

static unsigned int take_lock(esp_trace_encoder_t *enc, uint32_t tmo_us)
{
    (void)enc;
    if (esp_trace_lock_take(&s_lock, tmo_us) != ESP_OK) {
        return 0;
    }

    return s_lock.int_state;
}

static void give_lock(esp_trace_encoder_t *enc, unsigned int_state)
{
    (void)enc;
    s_lock.int_state = int_state;
    esp_trace_lock_give(&s_lock);
}

static void function_enter(esp_trace_encoder_t *enc, void *func, void *call_site)
{
    (void)enc;
    const uint32_t payload[] = { (uint32_t)func, (uint32_t)call_site };
    ctf_record(ESP_TRACE_CTF_EVT_FUNC_ENTER, payload, sizeof(payload));
}

static void function_exit(esp_trace_encoder_t *enc, void *func, void *call_site)
{
    (void)enc;
    const uint32_t payload[] = { (uint32_t)func, (uint32_t)call_site };
    ctf_record(ESP_TRACE_CTF_EVT_FUNC_EXIT, payload, sizeof(payload));
}

static const esp_trace_encoder_vtable_t s_ctf_vt = {
    .init                  = init,
    .write                 = write,
    .panic_handler         = panic_handler,
    .start                 = start,
    .stop                  = stop,
    .flush                 = flush,
    .take_lock             = take_lock,
    .give_lock             = give_lock,
    .function_enter        = function_enter,
    .function_exit         = function_exit,
};

ESP_TRACE_REGISTER_ENCODER("ctf", &s_ctf_vt);

#define ESP_CTF_FIELD_SIZE(x) + sizeof(x)

#define ESP_CTF_FIELD_APPEND(x) \
    { \
        memcpy(payload_ptr, &(x), sizeof(x)); \
        payload_ptr += sizeof(x); \
    }

/* Events without fields call ctf_record() directly */
#define ESP_CTF_EVENT(event_id, ...) \
    { \
        uint8_t payload[0 MAP(ESP_CTF_FIELD_SIZE, __VA_ARGS__)]; \
        uint8_t *payload_ptr = payload; \
        MAP(ESP_CTF_FIELD_APPEND, __VA_ARGS__) \
        ctf_record(event_id, payload, sizeof(payload)); \
    }

void esp_trace_ctf_task_delay(uint32_t xTicksToDelay)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_DELAY, xTicksToDelay);
}

void esp_trace_ctf_task_notify_take(uint32_t xClearCountOnExit, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_NOTIFY_TAKE, xClearCountOnExit, xTicksToWait);
}

void esp_trace_ctf_task_delay_until(void)
{
    ctf_record(ESP_TRACE_CTF_EVT_TASK_DELAY_UNTIL, NULL, 0);
}

void esp_trace_ctf_task_notify_give_from_isr(uint32_t pxTCB, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_NOTIFY_GIVE_FROM_ISR,
                  pxTCB, pxHigherPriorityTaskWoken);
}

void esp_trace_ctf_task_priority_inherit(uint32_t pxMutexHolder)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_PRIORITY_INHERIT, pxMutexHolder);
}

void esp_trace_ctf_task_resume(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_RESUME, pxTCB);
}

void esp_trace_ctf_increase_tick_count(uint32_t xTicksToJump)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_INCREASE_TICK_COUNT, xTicksToJump);
}

void esp_trace_ctf_task_suspend(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_SUSPEND, pxTCB);
}

void esp_trace_ctf_task_priority_disinherit(uint32_t pxMutexHolder)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_PRIORITY_DISINHERIT, pxMutexHolder);
}

void esp_trace_ctf_task_resume_from_isr(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_RESUME_FROM_ISR, pxTCB);
}

void esp_trace_ctf_task_notify(uint32_t pxTCB, uint32_t ulValue, uint32_t eAction,
                               uint32_t pulPreviousNotificationValue)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_NOTIFY,
                  pxTCB, ulValue, eAction, pulPreviousNotificationValue);
}

void esp_trace_ctf_task_notify_from_isr(uint32_t pxTCB, uint32_t ulValue, uint32_t eAction,
                                        uint32_t pulPreviousNotificationValue, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_NOTIFY_FROM_ISR,
                  pxTCB, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken);
}

void esp_trace_ctf_task_notify_wait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
                                    uint32_t pulNotificationValue, uint32_t xTicksToWait)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_NOTIFY_WAIT,
                  ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait);
}

void esp_trace_ctf_queue_create(uint32_t uxQueueLength, uint32_t uxItemSize, uint32_t ucQueueType)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_CREATE,
                  uxQueueLength, uxItemSize, ucQueueType);
}

void esp_trace_ctf_queue_delete(uint32_t pxQueue)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_DELETE, pxQueue);
}

void esp_trace_ctf_queue_peek(uint32_t pxQueue, uint32_t pvBuffer,
                              uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_PEEK,
                  pxQueue, pvBuffer, xTicksToWait, param);
}

void esp_trace_ctf_queue_peek_from_isr(uint32_t pxQueue, uint32_t pvBuffer)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_PEEK_FROM_ISR, pxQueue, pvBuffer);
}

void esp_trace_ctf_queue_peek_from_isr_failed(uint32_t pxQueue, uint32_t pvBuffer)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_PEEK_FROM_ISR_FAILED, pxQueue, pvBuffer);
}

void esp_trace_ctf_queue_receive(uint32_t pxQueue, uint32_t pvBuffer,
                                 uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_RECEIVE,
                  pxQueue, pvBuffer, xTicksToWait, param);
}

void esp_trace_ctf_queue_receive_failed(uint32_t pxQueue, uint32_t pvBuffer,
                                        uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FAILED,
                  pxQueue, pvBuffer, xTicksToWait, param);
}

void esp_trace_ctf_queue_semaphore_receive(uint32_t pxQueue, uint32_t pvBuffer,
                                           uint32_t xTicksToWait, uint32_t param)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_SEMAPHORE_RECEIVE,
                  pxQueue, pvBuffer, xTicksToWait, param);
}

void esp_trace_ctf_queue_receive_from_isr(uint32_t pxQueue, uint32_t pvBuffer,
                                          uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FROM_ISR,
                  pxQueue, pvBuffer, pxHigherPriorityTaskWoken);
}

void esp_trace_ctf_queue_receive_from_isr_failed(uint32_t pxQueue, uint32_t pvBuffer,
                                                 uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FROM_ISR_FAILED,
                  pxQueue, pvBuffer, pxHigherPriorityTaskWoken);
}

void esp_trace_ctf_queue_registry_add(uint32_t xQueue, uint32_t pcQueueName)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_REGISTRY_ADD,
                  xQueue, pcQueueName);
}

void esp_trace_ctf_queue_send_failed(uint32_t pxQueue, uint32_t pvItemToQueue,
                                     uint32_t xTicksToWait, uint32_t xCopyPosition)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_SEND_FAILED,
                  pxQueue, pvItemToQueue, xTicksToWait, xCopyPosition);
}

void esp_trace_ctf_queue_send_from_isr(uint32_t pxQueue, uint32_t pvItemToQueue,
                                       uint32_t pxHigherPriorityTaskWoken, uint32_t xCopyPosition)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_SEND_FROM_ISR,
                  pxQueue, pvItemToQueue, pxHigherPriorityTaskWoken, xCopyPosition);
}

void esp_trace_ctf_queue_send_from_isr_failed(uint32_t pxQueue, uint32_t pvItemToQueue,
                                              uint32_t pxHigherPriorityTaskWoken, uint32_t xCopyPosition)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_SEND_FROM_ISR_FAILED,
                  pxQueue, pvItemToQueue, pxHigherPriorityTaskWoken, xCopyPosition);
}

void esp_trace_ctf_queue_give_from_isr(uint32_t pxQueue, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_GIVE_FROM_ISR,
                  pxQueue, pxHigherPriorityTaskWoken);
}

void esp_trace_ctf_queue_give_from_isr_failed(uint32_t pxQueue, uint32_t pxHigherPriorityTaskWoken)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_QUEUE_GIVE_FROM_ISR_FAILED, pxQueue, pxHigherPriorityTaskWoken);
}

void esp_trace_ctf_stream_buffer_create(uint32_t xIsMessageBuffer, uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_CREATE,
                  xIsMessageBuffer, pxStreamBuffer);
}

void esp_trace_ctf_stream_buffer_create_failed(uint32_t xIsMessageBuffer, uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_CREATE_FAILED,
                  xIsMessageBuffer, pxStreamBuffer);
}

void esp_trace_ctf_stream_buffer_delete(uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_DELETE, pxStreamBuffer);
}

void esp_trace_ctf_stream_buffer_reset(uint32_t pxStreamBuffer)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_RESET, pxStreamBuffer);
}

void esp_trace_ctf_stream_buffer_send(uint32_t pxStreamBuffer, uint32_t xBytesSent)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND, pxStreamBuffer, xBytesSent);
}

void esp_trace_ctf_stream_buffer_send_failed(uint32_t pxStreamBuffer, uint32_t xBytesSent)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND_FAILED, pxStreamBuffer, xBytesSent);
}

void esp_trace_ctf_stream_buffer_send_from_isr(uint32_t pxStreamBuffer, uint32_t xBytesSent)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND_FROM_ISR, pxStreamBuffer, xBytesSent);
}

void esp_trace_ctf_stream_buffer_receive(uint32_t pxStreamBuffer, uint32_t xReceivedLength)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE, pxStreamBuffer, xReceivedLength);
}

void esp_trace_ctf_stream_buffer_receive_failed(uint32_t pxStreamBuffer, uint32_t xReceivedLength)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE_FAILED,
                  pxStreamBuffer, xReceivedLength);
}

void esp_trace_ctf_stream_buffer_receive_from_isr(uint32_t pxStreamBuffer, uint32_t xReceivedLength)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE_FROM_ISR,
                  pxStreamBuffer, xReceivedLength);
}

void esp_trace_ctf_task_delete(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_DELETE, pxTCB);
}

void esp_trace_ctf_task_priority_set(uint32_t pxTask, uint32_t uxNewPriority)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_PRIORITY_SET, pxTask, uxNewPriority);
}

void esp_trace_ctf_idle(void)
{
    ctf_record(ESP_TRACE_CTF_EVT_IDLE, NULL, 0);
}

void esp_trace_ctf_task_switched_in(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_SWITCHED_IN, pxTCB);
}

void esp_trace_ctf_task_to_ready_state(uint32_t pxTCB)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_TO_READY_STATE, pxTCB);
}

void esp_trace_ctf_task_to_delayed_list(uint32_t pxTCB, uint32_t cause)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_TO_DELAYED_LIST, pxTCB, cause);
}

void esp_trace_ctf_task_to_overflow_delayed_list(uint32_t pxTCB, uint32_t cause)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_TO_OVERFLOW_DELAYED_LIST, pxTCB, cause);
}

void esp_trace_ctf_task_to_suspended_list(uint32_t pxTCB, uint32_t cause)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_TASK_TO_SUSPENDED_LIST, pxTCB, cause);
}

void esp_trace_ctf_isr_exit_to_scheduler(void)
{
    ctf_record(ESP_TRACE_CTF_EVT_ISR_EXIT_TO_SCHEDULER, NULL, 0);
}

void esp_trace_ctf_isr_exit(void)
{
    ctf_record(ESP_TRACE_CTF_EVT_ISR_EXIT, NULL, 0);
}

void esp_trace_ctf_isr_enter(uint32_t n)
{
    ESP_CTF_EVENT(ESP_TRACE_CTF_EVT_ISR_ENTER, n);
}

void esp_trace_ctf_task_create(uint32_t pxNewTCB, uint32_t uxPriority, const char *pcName)
{
    const uint32_t fixed[] = { pxNewTCB, uxPriority };
    ctf_record_with_str(ESP_TRACE_CTF_EVT_TASK_CREATE, fixed, sizeof(fixed), pcName);
}

void esp_trace_ctf_print_event(const char *str)
{
    ctf_record_with_str(ESP_TRACE_CTF_EVT_PRINT_EVENT, NULL, 0, str);
}
