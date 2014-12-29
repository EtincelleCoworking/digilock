//
//  led.cpp
//  digilock
//
//  Created by Olivier on 29/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#include "led.h"

void led_show(int aLed, bool aEnable) {
    led_show(aLed, aEnable, false, -1, true);
}

void led_show(int aLed, bool aEnable, bool aDisableOthers) {
    led_show(aLed, aEnable, false, -1, aDisableOthers);
}

void led_show(int aLed, bool aEnable, bool aBlink, int aDurationMS, bool aDisableOthers) {
 
}