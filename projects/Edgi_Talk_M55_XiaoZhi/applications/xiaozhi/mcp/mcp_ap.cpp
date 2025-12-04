#include "mcp_api.h"
#include "mcp_server.h"
#include "cJSON.h"
#include <rtthread.h>
#include <string.h>


extern "C" 
{
    //注册IoT设备(可多个设备)
    void McpServer_ParseMessage(const char* message)
    {
        McpServer::GetInstance().ParseMessage(message);                
    }

} // extern "C"
