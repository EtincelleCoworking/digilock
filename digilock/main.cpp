/*
    Library example for controlling the GT-511C3 Finger Print Scanner (FPS)
    Created by Josh Hawley, July 23rd 2013
    Licensed for non-commercial use, must include this license message
    basically, Feel free to hack away at it, but just give me credit for my work =)
    TLDR; Wil Wheaton's Law
*/

#include "FPS_GT511Linux.h"
#include "db.h"
#ifdef __APPLE__
#  include <sys/malloc.h>
#  include <pthread/pthread.h>
#else
#  include <malloc.h>
#  include <pthread.h>
#endif
#include "req.h"

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

#define COMMAND_EXIT            "exit"
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


#define FPS_BAUD                9600
#define FPS_MODE                {'8', 'N', '1', 0}

static volatile bool            sBlinkLoop;
static volatile bool            sScanEntryLoop;
static volatile bool            sScanExitLoop;

static char sMode[]             = {'8', 'N', '1', 0};
static FPS_GT511                sEntryFPS(GT511_PORT_ENTRY, FPS_BAUD, sMode);
static FPS_GT511                sExitFPS(GT511_PORT_EXIT, FPS_BAUD, sMode);
#define sEnrollFPS              sExitFPS



char * getline(void) {
    size_t lenmax = 100, len = lenmax;
    char * line = (char *)malloc(lenmax), * linep = line;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            char * linen = (char *)realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n') {
            *(line - 1) = '\0';
            break;
        }
    }
    return linep;
}


int enroll() {
    FPS_GT511 fps = sEnrollFPS;
    fps.Open();
    fps.UseSerialDebug = false;
    fps.SetLED(true);

    // =============================
    // ask for user ID
    printf("Welcome to user creation and enrollment.\nPlease input user email and press return:\n");
    char * email = getline();

    int userid = db_get_user_id(email);
    if(userid == -1) {
        userid = db_insert_user(email);
        if(userid == -1) {
            printf("ERROR: Could not create user !\n");
            return -1;
        }
        else {
            printf("User created with ID: %d\n", userid);
        }
    }
    else {
        printf("User already exists with ID: %d\n", userid);
    }


    // =============================
    // find open enroll id
    int enrollid = 0;
    bool usedid = true;
    while (usedid == true)
    {
        usedid = fps.CheckEnrolled(enrollid);
        if (usedid == true) enrollid++;
    }
    fps.EnrollStart(enrollid);

    // enroll
    printf("Enroll ID for '%s'' will be %d.\n", email, enrollid);
    printf("Press finger to Enroll\n");

    while(fps.IsPressFinger() == false)
        usleep(100 * 1000);
    bool bret = fps.CaptureFinger(true);
    int iret = 0;
    if (bret != false)
    {
        printf("Remove finger\n");
        fps.Enroll1();
        while(fps.IsPressFinger() == true)
            usleep(100 * 1000);
        printf("Press same finger again\n");
        while(fps.IsPressFinger() == false)
            usleep(100 * 1000);
        bret = fps.CaptureFinger(true);
        if (bret != false)
        {
            printf("Remove finger\n");
            fps.Enroll2();
            while(fps.IsPressFinger() == true)
                usleep(100 * 1000);
            printf("Press same finger yet again\n");
            while(fps.IsPressFinger() == false)
                usleep(100 * 1000);
            bret = fps.CaptureFinger(true);
            if (bret != false)
            {
                printf("Remove finger\n");
                iret = fps.Enroll3();
                if (iret == 0)
                {
                    printf("Enrolling Successful :)\n");
                    printf("Binding UserID %d and Fingerprint ID %d...\n", userid, enrollid);
                    db_insert_fingerprint(userid, enrollid, 0, NULL);
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

    // loop
//    while(1) {
//        usleep(60 * 1000 * 1000);
//    }
    return 0;
}





void showLED(int aLed, bool aEnable, bool aBlink, int aDurationMS) {
 
    
    
    
}


static void * scan_entry(void * aUnused) {

    // setup
    sEntryFPS.SetLED(true);
    sScanEntryLoop = true;
    
    // loop
    while(1) {
        if(sScanEntryLoop) {
            if(sEntryFPS.IsPressFinger()) {
                // turn on orange LED
                sEntryFPS.CaptureFinger(false);
                int id = sEntryFPS.Identify1_N();

                if(id < MAX_FGP_COUNT) {
                    // turn on green LED
                    
                    printf("Verified ID: %d\n", id);
                    db_insert_event(id, EEventTypeEntry, true);
                }
                else {
                    // turn on red LED
                    
                    printf("Finger not found\n");
                    db_insert_event(-1, EEventTypeEntry, false);
                }
            }
            else {
                // turn off orange LED
            }
        }
        usleep(100 * 1000);
    }
}


static void * scan_exit(void * aUnused) {
    
    // setup
    sExitFPS.SetLED(true);
    sScanExitLoop = true;
    
    // loop
    while(1) {
        if(sScanExitLoop) {
            if(sExitFPS.IsPressFinger()) {
                // turn on orange LED
                
                sExitFPS.CaptureFinger(false);
                int id = sExitFPS.Identify1_N();
                if(id < MAX_FGP_COUNT) {
                    // turn on green LED
                    
                    printf("Verified ID: %d\n", id);
                    db_insert_event(id, EEventTypeExit, true);
                }
                else {
                    // turn on red LED
                    
                    printf("Finger not found\n");
                    db_insert_event(-1, EEventTypeExit, false);
                }
            }
            else {
                // turn off orange LED
            }
        }
        usleep(100 * 1000);
    }
}


/*
 * Used for testing communication w/ module.
 */
static void * blink(void * aPort) {
    sBlinkLoop = true;

    // loop
    while(sBlinkLoop) {
        if(sEntryFPS.IsAvailable()) sEntryFPS.SetLED(true);
        if(sExitFPS.IsAvailable()) sExitFPS.SetLED(true);
        usleep(500 * 1000);
        if(sEntryFPS.IsAvailable()) sEntryFPS.SetLED(false);
        if(sExitFPS.IsAvailable()) sExitFPS.SetLED(false);
        usleep(500 * 1000);
    }
    printf("BLINK thread stopped.\n");
    return NULL;
}

int main() {
    // init sqlite, curl and devices
    db_open();
    req_init();
    
    
    // TODO: parse command line
    sEntryFPS.UseSerialDebug=true;

    
    if(sEntryFPS.IsAvailable()) sEntryFPS.Open();
    else printf("ERROR: Entry GT511 not found !\n");
        
    if(sExitFPS.IsAvailable()) sExitFPS.Open();
    else printf("ERROR: Exit GT511 not found !\n");
    
    sEntryFPS.GetTemplate(0);
    sEntryFPS.GetImage();

    
    
    pthread_t thr_blink;
    pthread_t thr_entry_scan;
    pthread_t thr_exit_scan;
    
    if(0 == pthread_create(&thr_entry_scan, NULL, scan_entry, NULL)) {
        printf("ENTRY scan thread created !\n");
    }

    if(0 == pthread_create(&thr_exit_scan, NULL, scan_exit, NULL)) {
        printf("EXIT scan thread created !\n");
    }
    
    char command[256];
    
    do {
        printf("$> ");

        if (fgets(command, sizeof(command), stdin)) {
            
            // remove '\n'
            command[strlen(command) - 1] = '\0';
            if(strcasecmp(command, COMMAND_ENROLL) == 0) {
                sScanExitLoop = false;
                if(enroll() == 0) {
                    // TODO: sync GT511s
                }
                else {
                    
                }
            }
            else if(strcasecmp(command, COMMAND_PAUSE_ENTRY) == 0) {
                sScanEntryLoop = false;
                printf("ENTRY scan thread paused !\n");
            }
            else if(strcasecmp(command, COMMAND_RESUME_ENTRY) == 0) {
                sScanEntryLoop = true;
                printf("ENTRY scan thread resumed !\n");
            }
            else if(strcasecmp(command, COMMAND_PAUSE_EXIT) == 0) {
                sScanExitLoop = false;
                printf("EXIT scan thread paused !\n");
            }
            else if(strcasecmp(command, COMMAND_RESUME_EXIT) == 0) {
                sScanExitLoop = true;
                printf("EXIT scan thread resumed !\n");
            }
            else if(strcasecmp(command, COMMAND_DELETE_FGP) == 0) {
                printf("\nEnter user e-mail:");
                char * email = getline();
                int _id = db_get_user_id(email);
                if(_id == -1) {
                    printf("Unknown user e-mail.");
                }
                else {
                    printf("Do you really want to delete fingerprints for user %d / %s ?\n", _id, email);
                    char * response = getline();
                    if(0 == strcasecmp(response, COMMAND_YES)) {
                        db_delete_user_data(_id, false);
                        printf("Done.");
                    }
                    else {
                        printf("Cancelled.");
                    }
                }
            }
            else if(strcasecmp(command, COMMAND_DELETE_USER) == 0) {
                printf("Enter user e-mail: ");
                char * email = getline();
                int _id = db_get_user_id(email);
                if(_id == -1) {
                    printf("Unknown user e-mail.");
                }
                else {
                    printf("Do you really want to delete user %d / %s ?\nAll matching fingerprint data will be deleted: ", _id, email);
                    char * response = getline();
                    if(0 == strcasecmp(response, COMMAND_YES)) {
                        db_delete_user_data(_id, true);
                        printf("Done.");
                    }
                    else {
                        printf("Cancelled.");
                    }
                }
            }
            else if(strcasecmp(command, COMMAND_CREATE_USER) == 0) {
                printf("Enter user e-mail: ");
                char * email = getline();
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
                    char * response = getline();
                    if(0 == strcasecmp(response, COMMAND_YES)) {
                        enroll();
                    }
                }
            }
            else if(strcasecmp(command, COMMAND_EMPTY_FGP) == 0) {
            }
            else if(strcasecmp(command, COMMAND_SYNC) == 0) {
                printf("Synchronize device data now ?\nThe scanning will be paused: ");
                char * response = getline();
                if(0 == strcasecmp(response, COMMAND_YES)) {
                    sScanExitLoop = false;
                    sScanEntryLoop = false;

                    // TODO:
                    
                    printf("Done.");
                    sScanExitLoop = true;
                    sScanEntryLoop = true;
                }
                else {
                    printf("Cancelled.");
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
            else {
                printf("Unknown command.\n");
            }
        }
    }
    while (strcasecmp(command, COMMAND_EXIT));

    sEntryFPS.Close();
    sExitFPS.Close();
    
    req_cleanup();
    db_close();

}
