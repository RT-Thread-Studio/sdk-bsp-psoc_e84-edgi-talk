#ifndef __BTSTACK_AUDIO_RTT_H
#define __BTSTACK_AUDIO_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Register the RT-Thread sound0 backed audio sink with BTstack. */
void btstack_audio_rtt_init_instance(void);

#ifdef __cplusplus
}
#endif

#endif /* __BTSTACK_AUDIO_RTT_H */
