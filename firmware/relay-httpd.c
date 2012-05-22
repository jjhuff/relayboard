#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "relay-httpd.h"
#include "uip/uip.h"

static int handle_connection(struct httpd_state *s);

void relay_httpd_init(void)
{
    uip_listen(HTONS(80));
}

#if defined PORT_APP_MAPPER
void relay_httpd_appcall(void)
{
    struct httpd_state *s = &(httpd_state_list[0]);
#else
void relay_httpd_appcall(void)
{
    struct httpd_state *s = &(uip_conn->appstate);
#endif
  if(uip_connected()) {
      PSOCK_INIT(&s->sock, s->inputbuf, sizeof(s->inputbuf) - 1);
  }

  handle_connection(s);
}

static int handle_connection(struct httpd_state *s)
{
    PSOCK_BEGIN(&s->sock);

    // Read the method
    PSOCK_READTO(&s->sock, ' ');
    //TODO: switch these to progmem
    if(strncmp(s->inputbuf, "GET", 3) == 0) {
        s->request_type = REQUEST_GET;
    } else if(strncmp(s->inputbuf, "POST", 4) == 0) {
        s->request_type = REQUEST_POST;
    } else {
        PSOCK_CLOSE_EXIT(&s->sock);
    }

    // Read the path
    PSOCK_READTO(&s->sock, ' ');
    if(s->inputbuf[0] != '/') {
        PSOCK_CLOSE_EXIT(&s->sock);
    }
    // Null terminate the path
    s->inputbuf[PSOCK_DATALEN(&s->sock) - 1] = 0;

    strncpy(s->path, s->inputbuf, sizeof(s->path)-1);

    // Read the request headers
    while(1) {
        PSOCK_READTO(&s->sock, '\n');
        // Look for a empty line (\r\n)
        if(PSOCK_DATALEN(&s->sock) <= 2) {
            break;
        }
    }

    PSOCK_SEND_STR(&s->sock, "HTTP/1.0 200 OK\r\n");
    PSOCK_SEND_STR(&s->sock, "Content-Type: text/plain\r\n");
    PSOCK_SEND_STR(&s->sock, "\r\n");
    PSOCK_SEND_STR(&s->sock, "Hello World, From a relay httpd.");
    PSOCK_SEND_STR(&s->sock, s->path);

    PSOCK_CLOSE(&s->sock);
    PSOCK_END(&s->sock);
}

