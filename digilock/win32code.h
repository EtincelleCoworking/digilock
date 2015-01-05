//
//  win32code.h
//  digilock
//
//  Created by Olivier on 03/01/2015.
//  Copyright (c) 2015 etincelle. All rights reserved.
//

#ifndef __digilock__win32code__
#define __digilock__win32code__

#include <stdio.h>
#include "rs232.h"
#include <stdint.h>
#include <sys/time.h>

// ============================================================================
// PORT OF WIN32 CODE
// ============================================================================


typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint32_t DWORD;


#define SB_OEM_PKT_SIZE			12
#define SB_OEM_HEADER_SIZE		2
#define SB_OEM_DEV_ID_SIZE		2
#define SB_OEM_CHK_SUM_SIZE		2

// Header Of Cmd and Ack Packets
#define STX1				0x55	//Header1
#define STX2				0xAA	//Header2

// Header Of Data Packet
#define STX3				0x5A	//Header1
#define STX4				0xA5	//Header2

#define PKT_ERR_START	-500
#define PKT_COMM_ERR	PKT_ERR_START+1
#define PKT_HDR_ERR		PKT_ERR_START+2
#define PKT_DEV_ID_ERR	PKT_ERR_START+3
#define PKT_CHK_SUM_ERR	PKT_ERR_START+4
#define PKT_PARAM_ERR	PKT_ERR_START+5

#define COMM_DEF_TIMEOUT 15000
#define gCommTimeOut COMM_DEF_TIMEOUT

#define ACK_OK					 0x30
#define NACK_INFO				0x31


// Structure Of Cmd and Ack Packets
typedef struct {
    uint8_t 	Head1;
    uint8_t 	Head2;
    uint16_t	wDevId;
    int		nParam;
    uint16_t	wCmd;// or nAck
    uint16_t 	wChkSum;
} SB_OEM_PKT;


enum
{
    OEM_NONE					= -2000,
    OEM_COMM_ERR,
};

WORD oemp_CalcChkSumOfCmdAckPkt( SB_OEM_PKT* pPkt );
uint16_t oemp_CalcChkSumOfDataPkt( uint8_t* pDataPkt, int nSize );
int comm_send(uint8_t comport, BYTE* pbuf, int nsize, int ntimeout);
int comm_recv(uint8_t comport, BYTE* pbuf, int nsize, int ntimeout);

int oemp_SendCmdOrAck(uint8_t comport, WORD wDevID, WORD wCmdOrAck, int nParam );
long long GetTickCount();
int oemp_ReceiveCmdOrAck(uint8_t comport, WORD wDevID, WORD* pwCmdOrAck, int* pnParam );
int oem_CommandRun(uint8_t comport, WORD wCmd, int nCmdParam);
int oemp_ReceiveData(uint8_t comport, WORD wDevID, BYTE* pBuf, int nSize );
int oem_add_template(int comport, BYTE* data,  int nPos);


#endif /* defined(__digilock__win32code__) */
