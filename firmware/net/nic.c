//  Vladimir S. Fonov ~ vladimir.fonov <at> gmail.com
// based on original code by
//                          edi87 [at] fibertel.com.ar
//                              Jonathan Granade

#include "uNetConfigure.h"

#include <stdio.h>
#include <avr/pgmspace.h>
#include "nic.h"
#include "enc28j60.h"

void nic_init(uint8_t *eth_addr) {
    enc28j60Init(eth_addr);
}

void nic_send(void)
{
    enc28j60PacketSend((u8 *)uip_buf, uip_len);
}

uint16_t nic_poll(void)
{
    uint16_t packetLength;

    packetLength = enc28j60BeginPacketReceive();

    // if there's no packet or an error - exit without ending the operation
    if( !packetLength )
      return 0;

    // drop anything too big for the buffer
    if( packetLength > UIP_BUFSIZE ) {
      enc28j60EndPacketReceive();
      return 0;
    }

    // copy the packet data into the uIP packet buffer
    enc28j60PacketReceive( uip_buf, packetLength );
    enc28j60EndPacketReceive();

    return packetLength;
}


uint8_t nic_sending(void)
{
    return enc28j60PollPacketSending();
}
