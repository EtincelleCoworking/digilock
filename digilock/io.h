//
//  io.h
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#ifndef _IO_H_
#define _IO_H_

#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>

typedef enum {
    EPinLockRelay = 3, // GPIO3 / header 15
    // EIOLockManual,    // NO: faire du pur analog, pour sécurité !!!
    EPinIntercomBuzzerIN = 21,
    EPinIntercomBuzzerOUT = 22,
    EPinIntercomButtonOUT = 23,
    ELEDPinEntryOK = 7, // GPIO7 / header 7
    ELEDPinEntryNOK = 0, // GPIO0 / header 11
    ELEDPinEntryWait = 2, // GPIO2 / header 13
    ELEDPinExitOK = 4, // GPIO4 / header 16
    ELEDPinExitNOK = 5, // GPIO5 / header 18
    ELEDPinExitWait = 6, // GPIO6 / header 22
} EIOPin;



void open_relay();


class Intercom {
public:
    Intercom();
    ~Intercom();
    void        SetEnabled(bool aEnabled, int aStartTime, int aEndTime);
    int         GetStartTime();
    int         GetEndTime();
    bool        IsEnabled();
private:
    pthread_t   _thread;
    bool        _enabled;
    int         _end_time;
    int         _start_time;
};


#endif /* defined(_IO_H_) */
