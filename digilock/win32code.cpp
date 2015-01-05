//
//  win32code.c
//  digilock
//
//  Created by Olivier on 03/01/2015.
//  Copyright (c) 2015 etincelle. All rights reserved.
//

#include "win32code.h"


WORD oemp_CalcChkSumOfCmdAckPkt( SB_OEM_PKT* pPkt )
{
    WORD wChkSum = 0;
    BYTE* pBuf = (BYTE*)pPkt;
    int i;
    
    for(i=0;i<(sizeof(SB_OEM_PKT)-SB_OEM_CHK_SUM_SIZE);i++)
        wChkSum += pBuf[i];
    return wChkSum;
}

uint16_t oemp_CalcChkSumOfDataPkt( uint8_t* pDataPkt, int nSize )
{
    int i;
    uint16_t wChkSum = 0;
    uint8_t* pBuf = (uint8_t*)pDataPkt;
    
    for(i=0;i<nSize;i++)
        wChkSum += pBuf[i];
    return wChkSum;
}


int comm_send(uint8_t comport, BYTE* pbuf, int nsize, int ntimeout)
{
    return RS232_SendBuf(comport, pbuf, nsize);
}



int comm_recv(uint8_t comport, BYTE* pbuf, int nsize, int ntimeout)
{
    return RS232_PollComport(comport, pbuf, nsize);
}

int oemp_SendCmdOrAck(uint8_t comport, WORD wDevID, WORD wCmdOrAck, int nParam )
{
    SB_OEM_PKT pkt;
    int nSentBytes;
    
    pkt.Head1 = (BYTE)STX1;
    pkt.Head2 = (BYTE)STX2;
    pkt.wDevId = wDevID;
    pkt.wCmd = wCmdOrAck;
    pkt.nParam = nParam;
    pkt.wChkSum = oemp_CalcChkSumOfCmdAckPkt( &pkt );
    
    nSentBytes = comm_send(comport, (BYTE*)&pkt, SB_OEM_PKT_SIZE, gCommTimeOut );
    if( nSentBytes != SB_OEM_PKT_SIZE )
        return PKT_COMM_ERR;
    
    return 0;
}

long long GetTickCount() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}

WORD gwDevID = 1;
WORD gwLastAck = 0;
int  gwLastAckParam = 0;
long long  gnPassedTime = 0;



int oem_CommandRun(uint8_t comport, WORD wCmd, int nCmdParam)
{
    if( oemp_SendCmdOrAck(comport, gwDevID, wCmd, nCmdParam ) < 0 )
        return OEM_COMM_ERR;
    gnPassedTime = GetTickCount();
    if( oemp_ReceiveCmdOrAck(comport, gwDevID, &gwLastAck, &gwLastAckParam ) < 0 )
        return OEM_COMM_ERR;
    gnPassedTime = GetTickCount() - gnPassedTime;
    return 0;
}

int oemp_ReceiveCmdOrAck(uint8_t comport, WORD wDevID, WORD* pwCmdOrAck, int* pnParam )
{
    SB_OEM_PKT pkt;
    int nReceivedBytes;
    
    if( ( pwCmdOrAck == NULL ) ||
       ( pnParam == NULL ) )
    {
        return PKT_PARAM_ERR;
    }
    
    nReceivedBytes = comm_recv(comport, (BYTE*)&pkt, SB_OEM_PKT_SIZE, gCommTimeOut );
    if( nReceivedBytes != SB_OEM_PKT_SIZE )
        return PKT_COMM_ERR;
    
    if( ( pkt.Head1 != STX1 ) ||
       ( pkt.Head2 != STX2 ) )
    {
        return PKT_HDR_ERR;
    }
    
    if( pkt.wDevId != wDevID )
        return PKT_DEV_ID_ERR;
    
    if( pkt.wChkSum != oemp_CalcChkSumOfCmdAckPkt( &pkt ) )
        return PKT_CHK_SUM_ERR;
    
    *pwCmdOrAck = pkt.wCmd;
    *pnParam = pkt.nParam;
    
    return 0;
}

int oemp_SendData(int comport, WORD wDevID, BYTE* pBuf, int nSize )
{
    WORD wChkSum = 0;
    BYTE Buf[4], *pCommBuf;
    int nSentBytes;
    
    if( pBuf == NULL )
        return PKT_PARAM_ERR;
    
    Buf[0] = (BYTE)STX3;
    Buf[1] = (BYTE)STX4;
    *((WORD*)(&Buf[SB_OEM_HEADER_SIZE])) = wDevID;
    
    wChkSum = oemp_CalcChkSumOfDataPkt( Buf, SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE  );
    wChkSum += oemp_CalcChkSumOfDataPkt( pBuf, nSize );
    
    
    ////////////// pc start ///////////////
    pCommBuf = new BYTE[nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE+SB_OEM_CHK_SUM_SIZE];
    memcpy(pCommBuf, Buf, SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE);
    memcpy(pCommBuf+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE, pBuf, nSize);
    *(WORD*)(pCommBuf+nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE) = wChkSum;
    
    nSentBytes = comm_send(comport, pCommBuf, nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE+SB_OEM_CHK_SUM_SIZE, gCommTimeOut );
    if( nSentBytes != nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE+SB_OEM_CHK_SUM_SIZE )
    {
        if(pCommBuf)
            delete pCommBuf;
        return PKT_COMM_ERR;
    }
    
    if(pCommBuf)
        delete pCommBuf;
    ////////////// pc end ///////////////
    
    return 0;
}


int oemp_ReceiveData(uint8_t comport, WORD wDevID, BYTE* pBuf, int nSize )
{
    WORD wReceivedChkSum, wChkSum;
    BYTE Buf[4],*pCommBuf;
    int nReceivedBytes;
    
    if( pBuf == NULL )
        return PKT_PARAM_ERR;
    
    
    /*AVW modify*/
    pCommBuf = new BYTE[nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE+SB_OEM_CHK_SUM_SIZE];
    nReceivedBytes = comm_recv(comport, pCommBuf, nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE+SB_OEM_CHK_SUM_SIZE, gCommTimeOut );
    if( nReceivedBytes != nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE+SB_OEM_CHK_SUM_SIZE )
    {
        if(pCommBuf)
            delete pCommBuf;
        return PKT_COMM_ERR;
    }
    memcpy(Buf, pCommBuf, SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE);
    memcpy(pBuf, pCommBuf+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE, nSize);
    wReceivedChkSum = *(WORD*)(pCommBuf+nSize+SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE);
    if(pCommBuf)
        delete pCommBuf;
    ////////////// pc end ///////////////
    
    if( ( Buf[0] != STX3 ) ||
       ( Buf[1] != STX4 ) )
    {
        return PKT_HDR_ERR;
    }
    
    if( *((WORD*)(&Buf[SB_OEM_HEADER_SIZE])) != wDevID )
        return PKT_DEV_ID_ERR;
    
    wChkSum = oemp_CalcChkSumOfDataPkt( Buf, SB_OEM_HEADER_SIZE+SB_OEM_DEV_ID_SIZE  );
    wChkSum += oemp_CalcChkSumOfDataPkt( pBuf, nSize );
    
    if( wChkSum != wReceivedChkSum ) 
        return PKT_CHK_SUM_ERR;
    /*AVW modify*/
    return 0;
}


#define CMD_ADD_TEMPLATE		0x71
#define FP_TEMPLATE_SIZE        498

int oem_add_template(int comport, BYTE *data, int nPos)
{
    if( oem_CommandRun(comport, CMD_ADD_TEMPLATE, nPos ) < 0 )
        return OEM_COMM_ERR;
    
    if(gwLastAck == ACK_OK) {
        if( oemp_SendData(comport, gwDevID, /*&gbyTemplate[0]*/data, FP_TEMPLATE_SIZE ) < 0 )
            return OEM_COMM_ERR;
        
        if( oemp_ReceiveCmdOrAck(comport, gwDevID, &gwLastAck, &gwLastAckParam ) < 0 )
            return OEM_COMM_ERR;
    }
    
    return 0;
}
