#include "mcp_server.h"
#include <algorithm>
#include <cstring>
#include "../iot/thing_manager.h"
#include <webclient.h>
// #include "lwip/apps/websocket_client.h"   // 提供 wsock_write 和 OPCODE_TEXT 定义
#include "xiaozhi.h"        // 提供 g_xz_ws 定义

extern "C" {
#include "drv_es8388.h"
}

extern xiaozhi_ws_t g_xz_ws;   
#define TAG "MCP"
#define BOARD_NAME "psocedge84"
#define DEFAULT_TOOLCALL_STACK_SIZE 6144

McpServer::McpServer() {
}

McpServer::~McpServer() {
    for (auto tool : tools_) { 
        delete tool;
    }
    tools_.clear();
}

void McpServer::AddCommonTools() {
    // To speed up the response time, we add the common tools to the beginning of
    // the tools list to utilize the prompt cache.
    // Backup the original tools list and restore it after adding the common tools.
   auto original_tools = std::move(tools_);
#if 1   
   auto speaker = iot::ThingManager::GetInstance().GetThing("Speaker");
    if (speaker) {
        //设置音量工具
        AddTool("self.audio_speaker.set_volume",
        "Set the volume of the audio speaker,Must not exceed 100.",
        PropertyList({
            Property("volume", kPropertyTypeInteger, 0, 100)
        }),
        [=](const PropertyList& properties) -> ReturnValue {
            int volume = properties["volume"].value<int>();
            auto json_str = R"({"method":"SetVolume","parameters":{"volume":)" + std::to_string(volume) + "}}";
            auto command = cJSON_Parse(json_str.c_str());
            if (command) {
                speaker->Invoke(command);
                cJSON_Delete(command); // 使用完记得释放内存
            }
            return true;
        });

         //获取当前音量工具
        AddTool("self.audio_speaker.get_volume",
        "Get the current volume of the audio speaker.",
        PropertyList(),
        [=](const PropertyList&) -> ReturnValue {
            const char* json_str = R"({"method":"GetVolume","parameters":{}})";
            cJSON* cmd = cJSON_Parse(json_str); // 直接使用 const char*
            speaker->Invoke(cmd);
            cJSON_Delete(cmd);
            return es8388_volume_get();
        });
    }
    
#endif 
    // Restore the original tools list to the end of the tools list
    tools_.insert(tools_.end(), original_tools.begin(), original_tools.end());
}

void McpServer::SendText(const std::string& text)
{
    rt_kprintf("me send to websocket (MCP):\n");
    rt_kprintf("[MCP] Current tools count: %d\n", tools_.size());
    rt_kprintf("%s\n", text.c_str());  // 打印发送的内容
    wsock_write(&g_xz_ws.clnt, text.c_str(), text.length(), OPCODE_TEXT);
}
void McpServer::SendmcpMessage(const std::string& payload) {
//    std::string message = "{\"session_id\":\"" + g_xz_ws.session_id + 
//                          "\",\"type\":\"mcp\",\"payload\":" + payload + "}";
   std::string message = "{\"session_id\":\"" + std::string(reinterpret_cast<const char*>(g_xz_ws.session_id)) +
                         "\",\"type\":\"mcp\",\"payload\":" + payload + "}";
   McpServer::SendText(message);
}

void McpServer::AddTool(McpTool* tool) {
    // Prevent adding duplicate tools
    if (std::find_if(tools_.begin(), tools_.end(), [tool](const McpTool* t) { return t->name() == tool->name(); }) != tools_.end()) {
        rt_kprintf(TAG, "Tool %s already added", tool->name().c_str());
        delete tool;
        return;
    }

    rt_kprintf("[MCP] Add tool: %s\n", tool->name().c_str());
    tools_.push_back(tool);
}

void McpServer::AddTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    AddTool(new McpTool(name, description, properties, callback));
}

void McpServer::ParseMessage(const std::string& message) {
    cJSON* json = cJSON_Parse(message.c_str());
    if (json == nullptr) {
        rt_kprintf(TAG, "Failed to parse MCP message: %s", message.c_str());
        return;
    }
    ParseMessage(json);
    cJSON_Delete(json);
}

void McpServer::ParseCapabilities(const cJSON* capabilities) {
 
}

void McpServer::ParseMessage(const cJSON* json) 
{
    // Check JSONRPC version
    auto version = cJSON_GetObjectItem(json, "jsonrpc");
    if (version == nullptr || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
        rt_kprintf(TAG, "Invalid JSONRPC version: %s", version ? version->valuestring : "null");
        return;
    }
    
    // Check method
    auto method = cJSON_GetObjectItem(json, "method");
    if (method == nullptr || !cJSON_IsString(method)) {
        rt_kprintf(TAG, "Missing method");
        return;
    }
    
    auto method_str = std::string(method->valuestring);
    rt_kprintf(TAG, "Received method: %s", method_str.c_str());
    if (method_str.find("notifications") == 0) {
        return;
    }
    
    // Check params
    auto params = cJSON_GetObjectItem(json, "params");
    if (params != nullptr && !cJSON_IsObject(params)) {
        rt_kprintf(TAG, "Invalid params for method: %s", method_str.c_str());
        return;
    }

    auto id = cJSON_GetObjectItem(json, "id");
    if (id == nullptr || !cJSON_IsNumber(id)) {
        rt_kprintf(TAG, "Invalid id for method: %s", method_str.c_str());
        return;
    }
    auto id_int = id->valueint;
    
    if (method_str == "initialize") {
        
        
        //auto app_desc = esp_app_get_description();
        std::string message = "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\"";
        message += "1.0.0";
        message += "\"}}";
        ReplyResult(id_int, message);
    } else if (method_str == "tools/list") {
        std::string cursor_str = "";
        if (params != nullptr) {
            auto cursor = cJSON_GetObjectItem(params, "cursor");
            if (cJSON_IsString(cursor)) {
                cursor_str = std::string(cursor->valuestring);
            }
        }
        GetToolsList(id_int, cursor_str);
    } else if (method_str == "tools/call") {
        if (!cJSON_IsObject(params)) {
            rt_kprintf(TAG, "tools/call: Missing params");
            ReplyError(id_int, "Missing params");
            return;
        }
        auto tool_name = cJSON_GetObjectItem(params, "name");
        if (!cJSON_IsString(tool_name)) {
            rt_kprintf(TAG, "tools/call: Missing name");
            ReplyError(id_int, "Missing name");
            return;
        }
        auto tool_arguments = cJSON_GetObjectItem(params, "arguments");
        if (tool_arguments != nullptr && !cJSON_IsObject(tool_arguments)) {
            rt_kprintf(TAG, "tools/call: Invalid arguments");
            ReplyError(id_int, "Invalid arguments");
            return;
        }
        auto stack_size = cJSON_GetObjectItem(params, "stackSize");
        if (stack_size != nullptr && !cJSON_IsNumber(stack_size)) {
            rt_kprintf(TAG, "tools/call: Invalid stackSize");
            ReplyError(id_int, "Invalid stackSize");
            return;
        }
        DoToolCall(id_int, std::string(tool_name->valuestring), tool_arguments, stack_size ? stack_size->valueint : DEFAULT_TOOLCALL_STACK_SIZE);
    } else {
        rt_kprintf(TAG, "Method not implemented: %s", method_str.c_str());
        ReplyError(id_int, "Method not implemented: " + method_str);
    }
}

void McpServer::ReplyResult(int id, const std::string& result) {
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id) + ",\"result\":";
    payload += result;
    payload += "}";
    McpServer::SendmcpMessage(payload);
    //wsock_write(&g_xz_ws.clnt, payload, strlen(payload), OPCODE_TEXT)
}
void McpServer::ReplyError(int id, const std::string& message) {
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id);
    payload += ",\"error\":{\"message\":\"";
    payload += message;
    payload += "\"}}";
    McpServer::SendmcpMessage(payload);
    //wsock_write(&g_xz_ws.clnt, payload, strlen(payload), OPCODE_TEXT)
}

void McpServer::GetToolsList(int id, const std::string& cursor) {
    const int max_payload_size = 8000;
    std::string json = "{\"tools\":[";
    
    bool found_cursor = cursor.empty();
    auto it = tools_.begin();
    std::string next_cursor = "";
    
    while (it != tools_.end()) {
        // 如果我们还没有找到起始位置，继续搜索
        if (!found_cursor) {
            if ((*it)->name() == cursor) {
                found_cursor = true;
            } else {
                ++it;
                continue;
            }
        }
        
        // 添加tool前检查大小
        std::string tool_json = (*it)->to_json() + ",";
        if (json.length() + tool_json.length() + 30 > max_payload_size) {
            // 如果添加这个tool会超出大小限制，设置next_cursor并退出循环
            next_cursor = (*it)->name();
            break;
        }
        
        json += tool_json;
        ++it;
    }
    
    if (json.back() == ',') {
        json.pop_back();
    }
    
    if (json.back() == '[' && !tools_.empty()) {
        // 如果没有添加任何tool，返回错误
        rt_kprintf(TAG, "tools/list: Failed to add tool %s because of payload size limit", next_cursor.c_str());
        ReplyError(id, "Failed to add tool " + next_cursor + " because of payload size limit");
        return;
    }

    if (next_cursor.empty()) {
        json += "]}";
    } else {
        json += "],\"nextCursor\":\"" + next_cursor + "\"}";
    }
    
    ReplyResult(id, json);
}

void McpServer::DoToolCall(int id, const std::string& tool_name, const cJSON* tool_arguments, int stack_size) {
    auto tool_iter = std::find_if(tools_.begin(), tools_.end(), 
                                 [&tool_name](const McpTool* tool) { 
                                     return tool->name() == tool_name; 
                                 });
    
    if (tool_iter == tools_.end()) {
        rt_kprintf(TAG, "tools/call: Unknown tool: %s", tool_name.c_str());
        ReplyError(id, "Unknown tool: " + tool_name);
        return;
    }

    PropertyList arguments = (*tool_iter)->properties();
    //try {
        for (auto& argument : arguments) {
            bool found = false;
            if (cJSON_IsObject(tool_arguments)) {
                auto value = cJSON_GetObjectItem(tool_arguments, argument.name().c_str());
                if (argument.type() == kPropertyTypeBoolean && cJSON_IsBool(value)) {
                    argument.set_value<bool>(value->valueint == 1);
                    found = true;
                } else if (argument.type() == kPropertyTypeInteger && cJSON_IsNumber(value)) {
                    int value_int = value->valueint;
                    rt_kprintf("value_int: %d\n", value_int);
                    if (argument.has_range()) {
                    value_int = std::clamp(value_int, argument.min_value(), argument.max_value());
                }

                    argument.set_value<int>(value_int);
                    found = true;
                } else if (argument.type() == kPropertyTypeString && cJSON_IsString(value)) {
                    argument.set_value<std::string>(value->valuestring);
                    found = true;
                }
            }

            if (!argument.has_default_value() && !found) {
                rt_kprintf(TAG, "tools/call: Missing valid argument: %s", argument.name().c_str());
                ReplyError(id, "Missing valid argument: " + argument.name());
                return;
            }
        }
    // } catch (const std::runtime_error& e) {
    //     rt_kprintf(TAG, "tools/call: %s", e.what());
    //     ReplyError(id, e.what());
    //     return;
    // }

    // Start a task to receive data with stack size
    // esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
    // cfg.thread_name = "tool_call";
    // cfg.stack_size = stack_size;
    // cfg.prio = 1;
    // esp_pthread_set_cfg(&cfg);

    // Use a thread to call the tool to avoid blocking the main thread
    // tool_call_thread_ = std::thread([this, id, tool_iter, args = arguments]() {
    //     try {
           
    //     } catch (const std::runtime_error& e) {
    //         rt_kprintf(TAG, "tools/call: %s", e.what());
    //         ReplyError(id, e.what());
    //     }
    // });
    // tool_call_thread_.detach();
     ReplyResult(id, (*tool_iter)->Call(arguments));
}
