#ifndef __RELAY_HTTPD_H__
#define __RELAY_HTTPD_H__
#include "uip/psock.h"

#define REQUEST_GET 0
#define REQUEST_POST 1

#define STATE_READ_REQ 0
#define STATE_SEND_RESP 1

struct httpd_state {
    struct psock sock;
    char inputbuf[50];
    uint8_t request_type;
    char path[50];
};


void relay_httpd_appcall(void);
void relay_httpd_init(void);

#if defined PORT_APP_MAPPER
    #define RELAY_HTTPD_APP_CALL_MAP {relay_httpd_appcall, 80, 0},
    struct httpd_state httpd_state_list[UIP_CONF_MAX_CONNECTIONS];
#else
    #define RELAY_HTTPD_APP_CALL_MAP
    #define UIP_APPCALL relay_httpd_appcall
    typedef struct httpd_state uip_tcp_appstate_t;
#endif

#endif /* __RELAY_HTTPD_H__ */
