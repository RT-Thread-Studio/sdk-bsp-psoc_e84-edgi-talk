#ifndef MCP_API_H
#define MCP_API_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void McpServer_ParseMessage(const char* message);

#ifdef __cplusplus
}
#endif

#endif // MCP_API_H