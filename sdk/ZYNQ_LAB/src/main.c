/*
 * Copyright (C) 2009 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include "PL_Control_Unit.h"
#include "xparameters.h"
#include "netif/xadapter.h"
#include "string.h"
#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif
#include "sleep.h"
#include "lwip/tcp.h"
#include "xil_cache.h"
#include "lwip/udp.h"
#include "lwip/init.h"
#include "xaxidma.h"
#include <inttypes.h>
#if LWIP_IPV6==1
#include "lwip/ip.h"
#else
#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif
#endif

const uint8_t BOARD_IP[4] 	= {192,168,1,130};
const uint8_t DST_IP[4] 	= {192,168,1,120}; 
#define Eth_Port				2023
#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID
#ifndef DDR_BASE_ADDR
#warning CHECK FOR THE VALID DDR ADDRESS IN XPARAMETERS.H, \
		 DEFAULT SET TO 0x01000000
#define MEM_BASE_ADDR		0x01000000
#endif

#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)
#define RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x004FFFFF)

/*PL to PS Data length in Byte*/
#define DMA_PACKET_SIZE 2048


/* defined by each RAW mode application */
void print_app_header();
int start_application();
int transfer_data();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

/* missing declaration in lwIP */
void lwip_init();

#if LWIP_IPV6==0
#if LWIP_DHCP==1
extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
#endif
#endif

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
static struct netif server_netif;
struct netif *echo_netif;
struct pbuf *P_1;
static struct udp_pcb *pcb;
ip_addr_t DEST_IP_ADDR;
u8 *TxBufferPtr;
u32 rcv_data;
const char START_COMMAND[] = "StartLog";
const char STOP_COMMAND[] = "StopLog";
u8 WRONG_RE[]={"WRONG COMMAND!"};


volatile u8 recv_data_ready =0;


XAxiDma AxiDma;
XAxiDma_Config *DmaConf;

#if LWIP_IPV6==1
void print_ip6(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf(" %x:%x:%x:%x:%x:%x:%x:%x\n\r",
			IP6_ADDR_BLOCK1(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK2(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK3(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK4(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK5(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK6(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK7(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK8(&ip->u_addr.ip6));

}
#else
void
print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void
print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}
#endif
void UDP_SEND(u8 *ptr, int size)
{
    struct pbuf *P_1 = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
    if (!P_1) return;

    if (P_1->len >= size)
    {
        memcpy(P_1->payload, ptr, size);
        udp_sendto(pcb, P_1, &DEST_IP_ADDR, Eth_Port);
    }

    pbuf_free(P_1);
}
/** Receive data on a udp session */
static void udp_recv_perf_traffic(void *arg, struct udp_pcb *tpcb,
		struct pbuf *p_rcv, const ip_addr_t *addr, u16_t port)
{
	if (!ip_addr_cmp(addr, &DEST_IP_ADDR) || Eth_Port != port) {
	    pbuf_free(p_rcv);
	    return; /*This is not the port and IP we was waiting for!*/
	}
	TxBufferPtr = malloc(p_rcv->len);
	memcpy((u8 *)TxBufferPtr,p_rcv->payload,p_rcv->len);

	/* START_COMMAND */
	if((p_rcv->len >= strlen(START_COMMAND)) &&
			memcmp(TxBufferPtr,START_COMMAND,strlen(START_COMMAND)) == 0)
	{
		recv_data_ready = 1;
		Set_Fs(10000000);
		Set_Sine_Wave_Frq(40000);
		PL_Control_Set_Logging_State(1);
	}
	/* STOP_COMMAND */
	else if((p_rcv->len >= strlen(STOP_COMMAND)) &&
			memcmp(TxBufferPtr,STOP_COMMAND,strlen(STOP_COMMAND)) == 0)
	{
		recv_data_ready = 1;
	}
	/*Unknown Command*/
	else printf("Unknown command received!\r\n");
	pbuf_free(p_rcv);
	free(TxBufferPtr);
}
void UDP_INIT(void)
{
	err_t err;

	/* Create Server PCB */
	pcb = udp_new();
	if (!pcb) {
		xil_printf("UDP server: Error creating PCB. Out of Memory\r\n");
		return;
	}

	err = udp_bind(pcb, IP_ADDR_ANY, Eth_Port);
	if (err != ERR_OK) {
		xil_printf("UDP server: Unable to bind to port");
		xil_printf(" %d: err = %d\r\n", Eth_Port, err);
		udp_remove(pcb);
		return;
	}

	/* specify callback to use for incoming connections */
	udp_recv(pcb, udp_recv_perf_traffic, NULL);

	return;
}


#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
int ProgramSi5324(void);
int ProgramSfpPhy(void);
#endif
#endif

#ifdef XPS_BOARD_ZCU102
#ifdef XPAR_XIICPS_0_DEVICE_ID
int IicPhyReset(void);
#endif
#endif
int main()
{

	pcb->local_port = Eth_Port;
	pcb->remote_port = Eth_Port;
	u8 *RxBufferPtr;
	RxBufferPtr = (u8 *)RX_BUFFER_BASE;
	/* Initialize the XAxiDma device.*/
	DmaConf = XAxiDma_LookupConfig(DMA_DEV_ID);
	XAxiDma_CfgInitialize(&AxiDma, DmaConf);

	/* Disable interrupts, we use polling mode */
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DEVICE_TO_DMA);
#if LWIP_IPV6==0
	ip_addr_t ipaddr, netmask, gw;

#endif
	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] =
	{ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	echo_netif = &server_netif;
#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
	ProgramSi5324();
	ProgramSfpPhy();
#endif
#endif

/* Define this board specific macro in order perform PHY reset on ZCU102 */
#ifdef XPS_BOARD_ZCU102
	if(IicPhyReset()) {
		xil_printf("Error performing PHY reset \n\r");
		return -1;
	}
#endif

	init_platform();

#if LWIP_IPV6==0
#if LWIP_DHCP==1
    ipaddr.addr = 0;
	gw.addr = 0;
	netmask.addr = 0;
#else
	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  BOARD_IP[0],BOARD_IP[1],BOARD_IP[2],BOARD_IP[3]); //10, 42,   0, 121
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);
	IP4_ADDR(&DEST_IP_ADDR, DST_IP[0], DST_IP[1], DST_IP[2], DST_IP[3]); // 10, 42,   0, 120

#endif
#endif

	lwip_init();

#if (LWIP_IPV6 == 0)
	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
#else
	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, NULL, NULL, NULL, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	echo_netif->ip6_autoconfig_enabled = 1;

	netif_create_ip6_linklocal_address(echo_netif, 1);
	netif_ip6_addr_set_state(echo_netif, 0, IP6_ADDR_VALID);

	print_ip6("\n\rBoard IPv6 address ", &echo_netif->ip6_addr[0].u_addr.ip6);

#endif
	netif_set_default(echo_netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(echo_netif);

#if (LWIP_IPV6 == 0)
#if (LWIP_DHCP==1)
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	dhcp_start(echo_netif);
	dhcp_timoutcntr = 24;

	while(((echo_netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
		xemacif_input(echo_netif);

	if (dhcp_timoutcntr <= 0) {
		if ((echo_netif->ip_addr.addr) == 0) {
			xil_printf("DHCP Timeout\r\n");
			xil_printf("Configuring default IP of 192.168.1.10\r\n");
			IP4_ADDR(&(echo_netif->ip_addr),  192, 168,   1, 10);
			IP4_ADDR(&(echo_netif->netmask), 255, 255, 255,  0);
			IP4_ADDR(&(echo_netif->gw),      192, 168,   1,  1);
		}
	}

	ipaddr.addr = echo_netif->ip_addr.addr;
	gw.addr = echo_netif->gw.addr;
	netmask.addr = echo_netif->netmask.addr;
#endif

	print_ip_settings(&ipaddr, &netmask, &gw);

#endif
	UDP_INIT();
	while (1) {
		xemacif_input(echo_netif);
		if(recv_data_ready)
		{
			/* transmit data to DDR */
			xemacif_input(echo_netif);
			XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) RxBufferPtr,DMA_PACKET_SIZE, XAXIDMA_DEVICE_TO_DMA);
			while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA))) {}
			Xil_DCacheInvalidateRange((UINTPTR)RX_BUFFER_BASE, DMA_PACKET_SIZE);				
			UDP_SEND((u8*)RxBufferPtr,DMA_PACKET_SIZE);
		}
	}
	/* never reached */
	cleanup_platform();

	return 0;
}
