#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "relay-httpd.h"
#include "uip/uip.h"

static void handle_connection(struct httpd_state *s);

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
      PSOCK_INIT(&s->sin, s->inputbuf, sizeof(s->inputbuf) );
//      PSOCK_INIT(&s->sout, s->inputbuf, sizeof(s->inputbuf));
//      PT_INIT(&s->outputpt);
      s->state = STATE_READ_REQ;
  }

  handle_connection(s);
}

static
PT_THREAD(send_headers(struct httpd_state *s)) {
    PSOCK_BEGIN(&s->sout);

    PSOCK_SEND_STR(&s->sout, "HTTP/1.0 200 OK\r\n");

//    PSOCK_SEND_STR(&s->sout, "Content-Type: text/html\r\n\r\n");

//    PSOCK_SEND_STR(&s->sout, "<html><body><form method='GET'>");
    //PSOCK_SEND_STR(&s->sout, "<input type='submit' name='relay1' value='On'>");
//    PSOCK_SEND_STR(&s->sout, "</form></body></html>");

    PSOCK_END(&s->sout);
}

static PT_THREAD(handle_output(struct httpd_state *s))
{
    PT_BEGIN(&s->outputpt);

    PT_WAIT_THREAD(&s->outputpt, send_headers(s));

    PSOCK_CLOSE(&s->sout);

    PT_END(&s->outputpt);
}

static PT_THREAD(handle_input(struct httpd_state *s))
{
    PSOCK_BEGIN(&s->sin);

    // Read the method
    PSOCK_READTO(&s->sin, ' ');

    if(strncmp(s->inputbuf, "GET", 3) == 0) {
        s->request_type = REQUEST_GET;
    } else if(strncmp(s->inputbuf, "POST", 4) == 0) {
        s->request_type = REQUEST_POST;
    } else {
        PSOCK_CLOSE_EXIT(&s->sin);
    }

    // Read the path
    PSOCK_READTO(&s->sin, ' ');
    if(s->inputbuf[0] != '/') {
        PSOCK_CLOSE_EXIT(&s->sin);
    }
    // Null terminate the path
    s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = 0;

    strncpy(s->path, s->inputbuf, sizeof(s->path)-1);

    s->state = STATE_SEND_RESP;

    // Read the request headers
    //while(1) {
    //    PSOCK_READTO(&s->sin, '\n');
    //}

    PSOCK_END(&s->sin);
}

static void handle_connection(struct httpd_state *s)
{
  handle_input(s);
//  if(s->state == STATE_SEND_RESP) {
//    handle_output(s);
//  }
}
