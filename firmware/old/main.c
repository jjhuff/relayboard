#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>

#include "uip/timer.h"

#include "global-conf.h"
#include "uip/uip_arp.h"
#include "enc28j60.h"

#include <string.h>
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

unsigned int network_read(void){
	return ((uint16_t) enc28j60PacketReceive(UIP_BUFSIZE, (uint8_t *)uip_buf));
}

void network_send(void){
    enc28j60PacketSend(uip_len, (uint8_t *)uip_buf, 0, 0);
}

int main(void)
{
	enc28j60Init();

	CLKPR = (1<<CLKPCE);	//Change prescaler
	CLKPR = (1<<CLKPS0);	//Use prescaler 2
	enc28j60Write(ECOCON, 1 & 0x7);	//Get a 25MHz signal from enc28j60

	int i;
	uip_ipaddr_t ipaddr;
	struct timer periodic_timer, arp_timer;

	clock_init();

	timer_set(&periodic_timer, CLOCK_SECOND / 2);
	timer_set(&arp_timer, CLOCK_SECOND * 10);

	uip_init();

	struct uip_eth_addr mac = {{UIP_ETHADDR0, UIP_ETHADDR1, UIP_ETHADDR2, UIP_ETHADDR3, UIP_ETHADDR4, UIP_ETHADDR5}};

	uip_setethaddr(mac);

	relay_httpd_init();

//#ifdef __DHCPC_H__
//	dhcpc_init(&mac, 6);
//#else
    uip_ipaddr(ipaddr, 192,168,2,55);
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, 192,168,2,1);
    uip_setdraddr(ipaddr);
    uip_ipaddr(ipaddr, 255,255,255,0);
    uip_setnetmask(ipaddr);

//#endif /*__DHCPC_H__*/

	while(1){
		uip_len = network_read();

		if(uip_len > 0) {
			if(BUF->type == htons(UIP_ETHTYPE_IP)){
				uip_arp_ipin();
				uip_input();
				if(uip_len > 0) {
					uip_arp_out();
					network_send();
				}
			}else if(BUF->type == htons(UIP_ETHTYPE_ARP)){
				uip_arp_arpin();
				if(uip_len > 0){
					network_send();
				}
			}

		}else if(timer_expired(&periodic_timer)) {
			timer_reset(&periodic_timer);

			for(i = 0; i < UIP_CONNS; i++) {
				uip_periodic(i);
				if(uip_len > 0) {
					uip_arp_out();
					network_send();
				}
			}

			if(timer_expired(&arp_timer)) {
				timer_reset(&arp_timer);
				uip_arp_timer();
			}
		}
	}
	return 0;
}

void uip_log(char *m)
{
	//TODO: Get debug information out here somehow, does anybody know a smart way to do that?
}


/*---------------------------------------------------------------------------*/

/*#ifdef __DHCPC_H__
void dhcpc_configured(const struct dhcpc_state *s)
{
	uip_sethostaddr(s->ipaddr);
	uip_setnetmask(s->netmask);
	uip_setdraddr(s->default_router);
	//resolv_conf(s->dnsaddr);
	//red_high();
}
#endif
*/
/*---------------------------------------------------------------------------*/
