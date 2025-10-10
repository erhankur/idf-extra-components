/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_TRACE_CTF_EVT_ISR_ENTER                       0x10
#define ESP_TRACE_CTF_EVT_ISR_EXIT                        0x11
#define ESP_TRACE_CTF_EVT_TASK_DELAY                      0x12
#define ESP_TRACE_CTF_EVT_TASK_NOTIFY_TAKE                0x13
#define ESP_TRACE_CTF_EVT_TASK_DELAY_UNTIL                0x14
#define ESP_TRACE_CTF_EVT_TASK_NOTIFY_GIVE_FROM_ISR       0x15
#define ESP_TRACE_CTF_EVT_TASK_PRIORITY_INHERIT           0x16
#define ESP_TRACE_CTF_EVT_TASK_RESUME                     0x17
#define ESP_TRACE_CTF_EVT_INCREASE_TICK_COUNT             0x18
#define ESP_TRACE_CTF_EVT_TASK_SUSPEND                    0x19
#define ESP_TRACE_CTF_EVT_TASK_PRIORITY_DISINHERIT        0x1A
#define ESP_TRACE_CTF_EVT_TASK_RESUME_FROM_ISR            0x1B
#define ESP_TRACE_CTF_EVT_TASK_NOTIFY                     0x1C
#define ESP_TRACE_CTF_EVT_TASK_NOTIFY_FROM_ISR            0x1D
#define ESP_TRACE_CTF_EVT_TASK_NOTIFY_WAIT                0x1E
#define ESP_TRACE_CTF_EVT_QUEUE_CREATE                    0x1F
#define ESP_TRACE_CTF_EVT_QUEUE_DELETE                    0x20
#define ESP_TRACE_CTF_EVT_QUEUE_PEEK                      0x21
#define ESP_TRACE_CTF_EVT_QUEUE_PEEK_FROM_ISR             0x22
#define ESP_TRACE_CTF_EVT_QUEUE_PEEK_FROM_ISR_FAILED      0x23
#define ESP_TRACE_CTF_EVT_QUEUE_RECEIVE                   0x24
#define ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FAILED            0x25
#define ESP_TRACE_CTF_EVT_QUEUE_SEMAPHORE_RECEIVE         0x26
#define ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FROM_ISR          0x27
#define ESP_TRACE_CTF_EVT_QUEUE_RECEIVE_FROM_ISR_FAILED   0x28
#define ESP_TRACE_CTF_EVT_QUEUE_REGISTRY_ADD              0x29
#define ESP_TRACE_CTF_EVT_QUEUE_SEND_FAILED               0x2A
#define ESP_TRACE_CTF_EVT_QUEUE_SEND_FROM_ISR             0x2B
#define ESP_TRACE_CTF_EVT_QUEUE_SEND_FROM_ISR_FAILED      0x2C
#define ESP_TRACE_CTF_EVT_QUEUE_GIVE_FROM_ISR             0x2D
#define ESP_TRACE_CTF_EVT_QUEUE_GIVE_FROM_ISR_FAILED      0x2E
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_CREATE            0x2F
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_CREATE_FAILED     0x30
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_DELETE            0x31
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_RESET             0x32
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND              0x33
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND_FAILED       0x34
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_SEND_FROM_ISR     0x35
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE           0x36
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE_FAILED    0x37
#define ESP_TRACE_CTF_EVT_STREAM_BUFFER_RECEIVE_FROM_ISR  0x38
#define ESP_TRACE_CTF_EVT_TASK_DELETE                     0x39
#define ESP_TRACE_CTF_EVT_TASK_CREATE                     0x3A
#define ESP_TRACE_CTF_EVT_TASK_PRIORITY_SET               0x3B
#define ESP_TRACE_CTF_EVT_TASK_SWITCHED_IN                0x3C
#define ESP_TRACE_CTF_EVT_IDLE                            0x3D
#define ESP_TRACE_CTF_EVT_TASK_TO_READY_STATE             0x3E
#define ESP_TRACE_CTF_EVT_TASK_TO_DELAYED_LIST            0x3F
#define ESP_TRACE_CTF_EVT_TASK_TO_OVERFLOW_DELAYED_LIST   0x40
#define ESP_TRACE_CTF_EVT_TASK_TO_SUSPENDED_LIST          0x41
#define ESP_TRACE_CTF_EVT_ISR_EXIT_TO_SCHEDULER           0x42
#define ESP_TRACE_CTF_EVT_PRINT_EVENT                     0x43
#define ESP_TRACE_CTF_EVT_TRACE_INFO                      0x44
#define ESP_TRACE_CTF_EVT_EVENTS_LOST                     0x45
#define ESP_TRACE_CTF_EVT_FUNC_ENTER                      0x46
#define ESP_TRACE_CTF_EVT_FUNC_EXIT                       0x47

/* Max string length in print and task create events, including NUL */
#define ESP_TRACE_CTF_MAX_STR_LEN                         64

void esp_trace_ctf_isr_enter(uint32_t n);
void esp_trace_ctf_isr_exit(void);
void esp_trace_ctf_isr_exit_to_scheduler(void);
void esp_trace_ctf_task_delay(uint32_t xTicksToDelay);
void esp_trace_ctf_task_notify_take(uint32_t xClearCountOnExit, uint32_t xTicksToWait);
void esp_trace_ctf_task_delay_until(void);
void esp_trace_ctf_task_notify_give_from_isr(uint32_t pxTCB, uint32_t pxHigherPriorityTaskWoken);
void esp_trace_ctf_task_priority_inherit(uint32_t pxMutexHolder);
void esp_trace_ctf_task_resume(uint32_t pxTCB);
void esp_trace_ctf_increase_tick_count(uint32_t xTicksToJump);
void esp_trace_ctf_task_suspend(uint32_t pxTCB);
void esp_trace_ctf_task_priority_disinherit(uint32_t pxMutexHolder);
void esp_trace_ctf_task_resume_from_isr(uint32_t pxTCB);
void esp_trace_ctf_task_notify(uint32_t pxTCB, uint32_t ulValue, uint32_t eAction,
                               uint32_t pulPreviousNotificationValue);
void esp_trace_ctf_task_notify_from_isr(uint32_t pxTCB, uint32_t ulValue, uint32_t eAction,
                                        uint32_t pulPreviousNotificationValue, uint32_t pxHigherPriorityTaskWoken);
void esp_trace_ctf_task_notify_wait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
                                    uint32_t pulNotificationValue, uint32_t xTicksToWait);
void esp_trace_ctf_queue_create(uint32_t uxQueueLength, uint32_t uxItemSize, uint32_t ucQueueType);
void esp_trace_ctf_queue_delete(uint32_t pxQueue);
void esp_trace_ctf_queue_peek(uint32_t pxQueue, uint32_t pvBuffer, uint32_t xTicksToWait, uint32_t param);
void esp_trace_ctf_queue_peek_from_isr(uint32_t pxQueue, uint32_t pvBuffer);
void esp_trace_ctf_queue_peek_from_isr_failed(uint32_t pxQueue, uint32_t pvBuffer);
void esp_trace_ctf_queue_receive(uint32_t pxQueue, uint32_t pvBuffer, uint32_t xTicksToWait, uint32_t param);
void esp_trace_ctf_queue_receive_failed(uint32_t pxQueue, uint32_t pvBuffer, uint32_t xTicksToWait, uint32_t param);
void esp_trace_ctf_queue_semaphore_receive(uint32_t pxQueue, uint32_t pvBuffer, uint32_t xTicksToWait, uint32_t param);
void esp_trace_ctf_queue_receive_from_isr(uint32_t pxQueue, uint32_t pvBuffer, uint32_t pxHigherPriorityTaskWoken);
void esp_trace_ctf_queue_receive_from_isr_failed(uint32_t pxQueue, uint32_t pvBuffer, uint32_t pxHigherPriorityTaskWoken);
void esp_trace_ctf_queue_registry_add(uint32_t xQueue, uint32_t pcQueueName);
void esp_trace_ctf_queue_send_failed(uint32_t pxQueue, uint32_t pvItemToQueue,
                                     uint32_t xTicksToWait, uint32_t xCopyPosition);
void esp_trace_ctf_queue_send_from_isr(uint32_t pxQueue, uint32_t pvItemToQueue,
                                       uint32_t pxHigherPriorityTaskWoken, uint32_t xCopyPosition);
void esp_trace_ctf_queue_send_from_isr_failed(uint32_t pxQueue, uint32_t pvItemToQueue,
                                              uint32_t pxHigherPriorityTaskWoken, uint32_t xCopyPosition);
void esp_trace_ctf_queue_give_from_isr(uint32_t pxQueue, uint32_t pxHigherPriorityTaskWoken);
void esp_trace_ctf_queue_give_from_isr_failed(uint32_t pxQueue, uint32_t pxHigherPriorityTaskWoken);
void esp_trace_ctf_stream_buffer_create(uint32_t xIsMessageBuffer, uint32_t pxStreamBuffer);
void esp_trace_ctf_stream_buffer_create_failed(uint32_t xIsMessageBuffer, uint32_t pxStreamBuffer);
void esp_trace_ctf_stream_buffer_delete(uint32_t pxStreamBuffer);
void esp_trace_ctf_stream_buffer_reset(uint32_t pxStreamBuffer);
void esp_trace_ctf_stream_buffer_send(uint32_t pxStreamBuffer, uint32_t xBytesSent);
void esp_trace_ctf_stream_buffer_send_failed(uint32_t pxStreamBuffer, uint32_t xBytesSent);
void esp_trace_ctf_stream_buffer_send_from_isr(uint32_t pxStreamBuffer, uint32_t xBytesSent);
void esp_trace_ctf_stream_buffer_receive(uint32_t pxStreamBuffer, uint32_t xReceivedLength);
void esp_trace_ctf_stream_buffer_receive_failed(uint32_t pxStreamBuffer, uint32_t xReceivedLength);
void esp_trace_ctf_stream_buffer_receive_from_isr(uint32_t pxStreamBuffer, uint32_t xReceivedLength);
void esp_trace_ctf_task_delete(uint32_t pxTCB);
void esp_trace_ctf_task_create(uint32_t pxNewTCB, uint32_t uxPriority, const char *pcName);
void esp_trace_ctf_task_priority_set(uint32_t pxTask, uint32_t uxNewPriority);
void esp_trace_ctf_idle(void);
void esp_trace_ctf_task_switched_in(uint32_t pxTCB);
void esp_trace_ctf_task_to_ready_state(uint32_t pxTCB);
void esp_trace_ctf_task_to_delayed_list(uint32_t pxTCB, uint32_t cause);
void esp_trace_ctf_task_to_overflow_delayed_list(uint32_t pxTCB, uint32_t cause);
void esp_trace_ctf_task_to_suspended_list(uint32_t pxTCB, uint32_t cause);

/**
 * @brief Record a user string in the trace.
 *
 * Strings longer than ESP_TRACE_CTF_MAX_STR_LEN - 1 are truncated.
 *
 * @param str NUL-terminated string
 */
void esp_trace_ctf_print_event(const char *str);

#ifdef __cplusplus
}
#endif
