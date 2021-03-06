/* ------------------------------------------------------------------------ */
/**
 * \addtogroup e2forlife_w5500
 * @{
 */
/**
 * Copyright (c) 2015, E2ForLife.com
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of E2ForLife.com nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file tcp.c
 * \author Chuck Erhardt (chuck@e2forlife.com)
 * 
 * Implementation of the TCP protocol using the internal WizNET protocol stack
 * in the iEthernet devices.
 */
/* ======================================================================== */
#include <cytypes.h>
#include <cylib.h>
#include <string.h>

#include "`$INSTANCE_NAME`.h"

extern uint8 `$INSTANCE_NAME`_socketStatus[`$INSTANCE_NAME`_MAX_SOCKETS];

/* ------------------------------------------------------------------------ */
uint8 `$INSTANCE_NAME`_UdpOpen( uint16 port )
{
	uint8 socket;
	uint8 status;
	uint8 tries;
	
	tries = 0;
	status = `$INSTANCE_NAME`_SR_CLOSED;
	do {
		socket = `$INSTANCE_NAME`_SocketOpen(port, `$INSTANCE_NAME`_PROTO_UDP);
	
		if (socket < `$INSTANCE_NAME`_MAX_SOCKETS) {
			`$INSTANCE_NAME`_Send(`$INSTANCE_NAME`_SREG_SR, `$INSTANCE_NAME`_SOCKET_BASE(socket), 0, &status, 1);
			if (status != `$INSTANCE_NAME`_SR_UDP) {
				`$INSTANCE_NAME`_SocketClose(socket,0);
				socket = 0xFF;
			}
		}
		++tries;
	}
	while ( (tries < 5) && (status != `$INSTANCE_NAME`_SR_UDP) );
	
	return socket;
}
/* ------------------------------------------------------------------------ */
/**
 * \brief Send a block of data using UDP.
 */
uint16 `$INSTANCE_NAME`_UdpSend(uint8 socket, uint32 ip, uint16 port, uint8 *buffer, uint16 len, uint8 flags)
{
	uint16 tx_length;	

	tx_length = `$INSTANCE_NAME`_GetTxLength(socket,len,flags);
	
	/* fix endian-ness */
	port = CYSWAP_ENDIAN16(port);
	/* setup destination information */
	`$INSTANCE_NAME`_Send(`$INSTANCE_NAME`_SREG_DIPR,`$INSTANCE_NAME`_SOCKET_BASE(socket),1,(uint8*)&ip,4);
	`$INSTANCE_NAME`_Send(`$INSTANCE_NAME`_SREG_DPORT, `$INSTANCE_NAME`_SOCKET_BASE(socket),1,(uint8*)&port,2);

	`$INSTANCE_NAME`_WriteTxData(socket, buffer, tx_length, flags);
	
	return tx_length;
	
}
/* ------------------------------------------------------------------------ */
uint16 `$INSTANCE_NAME`_UdpReceive(uint8 socket, uint8 *header, uint8 *buffer, uint16 len, uint8 flags)
{
	uint16 rx_size;
	uint16 bytes;
	uint16 ptr;
	
	bytes = 0;
	/*
	 * request the length of the data block available for reading, but, add
	 * the header size (8 bytes) to the length of data requested to account
	 * for the header sitting in the Rx Buffers.
	 */
	do {
		rx_size = `$INSTANCE_NAME`_RxDataReady( socket );
	}
	while ( (rx_size < 8) && (flags&`$INSTANCE_NAME`_TXRX_FLG_WAIT) );
	
	/*
	 * if there is data to read from the buffer...
	 */
	if (rx_size > 7) {
		/* 
		 * calculate the number of bytes to receive using the available data
		 * and the requested length of data.
		 */
		bytes = (rx_size > len) ? len : rx_size;
		/* Read the starting memory pointer address, and endian correct */
		`$INSTANCE_NAME`_Send( `$INSTANCE_NAME`_SREG_RX_RD, `$INSTANCE_NAME`_SOCKET_BASE(socket),0,(uint8*)&ptr,2);
		ptr = CYSWAP_ENDIAN16( ptr );
		/* Read the UDP header block from the memory */
		`$INSTANCE_NAME`_Send( ptr,`$INSTANCE_NAME`_RX_BASE(socket),0,header,8);
		ptr += 8;
		/* read the number of bytes to read from the UDP header */
		bytes = header[6];
		bytes = (bytes<<8) + header[7];
		
		/*
		 * Retrieve the length of data from the received UDP packet, starting
		 * right after the end of the packet header.
		 */
		`$INSTANCE_NAME`_Send( ptr, `$INSTANCE_NAME`_RX_BASE(socket),0,buffer,bytes);
		/* 
		 * Calculate the new buffer pointer location, endian correct, and
		 * update the pointer register within the W5500 socket registers
		 */
		ptr += bytes;
		ptr = CYSWAP_ENDIAN16( ptr );
		`$INSTANCE_NAME`_Send(`$INSTANCE_NAME`_SREG_RX_RD, `$INSTANCE_NAME`_SOCKET_BASE(socket),1,(uint8*)&ptr,2);
		/*
		 * when all of the available data was read from the message, execute
		 * the receive command
		 */
		`$INSTANCE_NAME`_ExecuteSocketCommand( socket, `$INSTANCE_NAME`_CR_RECV );
		
	}
	
	return bytes;
}
/* ------------------------------------------------------------------------ */
/** \todo Open Multi-cast socket */
/* ------------------------------------------------------------------------ */
/** \todo Send Multi-cast data */
/* ======================================================================== */
/** @} */
/* [] END OF FILE */
