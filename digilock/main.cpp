/*
    Library example for controlling the GT-511C3 Finger Print Scanner (FPS)
    Created by Josh Hawley, July 23rd 2013
    Licensed for non-commercial use, must include this license message
    basically, Feel free to hack away at it, but just give me credit for my work =)
    TLDR; Wil Wheaton's Law
*/

#include "FPS_GT511Linux.h"
#include "db.h"
#include "req.h"
#include "led.h"
#include "scanner.h"
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef __APPLE__
#  include <sys/malloc.h>
#  include <pthread/pthread.h>
#else
#  include <malloc.h>
#  include <pthread.h>
#endif

#ifdef __APPLE__
#  define GT511_PORT_ENTRY      apple_ttyUSB0
#  define GT511_PORT_ENROLL     apple_ttyUSB0
#  define GT511_PORT_EXIT       apple_ttyUSB1
#else
#  define GT511_PORT_ENTRY      ttyUSB0
#  define GT511_PORT_EXIT       ttyUSB1
#  define GT511_PORT_ENROLL     ttyUSB0
#endif

#define COMMAND_PASSWORD        "sesame"
#define COMMAND_YES             "yes"

// thread
#define COMMAND_BLINK_START     "blink start"
#define COMMAND_BLINK_STOP      "blink stop"

#define COMMAND_QUIT            "quit"
#define COMMAND_ENROLL          "enroll"
#define COMMAND_COUNT           "count"
#define COMMAND_PAUSE_ENTRY     "pause entry"
#define COMMAND_PAUSE_EXIT      "pause exit"
#define COMMAND_RESUME_ENTRY    "resume entry"
#define COMMAND_RESUME_EXIT     "resume exit"
#define COMMAND_DUMP_ENTRY      "dump entry"
#define COMMAND_DUMP_EXIT       "dump exit"


// database
#define COMMAND_CREATE_USER     "create user"
#define COMMAND_EMPTY_FGP       "delete all fingerprints"
#define COMMAND_DELETE_FGP      "delete fingerprint"
#define COMMAND_DELETE_USER     "delete user"
#define COMMAND_SYNC            "sync fingerprints"



static volatile bool            sBlinkLoop;

static Scanner * scan_entry = new Scanner(GT511_PORT_ENTRY, false, "ENTRY", EEventTypeEntry, ELEDPinEntryOK, ELEDPinEntryWait, ELEDPinEntryNOK);
static Scanner * scan_exit = new Scanner(GT511_PORT_EXIT, false, "EXIT", EEventTypeExit, ELEDPinExitOK, ELEDPinExitWait, ELEDPinExitNOK);
#define scan_enroll scan_exit


#define BUFFER_LEN  256
char sBuffer[256];


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



int enroll(int aUserID) {
    FPS_GT511 *fps = scan_enroll->GetFPS();


    fps->SetLED(true);

    // =============================
    // ask for user ID
    printf("Welcome to user creation and enrollment.\n");
    if(aUserID == -1) {
        printf("Please input user email and press return: ");
        getline();
        aUserID = db_get_user_id(sBuffer);
        if(aUserID == -1) {
            aUserID = db_insert_user(sBuffer);
            if(aUserID == -1) {
                printf("ERROR: Could not create user !\n");
                return -1;
            }
            else {
                printf("User created with ID: %d\n", aUserID);
            }
        }
        else {
            printf("User already exists with ID: %d\n", aUserID);
        }
    }

    // =============================
    // find open enroll id
    int enrollid = 0;
    bool usedid = true;
    while (usedid == true)
    {
        usedid = fps->CheckEnrolled(enrollid);
        if (usedid == true) enrollid++;
    }
    fps->EnrollStart(enrollid);

    // enroll
    printf("Enroll ID will be %d.\n", enrollid);
    printf("Press finger to enroll...\n");

    while(fps->IsPressFinger() == false)
        usleep(100 * 1000);
    bool bret = fps->CaptureFinger(true);
    int iret = 0;
    if (bret != false)
    {
        printf("Remove finger\n");
        fps->Enroll1();
        while(fps->IsPressFinger() == true)
            usleep(100 * 1000);
        printf("Press same finger again\n");
        while(fps->IsPressFinger() == false)
            usleep(100 * 1000);
        bret = fps->CaptureFinger(true);
        if (bret != false)
        {
            printf("Remove finger\n");
            fps->Enroll2();
            while(fps->IsPressFinger() == true)
                usleep(100 * 1000);
            printf("Press same finger yet again\n");
            while(fps->IsPressFinger() == false)
                usleep(100 * 1000);
            bret = fps->CaptureFinger(true);
            if (bret != false)
            {
                printf("Remove finger\n");
                iret = fps->Enroll3();
                if (iret == 0)
                {
                    printf("Enrolling Successful :)\n");
                    printf("Binding UserID %d and Fingerprint ID %d...\n", aUserID, enrollid);
                 
                    
                    /// GET TEMPLATE
                    byte * buf = new byte[TEMPLATE_DATA_LEN];
                    int retcode = fps->GetTemplate(enrollid, buf);

                    db_insert_fingerprint(aUserID, enrollid, buf);
                    db_insert_event(enrollid, EEventTypeEnroll, true);
                    printf("Done.");
                    
                    
                    FPS_GT511 * entry = scan_entry->GetFPS();
                    entry->SetTemplate(buf, enrollid, false);
                }
                else
                {
                    printf("Enrolling Failed with error code: %d\n", iret);
                    db_insert_event(enrollid, EEventTypeEnroll, false);
                }
            }
            else printf("Failed to capture third finger\n");
        }
        else printf("Failed to capture second finger\n");
    }
    else printf("Failed to capture first finger\n");

    
    fps->SetLED(false);
    return 0;
}



/*
 * Used for testing communication w/ module.
 */
static void * blink(void * aPort) {
    sBlinkLoop = true;

    scan_entry->SetEnabled(false);
    scan_exit->SetEnabled(false);
    
    // loop
    while(sBlinkLoop) {
        scan_entry->GetFPS()->SetLED(true);
        scan_exit->GetFPS()->SetLED(true);
        usleep(500 * 1000);
        scan_entry->GetFPS()->SetLED(false);
        scan_exit->GetFPS()->SetLED(false);
        usleep(500 * 1000);
    }
    printf("BLINK thread stopped.\n");
    return NULL;
}

int main() {
    
    // init sqlite, curl and devices
    db_open();
    req_init();
    
    
    // =================================================================
//    scan_enroll->GetFPS()->UseSerialDebug = true;
//    scan_entry->GetFPS()->UseSerialDebug = true;
    scan_entry->GetFPS()->DeleteAll();
    scan_enroll->GetFPS()->DeleteAll();
    
    db_drop_tables();
    db_close();
    db_open();
    enroll(-1);
    
    
//    int count = scan_enroll->GetFPS()->GetEnrollCount();
//    if(count > 0) {
//        byte * buf = new byte[TEMPLATE_DATA_LEN];
//
//        if(0 == scan_enroll->GetFPS()->GetTemplate(count - 1, buf)) {
//            FILE* f = fopen("/Users/olivier/template-2.dat", "w");
//            for(int i = 0; i < TEMPLATE_DATA_LEN; i++) {
//                fputc(buf[i], f);
//            }
//            fflush(f);
//            fclose(f);
//
//            
//            scan_entry->GetFPS()->SetTemplate(buf, count - 1, false);
//            
//            
//            delete buf;
//        }
//    }
    // =================================================================
    
    
    scan_entry->SetEnabled(true);
    scan_exit->SetEnabled(true);
    
    
    
    
    pthread_t thr_blink;
    
    do {
        printf("$> ");
        getline();
        if (1) {
            if(strcasecmp(sBuffer, COMMAND_ENROLL) == 0) {
                scan_exit->SetEnabled(false);
                if(enroll(-1) == 0) {
                    // TODO: sync GT511s
                }
                else {
                    
                }
            }
            else if(strcasecmp(sBuffer, COMMAND_PAUSE_ENTRY) == 0) {
                scan_entry->SetEnabled(false);
                printf("ENTRY scan thread paused !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_RESUME_ENTRY) == 0) {
                scan_entry->SetEnabled(true);
                printf("ENTRY scan thread resumed !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_PAUSE_EXIT) == 0) {
                scan_exit->SetEnabled(false);
                printf("EXIT scan thread paused !\n");
            }
            else if(strcasecmp(sBuffer, COMMAND_RESUME_EXIT) == 0) {
                scan_exit->SetEnabled(true);
                printf("EXIT scan thread resumed !\n");
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
                printf("Enter user e-mail: ");
                getline();
                bool enroll_ok = true;
                int _id = db_get_user_id(sBuffer);
                if(_id == -1) {
                    _id = db_insert_user(sBuffer);
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
            }
            else if(strcasecmp(sBuffer, COMMAND_SYNC) == 0) {
                printf("Synchronize device data now ?\nThe scanning will be paused: ");

                getline();
                if(0 == strcasecmp(sBuffer, COMMAND_YES)) {
                    scan_entry->SetEnabled(false);
                    scan_exit->SetEnabled(false);

                    // TODO:
                    
                    printf("Done.");
                    scan_entry->SetEnabled(true);
                    scan_exit->SetEnabled(true);
                }
                else {
                    printf("Cancelled.");
                }
            
            }
            else if(strcasecmp(sBuffer, COMMAND_BLINK_START) == 0) {
                sBlinkLoop = true;
                if(0 == pthread_create(&thr_blink, NULL, blink, NULL)) {
                    printf("BLINK thread created !\n");
                }
            }
            else if(strcasecmp(sBuffer, COMMAND_COUNT) == 0) {
                printf("%s has %d enrolls.\n", scan_entry->GetName(), scan_entry->GetFPS()->GetEnrollCount());
                printf("%s has %d enrolls.\n", scan_exit->GetName(), scan_exit->GetFPS()->GetEnrollCount());
            }
            else if(strcasecmp(sBuffer, COMMAND_BLINK_STOP) == 0) {
                printf("BLINK thread stopping...\n");
                sBlinkLoop = false;
            }
            else if(strcasecmp(sBuffer, COMMAND_DUMP_ENTRY) == 0) {
                scan_entry->Dump();
            }
            if(strcasecmp(sBuffer, COMMAND_DUMP_EXIT) == 0) {
                scan_exit->Dump();
            }
            else if(strcasecmp(sBuffer, "") == 0)  {
            }
            else if(strcasecmp(sBuffer, COMMAND_QUIT)) {
                printf("Unknown command.\n");
            }
        }
        
        fflush(stdin);
    }
    while (strcasecmp(sBuffer, COMMAND_QUIT));

    // close libcurl + sqlite
    req_cleanup();
    db_close();

    // stop scans
    scan_entry->SetEnabled(false);
    scan_exit->SetEnabled(false);

    delete scan_entry;
    delete scan_exit;
    
    printf("Finished :)\n");
    return 0;
}
