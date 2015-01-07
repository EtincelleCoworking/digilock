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

void open_door(int aNumPresses) {
    printf("OPEN DOOR START\n");
    digitalWrite(EPinIntercomButtonOUT, HIGH);
    usleep(DO_BUTTON_DELAY_MS * 1000);
    digitalWrite(EPinIntercomButtonOUT, LOW);
    printf("OPEN DOOR STOP\n");
    db_insert_intercom_event(aNumPresses, true);
}

void ring_buzzer(int aNumPresses) {
    printf("RING BUZZER START\n");
    digitalWrite(EPinIntercomBuzzerOUT, HIGH);
    usleep(DO_BUZZER_DELAY_MS * 1000);
    digitalWrite(EPinIntercomBuzzerOUT, LOW);
    printf("RING BUZZER STOP\n");
    db_insert_intercom_event(aNumPresses, true);
}


void loop_open() {
    while (digitalRead(EPinIntercomBuzzerIN) == HIGH) {
        open_door(-1);
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
            open_door(CHEAT_PRESS_NUM);
        }
#endif
        else {
            // not press long enough, make buzz ring
            ring_buzzer(presses);
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


Intercom::Intercom() {
    pinMode(EPinIntercomButtonOUT, OUTPUT);
    pinMode(EPinIntercomBuzzerOUT, OUTPUT);
    pinMode(EPinIntercomBuzzerIN, INPUT);
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

void Intercom::SetEnabled(bool aEnabled, int aStartTime, int aEndTime) {
    _enabled = aEnabled;
    if(_enabled == false) {
        // wait for thread to end
        if(0 != pthread_join(_thread, NULL)) {
            printf("pthread_join error");
        }
    }
    else {
        // loop
        _start_time = aStartTime;
        _end_time = aEndTime;
        if(0 != pthread_create(&_thread, NULL, loop_thread, this)) {
            printf("pthread_create error");
        }
    }
    
    printf("INTERCOM: %s ", _enabled ? "ON" : "OFF");
    if(aEnabled) {
        if(aStartTime == aEndTime)
            printf("CHEAT CODE ALWAYS ON\n");
        else
            printf("FREE OPEN START TIME %.2d:00 / END TIME %.2d:00\n", aStartTime, aEndTime);
    }

}




