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
//#include "io.h"
#include "req.h"
#include "FPS_GT511Linux.h"
#ifndef __APPLE__
#  include "wiringPi.h"
#  include "lcd_i2c.h"
#endif


typedef enum {
    ELEDTypeOK = 0,
    ELEDTypeNOK,
    ELEDTypeWait,
} ELEDType;


class Scanner
{
    
public:
    Scanner(char * aPortName, int aBaudRate, bool aDebug, const char * aName, const char * aWelcome, EEventType aEventType, int aPinLockRelay, int aLedOK, int aLedWait, int aLedNOK, int aRelayIntervalMS, int aHeartbeatIntervalMS, int aPinEmergencyButton);
    ~Scanner();

    void            SetCommonStrings(char * aDefault0, char * aDefault1, char * aForbidden0, char * aForbidden1);
    void            SetEnabled(bool aEnabled);
    FPS_GT511 *     GetFPS();
    bool            IsEnabled();
    bool            IsEmergencyPressed();
    void            ShowLED(ELEDType aLEDType, bool aEnable);
    const char *    GetName();
    EEventType      GetEvent();
    void            ShutdownLEDs();
    void            Dump();
    void            Heartbeat();
    void            ShowLCDMessage(const char * aLine0, const char * aLine1);
    static void     CreateEmergencyThread();
    
private:
    bool            _debug;
    volatile bool   _enabled;
    int             _led_ok;
    int             _led_nok;
    int             _led_wait;
    FPS_GT511 *     _fps;
    char            _name[16+1]; // TODO: get this from LCD char width
    char            _message[16+1];
    EEventType      _event;
    pthread_t       _scan_thread;
    pthread_t       _led_thread;
    pthread_t       _lcd_thread;
    
};


#endif /* defined(__digilock__scanner__) */
