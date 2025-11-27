#include "iot_c_api.h"
#include "thing_manager.h"
#include "cJSON.h"
#include <rtthread.h>
#include <string.h>
#include "../mcp/mcp_server.h"

static std::string descriptors_json;
static std::string states_json;

extern "C" {
//注册IoT设备(可多个设备)
void iot_initialize() {
    auto& manager = iot::ThingManager::GetInstance();
    rt_kprintf("Registering Speaker...\n");
    manager.AddThing(iot::CreateThing("Speaker"));

    McpServer::GetInstance().AddCommonTools();
}

void iot_invoke(const uint8_t* data, uint16_t len) {
    char* json_str = (char*)rt_malloc(len + 1);
    memcpy(json_str, data, len);
    json_str[len] = '\0';

    cJSON* root = cJSON_Parse(json_str);
    if (root == nullptr) {
        rt_kprintf("[IoT] Failed to parse command JSON\n");
        rt_free(json_str);
        return;
    }

    // 打印完整 JSON
    char* print_str = cJSON_PrintUnformatted(root);
    if (print_str) {
        rt_kprintf("[IoT] Received command: %s\n", print_str);
        McpServer::GetInstance().ParseMessage(print_str);
        cJSON_free(print_str);
    }

    // 直接调用 ThingManager::Invoke(root)
    auto& manager = iot::ThingManager::GetInstance();
    manager.Invoke(root);  // 这里直接传 root 即可
    
    cJSON_Delete(root);
    rt_free(json_str);
}

const char* iot_get_descriptors_json() {
    auto& manager = iot::ThingManager::GetInstance();
    descriptors_json = manager.GetDescriptorsJson();
    return descriptors_json.c_str();
}

const char* iot_get_states_json() {
    auto& manager = iot::ThingManager::GetInstance();
    states_json.clear(); // 清空旧数据
    manager.GetStatesJson(states_json, false); // 获取所有设备的状态
    return states_json.c_str();
}

} // extern "C"
