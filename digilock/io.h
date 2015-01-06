//
//  io.h
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#ifndef _IO_H_
#define _IO_H_


//#ifdef __cplusplus
//extern "C" {
//#endif


typedef enum {
    EPinLockRelay = 3, // GPIO3 / header 15
    // EIOLockManual,    // NO: faire du pur analog, pour sécurité !!!
    EPinIntercom1_I,
    EPinIntercom1_0,
    EPinIntercom2_I,
    EPinIntercom2_0,
    ELEDPinEntryOK = 7, // GPIO7 / header 7
    ELEDPinEntryNOK = 0, // GPIO0 / header 11
    ELEDPinEntryWait = 2, // GPIO2 / header 13
    ELEDPinExitOK = 4, // GPIO4 / header 16
    ELEDPinExitNOK = 5, // GPIO5 / header 18
    ELEDPinExitWait = 6, // GPIO6 / header 22
} EIOPin;


//#ifdef __cplusplus
//} /* extern "C" */
//#endif

#endif /* defined(_IO_H_) */
