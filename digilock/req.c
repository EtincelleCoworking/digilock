//
//  req.c
//  gt511
//
//  Created by Olivier on 26/12/2014.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#include "req.h"
#include <string.h>
#include <ctype.h>

#define SERVER_BASE_URL     "http://sesame.coworking-toulouse.com/"

#include <inttypes.h>
#include <string.h>

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = (char *)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = (char *)malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') {
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


int req_init() {
    curl_global_init(CURL_GLOBAL_ALL);
    return 0;
}


int req_cleanup() {
    curl_global_cleanup();
    return 0;
}


int req_log(long long aTimestamp, int aEventType, int aFingerprintID, int aResult) {

    CURL *curl;
    CURLcode res = (CURLcode)-1;

    /* In windows, this will init the winsock stuff */
    // curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
          /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        char url[256];
        sprintf(url, "%s%s", SERVER_BASE_URL, "api/log");
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* Now specify the POST data */
        char str[256];
        sprintf(str, "when=%lld&kind=%d&fingerprint_id=%d&result=%d", aTimestamp, aEventType, aFingerprintID, aResult);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (str));

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "req_log() failed: %s\n", curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return (int)res;
}



int req_user(int aUserID, char * aEmail) {

    CURL *curl;
    CURLcode res = (CURLcode)-1;

    curl = curl_easy_init();
    if(curl) {

       char url[256];
       sprintf(url, "%s%s", SERVER_BASE_URL, "api/user");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

        char str[256];
      sprintf(str, "id=%d&email=%s", aUserID, url_encode(aEmail));
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);

      res = curl_easy_perform(curl);
      if(res != CURLE_OK)
            fprintf(stderr, "req_user() failed: %s\n",  curl_easy_strerror(res));

      curl_easy_cleanup(curl);
    }
    return (int)res;
}




int req_fgp(int aUserID, int aFingerprintID, uint8_t * aData) {
    CURL *curl;
    CURLcode res = (CURLcode)-1;

    curl = curl_easy_init();
    if(curl) {

        char url[256];
        sprintf(url, "%s%s%d%s", SERVER_BASE_URL, "api/user/", aUserID, "/fingerprint");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

        char str[256];
        
        // TODO: make base64 string w/ 
        const char * data64 = "TODO :[";
        sprintf(str, "id=%d&image=%s", aFingerprintID, url_encode((char *)data64));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "req_fgp() failed: %s\n",  curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    return (int)res;
}

