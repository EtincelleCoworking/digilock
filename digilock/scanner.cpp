//
//  scanner.cpp
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#include "scanner.h"
#include <string>

#ifndef __APPLE__
#  include "wiringPi.h"
#endif

#define FPS_BAUD        9600
static char sMode[] =   {'8', 'N', '1', 0};


static void * scan_thread(void * aScanner) {
    Scanner * scanner = (Scanner *)aScanner;
    FPS_GT511 * fps = scanner->GetFPS();
    
    
    // setup
    fps->SetLED(true);
    
    bool detect = true;
    bool led = true;    
    
    // loop
    while(scanner->IsEnabled()) {
        if(led == false) {
            led = !led;
            fps->SetLED(led);
        }
        if(fps->IsPressFinger()) {
            // finger pressed, continue if there was no finger before
            if(detect) {
                // turn on orange LED
                scanner->ShowLED(ELEDTypeWait, true);
                fps->CaptureFinger(false);
                int id = fps->Identify1_N();
                
                if(id < MAX_FGP_COUNT) {
                    // turn on green LED
                    printf("%s OK for fingerprint ID %d\n", scanner->GetName(), id);
                    scanner->ShowLED(ELEDTypeOK, true);
                    db_insert_event(id, scanner->GetEvent(), true);
                }
                else {
                    // turn on red LED
                    printf("%s forbidden for unknown fingerprint ID\n", scanner->GetName());
                    scanner->ShowLED(ELEDTypeNOK, true);
                    db_insert_event(-1, scanner->GetEvent(), false);
                }
                // turn off detection, will be turned on when finger is released
                detect = false;
            }
        }
        else {
            // finger not pressed, detection always on
            scanner->ShowLED(ELEDTypeWait, false);
            detect = true;
        }
        usleep(10 * 1000);
    }
    
    // scan thread exit : turn off led
    if(led == true) {
        fps->SetLED(false);
    }
    
    printf("%s thread exit.\n", scanner->GetName());
    return NULL;
}


void Scanner::Dump() {
    bool enabled = IsEnabled();
    if(enabled)
        this->SetEnabled(false);
    
    int retcode = -1;
    const int max = _fps->GetEnrollCount();
    byte * buffer = new byte[TEMPLATE_DATA_LEN];
    char name[32];
    for(int i = 0; i < max; i++) {
        memset(buffer, 0x0, TEMPLATE_DATA_LEN);
        retcode = _fps->GetTemplate(i, buffer);
        if(retcode != -1) {
            sprintf(name, "%s-template-%d.bin", _name, i);
            FILE * f = fopen(name, "w");
            for(int j = 0; j < TEMPLATE_DATA_LEN; j++) {
                fputc(buffer[j], f);
            }
            fclose(f);
            printf("Dumped template %d/%d...\n", i + 1, max);
        }

        _fps->SetLED(true);
        _fps->SetLED(false);
    }
    
    delete[] buffer;

    if(enabled)
        this->SetEnabled(true);
}


FPS_GT511 * Scanner::GetFPS() {
    return _fps;
}

const char * Scanner::GetName() {
    return _name;
}

EEventType Scanner::GetEvent() {
    return _event;
}

bool Scanner::IsEnabled() {
    return _enabled;
}


void Scanner::ShowLED(ELEDType aLEDType, bool aEnable) {
    
#ifndef __APPLE__
    if(aEnable) {
        // turn off all LEDs
        digitalWrite(_led_ok, LOW);
        digitalWrite(_led_wait, LOW);
        digitalWrite(_led_nok, LOW);
    }
    
    switch (aLEDType) {
        case ELEDTypeOK:
            digitalWrite(_led_ok, HIGH);
            break;
        case ELEDTypeNOK:
            digitalWrite(_led_nok, HIGH);
            break;
        case ELEDTypeWait:
            digitalWrite(_led_wait, HIGH);
            break;
        default:
            break;
    }
#endif
}

void Scanner::SetEnabled(bool aEnabled) {
    _enabled = aEnabled;
    if(_enabled == false) {
        // wait for thread to end
        if(0 != pthread_join(_thread, NULL)) {
            printf("pthread_join error");
        }
    }
    else {
        // loop
        if(0 != pthread_create(&_thread, NULL, scan_thread, this)) {
            printf("pthread_create error");
        }
    }
    
    printf("%s: %s\n", _name, _enabled ? "ON" : "OFF");
}


Scanner::Scanner(int aPort, bool aDebug, const char * aName, EEventType aEventType, int aLedOK, int aLedWait, int aLedNOK) {
    _name = aName;
    _event = aEventType;
    _led_ok = aLedOK;
    _led_wait = aLedWait;
    _led_nok = aLedNOK;
    
    _fps = new FPS_GT511(aPort, FPS_BAUD, sMode);
    _fps->UseSerialDebug = aDebug;
    _fps->Open();

#ifndef __APPLE__
    pinMode (_led_ok, OUTPUT);
    pinMode (_led_wait, OUTPUT);
    pinMode (_led_nok, OUTPUT);
#endif
}


Scanner::~Scanner() {
    _fps->Close();
    delete _fps;
}







