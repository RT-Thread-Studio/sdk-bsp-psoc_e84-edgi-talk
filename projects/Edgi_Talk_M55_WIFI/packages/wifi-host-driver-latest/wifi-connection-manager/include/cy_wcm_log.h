/*
 * $ Copyright Cypress Semiconductor $
*/

#ifndef LIBS_WCM_INCLUDE_CY_WCM_LOG_H_
#define LIBS_WCM_INCLUDE_CY_WCM_LOG_H_

// #include "cy_log.h"

#ifdef ENABLE_WCM_LOGS
    #define cy_wcm_log_msg cy_log_msg
#else
    #define cy_wcm_log_msg(a,b,c,...)
#endif

#define cy_wps_assert(error_string, assertion)         do { if (!(assertion) ){ printf( (error_string) ); } } while (0)

#endif /* LIBS_WCM_INCLUDE_CY_WCM_LOG_H_ */
