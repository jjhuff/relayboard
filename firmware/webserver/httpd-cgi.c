// the file was modified to use program memory to store data
// also CGI functionality was enhanced to allow parsing url parameters
//  Vladimir S. Fonov ~ vladimir.fonov <at> gmail.com

/*
 * Copyright (c) 2004, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: httpd.c,v 1.2 2006/06/11 21:46:38 adam Exp $
 */


#include "uip.h"
#include "psock.h"
#include "httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"
#include "ee_settings.h"

#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HTTPD_CGI_CALL(cgi_settings, "settings", run_settings);
HTTPD_CGI_CALL(cgi_relay_set,  "relay_set",  run_relay_set);
HTTPD_CGI_CALL(cgi_relay_status,  "relay_status",  run_relay_status);
HTTPD_CGI_CALL(cgi_welcome,  "welcome",  run_welcome);

static const struct httpd_cgi_call *calls[] =
{
    &cgi_settings,
    &cgi_relay_set,
    &cgi_relay_status,
    &cgi_welcome,
    NULL
};

uint8_t http_get_parameters_parse(char *par, uint8_t mx) {
    uint8_t count=0;
    uint8_t i=0;
    for(;par[i]&&i<mx;i++) {
        if(par[i]=='='){
            count++;
            par[i]=0;
        } else if(par[i]=='&')
            par[i]=0;
        }
    return count;
}

char * http_get_parameter_name(char *par, uint8_t cnt, uint8_t mx) {
    uint8_t i,j;
    cnt*=2;
    for(i=0,j=0;j<mx&&i<cnt;j++)
        if(!par[j]) i++;
    return j==mx?"":par+j;
}

char* http_get_parameter_value(char *par, uint8_t cnt, uint8_t mx) {
    uint8_t i,j;
    cnt*=2;
    cnt++;
    for(i=0,j=0;j<mx&&i<cnt;j++)
        if(!par[j]) i++;
    return j==mx?"":par+j;
}

void http_url_decode(const char *in, char *out, uint8_t mx) {
    uint8_t i,j;
    char tmp[3]={0,0,0};
    for(i=0,j=0;j<mx&&in[i];i++) {
        if(in[i]=='%') {
            tmp[0]=in[++i];
            tmp[1]=in[++i];
            out[j++]=(char)strtol(tmp,NULL,16);
        } else {
            out[j++]=in[i];
        }
    }
}


static PT_THREAD(nullfunction(struct httpd_state *s, PGM_P ptr))
{
  PSOCK_BEGIN(&s->sout);
  PSOCK_SEND_PSTR(&s->sout,PSTR("Not Implemented"));
  PSOCK_END(&s->sout);
}

httpd_cgifunction httpd_cgi(char *name)
{
    const struct httpd_cgi_call **f;

    /* Find the matching name in the table, return the function. */
    for(f = calls; *f != NULL; ++f) {
        if(strncmp((*f)->name, name, strlen((*f)->name)) == 0) {
            return (*f)->function;
        }
    }

  return nullfunction;
}

httpd_cgifunction httpd_cgi_P(PGM_P name) {
    const struct httpd_cgi_call **f;

    /* Find the matching name in the table, return the function. */
    for(f = calls; *f != NULL; ++f) {
        if(strncmp_P((*f)->name, name, strlen((*f)->name)) == 0) {
            return (*f)->function;
        }
    }
    return nullfunction;
}

static uint8_t decode_ip(char *in, uint8_t *out) {
    uint8_t i;
    char tmp[20];
    strncpy(tmp,in,sizeof(tmp));
    char *dig;
    dig=strtok(tmp,".");

    for(i=0 ; i<4 && dig ;i++,dig=strtok(NULL,"."))
        out[i]=(uint8_t)strtoul(dig,NULL,10);

    return i;
}

static PT_THREAD(run_settings(struct httpd_state *s, PGM_P ptr)) {
    uint8_t pcount;
    uint8_t ip[4];

    PSOCK_BEGIN(&s->sout);

    //check if there are parameters passed
    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0) {
        //walk through parameters
        for(uint8_t i=0; i<pcount; i++) {
            char *pname, *pval;
            pname= http_get_parameter_name(s->param, i, sizeof(s->param));
            pval = http_get_parameter_value(s->param, i, sizeof(s->param));

            if(!strcmp_P(pname, PSTR("dhcp")) ) {
                uint8_t enable_dhcp = (pval[0]=='1');
                eeprom_write_byte(&ee_enable_dhcp, enable_dhcp);

            } else if(!strcmp_P(pname, PSTR("ip")) ) {
                if(decode_ip(pval, ip) == 4)
                    eeprom_write_block(ip, &ee_ip_addr, 4);

            } else if(!strcmp_P(pname, PSTR("netmask")) ) {
                if(decode_ip(pval, ip) == 4)
                    eeprom_write_block(ip, &ee_net_mask, 4);

            } else if(!strcmp_P(pname, PSTR("gw")) ) {
                if(decode_ip(pval, ip) == 4)
                    eeprom_write_block(ip, &ee_gateway, 4);
            }
        }
        PSOCK_SEND_PSTR(&s->sout, PSTR("<b>Parameters Accepted, cycle power to make active!</b>"));
    }

    // Generate the output
    char temp[30];
    PSOCK_SEND_PSTR(&s->sout, PSTR("<form action='/settings.shtml' method='get'><table>"));

    // Print the MAC
    uint8_t eth_addr[6];
    PSOCK_SEND_PSTR(&s->sout, PSTR("<tr><td>MAC:</td><td>"));
    eeprom_read_block ((void *)eth_addr, (const void *)&ee_eth_addr, 6);
    snprintf_P(temp, sizeof(temp), PSTR("%02x:%02x:%02x:%02x:%02x:%02x"),
        eth_addr[0], eth_addr[1], eth_addr[2], eth_addr[3], eth_addr[4], eth_addr[5]);
    PSOCK_SEND_STR(&s->sout,temp);
    PSOCK_SEND_PSTR(&s->sout, PSTR("</td></tr>"));

    // DHCP Enabled
    PSOCK_SEND_PSTR(&s->sout, PSTR("<tr><td>DHCP:</td><td><select name='dhcp'>"));
    uint8_t enable_dhcp=eeprom_read_byte(&ee_enable_dhcp);
    if(enable_dhcp)
        PSOCK_SEND_PSTR(&s->sout, PSTR("<option value='1' selected>Enabled</option><option value='0'>Disabled</option>"));
    else
        PSOCK_SEND_PSTR(&s->sout, PSTR("<option value='1'>Enabled</option><option value='0' selected>Disabled</option>"));
    PSOCK_SEND_PSTR(&s->sout, PSTR("</td></tr>"));


    // IP address
    PSOCK_SEND_PSTR(&s->sout, PSTR("<tr><td>IP:</td><td><input type=\"text\" name=\"ip\" size=\"15\" maxlength=\"15\" value=\""));
    eeprom_read_block((void *)ip, (const void *)&ee_ip_addr, 4);
    snprintf_P(temp, sizeof(temp), PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    PSOCK_SEND_STR(&s->sout,temp);

    // Netmask
    PSOCK_SEND_PSTR(&s->sout,PSTR("\"></td></tr><tr><td>Netmask:</td><td><input type=\"text\" name=\"netmask\" size=\"15\" maxlength=\"15\" value=\""));
    eeprom_read_block((void *)ip, (const void *)&ee_net_mask, 4);
    snprintf_P(temp, sizeof(temp), PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    PSOCK_SEND_STR(&s->sout,temp);

    // Gateway
    PSOCK_SEND_PSTR(&s->sout,PSTR("\"/></td></tr><tr><td>Gateway:</td><td><input type=\"text\" name=\"gw\" size=\"15\" maxlength=\"15\" value=\""));
    eeprom_read_block((void *)ip, (const void *)&ee_gateway, 4);
    snprintf_P(temp, sizeof(temp), PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    PSOCK_SEND_STR(&s->sout,temp);

    PSOCK_SEND_PSTR(&s->sout,PSTR("\"/></td></tr><tr align=\"justify\"><td colspan=2><INPUT TYPE=submit VALUE=\"Submit\"></td></tr></table></form>"));

    PSOCK_END(&s->sout);
}

static PT_THREAD(run_welcome(struct httpd_state *s, PGM_P ptr)) {
    //NOTE:local variables are not preserved during the calls to proto socket functins
    char temp[30];
    sprintf_P(temp,PSTR("System time: %u ticks"),(unsigned int)clock_time());

    PSOCK_BEGIN(&s->sout);
    PSOCK_SEND_STR(&s->sout,temp);
    PSOCK_END(&s->sout);
}

static PT_THREAD(run_relay_status(struct httpd_state *s, PGM_P ptr)) {
    PSOCK_BEGIN(&s->sout);

    //char temp[30];
    //sprintf_P(temp,PSTR("<td span=2>DDRC: %02x PORTC: %02x</td>"), DDRC, PORTC);
    //PSOCK_SEND_STR(&s->sout,temp);


    if( PORTC & _BV(5) ) {
        PSOCK_SEND_PSTR(&s->sout, PSTR("<td>On</td>"));
    }else{
        PSOCK_SEND_PSTR(&s->sout, PSTR("<td>Off</td>"));
    }

    if( PORTC & _BV(4) ) {
        PSOCK_SEND_PSTR(&s->sout, PSTR("<td>On</td>"));
    } else {
        PSOCK_SEND_PSTR(&s->sout, PSTR("<td>Off</td>"));
    }

    PSOCK_END(&s->sout);
}

static PT_THREAD(run_relay_set(struct httpd_state *s, PGM_P ptr)) {
    PSOCK_BEGIN(&s->sout);
    uint8_t pcount;

    //check if there are parameters passed
    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0) {
        //walk through parameters
        for(uint8_t i=0; i<pcount; i++) {
            char *pname, *pval;
            pname= http_get_parameter_name(s->param, i, sizeof(s->param));
            pval = http_get_parameter_value(s->param, i, sizeof(s->param));

            uint8_t pin = 0;
            if(!strcmp_P(pname, PSTR("relay1")) ) {
                pin = _BV(5);
            } else if(!strcmp_P(pname, PSTR("relay2")) ) {
                pin = _BV(4);
            }

            if (!strcmp(pval, "On")) {
                PORTC |= pin;
            } else {
                PORTC &= ~pin;
            }

        }
    }

    // Generate the output
    PSOCK_SEND_PSTR(&s->sout, PSTR("<a href='/'>Done</a>"));

    PSOCK_END(&s->sout);
}
