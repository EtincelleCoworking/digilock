//
//  req.h
//  gt511
//
//  Created by Olivier on 26/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#ifndef __gt511__req__
#define __gt511__req__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

int req_init(char * aSiteStr, char * aBaseURL);
int req_cleanup();
int req_log_fingerprint(long long aTimestamp, int aEventType, int aFingerprintID, int aDetectionMS, int aResult);
int req_log_intercom(long long aTimestamp, int aNumPresses, int aResult);
int req_user(int aUserID, char * aNick, char * aEmail);
int req_enroll(int aUserID, int aFingerprintID, char * aData);
int req_intercom_api(char * location_slug, char * location_key);

long long millisecs();

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* defined(__gt511__req__) */
