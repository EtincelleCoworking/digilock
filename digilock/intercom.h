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
#include <string.h>


void open_relay();

typedef enum {
	    ERingTypeCheatOK = 0,
	        ERingTypeCheatNOK,
		    ERingTypeNoCheat
} ERingType;

class Intercom {
public:
    Intercom(int aPinIntercomButtonOUT, int aPinIntercomBuzzerOUT, int aPinIntercomBuzzerIN, int aStartTime, int aEndTime, char * location_slug, char * location_key);
    ~Intercom();
    
    void        SetRingFiles(char * aCheatOK, char * aCheatNOK, char * aNoCheat);
    void        SetCommonIntervals(int aCheatPressNum, int aCheatPressInterval, int aBuzzerMS, int aButtonMS);
    void        SetEnabled(bool aEnabled);
    int         GetStartTime();
    int         GetEndTime();
    bool        IsEnabled();
    void        loop_cheat();
    void        loop_open();
    void        ring_buzzer(ERingType aRingType);
    void        open_door(int aNumPresses);

private:
    pthread_t   _thread;
    volatile bool        _enabled;
    int         _end_time;
    int         _start_time;
    char *      _location_slug;
    char *      _location_key;
};


#endif /* defined(_IO_H_) */
