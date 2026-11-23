#ifndef __BTSTACK_RUN_LOOP_RTT_H
#define __BTSTACK_RUN_LOOP_RTT_H

#include "btstack_run_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

const btstack_run_loop_t *btstack_run_loop_rtt_instance(void);
void btstack_run_loop_rtt_wakeup(void);
void btstack_run_loop_rtt_request_exit(void);
void btstack_run_loop_rtt_set_poll_callback(void (*callback)(void));

#ifdef __cplusplus
}
#endif

#endif /* __BTSTACK_RUN_LOOP_RTT_H */
