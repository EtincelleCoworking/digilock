//
//  main.cpp
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
#include "FPS_GT511Linux.h"
#include "db.h"
#include "req.h"
#include "intercom.h"
#include "scanner.h"
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include "iniparser.h"

#ifdef __APPLE__
#  include <sys/malloc.h>
#  include <pthread/pthread.h>
#else
#  include <malloc.h>
#  include <pthread.h>
#endif

#define COMMAND_PASSWORD        "sesame"
#define COMMAND_YES             "yes"

// thread
#define COMMAND_BLINK_START     "blink start"
#define COMMAND_BLINK_STOP      "blink stop"

#define COMMAND_QUIT            "quit"
#define COMMAND_RESTART         "restart"
#define COMMAND_ENROLL          "enroll"
#define COMMAND_COUNT           "count"
#define COMMAND_ENTRY_STOP      "entry stop"
#define COMMAND_EXIT_STOP       "exit stop"
#define COMMAND_INTERCOM_STOP   "intercom stop"
#define COMMAND_ENTRY_START     "entry start"
#define COMMAND_EXIT_START      "exit start"
#define COMMAND_INTERCOM_START  "intercom start"
#define COMMAND_DUMP_ENTRY      "dump entry"
#define COMMAND_DUMP_EXIT       "dump exit"


// database
#define COMMAND_CREATE_USER     "create user"
#define COMMAND_EMPTY_FGP       "delete all fingerprints"
#define COMMAND_DELETE_FGP      "delete fingerprint"
#define COMMAND_DELETE_USER     "delete user"
#define COMMAND_SYNC            "sync fingerprints"
#define COMMAND_RELOAD          "reload fingerprints"


static volatile bool            sBlinkLoop =            false;
static Scanner *                gScanEntry =            NULL;
static Scanner *                gScanExit =             NULL;
static Intercom *               gScanIntercom =         NULL;
#define                         gScanEnroll             gScanExit
#define                         BUFFER_LEN              256
char                            sBuffer[BUFFER_LEN] =   "";
static long long                gWatchdogTS =           0;
static int                      gWatchdogIntervalMS =   0;

void getline() {
    fgets(sBuffer, BUFFER_LEN, stdin);
    char *p = strchr(sBuffer, '\n');
    if (p) {
        *p = 0;
    }
    else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
}


void cleanup() {
    // close libcurl + sqlite
    req_cleanup();
    db_close();

    // stop scans
    gScanEntry->SetEnabled(false);
    gScanExit->SetEnabled(false);
    gScanIntercom->SetEnabled(false);

    delete gScanEntry;
    delete gScanExit;
    delete gScanIntercom;
}


void quit(int aIgnored) {
    cleanup();
    printf("Finished :)\n");
    exit(0);
}


void init() {
    gWatchdogTS = millisecs();
    dictionary * dic = iniparser_load("./config.ini");
    if(dic) {
        // hw config
        int FPS_BAUD = iniparser_getint(dic, "HW_CONFIG:FPS_BAUD", -1);
        if(FPS_BAUD == -1) {
            printf("***WARNING! Bad baud rate in config file.\n");
        }
        char * COM_ENTRY = iniparser_getstring(dic, "HW_CONFIG:COM_ENTRY", NULL);
        char * COM_EXIT = iniparser_getstring(dic, "HW_CONFIG:COM_EXIT", NULL);
        char * STR_ENTRY = iniparser_getstring(dic, "HW_CONFIG:STR_ENTRY", NULL);
        char * STR_EXIT = iniparser_getstring(dic, "HW_CONFIG:STR_EXIT", NULL);
        int SCAN_INTERCOM = iniparser_getint(dic, "HW_CONFIG:SCAN_INTERCOM", 0);
        int USE_LCD = iniparser_getint(dic, "HW_CONFIG:USE_LCD", 0);

        // pins
        int EPinLockRelay = iniparser_getint(dic, "HW_PINS:EPinLockRelay", -1);
        if(EPinLockRelay == -1) {
            printf("***WARNING! Bad EPinLockRelay in config file.\n");
        }
        int EPinIntercomBuzzerIN = iniparser_getint(dic, "HW_PINS:EPinIntercomBuzzerIN", -1);
        if(EPinIntercomBuzzerIN == -1) {
            printf("***WARNING! Bad EPinIntercomBuzzerIN in config file.\n");
        }
        int EPinIntercomBuzzerOUT = iniparser_getint(dic, "HW_PINS:EPinIntercomBuzzerOUT", -1);
        if(EPinIntercomBuzzerOUT == -1) {
            printf("***WARNING! Bad EPinIntercomBuzzerOUT in config file.\n");
        }
        int EPinIntercomButtonOUT = iniparser_getint(dic, "HW_PINS:EPinIntercomButtonOUT", -1);
        if(EPinIntercomButtonOUT == -1) {
            printf("***WARNING! Bad EPinIntercomButtonOUT in config file.\n");
        }
        int EPinEmergencyButton = iniparser_getint(dic, "HW_PINS:EPinEmergencyButton", -1);
        if(EPinEmergencyButton == -1) {
            printf("***WARNING! Bad EPinEmergencyButton in config file.\n");
        }
        int ELEDPinEntryOK = iniparser_getint(dic, "HW_PINS:ELEDPinEntryOK", -1);
        if(ELEDPinEntryOK == -1) {
            printf("***WARNING! Bad ELEDPinEntryOK in config file.\n");
        }
        int ELEDPinEntryNOK = iniparser_getint(dic, "HW_PINS:ELEDPinEntryNOK", -1);
        if(ELEDPinEntryNOK == -1) {
            printf("***WARNING! Bad ELEDPinEntryNOK in config file.\n");
        }
        int ELEDPinEntryWait = iniparser_getint(dic, "HW_PINS:ELEDPinEntryWait", -1);
        if(ELEDPinEntryWait == -1) {
            printf("***WARNING! Bad ELEDPinEntryWait in config file.\n");
        }
        int ELEDPinExitOK = iniparser_getint(dic, "HW_PINS:ELEDPinExitOK", -1);
        if(ELEDPinExitOK == -1) {
            printf("***WARNING! Bad ELEDPinExitOK in config file.\n");
        }
        int ELEDPinExitNOK = iniparser_getint(dic, "HW_PINS:ELEDPinExitNOK", -1);
        if(ELEDPinExitNOK == -1) {
            printf("***WARNING! Bad ELEDPinExitNOK in config file.\n");
        }
        int ELEDPinExitWait = iniparser_getint(dic, "HW_PINS:ELEDPinExitWait", -1);
        if(ELEDPinExitWait == -1) {
            printf("***WARNING! Bad ELEDPinExitWait in config file.\n");
        }
        // other
        char * SITE_STR = iniparser_getstring(dic, "SW_CONFIG:SITE_STR", NULL);
        char * SERVER_BASE_URL = iniparser_getstring(dic, "SW_CONFIG:SERVER_BASE_URL", NULL);
        char * DATABASE_FILE = iniparser_getstring(dic, "SW_CONFIG:DATABASE_FILE", NULL);

        int INTERCOM_START = iniparser_getint(dic, "SW_CONFIG:INTERCOM_START", -1);
        if(INTERCOM_START == -1) {
            printf("***WARNING! Bad INTERCOM_START in config file.\n");
        }
        int INTERCOM_STOP = iniparser_getint(dic, "SW_CONFIG:INTERCOM_STOP", -1);
        if(INTERCOM_STOP == -1) {
            printf("***WARNING! Bad INTERCOM_STOP in config file.\n");
        }

        char * STR_WELCOME = iniparser_getstring(dic, "SW_CONFIG:STR_WELCOME", NULL);
        char * STR_BYE = iniparser_getstring(dic, "SW_CONFIG:STR_BYE", NULL);
        char * STR_FORBIDDEN0 = iniparser_getstring(dic, "SW_CONFIG:STR_FORBIDDEN0", NULL);
        char * STR_FORBIDDEN1 = iniparser_getstring(dic, "SW_CONFIG:STR_FORBIDDEN1", NULL);
        char * STR_DEFAULT0 = iniparser_getstring(dic, "SW_CONFIG:STR_DEFAULT0", NULL);
        char * STR_DEFAULT1 = iniparser_getstring(dic, "SW_CONFIG:STR_DEFAULT1", NULL);

        int RELAY_INTERVAL_MS = iniparser_getint(dic, "SW_CONFIG:RELAY_INTERVAL_MS", -1);
        if(RELAY_INTERVAL_MS == -1) {
            printf("***WARNING! Bad RELAY_INTERVAL_MS in config file.\n");
        }
        int HEARTBEAT_INTERVAL_MS = iniparser_getint(dic, "SW_CONFIG:HEARTBEAT_INTERVAL_MS", -1);
        if(HEARTBEAT_INTERVAL_MS == -1) {
            printf("***WARNING! Bad HEARTBEAT_INTERVAL_MS in config file.\n");
        }
        int INTERCOM_CHEAT_PRESS_NUM = iniparser_getint(dic, "SW_CONFIG:INTERCOM_CHEAT_PRESS_NUM", -1);
        if(INTERCOM_CHEAT_PRESS_NUM == -1) {
            printf("***WARNING! Bad INTERCOM_CHEAT_PRESS_NUM in config file.\n");
        }
        int INTERCOM_CHEAT_PRESS_INTERVAL_MS = iniparser_getint(dic, "SW_CONFIG:INTERCOM_CHEAT_PRESS_INTERVAL_MS", -1);
        if(INTERCOM_CHEAT_PRESS_INTERVAL_MS == -1) {
            printf("***WARNING! Bad INTERCOM_CHEAT_PRESS_INTERVAL_MS in config file.\n");
        }
        int INTERCOM_DO_BUZZER_MS = iniparser_getint(dic, "SW_CONFIG:INTERCOM_DO_BUZZER_MS", -1);
        if(INTERCOM_DO_BUZZER_MS == -1) {
            printf("***WARNING! Bad INTERCOM_DO_BUZZER_MS in config file.\n");
        }
        int INTERCOM_DO_BUTTON_MS = iniparser_getint(dic, "SW_CONFIG:INTERCOM_DO_BUTTON_MS", -1);
        if(INTERCOM_DO_BUTTON_MS == -1) {
            printf("***WARNING! Bad INTERCOM_DO_BUTTON_MS in config file.\n");
        }

        gWatchdogIntervalMS = iniparser_getint(dic, "SW_CONFIG:WATCHDOG_INTERVAL", 0);
        if(gWatchdogIntervalMS != 0) {
            printf("WATCHDOG set to %d hours.\n", gWatchdogIntervalMS);
            gWatchdogIntervalMS *= 1000;
            gWatchdogIntervalMS *= 3600; // convert hours to millisecs
        }
        
        // buzzer sounds
        char * INTERCOM_RING_NOCHEAT = iniparser_getstring(dic, "SW_CONFIG:INTERCOM_RING_NOCHEAT", (char *)"./audio/nocheat.wav");
        char * INTERCOM_RING_CHEAT_NOK = iniparser_getstring(dic, "SW_CONFIG:INTERCOM_RING_CHEAT_NOK", (char *)"./audio/cheat_nok.wav");
        char * INTERCOM_RING_CHEAT_OK = iniparser_getstring(dic, "SW_CONFIG:INTERCOM_RING_CHEAT_OK", (char *)"./audio/cheat_ok.wav");
        

        gScanEntry = new Scanner(COM_ENTRY, FPS_BAUD, false, STR_ENTRY, STR_WELCOME, EEventTypeEntry, EPinLockRelay, ELEDPinEntryOK, ELEDPinEntryWait, ELEDPinEntryNOK, RELAY_INTERVAL_MS, HEARTBEAT_INTERVAL_MS, EPinEmergencyButton, USE_LCD);
        gScanExit = new Scanner(COM_EXIT, FPS_BAUD, false, STR_EXIT, STR_BYE, EEventTypeExit, EPinLockRelay, ELEDPinExitOK, ELEDPinExitWait, ELEDPinExitNOK, RELAY_INTERVAL_MS, HEARTBEAT_INTERVAL_MS, EPinEmergencyButton, USE_LCD);
        gScanEntry->SetCommonStrings(STR_DEFAULT0, STR_DEFAULT1, STR_FORBIDDEN0, STR_FORBIDDEN1);
        
        gScanIntercom = new Intercom(EPinIntercomButtonOUT, EPinIntercomBuzzerOUT, EPinIntercomBuzzerIN, INTERCOM_START, INTERCOM_STOP);
        gScanIntercom->SetCommonIntervals(INTERCOM_CHEAT_PRESS_NUM, INTERCOM_CHEAT_PRESS_INTERVAL_MS, INTERCOM_DO_BUZZER_MS, INTERCOM_DO_BUTTON_MS);
        gScanIntercom->SetRingFiles(INTERCOM_RING_CHEAT_OK, INTERCOM_RING_CHEAT_NOK, INTERCOM_RING_NOCHEAT);
        
        // catch ctrl-c
        signal(SIGINT, quit);
        
        // init sqlite, curl and devices
        db_open(DATABASE_FILE);
        req_init(SITE_STR, SERVER_BASE_URL);
        
        // create emergency button thread
        Scanner::CreateEmergencyThread();
        
        // launch scanners
        gScanEntry->SetEnabled(true);
        gScanExit->SetEnabled(true);
        if(SCAN_INTERCOM) {
            gScanIntercom->SetEnabled(true);
        }

        iniparser_freedict(dic);
    }
    else {
        printf("Cannot open config file !\nBye.\n");
        exit(-2);
    }
}


void restart() {
    printf("Cleanup...\n");
    cleanup();
    usleep(500 * 1000);
    printf("Restart...\n");
    init();
}


int enroll(int aUserID) {
    FPS_GT511 * fps_enroll = gScanEnroll->GetFPS();
    FPS_GT511 * fps_entry = gScanEntry->GetFPS();

    int enrollid = 0;
    bool usedid = true;
    bool failed = true;
    bool enrolled = false;
    int iret = 0;

    fps_enroll->SetLED(true);
    
    
    // =============================
    // ask for user ID
    printf("Welcome to user creation and enrollment.\n");
    if(aUserID == -1) {
        char nick[16 + 1] = "";
        do {
            printf("Please input a 16-character-max (nick)name: ");
            getline();
        } while(strlen(sBuffer) > 16);
        strcpy(nick, sBuffer);
        printf("Please input user email and press return: ");
        getline();
        aUserID = db_get_user_id(sBuffer);
        if(aUserID == -1) {
            aUserID = db_insert_user(sBuffer, nick);
            if(aUserID == -1) {
                printf("ERROR: Could not create user !\n");
                return -1;
            }
            else {
                printf("User %s created with ID: %d\n", sBuffer, aUserID);
            }
        }
        else {
            printf("User %s already exists with ID: %d\n", sBuffer, aUserID);
        }
    }
    

    // =============================
    // find free enroll id
    while(usedid == true) {
        usedid = fps_enroll->CheckEnrolled(enrollid);
        if (usedid == true) enrollid++;
    }
    fps_enroll->EnrollStart(enrollid);

    
    // =============================
    // enroll
    printf("Enroll ID will be %d.\n", enrollid);
    printf("Press finger to enroll...\n");

    // wait for finger press, then capture image
    while(fps_enroll->IsPressFinger() == false) usleep(100 * 1000);
    if(fps_enroll->CaptureFinger(true)) {
        iret = fps_enroll->Enroll1();
        if(iret == 0) {
            // wait for finger release
            printf("Remove finger\n");
            while(fps_enroll->IsPressFinger() == true) usleep(100 * 1000);
            
            // wait for finger press, then capture image
            printf("Press same finger again\n");
            while(fps_enroll->IsPressFinger() == false) usleep(100 * 1000);
            if (fps_enroll->CaptureFinger(true)) {
                iret = fps_enroll->Enroll2();
                if(iret == 0) {
                    // wait for finger release
                    printf("Remove finger\n");
                    while(fps_enroll->IsPressFinger() == true) usleep(100 * 1000);
                    
                    // wait for finger press, then capture image
                    printf("Press same finger yet again\n");
                    while(fps_enroll->IsPressFinger() == false) usleep(100 * 1000);
                    if (fps_enroll->CaptureFinger(true)) {
                        iret = fps_enroll->Enroll3();
                        if (iret == 0) {
                            enrolled = true;
                            printf("Enrolling Successful :)\n");
                            printf("Binding UserID %d and Fingerprint ID %d...\n", aUserID, enrollid);
                            
                            // get template
                            byte * buf = new byte[TEMPLATE_DATA_LEN];
                            if(0 == fps_enroll->GetTemplate(enrollid, buf)) {
                                
                                // upload template
                                if(0 == fps_entry->SetTemplate(buf, enrollid, false)) {

                                    printf("Template uploaded, updating database...\n");
                                    db_insert_fingerprint(aUserID, enrollid, buf, TEMPLATE_DATA_LEN);
                                    db_insert_fingerprint_event(enrollid, 0, EEventTypeEnroll, true);

                                    // =============================
                                    // SUCCESS !!!
                                    printf("Template uploaded, all OK :)\n");
                                    failed = false;
                                    
                                } // fi set template
                                else {
                                    printf("Template upload failed :(\n");
                                }
                            } // fi get template
                            else {
                                printf("Template download failed :(\n");
                            }
                        } // fi enroll 3
                        else {
                            printf("Failed to enroll third finger (%d)\n", iret);
                        }
                    } // fi capture 3
                    else {
                        printf("Failed to capture third finger\n");
                    }
                } // fi enroll 2
                else {
                    printf("Failed to enroll second finger (%d)\n", iret);
                }
            } // fi capture 2
            else {
                printf("Failed to capture second finger\n");
            }
        } // fi enroll 1
        else {
            printf("Failed to enroll first finger (%d)\n", iret);
        }
    } // fi capture 1
    else {
        printf("Failed to capture first finger\n");
    }
    
    if (failed) {
        // remove fgp id cuz something went wrong
        if(enrolled) {
            printf("Removing fingerprint...");
            if(fps_enroll->DeleteID(enrollid) == false) {
                printf(" error ! Please hack !\n");
            }
            else {
                printf(" done.\n");
            }
        }
        db_insert_fingerprint_event(enrollid, 0, EEventTypeEnroll, false);
    }
    
    strcpy(sBuffer, "");
    fps_enroll->SetLED(false);
    return failed ? -1 : 0;
}



/*
 * Used for testing communication w/ module.
 */
static void * blink(void * aPort) {
    sBlinkLoop = true;

    gScanEntry->SetEnabled(false);
    gScanExit->SetEnabled(false);
    
    // loop
    while(sBlinkLoop) {
        gScanEntry->GetFPS()->SetLED(true);
        gScanExit->GetFPS()->SetLED(true);
        usleep(500 * 1000);
        gScanEntry->GetFPS()->SetLED(false);
        gScanExit->GetFPS()->SetLED(false);
        usleep(500 * 1000);
    }
    printf("BLINK thread stopped.\n");
    return NULL;
}


int main() {
    
    init();
    pthread_t thr_blink;

    do {

        // if we got a watchdog, restart stuff when needed
        if((gWatchdogIntervalMS > 0) && (millisecs() - gWatchdogTS) > gWatchdogIntervalMS) {
            printf("WATCHDOG TRIGGERED !\n");
            restart();
        }

        // prompt, ask for input, parse command
        printf("digilock> ");
        getline();
        if (1) {
            if(strcasecmp(sBuffer, COMMAND_ENROLL) == 0) {
                gScanExit->SetEnabled(false);
                gScanEntry->SetEnabled(false);
                enroll(-1);
                gScanExit->SetEnabled(true);
                gScanEntry->SetEnabled(true);
            }
            else if(strcasecmp(sBuffer, COMMAND_ENTRY_STOP) == 0) {
                gScanEntry->SetEnabled(false);
                printf("ENTRY scan thread stopped !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_ENTRY_START) == 0) {
                gScanEntry->SetEnabled(true);
                printf("ENTRY scan thread started !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_EXIT_STOP) == 0) {
                gScanExit->SetEnabled(false);
                printf("EXIT scan thread stopped !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_EXIT_START) == 0) {
                gScanExit->SetEnabled(true);
                printf("EXIT scan thread started !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_INTERCOM_STOP) == 0) {
                gScanIntercom->SetEnabled(false);
                printf("INTERCOM scan thread stopped !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_INTERCOM_START) == 0) {
                gScanIntercom->SetEnabled(true);
                printf("INTERCOM scan thread started !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_DELETE_FGP) == 0) {
                printf("\nEnter user e-mail:");

                getline();
                int _id = db_get_user_id(sBuffer);
                if(_id == -1) {
                    printf("Unknown user e-mail.");
                }
                else {
                    printf("Do you really want to delete fingerprints for user %d / %s ?\n", _id, sBuffer);
                    getline();
                    if(0 == strcasecmp(sBuffer, COMMAND_YES)) {
                        db_delete_user_data(_id, false);
                        printf("Done.");
                    }
                    else {
                        printf("Cancelled.");
                    }
                }
            }
            else if(strcasecmp(sBuffer, COMMAND_DELETE_USER) == 0) {
                printf("Enter user e-mail: ");
                int _id = db_get_user_id(sBuffer);
                if(_id == -1) {
                    printf("Unknown user e-mail.");
                }
                else {
                    printf("Do you really want to delete user %d / %s ?\nAll matching fingerprint data will be deleted: ", _id, sBuffer);
                    getline();
                    if(0 == strcasecmp(sBuffer, COMMAND_YES)) {
                        db_delete_user_data(_id, true);
                        printf("Done.");
                    }
                    else {
                        printf("Cancelled.");
                    }
                
                }
            
            }
            else if(strcasecmp(sBuffer, COMMAND_CREATE_USER) == 0) {
                char nick[16];
                do {
                    printf("Enter a 16-character-max (nick)name: ");
                    getline();
                    strcpy(nick, sBuffer);
                } while(strlen(nick) > 16);
                printf("Enter user e-mail: ");
                getline();

                bool enroll_ok = true;
                int _id = db_get_user_id(sBuffer);
                if(_id == -1) {
                    _id = db_insert_user(sBuffer, nick);
                    if(_id == -1) {
                        printf("User creation failed.\n");
                        enroll_ok = false;
                    }
                    else {
                        printf("User created with id %d.\n", _id);
                    }
                }
                else {
                    printf("User already exists.\n");
                }
                
                if(enroll_ok) {
                    printf("Enroll user now ? ");
                    getline();
                    if(0 == strcasecmp(sBuffer, COMMAND_YES)) {
                        enroll(_id);
                    }
                }
            }
            else if(strcasecmp(sBuffer, COMMAND_EMPTY_FGP) == 0) {
                gScanEntry->SetEnabled(false);
                gScanExit->SetEnabled(false);
                gScanEntry->GetFPS()->DeleteAll();
                gScanExit->GetFPS()->DeleteAll();
            }
            else if(strcasecmp(sBuffer, COMMAND_SYNC) == 0) {
                /*printf("Synchronize device data now ?\nThe scanning will be paused: ");

                getline();
                if(0 == strcasecmp(sBuffer, COMMAND_YES)) {
                    gScanEntry->SetEnabled(false);
                    gScanExit->SetEnabled(false);

                    // TODO:
                    
                    printf("Done.");
                    gScanEntry->SetEnabled(true);
                    gScanExit->SetEnabled(true);
                }
                else {
                    printf("Cancelled.");
                }*/
            }
            else if(strcasecmp(sBuffer, COMMAND_RELOAD) == 0) {
                // delete all fingerprints and reload templates from database
                gScanEntry->SetEnabled(false);
                gScanExit->SetEnabled(false);
                gScanEntry->GetFPS()->DeleteAll();
                gScanExit->GetFPS()->DeleteAll();

                int max_fgp = db_count_fingerprints();

                printf("%d fingerprints in database.\n", max_fgp);

                for(int idx = 0; idx < max_fgp; idx++) {
                    byte * fgp = db_get_fingerprint(idx);
                    if(fgp != NULL) {
                        gScanEntry->GetFPS()->SetTemplate(fgp, idx, false);
                        gScanExit->GetFPS()->SetTemplate(fgp, idx, false);
                        free(fgp);
                        printf("Reloaded fingerprint template ID: %d\n", idx);
                    }
                }

                printf("Reload done.\n");

                gScanEntry->SetEnabled(true);
                gScanExit->SetEnabled(true);
            }
            else if(strcasecmp(sBuffer, COMMAND_BLINK_START) == 0) {
                sBlinkLoop = true;
                if(0 == pthread_create(&thr_blink, NULL, blink, NULL)) {
                    printf("BLINK thread created !\n");
                }
            }
            else if(strcasecmp(sBuffer, COMMAND_COUNT) == 0) {
                printf("%s has %d enrolls.\n", gScanEntry->GetName(), gScanEntry->GetFPS()->GetEnrollCount());
                printf("%s has %d enrolls.\n", gScanExit->GetName(), gScanExit->GetFPS()->GetEnrollCount());
            }
            else if(strcasecmp(sBuffer, COMMAND_BLINK_STOP) == 0) {
                printf("BLINK thread stopping...\n");
                sBlinkLoop = false;
            }
            else if(strcasecmp(sBuffer, COMMAND_DUMP_ENTRY) == 0) {
                gScanEntry->Dump();
            }
            else if(strcasecmp(sBuffer, COMMAND_DUMP_EXIT) == 0) {
                gScanExit->Dump();
            }
            else if(strcasecmp(sBuffer, COMMAND_RESTART) == 0) {
                restart();
            }
            else if(strcasecmp(sBuffer, "") == 0)  {
            }
            else if(strcasecmp(sBuffer, COMMAND_QUIT)) {
                printf("Unknown command.\n");
            }
        }
        
        fflush(stdin);
        usleep(1000);
    }
    while (strcasecmp(sBuffer, COMMAND_QUIT));

    quit(0);
    return 0;
}
