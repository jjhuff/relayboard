// modified for the ENC28J60 Rev. B7 Silicon Errata
//  Vladimir S. Fonov ~ vladimir.fonov <at> gmail.com
//
// File Name    : 'enc28j60.c'
// Title        : Microchip ENC28J60 Ethernet Interface Driver
// Author       : Pascal Stang (c)2005
// Created      : 9/22/2005
// Revised      : 9/22/2005
// Version      : 0.1
// Target MCU   : Atmel AVR series
// Editor Tabs  : 4
//
// Description  : This driver provides initialization and transmit/receive
//  functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//

/************************************************************
*
*   This is the general driver for the ENC28J60
*   I changed some things to make it work with uIP
*   Some files of uIP have changes too.
*
*                               edi87 [at] fibertel.com.ar
*                               Jonathan Granade
*
************************************************************/

#include "compiler.h"
#include "uip.h"
#include "enc28j60.h"
#include "enc28j60def.h"
#include <util/delay.h>

u8 Enc28j60Bank;
u16 NextPacketPtr;

u8 enc28j60ReadOp(u8 op, u8 address)
{
    u8 data;

    // assert CS
    ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_SS_PIN);

    // issue read command
    SPDR = op | (address & ADDR_MASK);
    while(!(SPSR & (1<<SPIF)));
    // read data
    SPDR = 0x00;
    while(!(SPSR & (1<<SPIF)));
    // do dummy read if needed
    if(address & 0x80)
    {
        SPDR = 0x00;
        while(!(SPSR & (1<<SPIF)));
    }
    data = SPDR;

    // release CS
    ENC28J60_CONTROL_PORT |= (1<<ENC28J60_SS_PIN);

    return data;
}

void enc28j60WriteOp(u8 op, u8 address, u8 data)
{
    // assert CS
    ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_SS_PIN);

    // issue write command
    SPDR = op | (address & ADDR_MASK);
    while(!(SPSR & (1<<SPIF)));
    // write data
    SPDR = data;
    while(!(SPSR & (1<<SPIF)));

    // release CS
    ENC28J60_CONTROL_PORT |= (1<<ENC28J60_SS_PIN);
}

void enc28j60ReadBuffer(u16 len, u8* data)
{
    // assert CS
    ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_SS_PIN);

    // issue read command
    SPDR = ENC28J60_READ_BUF_MEM;
    while(!(SPSR & (1<<SPIF)));
    while(len--)
    {
        // read data
        SPDR = 0x00;
        while(!(SPSR & (1<<SPIF)));
        *data++ = SPDR;
    }
    // release CS
    ENC28J60_CONTROL_PORT |= (1<<ENC28J60_SS_PIN);
}

void enc28j60WriteBuffer(u16 len, u8* data)
{
    // assert CS
    ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_SS_PIN);

    // issue write command
    SPDR = ENC28J60_WRITE_BUF_MEM;
    while(!(SPSR & (1<<SPIF)));
    while(len--)
    {
        // write data
        SPDR = *data++;
        while(!(SPSR & (1<<SPIF)));
    }
    // release CS
    ENC28J60_CONTROL_PORT |= (1<<ENC28J60_SS_PIN);
}

void enc28j60SetBank(u8 address)
{
    // set the bank (if needed)
    if((address & BANK_MASK) != Enc28j60Bank)
    {
        // set the bank
        enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
        enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
        Enc28j60Bank = (address & BANK_MASK);
    }
}

u8 enc28j60Read(u8 address)
{
    // set the bank
    enc28j60SetBank(address);
    // do the read
    return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

u16 enc28j60Read16(u8 address)
{
  u16 data=enc28j60Read(address);
  data|=enc28j60Read(address+1)<<8;
  return data;
}

void enc28j60Write(u8 address, u8 data)
{
    // set the bank
    enc28j60SetBank(address);
    // do the write
    enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60Write16(u8 address, u16 data)
{
  enc28j60Write(address,data&0xff);
  enc28j60Write(address+1,data>>8);
}


u16 enc28j60PhyRead(u8 address)
{
    u16 data;

    // Set the right address and start the register read operation
    enc28j60Write(MIREGADR, address);
    enc28j60Write(MICMD, MICMD_MIIRD);

    // wait until the PHY read completes
    while(enc28j60Read(MISTAT) & MISTAT_BUSY);

    // quit reading
    enc28j60Write(MICMD, 0x00);

    // get data value
    data  = enc28j60Read(MIRDL);
    data<<=8;
    data |= enc28j60Read(MIRDH);
    // return the data
    return data;
}

void enc28j60PhyWrite(u8 address, u16 data)
{
    // set the PHY register address
    enc28j60Write(MIREGADR, address);

    // write the PHY data
    enc28j60Write(MIWRL, data);
    enc28j60Write(MIWRH, data>>8);

    // wait until the PHY write completes
    while(enc28j60Read(MISTAT) & MISTAT_BUSY);
}


void enc28j60SoftwareReset(void)
{
    // perform system reset
    enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);

    _delay_ms(20); //wait for 20 ms
}


void enc28j60Init(uint8_t *eth_addr)
{
    // initialize I/O
    sbi(ENC28J60_CONTROL_DDR,  ENC28J60_SS_PIN);
    sbi(ENC28J60_CONTROL_PORT, ENC28J60_SS_PIN);
#ifdef ENC28J60_ENABLE_RESET
  sbi(ENC28J60_CONTROL_PORT, ENC28J60_RESET_PIN);
  sbi(ENC28J60_CONTROL_DDR,  ENC28J60_RESET_PIN);
#endif //ENC28J60_ENABLE_RESET
#ifdef ENC28J60_ENABLE_INT
  sbi(ENC28J60_CONTROL_PORT, ENC28J60_INT_PIN);
  cbi(ENC28J60_CONTROL_DDR,  ENC28J60_INT_PIN);
#endif //ENC28J60_ENABLE_INT

    // setup SPI I/O pins
    sbi(PORTB, ENC28J60_SCK_PIN);   // set SCK hi
    sbi(ENC28J60_CONTROL_DDR, ENC28J60_SCK_PIN);    // set SCK as output
    cbi(ENC28J60_CONTROL_DDR, ENC28J60_MISO_PIN);   // set MISO as input
    sbi(ENC28J60_CONTROL_DDR, ENC28J60_MOSI_PIN);   // set MOSI as output
    sbi(ENC28J60_CONTROL_DDR, ENC28J60_SS_PIN);   // SS must be output for Master mode to work

    // initialize SPI interface
    // master mode
    sbi(SPCR, MSTR);
    // select clock phase positive-going in middle of data
    cbi(SPCR, CPOL);
    // Data order MSB first
    cbi(SPCR,DORD);

    // switch to f/4 2X = f/2 bitrate
    cbi(SPCR, SPR0);
    cbi(SPCR, SPR1);
    sbi(SPSR, SPI2X);

    // enable SPI
    sbi(SPCR, SPE);

    enc28j60SoftwareReset();

    enc28j60Write(ECOCON, 1 & 0x7); //Get a 25MHz signal from enc28j60

    // do bank 0 stuff
    // initialize receive buffer
    // 16-bit transfers, must write low byte first
    // set receive buffer start address
    NextPacketPtr = ENC28J60_RXSTART_INIT;

    enc28j60Write16(ERXSTL, ENC28J60_RXSTART_INIT);
    // set receive pointer address
    enc28j60Write16(ERXRDPTL, ENC28J60_RXSTART_INIT);
    // set receive buffer end
    enc28j60Write16(ERXNDL, ENC28J60_RXSTOP_INIT);
    // set transmit buffer start
    enc28j60Write16(ETXSTL, ENC28J60_TXSTART_INIT);

    // allow Unicast to US and Broad cast packets with correct CRC
    enc28j60Write(ERXFCON, ERXFCON_CRCEN|ERXFCON_UCEN|ERXFCON_BCEN);//ERXFCON_UCEN||ERXFCON_PMEN

    // do bank 2 stuff
    // enable MAC receive
    enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    // bring MAC out of reset
    enc28j60Write(MACON2, 0x00);
    // enable automatic padding and CRC operations
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
    // enc28j60Write(MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
    // set inter-frame gap (non-back-to-back)
    enc28j60Write(MAIPGL, 0x12);
    enc28j60Write(MAIPGH, 0x0C);
    // set inter-frame gap (back-to-back)
    enc28j60Write(MABBIPG, 0x12);
    // Set the maximum packet size which the controller will accept
    enc28j60Write16(MAMXFLL, ENC28J60_MAX_FRAMELEN);

    // do bank 3 stuff
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    enc28j60Write(MAADR5, eth_addr[0]);
    enc28j60Write(MAADR4, eth_addr[1]);
    enc28j60Write(MAADR3, eth_addr[2]);
    enc28j60Write(MAADR2, eth_addr[3]);
    enc28j60Write(MAADR1, eth_addr[4]);
    enc28j60Write(MAADR0, eth_addr[5]);

    // no loopback of transmitted frames
    enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

    // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
    enc28j60PhyWrite(PHLCON,0x476);
    //enc28j60PhyWrite(PHLCON, 0x880); //all on

    // switch to bank 0
    // enable interrutps
    enc28j60Write(EIE, EIE_INTIE|EIE_PKTIE);

    // Reenable receive logic
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

}

// link status
u8 enc28j60linkup(void)
{
  // bit 10 (= bit 3 in upper reg)
  return (enc28j60PhyRead(PHSTAT2) && (1<<10))?1:0;
}

void enc28j60PacketSend(u8 * packet, u16 len)
{
    // Check no transmit in progress
    while (enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
    {
        // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
        if( (enc28j60Read(EIR) & EIR_TXERIF) ) {
            enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
            enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
        }
    }

    // Set the write pointer to start of transmit buffer area
    enc28j60Write16(EWRPTL, ENC28J60_TXSTART_INIT);
    // Set the TXND pointer to correspond to the packet size given
    enc28j60Write16(ETXNDL, (ENC28J60_TXSTART_INIT+len));

    // write per-packet control byte
    enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // copy the packet into the transmit buffer
    enc28j60WriteBuffer(len, packet);

    // send the contents of the transmit buffer onto the network
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

u8 enc28j60PollPacketSending(void)
{
    return enc28j60Read(ECON1) & ECON1_TXRTS;
}

u16 enc28j60BeginPacketReceive(void)
{
    u16 rxstat;
    u16 len;

    // check if a packet has been received and buffered
    if( enc28j60Read(EPKTCNT) ==0 )
        return 0;

    // Set the read pointer to the start of the received packet
    enc28j60Write16(ERDPTL, NextPacketPtr);

    // read the next packet pointer
    NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
    // read the packet length
    len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
    // read the receive status
    rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
  len-=4;

//  limit retrieve length
// (we reduce the MAC-reported length by 4 to remove the CRC)
//  if(len>maxlen) len=maxlen;
//  if(len<=0) return 0;
//  else return len;
  if ((rxstat & 0x80)==0){
    // invalid
    len=0;
  }

    return len;
}

void enc28j60PacketReceive(u8 * packet, u16 maxlen)
{
    // copy the packet from the receive buffer
    enc28j60ReadBuffer(maxlen, packet);
}

void enc28j60EndPacketReceive(void)
{
    if( ((NextPacketPtr - 1) < ENC28J60_RXSTART_INIT) || ((NextPacketPtr - 1) > ENC28J60_RXSTOP_INIT))
        enc28j60Write16(ERXRDPTL, ENC28J60_RXSTOP_INIT);
    else
        enc28j60Write16(ERXRDPTL, NextPacketPtr-1);
    // Move the RX read pointer to the start of the next received packet
    // This frees the memory we just read out
    // decrement the packet counter indicate we are done with this packet
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
}

