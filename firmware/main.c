//  Vladimir S. Fonov ~ vladimir.fonov <at> gmail.com
#include "uNetConfigure.h"

#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "net/clock.h"
#include "net/timer.h"
#include "net/uip.h"
#include "net/nic.h"
#include "net/uip_arp.h"
#include "compiler.h"
#include "net_app/dhcpc.h"
#if UIP_SPLIT_HACK
#include "net/uip-split.h"
#elif UIP_EMPTY_PACKET_HACK
#include "net/uip-empty-packet.h"
#endif

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

void NetTask(void);

struct timer periodic_timer, arp_timer;
static clock_time_t timerCounter=0;

static struct uip_eth_addr uNet_eth_address;

//EEPROM parameters (TCP/IP parameters)
uint8_t EEMEM ee_enable_dhcp = USE_DHCP;

uint8_t EEMEM ee_eth_addr[6] = {UIP_ETHADDR0,UIP_ETHADDR1,UIP_ETHADDR2,UIP_ETHADDR3,UIP_ETHADDR4,UIP_ETHADDR5};

uint8_t EEMEM ee_ip_addr[4] = {UIP_IPADDR0, UIP_IPADDR1, UIP_IPADDR2, UIP_IPADDR3};

uint8_t EEMEM ee_net_mask[4] = {UIP_NETMASK0, UIP_NETMASK1, UIP_NETMASK2, UIP_NETMASK3};

uint8_t EEMEM ee_gateway[4] = {UIP_DRIPADDR0, UIP_DRIPADDR1, UIP_DRIPADDR2, UIP_DRIPADDR3};

// TCP/IP parameters in data memory
uint8_t _enable_dhcp;
uint8_t _eth_addr[6];
uint8_t _ip_addr[4];
uint8_t _net_mask[4];
uint8_t _gateway[4];

clock_time_t clock_time(void) {
  return timerCounter;
}

//! initialize clock
void clock_init(void) {
  TCCR0B = (1<<CS02)|(1<<CS00); //set prescaler (1024)
  TIMSK0 |=(1<<TOIE0);          //enable overflow interrupt
  TCCR0A = 0;                   //start the timer
  timerCounter = 0;
}

//! clock interrupt handling
ISR(TIMER0_OVF_vect) {
  timerCounter++;
}

//! log uip messages
void uip_log(char *msg) {
}

// Callback for when DHCP client has been configured.
void dhcpc_configured(const struct dhcpc_state *s) {
    uip_sethostaddr(&s->ipaddr);
    uip_setnetmask(&s->netmask);
    uip_setdraddr(&s->default_router);
}

int main(void) {
    uint8_t i;
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable Clock Division */
    clock_prescale_set(clock_div_2);

    _enable_dhcp=eeprom_read_byte(&ee_enable_dhcp);
    eeprom_read_block ((void *)_eth_addr, (const void *)&ee_eth_addr,6);

    for(i=0;i<6;i++)
        uNet_eth_address.addr[i]=_eth_addr[i];

    nic_init(_eth_addr);

    uip_ipaddr_t ipaddr;

    uip_setethaddr(uNet_eth_address);

    //init uIP
    uip_init();

    //init ARP cache
    uip_arp_init();

    // init periodic timer
    clock_init();

    sei();

    if(!_enable_dhcp) {
        eeprom_read_block ((void *)_ip_addr, (const void *)&ee_ip_addr,4);
        eeprom_read_block ((void *)_net_mask,(const void *)&ee_net_mask,4);
        eeprom_read_block ((void *)_gateway, (const void *)&ee_gateway,4);

        uip_ipaddr(ipaddr, _ip_addr[0], _ip_addr[1], _ip_addr[2], _ip_addr[3]);
        uip_sethostaddr(ipaddr);

        uip_ipaddr(ipaddr, _net_mask[0], _net_mask[1], _net_mask[2], _net_mask[3]);
        uip_setnetmask(ipaddr);

        uip_ipaddr(ipaddr, _gateway[0], _gateway[1], _gateway[2], _gateway[3]);
        uip_setdraddr(ipaddr);
    } else {
        uip_ipaddr(ipaddr, 0, 0, 0, 0);
        uip_sethostaddr(ipaddr);
        uip_setnetmask(ipaddr);
        uip_setdraddr(ipaddr);
    }

    timer_set(&periodic_timer, CLOCK_SECOND / 2);
    timer_set(&arp_timer, CLOCK_SECOND * 10);

    httpd_init();

    if(_enable_dhcp) {
        dhcpc_init(&uNet_eth_address.addr[0], 6);
        dhcpc_request();
    }

    while(1) {
        NetTask();
    }
}

void NetTask(void) {
    u8_t i;

    uip_len = nic_poll();
    if(uip_len > 0) {

        if(BUF->type == htons(UIP_ETHTYPE_IP)) {
            uip_arp_ipin();
            uip_input();

            /* If the above function invocation resulted in data that
                    should be sent out on the network, the global variable
                    uip_len is set to a value > 0. */
            if(uip_len > 0) {
                uip_arp_out();
#if UIP_SPLIT_HACK
                uip_split_output();
#elif UIP_EMPTY_PACKET_HACK
                uip_emtpy_packet_output();
#else
                nic_send();
#endif
            }
        } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
            uip_arp_arpin();
            /* If the above function invocation resulted in data that
                    should be sent out on the network, the global variable
                    uip_len is set to a value > 0. */
            if(uip_len > 0) {
                nic_send();
            }
        }

    } else if(timer_expired(&periodic_timer)) {
        timer_reset(&periodic_timer);
        for(i = 0; i < UIP_CONNS; i++) {
            uip_periodic(i);
            /* If the above function invocation resulted in data that
                    should be sent out on the network, the global variable
                    uip_len is set to a value > 0. */
            if(uip_len > 0) {
                uip_arp_out();
#if UIP_SPLIT_HACK
                uip_split_output();
#elif UIP_EMPTY_PACKET_HACK
                uip_emtpy_packet_output();
#else
                nic_send();
#endif
            }
        }

#if UIP_UDP
        for(i = 0; i < UIP_UDP_CONNS; i++) {
            uip_udp_periodic(i);
            /* If the above function invocation resulted in data that
                    should be sent out on the network, the global variable
                    uip_len is set to a value > 0. */
            if(uip_len > 0) {
                uip_arp_out();
                nic_send();
            }
        }
#endif /* UIP_UDP */

        /* Call the ARP timer function every 10 seconds. */
        if(timer_expired(&arp_timer)) {
            timer_reset(&arp_timer);
            uip_arp_timer();
        }
    }
    while(nic_sending()) {asm("nop");}; //wait untill packet is sent away
}

