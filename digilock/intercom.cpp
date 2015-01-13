//
//  intercom.cpp
//  digilock
//
//  Created by Olivier Huguenot on 26/12/2014.
//  Copyright (c) 2014 Etincelle Coworking. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#include "intercom.h"


#ifdef __APPLE__
#include <sys/time.h>
#  define HIGH      1
#  define LOW       0
#  define INPUT     1
#  define OUTPUT    0
#  define digitalWrite(p, l)    printf("dwrite pin %d: %d\n", p, l)
#  define digitalRead(p)        (0)

long long millis() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}
#define pinMode(p, o)           printf("pin mode %d: %d\n", p, o)
#else
#  include "wiringPi.h"
#endif



//#define LONGPRESS_ONLY

static bool gLongpressOnly = true;
static int gCheatPressNum;
static int gCheatPressInterval;
static int gBuzzerMS;
static int gButtonMS;

static int gPinIntercomButtonOUT;
static int gPinIntercomBuzzerOUT;
static int gPinIntercomBuzzerIN;

void Intercom::SetCommonIntervals(int aCheatPressNum, int aCheatPressInterval, int aBuzzerMS, int aButtonMS) {
    gCheatPressInterval = aCheatPressInterval;
    gCheatPressNum = aCheatPressNum;
    gButtonMS = aButtonMS;
    gBuzzerMS = aBuzzerMS;
    gLongpressOnly = false;
}


void open_door(int aNumPresses) {
    printf("OPEN DOOR START\n");
    digitalWrite(gPinIntercomButtonOUT, HIGH);
    usleep(gButtonMS * 1000);
    digitalWrite(gPinIntercomButtonOUT, LOW);
    printf("OPEN DOOR STOP\n");
    db_insert_intercom_event(aNumPresses, true);
}

void ring_buzzer(int aNumPresses) {
    printf("RING BUZZER START\n");
    digitalWrite(gPinIntercomBuzzerOUT, HIGH);
    usleep(gBuzzerMS * 1000);
    digitalWrite(gPinIntercomBuzzerOUT, LOW);
    printf("RING BUZZER STOP\n");
    db_insert_intercom_event(aNumPresses, true);
}


void loop_open() {
    while (digitalRead(gPinIntercomBuzzerIN) == HIGH) {
        open_door(-1);
    }
}


void loop_cheat() {
    
    unsigned long ms = millis();

    if(digitalRead(gPinIntercomBuzzerIN) == HIGH) {
//#ifdef LONGPRESS_ONLY
        if(gLongpressOnly) {
            while (digitalRead(gPinIntercomBuzzerIN) == HIGH) {
                // wait till buzz is unpressed
                usleep(10 * 1000);
            }

            if(millis() - ms > gCheatPressInterval) {
                // pressed long enough, open doors
                open_door(-1);
            }
            else {
                // not press long enough, make buzz ring
                ring_buzzer(-1);
            }
//#else
        }
        else {
            unsigned char presses = 0;
            bool pressed = false;
            for (presses = 0; millis() - ms < gCheatPressInterval; ) {
                while (digitalRead(gPinIntercomBuzzerIN) == HIGH) {
                    // wait till buzz is unpressed
                    usleep(10 * 1000); // debounce
                    pressed = true;
                }

                if(pressed) {
                    presses++;
                    pressed = false;
                    printf("%d PRESSES\n", presses);
                }
            }

            if(presses == gCheatPressNum) {
                // pressed the right number of times during the given interval, open doors
                open_door(gCheatPressNum);
            }
            else {
                // not press w/ correct code, make buzz ring
                ring_buzzer(presses);
            }
//#endif
        }
    }
}


void * loop_thread(void * aIntercom) {
    Intercom * intercom = (Intercom *)aIntercom;
    while(intercom->IsEnabled()) {
        time_t t = time(NULL);
        struct tm timez = *localtime(&t);
        if(timez.tm_hour > intercom->GetStartTime() && timez.tm_hour < intercom->GetEndTime()) {
            loop_open();
        }
        else {
            loop_cheat();
        }
        usleep(10 * 1000);
    }
    
    return NULL;
}


Intercom::Intercom(int aPinIntercomButtonOUT, int aPinIntercomBuzzerOUT, int aPinIntercomBuzzerIN, int aStartTime, int aEndTime) {
    _start_time = aStartTime;
    _end_time = aEndTime;

    gPinIntercomButtonOUT = aPinIntercomButtonOUT;
    gPinIntercomBuzzerOUT = aPinIntercomBuzzerOUT;
    gPinIntercomBuzzerIN = aPinIntercomBuzzerIN;
    
    pinMode(gPinIntercomButtonOUT, OUTPUT);
    pinMode(gPinIntercomBuzzerOUT, OUTPUT);
    pinMode(gPinIntercomBuzzerIN, INPUT);
}

Intercom::~Intercom() {
    
}

int Intercom::GetEndTime() {
    return _end_time;
}

int Intercom::GetStartTime() {
    return _start_time;
}

bool Intercom::IsEnabled() {
    return _enabled;
}

void Intercom::SetEnabled(bool aEnabled) {
    _enabled = aEnabled;
    if(_enabled == false) {
        // wait for thread to end
        if(0 != pthread_join(_thread, NULL)) {
            printf("pthread_join error");
        }
    }
    else {
        // loop
        if(0 != pthread_create(&_thread, NULL, loop_thread, this)) {
            printf("pthread_create error");
        }
    }
    
    printf("INTERCOM: %s ", _enabled ? "ON" : "OFF");
    if(aEnabled) {
        if(_end_time == _start_time)
            printf("CHEAT CODE ALWAYS ON\n");
        else
            printf("FREE OPEN START TIME %.2d:00 / END TIME %.2d:00\n", _start_time, _end_time);
    }
}




