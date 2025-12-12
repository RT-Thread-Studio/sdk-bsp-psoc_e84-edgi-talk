/* XiaoZhi AI Wake Word Detection Header
 */

#ifndef XIAOZHI_WAKEWORD_H
#define XIAOZHI_WAKEWORD_H

#include <rtthread.h>
#include <rtdef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Wake word detection functions */
int xz_wakeword_init(void);
int xz_wakeword_deinit(void);
rt_bool_t xz_wakeword_is_enabled(void);
void xz_wakeword_info(void);

/* Wake word control functions */
int xz_wakeword_start(void);
int xz_wakeword_stop(void);

/* Wake word control events */
#define XZ_WAKEWORD_EVENT_START  (1 << 0)
#define XZ_WAKEWORD_EVENT_STOP   (1 << 1)
#define XZ_WAKEWORD_EVENT_EXIT   (1 << 2)

/* Wake word detection callback function - to be implemented in xiaozhi.cpp */
void xz_wakeword_detected_callback(const char *wake_word, float confidence);

#ifdef __cplusplus
}
#endif

#endif /* XIAOZHI_WAKEWORD_H */