/* Edge Impulse inferencing library - RT-Thread porting
 * Copyright (c) 2025 RT-Thread
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../ei_classifier_porting.h"
#if EI_PORTING_RTTHREAD == 1

#include <rtthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define EI_WEAK_FN __attribute__((weak))

EI_WEAK_FN EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}

EI_WEAK_FN EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {
    rt_thread_mdelay(time_ms);
    return EI_IMPULSE_OK;
}

uint64_t ei_read_timer_ms() {
    return (uint64_t)rt_tick_get_millisecond();
}

uint64_t ei_read_timer_us() {
    // RT-Thread tick resolution is typically 1ms, so multiply by 1000
    return (uint64_t)rt_tick_get_millisecond() * 1000;
}

void ei_serial_set_baudrate(int baudrate)
{
    // Not implemented for RT-Thread console
}

EI_WEAK_FN void ei_putchar(char c)
{
    rt_kprintf("%c", c);
}

EI_WEAK_FN char ei_getchar()
{
    return 0; // Not implemented
}

/**
 * Printf function uses vsnprintf and output using RT-Thread console
 */
__attribute__((weak)) void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        rt_kprintf("%s", print_buf);
    }
}

__attribute__((weak)) void ei_printf_float(float f) {
    rt_kprintf("%.6f", f);
}

__attribute__((weak)) void *ei_malloc(size_t size) {
    return rt_malloc(size);
}

__attribute__((weak)) void *ei_calloc(size_t nitems, size_t size) {
    return rt_calloc(nitems, size);
}

__attribute__((weak)) void ei_free(void *ptr) {
    rt_free(ptr);
}

#if defined(__cplusplus) && EI_C_LINKAGE == 1
extern "C"
#endif
__attribute__((weak)) void DebugLog(const char* s) {
    ei_printf("%s", s);
}

#endif // EI_PORTING_RTTHREAD == 1
