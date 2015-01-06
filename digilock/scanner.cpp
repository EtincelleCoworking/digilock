//
//  scanner.cpp
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#include "scanner.h"
#ifndef __APPLE__
//#  include "lcdpcf8574.h"
#  define LCD_DEFAULT_LINE_0    (char *)"Etincelle      "
#  define LCD_DEFAULT_LINE_1    (char *)"  Coworking    "
#  define LCD_WELCOME_LINE_0    (char *)"Bienvenue      "
#  define LCD_FORBIDDEN_LINE_0  (char *)"Acces non      "
#  define LCD_FORBIDDEN_LINE_1  (char *)" autorise !    "

#endif


#define FPS_BAUD        9600
static char sMode[] =   {'8', 'N', '1', 0};


#ifndef __APPLE__
//  lcdpcf8574 sLCD(0x27, 0, 0, 0); // TODO: beware of i2c address !!
#endif

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
                // turn off detection, will be turned on when finger is released
                detect = false;

                // turn on orange LED
                scanner->ShowLED(ELEDTypeWait, true);
                fps->CaptureFinger(false);
                int id = fps->Identify1_N();
                
                if(id < MAX_FGP_COUNT) {
                    // turn on green LED
                    printf("%s OK for fingerprint ID %d\n", scanner->GetName(), id);

                    char lcdname[16] = "";
                    sprintf(lcdname, "%.16s", db_get_user_name(-1, id, false));
                    scanner->ShowLED(ELEDTypeOK, true);
                    scanner->ShowLCDMessage(LCD_WELCOME_LINE_0, lcdname);

                    db_insert_event(id, scanner->GetEvent(), true);

                }
                else {
                    // turn on red LED
                    printf("%s forbidden for unknown fingerprint ID\n", scanner->GetName());
                    scanner->ShowLED(ELEDTypeNOK, true);
                    scanner->ShowLCDMessage(LCD_FORBIDDEN_LINE_0, LCD_FORBIDDEN_LINE_1);

                    db_insert_event(-1, scanner->GetEvent(), false);
                }
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


static void * thread_shut_lcd(void * aScanner) {
    usleep(2000 * 1000);
    printf("shut lcd\n");
//    Scanner * scanner = (Scanner *)aScanner;
//    sLCD.lcd_puts(LCD_DEFAULT_LINE_0, 0, 0);
//    sLCD.lcd_puts(LCD_DEFAULT_LINE_1, 1, 0);
//    printf("LCD thread stop\n");
//    pthread_exit(NULL);
}


void Scanner::ShowLCDMessage(const char * aLine0, const char * aLine1) {

    printf("LCD 0: %s\n", aLine0);
    printf("LCD 1: %s\n", aLine1);


//    sLCD.lcd_puts((char *)aLine0, 0, 0);
//    sLCD.lcd_puts((char *)aLine1, 1, 0);

//    pthread_create(&_lcd_thread, NULL, thread_shut_lcd, this);
}


static void * thread_shut_leds(void * aScanner) {
    // turn off all LEDs
    usleep(2000 * 1000);

    Scanner * scanner = (Scanner *)aScanner;
    scanner->ShutdownLEDs();
//    scanner->StopLCDThread();
    printf("led thread stop\n");
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
//    sLCD.lcd_clear();
//    sLCD.lcd_puts(LCD_DEFAULT_LINE_0, 0, 0);
//    sLCD.lcd_puts(LCD_DEFAULT_LINE_1, 1, 0);
    wiringPiSetup();
    pinMode (_led_ok, OUTPUT);
    pinMode (_led_wait, OUTPUT);
    pinMode (_led_nok, OUTPUT);
#endif
}


Scanner::~Scanner() {
    _fps->Close();
    delete _fps;
}







