//
//  io.c
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#include "io.h"


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

#define CHEAT_PRESS_NUM             4
#define CHEAT_PRESS_INTERVAL_MS     3000
#define DO_BUZZER_DELAY_MS          3000
#define DO_BUTTON_DELAY_MS          3000

//#define PIN_INTERCOM_BUTTON         2
//#define PIN_INTERCOM_BUZZER         3
//#define PIN_INTERCOM_DO_BUZZ        4

void open_door() {
    printf("OPEN DOOR\n");
    digitalWrite(EPinIntercomButtonOUT, HIGH);
    usleep(DO_BUTTON_DELAY_MS * 1000);
    digitalWrite(EPinIntercomButtonOUT, LOW);
}

void ring_buzzer() {
    printf("RING BUZZER\n");
    digitalWrite(EPinIntercomBuzzerOUT, HIGH);
    usleep(DO_BUZZER_DELAY_MS * 1000);
    digitalWrite(EPinIntercomBuzzerOUT, LOW);
}

void setup() {
    pinMode(EPinIntercomButtonOUT, OUTPUT);
    pinMode(EPinIntercomBuzzerOUT, OUTPUT);
    pinMode(EPinIntercomBuzzerIN, INPUT);
    printf("SETUP DONE\n");
}


void loop_open() {
    while (digitalRead(EPinIntercomBuzzerIN) == HIGH) {
        open_door();
    }
}


void loop_cheat() {
    
    unsigned long ms = millis();
    
    if(digitalRead(EPinIntercomBuzzerIN) == HIGH) {
#ifdef LONGPRESS_ONLY
        while (digitalRead(EPinIntercomBuzzerIN) == HIGH) {
            // wait till buzz is unpressed
            delay(10);
        }
        
        if(millis() - ms > CHEAT_PRESS_INTERVAL_MS) {
            // pressed long enough, open doors
            open_door();
        }
#else
        unsigned char presses = 0;
        bool pressed = false;
        for (presses = 0; millis() - ms < CHEAT_PRESS_INTERVAL_MS; ) {
            while (digitalRead(EPinIntercomBuzzerIN) == HIGH) {
                // wait till buzz is unpressed
                usleep(10 * 1000);
                pressed = true;
            }
            
            if(pressed) {
                presses++;
                pressed = false;
                printf("%d PRESSES\n", presses);
            }
        }
        
        if(presses == CHEAT_PRESS_NUM) {
            // pressed the right number of times during the given interval, open doors
            open_door();
        }
#endif
        else {
            // not press long enough, make buzz ring
            ring_buzzer();
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
    }
    
    return NULL;
}


Intercom::Intercom(int aStartTime, int aEndTime, bool aStartNow) {
    _start_time = aStartTime;
    _end_time = aEndTime;
    if(aStartNow) SetEnabled(true);
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
    
    printf("INTERCOM: %s\n", _enabled ? "ON" : "OFF");
}




