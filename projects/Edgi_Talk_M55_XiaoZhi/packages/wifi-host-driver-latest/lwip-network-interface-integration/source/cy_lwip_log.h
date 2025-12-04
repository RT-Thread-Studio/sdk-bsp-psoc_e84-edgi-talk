/*
 * $ Copyright Cypress Semiconductor $
*/

#ifndef LIBS_WIFI_ETHERENT_MW_CORE_LWIP_CY_LWIP_LOG_H_
#define LIBS_WIFI_ETHERENT_MW_CORE_LWIP_CY_LWIP_LOG_H_

// #include "cy_log.h"

#ifdef ENABLE_CONNECTIVITY_MIDDLEWARE_LOGS
    #define cm_cy_log_msg cy_log_msg
#else
    #define cm_cy_log_msg(a,b,c,...)
#endif

#endif /* LIBS_WIFI_ETHERENT_MW_CORE_LWIP_CY_LWIP_LOG_H_ */
