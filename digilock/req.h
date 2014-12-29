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


int req_init();
int req_cleanup();
int req_log(long long aTimestamp, int aEventType, int aFingerprintID, int aResult);
int req_user(int aUserID, char * aEmail);
int req_fgp(int aUserID, int aFingerprintID, char * aData64);
    
    
#ifdef __cplusplus
} /* extern "C" */
#endif
    
#endif /* defined(__gt511__req__) */
