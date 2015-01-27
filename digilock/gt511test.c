//
//  gt511test.c
//  digilock
//
//  Created by Olivier on 21/01/2015.
//  Copyright (c) 2015 etincelle. All rights reserved.
//

#include <stdio.h>
#include "gt511.h"


int gt511test_main() {
   
    GT511 * device = GT511_Init("/dev/tty.usbserial", TRUE);
    if(device) {
        GT511_SetLED(device, TRUE);
        GT511_SetLED(device, FALSE);
        GT511_SetLED(device, TRUE);
        GT511_SetLED(device, FALSE);
        GT511_SetLED(device, TRUE);
        GT511_SetLED(device, FALSE);
        GT511_SetLED(device, TRUE);
        GT511_SetLED(device, FALSE);
        GT511_SetLED(device, TRUE);
        
        
        while(1) {
            
            if(GT511_IsFingerPressed(device)) {
                int id;
                if(!GT511_Identify(device, &id)) {
                    printf("Identified %d\n", id);
                }
            }
            
            usleep(10);
        }
    
        
        uint8_t * template = GT511_GetTemplate(device, 0);
        for(int i = 0; i < TEMPLATE_DATA_LEN; i++) {
            printf("%.2X ", template[i]);
        }
        
        
        
        
        
        printf("Enroll count: %d\n", GT511_GetEnrollCount(device));
    }
    return 0;
    
}