/*
 * SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

#include "esp_trace_ctf.h"

#ifdef CONFIG_FREERTOS_SMP

#define traceQUEUE_SEND( pxQueue )

#else // CONFIG_FREERTOS_SMP

#if ( configUSE_QUEUE_SETS != 1 )
#define traceQUEUE_SEND( pxQueue )
#else
#define traceQUEUE_SEND( pxQueue )
#endif

#endif // CONFIG_FREERTOS_SMP

#define traceSTART()

#define traceTASK_NOTIFY_TAKE(uxIndexToWait) esp_trace_ctf_task_notify_take(xClearCountOnExit, xTicksToWait);
#define traceTASK_DELAY() esp_trace_ctf_task_delay(xTicksToDelay);
#define traceTASK_DELAY_UNTIL(xTimeToWake) esp_trace_ctf_task_delay_until();
#define traceTASK_NOTIFY_GIVE_FROM_ISR(uxIndexToNotify) esp_trace_ctf_task_notify_give_from_isr((uint32_t)pxTCB, (uint32_t)pxHigherPriorityTaskWoken);

#define traceTASK_PRIORITY_INHERIT( pxTCB, uxPriority ) esp_trace_ctf_task_priority_inherit((uint32_t)pxTCB);
#define traceTASK_RESUME( pxTCB ) esp_trace_ctf_task_resume((uint32_t)pxTCB);
#define traceINCREASE_TICK_COUNT( xTicksToJump ) esp_trace_ctf_increase_tick_count(xTicksToJump);
#define traceTASK_SUSPEND( pxTCB ) esp_trace_ctf_task_suspend((uint32_t)pxTCB);
#define traceTASK_PRIORITY_DISINHERIT( pxTCB, uxBasePriority ) esp_trace_ctf_task_priority_disinherit((uint32_t)pxMutexHolder);
#define traceTASK_RESUME_FROM_ISR( pxTCB ) esp_trace_ctf_task_resume_from_isr((uint32_t)pxTCB);
#define traceTASK_NOTIFY(uxIndexToNotify) esp_trace_ctf_task_notify((uint32_t)pxTCB, ulValue, eAction, (uint32_t)pulPreviousNotificationValue);
#define traceTASK_NOTIFY_FROM_ISR(uxIndexToWait) esp_trace_ctf_task_notify_from_isr((uint32_t)pxTCB, ulValue, eAction, (uint32_t)pulPreviousNotificationValue, (uint32_t)pxHigherPriorityTaskWoken);
#define traceTASK_NOTIFY_WAIT(uxIndexToWait) esp_trace_ctf_task_notify_wait(ulBitsToClearOnEntry, ulBitsToClearOnExit, (uint32_t)pulNotificationValue, xTicksToWait);

#define traceQUEUE_CREATE( pxNewQueue ) esp_trace_ctf_queue_create(uxQueueLength, uxItemSize, ucQueueType);
#define traceQUEUE_DELETE( pxQueue ) esp_trace_ctf_queue_delete((uint32_t)pxQueue);
#define traceQUEUE_PEEK( pxQueue ) esp_trace_ctf_queue_peek((uint32_t)pxQueue, (uint32_t)pvBuffer, xTicksToWait, 1);
#define traceQUEUE_PEEK_FROM_ISR( pxQueue ) esp_trace_ctf_queue_peek_from_isr((uint32_t)pxQueue, (uint32_t)pvBuffer);
#define traceQUEUE_PEEK_FROM_ISR_FAILED( pxQueue ) esp_trace_ctf_queue_peek_from_isr_failed((uint32_t)pxQueue, (uint32_t)pvBuffer);
#define traceQUEUE_RECEIVE( pxQueue ) esp_trace_ctf_queue_receive((uint32_t)pxQueue, 0, xTicksToWait, 1);
#define traceQUEUE_RECEIVE_FAILED( pxQueue ) esp_trace_ctf_queue_receive_failed((uint32_t)pxQueue, 0, xTicksToWait, 1);
#define traceQUEUE_SEMAPHORE_RECEIVE( pxQueue ) esp_trace_ctf_queue_semaphore_receive((uint32_t)pxQueue, 0, xTicksToWait, 0);
#define traceQUEUE_RECEIVE_FROM_ISR( pxQueue ) esp_trace_ctf_queue_receive_from_isr((uint32_t)pxQueue, (uint32_t)pvBuffer, (uint32_t)pxHigherPriorityTaskWoken);
#define traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue ) esp_trace_ctf_queue_receive_from_isr_failed((uint32_t)pxQueue, (uint32_t)pvBuffer, (uint32_t)pxHigherPriorityTaskWoken);
#define traceQUEUE_REGISTRY_ADD( xQueue, pcQueueName ) esp_trace_ctf_queue_registry_add((uint32_t)xQueue, (uint32_t)pcQueueName);
#define traceQUEUE_SEND_FAILED( pxQueue ) esp_trace_ctf_queue_send_failed((uint32_t)pxQueue, (uint32_t)pvItemToQueue, xTicksToWait, xCopyPosition);
#define traceQUEUE_SEND_FROM_ISR( pxQueue ) esp_trace_ctf_queue_send_from_isr((uint32_t)pxQueue, (uint32_t)pvItemToQueue, (uint32_t)pxHigherPriorityTaskWoken, xCopyPosition);
#define traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue ) esp_trace_ctf_queue_send_from_isr_failed((uint32_t)pxQueue, (uint32_t)pvItemToQueue, (uint32_t)pxHigherPriorityTaskWoken, xCopyPosition);
#define traceQUEUE_GIVE_FROM_ISR( pxQueue ) esp_trace_ctf_queue_give_from_isr((uint32_t)pxQueue, (uint32_t)pxHigherPriorityTaskWoken);
#define traceQUEUE_GIVE_FROM_ISR_FAILED( pxQueue ) esp_trace_ctf_queue_give_from_isr_failed((uint32_t)pxQueue, (uint32_t)pxHigherPriorityTaskWoken);
#define traceSTREAM_BUFFER_CREATE( pxStreamBuffer, xIsMessageBuffer ) esp_trace_ctf_stream_buffer_create((uint32_t)xIsMessageBuffer, (uint32_t)pxStreamBuffer);
#define traceSTREAM_BUFFER_CREATE_FAILED( xIsMessageBuffer ) esp_trace_ctf_stream_buffer_create_failed((uint32_t)xIsMessageBuffer, 0);
#define traceSTREAM_BUFFER_DELETE( xStreamBuffer ) esp_trace_ctf_stream_buffer_delete((uint32_t)xStreamBuffer)
#define traceSTREAM_BUFFER_RESET( xStreamBuffer ) esp_trace_ctf_stream_buffer_reset((uint32_t)xStreamBuffer)
#define traceSTREAM_BUFFER_SEND( xStreamBuffer, xBytesSent ) esp_trace_ctf_stream_buffer_send((uint32_t)xStreamBuffer, (uint32_t)xBytesSent)
#define traceSTREAM_BUFFER_SEND_FAILED( xStreamBuffer ) esp_trace_ctf_stream_buffer_send_failed((uint32_t)xStreamBuffer, 0)
#define traceSTREAM_BUFFER_SEND_FROM_ISR( xStreamBuffer, xBytesSent ) esp_trace_ctf_stream_buffer_send_from_isr((uint32_t)xStreamBuffer, (uint32_t)xBytesSent)
#define traceSTREAM_BUFFER_RECEIVE( xStreamBuffer, xReceivedLength ) esp_trace_ctf_stream_buffer_receive((uint32_t)xStreamBuffer, (uint32_t)xReceivedLength)
#define traceSTREAM_BUFFER_RECEIVE_FAILED( xStreamBuffer ) esp_trace_ctf_stream_buffer_receive_failed((uint32_t)xStreamBuffer, 0)
#define traceSTREAM_BUFFER_RECEIVE_FROM_ISR( xStreamBuffer, xReceivedLength ) esp_trace_ctf_stream_buffer_receive_from_isr((uint32_t)xStreamBuffer, (uint32_t)xReceivedLength)

#define traceTASK_DELETE( pxTCB ) esp_trace_ctf_task_delete((uint32_t)pxTCB);
#define traceTASK_CREATE(pxNewTCB)  if (pxNewTCB != NULL) { \
                                         esp_trace_ctf_task_create((uint32_t)pxNewTCB); \
                                     }

#define traceTASK_PRIORITY_SET(pxTask, uxNewPriority) esp_trace_ctf_task_priority_set((uint32_t)pxTask, uxNewPriority);
//
// Define INCLUDE_xTaskGetIdleTaskHandle as 1 in FreeRTOSConfig.h to allow identification of Idle state.
//
// SMP FreeRTOS uses unpinned IDLE tasks, so sometimes IDEL0 runs on CPU1, IDLE1 runs on CPU0 and so on.
// So IDLE state detection based on 'xTaskGetIdleTaskHandle' does not work for SMP kernel.
// We could compare current task handle with every element of the array returned by 'xTaskGetIdleTaskHandle',
// but it deos not seem to be efficient enough to be worth of making code more complex and less readabl.
// So always use task name comparison mechanism for SMP kernel.
#if ( INCLUDE_xTaskGetIdleTaskHandle == 1  && !defined(CONFIG_FREERTOS_SMP))
#define traceTASK_SWITCHED_IN()     if (prvGetTCBFromHandle(NULL) == xTaskGetIdleTaskHandle()) { \
                                        esp_trace_ctf_idle(); \
                                    } else { \
                                        esp_trace_ctf_task_switched_in((uint32_t)prvGetTCBFromHandle(NULL)); \
                                    }
#else
#define traceTASK_SWITCHED_IN()     if (memcmp(prvGetTCBFromHandle(NULL)->pcTaskName, "IDLE", 4) != 0) { \
                                        esp_trace_ctf_task_switched_in((uint32_t)prvGetTCBFromHandle(NULL)); \
                                    } else { \
                                        esp_trace_ctf_idle(); \
                                    }
#endif

#define traceMOVED_TASK_TO_READY_STATE(pxTCB) esp_trace_ctf_task_to_ready_state((uint32_t)pxTCB);
#define traceREADDED_TASK_TO_READY_STATE(pxTCB)

#define traceMOVED_TASK_TO_DELAYED_LIST() esp_trace_ctf_task_to_delayed_list((uint32_t)prvGetTCBFromHandle(NULL), (1u << 2));
#define traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST() esp_trace_ctf_task_to_overflow_delayed_list((uint32_t)prvGetTCBFromHandle(NULL), (1u << 2));
#define traceMOVED_TASK_TO_SUSPENDED_LIST(pxTCB) esp_trace_ctf_task_to_suspended_list((uint32_t)pxTCB, (3u << 3) | 3);

#define traceISR_EXIT_TO_SCHEDULER()
#define traceISR_EXIT() esp_trace_ctf_isr_exit();
#define traceISR_ENTER(n) esp_trace_ctf_isr_enter(n);
