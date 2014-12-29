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
#define COMMAND_PAUSE_ENTRY     "pause entry"
#define COMMAND_PAUSE_EXIT      "pause exit"
#define COMMAND_RESUME_ENTRY    "resume entry"
#define COMMAND_RESUME_EXIT     "resume exit"

// database
#define COMMAND_CREATE_USER     "create user"
#define COMMAND_EMPTY_FGP       "delete all fingerprints"
#define COMMAND_DELETE_FGP      "delete fingerprint"
#define COMMAND_DELETE_USER     "delete user"
#define COMMAND_SYNC            "sync fingerprints"


//#define FPS_BAUD                9600
//#define FPS_MODE                {'8', 'N', '1', 0}

static volatile bool            sBlinkLoop;
//static volatile bool            sScanEntryLoop;
//static volatile bool            sScanExitLoop;

//static char sMode[]             = {'8', 'N', '1', 0};
//static FPS_GT511                sEntryFPS(GT511_PORT_ENTRY, FPS_BAUD, sMode);
//static FPS_GT511                sExitFPS(GT511_PORT_EXIT, FPS_BAUD, sMode);
//#define sEnrollFPS              sExitFPS


//typedef struct FPSData {
//    int             led_ok;
//    int             led_nok;
//    int             led_wait;
//    volatile bool   enabled;
//    FPS_GT511       fps;
//    const char *    name;
//    EEventType      event;
//} FPSData, * pFPSData;

static Scanner * scan_entry = new Scanner(GT511_PORT_ENTRY, true, "ENTRY", EEventTypeEntry, ELEDPinEntryOK, ELEDPinEntryWait, ELEDPinEntryNOK);
static Scanner * scan_exit = new Scanner(GT511_PORT_EXIT, true, "EXIT", EEventTypeExit, ELEDPinExitOK, ELEDPinExitWait, ELEDPinExitNOK);
#define scan_enroll scan_exit


char * getline(char * buffer) {
    if(fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strlen(buffer) - 1] = '\0';
    }
    return buffer;
}


int enroll(int aUserID) {
    FPS_GT511 *fps = scan_enroll->GetFPS();


    fps->SetLED(true);

    // =============================
    // ask for user ID
    printf("Welcome to user creation and enrollment.\n");
    if(aUserID == -1) {
        char email[128];
        printf("Please input user email and press return: ");
        if(fgets(email, sizeof(email), stdin)) {
            aUserID = db_get_user_id(email);
            if(aUserID == -1) {
                aUserID = db_insert_user(email);
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
                    db_insert_fingerprint(aUserID, enrollid, 0, NULL);
                    db_insert_event(enrollid, EEventTypeEnroll, true);
                    printf("Done.");
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



//static void * scan(void * aUnused) {
//    
//    pFPSData fps_data = (pFPSData)aUnused;
//    
//    // setup
//    fps_data->fps.SetLED(true);
//    fps_data->enabled = true;
//    
//    bool detect = true;
//    bool stopmsg = true;
//    bool led = true;
//    
//    // loop
//    while(fps_data->enabled) {
//        stopmsg = true;
//        
//        if(led == false) {
//            led = !led;
//            fps_data->fps.SetLED(led);
//        }
//        if(fps_data->fps.IsPressFinger()) {
//            // finger pressed, continue if there was no finger before
//            if(detect) {
//                // turn on orange LED
//                led_show(fps_data->led_wait, true);
//                fps_data->fps.CaptureFinger(false);
//                int id = fps_data->fps.Identify1_N();
//                
//                if(id < MAX_FGP_COUNT) {
//                    // turn on green LED
//                    printf("%s OK for user %d\n", fps_data->name, id);
//                    led_show(fps_data->led_ok, true);
//                    db_insert_event(id, fps_data->event, true);
//                }
//                else {
//                    // turn on red LED
//                    printf("%s forbidden for unknown user\n", fps_data->name);
//                    led_show(fps_data->led_nok, true);
//                    db_insert_event(-1, fps_data->event, false);
//                }
//                // turn off detection, will be turned on when finger is released
//                detect = false;
//            }
//        }
//        else {
//            // finger not pressed, detection always on
//            led_show(fps_data->led_wait, false);
//            detect = true;
//        }
//        usleep(10 * 1000);
//    }
//    
//    
//    
//    if(led == true) {
//        led = !led;
//        fps_data->fps.SetLED(led);
//    }
//    if(stopmsg) {
//        printf("%s thread exit.\n", fps_data->name);
//        stopmsg = false;
//    }
//    
//    return NULL;
//}


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
    
    
    scan_entry->SetEnabled(true);
    scan_exit->SetEnabled(true);
    
    
    // TODO: parse command line
    

    
//    int count = sEntryFPS.GetEnrollCount();
//    for(int i = 0; i < count; i++) {
//
//        byte *tmp = new byte[498];
//        if(0==sEntryFPS.GetTemplate(i, tmp)) {
//            char str[32];
//            sprintf(str, "/template-%d.bin", i);
//            FILE * f = fopen(str, "w");
//            
//            
//            for(int j=0;j < 498; j++)
//                fputc(tmp[j], f);
//            
//            fclose(f);
//        }
//    }
    
    
    
    pthread_t thr_blink;
//    pthread_t thr_entry_scan;
//    pthread_t thr_exit_scan;
//    pthread_t thr_enroll;
//    
//    
//    FPSData* entry = (FPSData *)malloc(sizeof(struct FPSData));
//    entry->event = EEventTypeEntry;
//    entry->name = "ENTRY";
//    entry->led_ok = ELEDPinEntryOK;
//    entry->led_nok = ELEDPinEntryNOK;
//    entry->led_wait = ELEDPinEntryWait;
//    entry->fps = sEntryFPS;
//
//    FPSData* exit = (FPSData *)malloc(sizeof(struct FPSData));
//    exit->event = EEventTypeExit;
//    exit->name = "EXIT";
//    exit->led_ok = ELEDPinExitOK;
//    exit->led_nok = ELEDPinExitNOK;
//    exit->led_wait = ELEDPinExitWait;
//    exit->fps = sExitFPS;
//    
//    if(0 == pthread_create(&thr_entry_scan, NULL, scan, entry)) {
//        printf("ENTRY scan thread created !\n");
//    }
//
//    if(0 == pthread_create(&thr_exit_scan, NULL, scan, exit)) {
//        printf("EXIT scan thread created !\n");
//    }
    char command[256];
    char email[128];
    char response[32];
    
    
    do {
        printf("$> ");

        if (fgets(command, sizeof(command), stdin)) {
            
            // remove '\n'
            command[strlen(command) - 1] = '\0';
            if(strcasecmp(command, COMMAND_ENROLL) == 0) {

                scan_exit->SetEnabled(false);
                
                if(enroll(-1) == 0) {
                    // TODO: sync GT511s
                }
                else {
                    
                }
            }
            else if(strcasecmp(command, COMMAND_PAUSE_ENTRY) == 0) {
                scan_entry->SetEnabled(false);
                printf("ENTRY scan thread paused !\n");
            }
            else if(strcasecmp(command, COMMAND_RESUME_ENTRY) == 0) {
                scan_entry->SetEnabled(true);
                printf("ENTRY scan thread resumed !\n");
            }
            else if(strcasecmp(command, COMMAND_PAUSE_EXIT) == 0) {
                scan_exit->SetEnabled(false);
                printf("EXIT scan thread paused !\n");
            }
            else if(strcasecmp(command, COMMAND_RESUME_EXIT) == 0) {
                scan_exit->SetEnabled(true);
                printf("EXIT scan thread resumed !\n");
            }
            else if(strcasecmp(command, COMMAND_DELETE_FGP) == 0) {
                printf("\nEnter user e-mail:");
                if(fgets(email, sizeof(email), stdin)) {
                    int _id = db_get_user_id(email);
                    if(_id == -1) {
                        printf("Unknown user e-mail.");
                    }
                    else {
                        printf("Do you really want to delete fingerprints for user %d / %s ?\n", _id, email);
                        if(fgets(response, sizeof(response), stdin)) {
                            if(0 == strcasecmp(response, COMMAND_YES)) {
                                db_delete_user_data(_id, false);
                                printf("Done.");
                            }
                            else {
                                printf("Cancelled.");
                            }
                        }
                    }
                }
            }
            else if(strcasecmp(command, COMMAND_DELETE_USER) == 0) {
                printf("Enter user e-mail: ");
                if(fgets(email, sizeof(email), stdin)) {
                    int _id = db_get_user_id(email);
                    if(_id == -1) {
                        printf("Unknown user e-mail.");
                    }
                    else {
                        printf("Do you really want to delete user %d / %s ?\nAll matching fingerprint data will be deleted: ", _id, email);
                        if(fgets(response, sizeof(response), stdin)) {
                            if(0 == strcasecmp(response, COMMAND_YES)) {
                                db_delete_user_data(_id, true);
                                printf("Done.");
                            }
                            else {
                                printf("Cancelled.");
                            }
                        }
                    }
                }
            }
            else if(strcasecmp(command, COMMAND_CREATE_USER) == 0) {
                printf("Enter user e-mail: ");
                if(fgets(email, sizeof(email), stdin)) {
                    bool enroll_ok = true;
                    int _id = db_get_user_id(email);
                    if(_id == -1) {
                        _id = db_insert_user(email);
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
                        if(fgets(response, sizeof(response), stdin) && (0 == strcasecmp(response, COMMAND_YES))) {
                            enroll(_id);
                        }
                    }
                }
            }
            else if(strcasecmp(command, COMMAND_EMPTY_FGP) == 0) {
            }
            else if(strcasecmp(command, COMMAND_SYNC) == 0) {
                printf("Synchronize device data now ?\nThe scanning will be paused: ");
                if(fgets(response, sizeof(response), stdin)) {
                    if(0 == strcasecmp(response, COMMAND_YES)) {
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
            }
            else if(strcasecmp(command, COMMAND_BLINK_START) == 0) {
                sBlinkLoop = true;
                if(0 == pthread_create(&thr_blink, NULL, blink, NULL)) {
                    printf("BLINK thread created !\n");
                }
            }
            else if(strcasecmp(command, COMMAND_BLINK_STOP) == 0) {
                printf("BLINK thread stopping...\n");
                sBlinkLoop = false;
            }
            else if(strcasecmp(command, "") == 0)  {
            }
            else if(strcasecmp(command, COMMAND_QUIT)) {
                printf("Unknown command.\n");
            }
        }
    }
    while (strcasecmp(command, COMMAND_QUIT));

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
