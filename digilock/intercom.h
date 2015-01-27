//
//  intercom.h
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#ifndef _IO_H_
#define _IO_H_

#include "db.h"
#include "req.h"
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>


void open_relay();


class Intercom {
public:
    Intercom(int aPinIntercomButtonOUT, int aPinIntercomBuzzerOUT, int aPinIntercomBuzzerIN, int aStartTime, int aEndTime);
    ~Intercom();
    
    void        SetCommonIntervals(int aCheatPressNum, int aCheatPressInterval, int aBuzzerMS, int aButtonMS);
    void        SetEnabled(bool aEnabled);
    int         GetStartTime();
    int         GetEndTime();
    bool        IsEnabled();
private:
    pthread_t   _thread;
    volatile bool        _enabled;
    int         _end_time;
    int         _start_time;
};


#endif /* defined(_IO_H_) */
