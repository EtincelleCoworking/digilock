//
//  scanner.cpp
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
#include "scanner.h"

static char             sMode[] =   {'8', 'N', '1', 0};
static volatile bool    sLCDInit =  false;
static pthread_mutex_t  sLCDMutex;

#ifndef __APPLE__
    lcd_i2c_t sLCD = {0};
#else
#  define HIGH 1
#  define LOW 0
# define digitalWrite(p, i)  //printf("digitalWrite pin %d value %d\n", p, i)
#endif


static int gRelayIntervalMS;
static int gHeartbeatIntervalMS;
static int gPinLockRelay;
static int gPinEmergencyButton;
static char gLCDForbiddenLine0[16+1]="";
static char gLCDForbiddenLine1[16+1]="";
static char gLCDDefaultLine0[16+1]="";
static char gLCDDefaultLine1[16+1]="";
static bool gUseLCD;

static void * thread_open_relay(void * aIgnored) {
    printf("OPEN RELAY START\n");
    digitalWrite(gPinLockRelay, HIGH);
    usleep(gRelayIntervalMS * 1000);
    digitalWrite(gPinLockRelay, LOW);
    printf("OPEN RELAY STOP\n");
    return NULL;
}


static void * thread_scan_emergency(void * aIgnored) {
#ifndef __APPLE__
    while(1) {
        if(digitalRead(gPinEmergencyButton)) {
            // emergency button pressed ! open relay.
            printf("OPEN EMERGENCY START\n");
            digitalWrite(gPinLockRelay, HIGH);
            usleep(gRelayIntervalMS * 1000);
            digitalWrite(gPinLockRelay, LOW);
            printf("OPEN EMERGENCY STOP\n");
        }
        delay(10);
    }
#endif
    return NULL;
}


void Scanner::CreateEmergencyThread() {
    pthread_t thr;
    pthread_create(&thr, NULL, thread_scan_emergency, NULL);
}


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
            
            long long ts = millisecs();
            
            // finger pressed, continue if there was no finger before
            if(detect) {
                // turn off detection, will be turned on when finger is released
                detect = false;

                // turn on orange LED
                scanner->ShowLED(ELEDTypeWait, true);
                fps->CaptureFinger(false);
                int id = fps->Identify1_N();
                
                if(id < MAX_FGP_COUNT) {
                    // turn on green LED + open door
                    printf("%s OK for fingerprint ID %d\n", scanner->GetName(), id);
                    pthread_t thr;
                    pthread_create(&thr, NULL, thread_open_relay, NULL);
                    scanner->ShowLED(ELEDTypeOK, true);
                    scanner->ShowLCDMessage(NULL, db_get_user_name(-1, id, false));
                    db_insert_fingerprint_event(id, (int)(millisecs() - ts), scanner->GetEvent(), true);
                }
                else {
                    // turn on red LED
                    printf("%s forbidden for unknown fingerprint ID\n", scanner->GetName());
                    scanner->ShowLED(ELEDTypeNOK, true);
                    scanner->ShowLCDMessage(gLCDForbiddenLine0, gLCDForbiddenLine1);
                    db_insert_fingerprint_event(-1, 0, scanner->GetEvent(), false);
                }
            }
        }
        else {
            // finger not pressed, detection always on
            scanner->ShowLED(ELEDTypeWait, false);
            detect = true;
        }
        //usleep(10 * 1000);
        scanner->Heartbeat();

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

void lcd_default() {
#ifndef __APPLE__
    lcd_i2c_clear(&sLCD);
    lcd_i2c_gotoxy(&sLCD, 0, 0);
    lcd_i2c_puts(&sLCD, gLCDDefaultLine0);
    lcd_i2c_gotoxy(&sLCD, 0, 1);
    lcd_i2c_puts(&sLCD, gLCDDefaultLine1);
#endif
}


static void * thread_shut_lcd(void * aScanner) {
    usleep(2000 * 1000);
//    printf("shut lcd\n");
    lcd_default();
    return NULL;
}


void Scanner::ShowLCDMessage(const char * aLine0, const char * aLine1) {
#ifndef __APPLE__
//    printf("LCD 0: %s|\n", aLine0);
//    printf("LCD 1: %s|\n", aLine1);
    pthread_mutex_lock(&sLCDMutex);

    lcd_i2c_clear(&sLCD);
    lcd_i2c_gotoxy(&sLCD, 0, 0);

    if(aLine0) {
        lcd_i2c_puts(&sLCD, aLine0);
    }
    else {
        lcd_i2c_puts(&sLCD, _message);
    }
    lcd_i2c_gotoxy(&sLCD, 0, 1);
    if(aLine1) {
        lcd_i2c_puts(&sLCD, aLine1);
    }
    pthread_mutex_unlock(&sLCDMutex);
#endif
    pthread_create(&_lcd_thread, NULL, thread_shut_lcd, this);
}


static void * thread_shut_leds(void * aScanner) {
    // turn off all LEDs
    usleep(2000 * 1000);

    Scanner * scanner = (Scanner *)aScanner;
    scanner->ShutdownLEDs();
//    scanner->StopLCDThread();
//    printf("led thread stop\n");
    pthread_exit(NULL);
}


void Scanner::ShutdownLEDs() {
#ifndef __APPLE__

    digitalWrite(_led_ok, LOW);
    digitalWrite(_led_wait, LOW);
    digitalWrite(_led_nok, LOW);
    //printf("all leds off\n");
#endif
}

void Scanner::Heartbeat() {
    static long long ts = millisecs();
    if(millisecs() - ts > gHeartbeatIntervalMS) {

        digitalWrite(_led_ok, HIGH);
        usleep(10 * 1000);
        digitalWrite(_led_ok, LOW);
        usleep(10 * 1000);
        digitalWrite(_led_wait, HIGH);
        usleep(10 * 1000);
        digitalWrite(_led_wait, LOW);
        usleep(10 * 1000);
        digitalWrite(_led_nok, HIGH);
        usleep(10 * 1000);
        digitalWrite(_led_nok, LOW);

        ts = millisecs();
    }
}


void Scanner::ShowLED(ELEDType aLEDType, bool aEnable) {

#ifndef __APPLE__
    if(aEnable) {
        // turn off all LEDs
        ShutdownLEDs();
    }
    
    switch (aLEDType) {
        case ELEDTypeOK:
            digitalWrite(_led_ok, aEnable ? HIGH : LOW);
            if(aEnable) {
                pthread_t temp;
                pthread_create(&_led_thread, NULL, thread_shut_leds, this);
            }
            //printf("OK on\n");
            break;
        case ELEDTypeNOK:
            digitalWrite(_led_nok, aEnable ? HIGH : LOW);
            if(aEnable) {
                pthread_create(&_led_thread, NULL, thread_shut_leds, this);
            }
            //printf("NOK on\n");
            break;
        case ELEDTypeWait:
            digitalWrite(_led_wait, aEnable ? HIGH : LOW);
            //printf("WAIT on\n");
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
        if(0 != pthread_join(_scan_thread, NULL)) {
            printf("pthread_join error");
        }
    }
    else {
        // loop
        if(0 != pthread_create(&_scan_thread, NULL, scan_thread, this)) {
            printf("pthread_create error");
        }
    }
    
    printf("%s: %s\n", _name, _enabled ? "ON" : "OFF");
}


Scanner::Scanner(char * aPortName, int aBaudRate, bool aDebug, const char * aName, const char * aMessage, EEventType aEventType, int aPinLockRelay, int aLedOK, int aLedWait, int aLedNOK, int aRelayIntervalMS, int aHeartbeatIntervalMS, int aPinEmergencyButton, int aUseLCD) {
    strcpy(_name, aName);
    strcpy(_message, aMessage);
    _event = aEventType;
    _led_ok = aLedOK;
    _led_wait = aLedWait;
    _led_nok = aLedNOK;
    
    // TODO: create static routine to init these
    gRelayIntervalMS = aRelayIntervalMS;
    gHeartbeatIntervalMS = aHeartbeatIntervalMS;
    gPinEmergencyButton = aPinEmergencyButton;
    gUseLCD = !!aUseLCD;
    
    _fps = new FPS_GT511(aPortName, aBaudRate, sMode);
    _fps->UseSerialDebug = aDebug;
    _fps->Open();

#ifndef __APPLE__
    if(gUseLCD == true && sLCDInit == false) {
        if(lcd_i2c_setup(&sLCD, LCD_I2C_PCF8574_ADDRESS_DEFAULT) == -1) {
            // TODO error

        }
        else {
            pthread_mutex_init(&sLCDMutex, NULL);
            sLCDInit = true;
            lcd_i2c_init(&sLCD);
            LCD_I2C_BACKLIGHT_ON(&sLCD);
        }
    }
    wiringPiSetup();
    pinMode (_led_ok, OUTPUT);
    pinMode (_led_wait, OUTPUT);
    pinMode (_led_nok, OUTPUT);
    pinMode (gPinEmergencyButton, INPUT);
#endif
}


void Scanner::SetCommonStrings(char *aDefault0, char *aDefault1, char *aForbidden0, char *aForbidden1 ) {
    strcpy(gLCDDefaultLine0, aDefault0);
    strcpy(gLCDDefaultLine1, aDefault1);
    strcpy(gLCDForbiddenLine0, aForbidden0);
    strcpy(gLCDForbiddenLine1, aForbidden1);
    lcd_default();
}


Scanner::~Scanner() {
    _fps->Close();
    delete _fps;
    if(sLCDInit == true) {
        sLCDInit = false;
#ifndef __APPLE__
        lcd_i2c_clear(&sLCD);
        LCD_I2C_BACKLIGHT_OFF(&sLCD);
        pthread_mutex_destroy(&sLCDMutex);
#endif
    }
}







