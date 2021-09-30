//
//  req.c
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
#include <string>
#include <cstring>
#include "req.h"
#include <ctype.h>

#include <inttypes.h>


static char * gBaseURL;
static char * gSiteStr;

#define BUFFER_LEN  512

long long millisecs() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}

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


int req_init(char * aSiteStr, char * aBaseURL) {
    gSiteStr = (char *)malloc(strlen(aSiteStr) + 1);
    strcpy(gSiteStr, aSiteStr);
    gBaseURL = (char *)malloc(strlen(aBaseURL) + 1);
    strcpy(gBaseURL, aBaseURL);
    curl_global_init(CURL_GLOBAL_ALL);
    return 0;
}


int req_cleanup() {
    curl_global_cleanup();
    return 0;
}


int req_log_fingerprint(long long aTimestamp, int aEventType, int aFingerprintID, int aDetectionMS, int aResult) {

    CURL *curl;
    CURLcode res = (CURLcode)-1;

    /* In windows, this will init the winsock stuff */
    // curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        // http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

          /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        char url[BUFFER_LEN];
        sprintf(url, "%s%s", gBaseURL, "api/log/fingerprint");
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* Now specify the POST data */
        char str[BUFFER_LEN];
        sprintf(str, "site=%s&when=%lld&kind=%d&fingerprint_id=%d&detection_ms=%d&result=%d", gSiteStr, aTimestamp, aEventType, aFingerprintID, aDetectionMS, aResult);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (str));

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            printf("req_log_fingerprint() failed: %s\n", curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return (int)res;
}


int req_log_intercom(long long aTimestamp, int aNumPresses, int aResult) {

    CURL *curl;
    CURLcode res = (CURLcode)-1;

    /* In windows, this will init the winsock stuff */
    // curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        // http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

          /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        char url[BUFFER_LEN];
        sprintf(url, "%s%s", gBaseURL, "api/log/intercom");
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* Now specify the POST data */
        char str[BUFFER_LEN];
        sprintf(str, "site=%s&when=%lld&numpresses=%d&result=%d", gSiteStr, aTimestamp, aNumPresses, aResult);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (str));

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            printf("req_log_intercom() failed: %s\n", curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return (int)res;
}


int req_user(int aUserID, char * aNick, char * aEmail) {

    CURL *curl;
    CURLcode res = (CURLcode)-1;

    curl = curl_easy_init();
    if(curl) {
        // http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

        char url[BUFFER_LEN];
        sprintf(url, "%s%s", gBaseURL, "api/user");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

        char str[BUFFER_LEN];

        char * enc_nick = url_encode(aNick);
        char * enc_mail = url_encode(aEmail);


        sprintf(str, "site=%s&id=%d&nick=%s&email=%s", gSiteStr, aUserID, enc_nick, enc_mail);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            printf("req_user() failed: %s\n",  curl_easy_strerror(res));

        free(enc_mail);
        free(enc_nick);

        curl_easy_cleanup(curl);
    }
    return (int)res;
}



int req_enroll(int aUserID, int aFingerprintID, char * aData64) {
    CURL *curl;
    CURLcode res = (CURLcode)-1;

    curl = curl_easy_init();
    if(curl) {
        // http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

        char url[BUFFER_LEN];
        sprintf(url, "%s%s%d%s", gBaseURL, "api/user/", aUserID, "/fingerprint");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

        char * enc_data64 = url_encode(aData64);
        char str[BUFFER_LEN + strlen(enc_data64)];

        sprintf(str, "site=%s&id=%d&image=%s", gSiteStr, aFingerprintID, enc_data64);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            printf("req_fgp() failed: %s\n",  curl_easy_strerror(res));

        free(enc_data64);

        curl_easy_cleanup(curl);
    }
    return (int)res;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

int req_intercom_api(char * location_slug, char * location_key) {
    if(strlen(location_slug) == 0){
	printf("WARNING - LOCATION_SLUG is empty - ALWAYS reply Yes to remains compatible with other systems\n");
	return 1;
    }
    

    CURL *curl;
    CURLcode res = (CURLcode)-1;
    std::string readBuffer;

    curl = curl_easy_init();
    if(!curl) {
        return 0;
    }
    // http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    char url[BUFFER_LEN];
    sprintf(url, "https://etincelle.at/api/intercom/toulouse-wilson3");
    
    printf("API URL: %s\n", url);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    readBuffer.clear();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);

    printf("API CALL : %s\n", readBuffer.c_str());
    if(res != CURLE_OK)
        printf("req_fgp() failed: %s\n",  curl_easy_strerror(res));

    curl_easy_cleanup(curl);
    return (strcmp(readBuffer.c_str(), "Yes") == 0);
}


