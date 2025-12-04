#ifndef IOT_C_API_H
#define IOT_C_API_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void iot_initialize();
void iot_invoke(const uint8_t* data, uint16_t len);

// 新增接口
const char *iot_get_descriptors_json();   // 获取所有设备的描述信息
const char *iot_get_states_json();         // 获取所有设备的状态信息

#ifdef __cplusplus
}
#endif

#endif // IOT_C_API_H