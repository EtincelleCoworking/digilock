//
//  scanner.h
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#ifndef __digilock__scanner__
#define __digilock__scanner__

#include <stdio.h>
#include <pthread.h>
#include "db.h"
#include "req.h"
#include "FPS_GT511Linux.h"
#ifndef __APPLE__
#  include "wiringPi.h"
#endif


typedef enum {
    ELEDTypeOK = 0,
    ELEDTypeNOK,
    ELEDTypeWait,
} ELEDType;


class Scanner
{
    
public:
    Scanner(int aPort, bool aDebug, const char * aName, EEventType aEventType, int aLedOK, int aLedWait, int aLedNOK);
    ~Scanner();

    
    void            SetEnabled(bool aEnabled);
    FPS_GT511 *     GetFPS();
    bool            IsEnabled();
    void            ShowLED(ELEDType aLEDType, bool aEnable);
    const char *    GetName();
    EEventType      GetEvent();
    void            ShutdownLEDs();
    void            Dump();
    void            ShowLCDMessage(char * aLine0, char * aLine1);
    
private:
//    bool            _available;
    bool            _debug;
    volatile bool   _enabled;
    int             _led_ok;
    int             _led_nok;
    int             _led_wait;
    FPS_GT511 *     _fps;
    const char *    _name;
    EEventType      _event;
    pthread_t       _thread;
    
};


#endif /* defined(__digilock__scanner__) */
