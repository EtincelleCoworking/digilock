//
//  db.c
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

#include "db.h"
#include "req.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table() {

    decoding_table = (char*)malloc(256);
int i;
    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


void base64_cleanup() {
    free(decoding_table);
}

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = (char*)malloc(*output_length);
    if (encoded_data == NULL) return NULL;
int i,j;
    for (i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = (unsigned char*)malloc(*output_length);
    if (decoded_data == NULL) return NULL;
int i,j;
    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}




static sqlite3 * sDB;

sqlite3 * db_get_database() {
    return sDB;
}


//long long db_timestamp() {
//    struct timeval te;
//    gettimeofday(&te, NULL); // get current time
//    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
//    return milliseconds;
//}


int db_close() {
	int ret = sqlite3_close(sDB);
	if(ret != SQLITE_OK) {
		fprintf(stderr, "Can't close database: %s\n", sqlite3_errmsg(sDB));
	}
	else {
        fprintf(stdout, "Closed database successfully\n");
	}

	return ret;
}


//static int exec_callback(void *NotUsed, int argc, char **argv, char **azColName){
//	int i;
//	for(i = 0; i < argc; i++){
//		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//	}
//	printf("\n");
//	return 0;
//}


int alert(const char * aSubject, char * aMessage, EDBAlert aLevel) {
	// TODO:
    printf("***ALERT LEVEL %d:\n%s\n%s\n", aLevel, aSubject, aMessage);
	return 0;
}


static int exec(char * aSQL, bool aCommit, EDBAlert aAlertLevel) {
	char *zErrMsg = 0;
	int rc;
    
    sqlite3_mutex_enter(sqlite3_db_mutex(sDB));


	if(aCommit) {
		rc = sqlite3_exec(sDB, "BEGIN TRANSACTION;", NULL, NULL, NULL);
		if(rc != SQLITE_OK ){
			alert("BEGIN TRANSACTION FAILED", aSQL, aAlertLevel);
			return rc;
		}
	}
	
	rc = sqlite3_exec(sDB, aSQL, NULL, 0, &zErrMsg);
	if(rc != SQLITE_OK ){
        printf("SQL error: %s\n", zErrMsg);
		alert(aSQL, zErrMsg, aAlertLevel);
		sqlite3_free(zErrMsg);
	} else {
        //printf("SQL OK: %s\n", aSQL);
		if(aCommit) {
			rc = sqlite3_exec(sDB, "END TRANSACTION;", NULL, NULL, NULL);
			if(rc != SQLITE_OK ){
				alert("END TRANSACTION FAILED", aSQL, aAlertLevel);
				return rc;
			}
		}
	}
    
    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));
	return rc;
}


int db_drop_tables() {
    char sql[64];
    int rc = -1;

    sprintf(sql, "DROP TABLE %s;", TABLE_EVENT);
    rc = exec(sql, false, EDBAlertNone);
    sprintf(sql, "DROP TABLE %s;", TABLE_FGP);
    rc = exec(sql, false, EDBAlertNone);
    sprintf(sql, "DROP TABLE %s;", TABLE_USER);
    rc = exec(sql, false, EDBAlertNone);
    sprintf(sql, "DROP TABLE %s;", TABLE_USER_FGP);
    rc = exec(sql, false, EDBAlertNone);
 
    return rc;
}


int db_open(char * aDatabaseFile) {
    int rc = -1;
    char sql[256];

	/* Open database */
    rc = sqlite3_open(aDatabaseFile, &sDB);
    
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(sDB));
		return -1;
	} else {
        fprintf(stdout, "Opened database %s successfully\n", aDatabaseFile);
	}

	/* Create tables */
	sprintf(sql, 
		 "CREATE TABLE IF NOT EXISTS %s ("  \
         "%s INTEGER PRIMARY KEY AUTOINCREMENT," \
         "%s TEXT NOT NULL, " \
         "%s TEXT NOT NULL);",
         TABLE_USER,
         TABLE_USER_ID,
         TABLE_USER_EMAIL,
         TABLE_USER_NICK);
	rc |= exec(sql, false, EDBAlertFatal);
	
	sprintf(sql, 
		 "CREATE TABLE IF NOT EXISTS %s ("  \
         "%s INT PRIMARY KEY NOT NULL," \
         "%s INT," \
         "%s VARCHAR(1024));",
         TABLE_FGP,
         TABLE_FGP_ID,
         TABLE_FGP_CHECKSUM,
         TABLE_FGP_DATA);
	rc |= exec(sql, false, EDBAlertFatal);
	
	sprintf(sql, 
		 "CREATE TABLE IF NOT EXISTS %s ("  \
         "%s INT SECONDARY KEY NOT NULL," \
         "%s INT SECONDARY KEY NOT NULL);",
         TABLE_USER_FGP,
         TABLE_USER_FGP_FID,
         TABLE_USER_FGP_UID);
	rc |= exec(sql, false, EDBAlertFatal);
         
	sprintf(sql, 
		 "CREATE TABLE IF NOT EXISTS %s ("  \
         "%s INTEGER PRIMARY KEY AUTOINCREMENT," \
         "%s INTEGER NOT NULL," \
         "%s INT," \
         "%s INT," \
         "%s INT," \
         "%s INT);",
         TABLE_EVENT,
         TABLE_EVENT_ID,
         TABLE_EVENT_TSTAMP,
         TABLE_EVENT_FGP_ID,
         TABLE_EVENT_DETECT,
         TABLE_EVENT_TYPE,
         TABLE_EVENT_RESULT);
	rc |= exec(sql, false, EDBAlertFatal);

    sprintf(sql,
            "CREATE TABLE IF NOT EXISTS %s ("  \
            "%s INTEGER PRIMARY KEY AUTOINCREMENT," \
            "%s INTEGER NOT NULL," \
            "%s INT," \
            "%s INT);",
            TABLE_INTERCOM,
            TABLE_INTERCOM_ID,
            TABLE_INTERCOM_TSTAMP,
            TABLE_INTERCOM_PRESSES,
            TABLE_INTERCOM_RESULT);
    rc |= exec(sql, false, EDBAlertFatal);

	return rc;
}


int db_insert_intercom_event(int aNumPresses, bool aResult) {
    int rc;
    char sql[256];
    
    long long ts = millisecs();
    sprintf(sql,
            "INSERT INTO %s (%s, %s, %s) "  \
            "VALUES (%lld, %d, %d);",
            TABLE_INTERCOM,
            TABLE_INTERCOM_TSTAMP,
            TABLE_INTERCOM_PRESSES,
            TABLE_INTERCOM_RESULT,
            ts,
            aNumPresses,
            (int)aResult);
    
    rc = exec(sql, true, EDBAlertError);
    if(rc == SQLITE_OK) {
        req_log_intercom(ts, aNumPresses, aResult);
    }
    return rc;
}


int db_insert_fingerprint_event(int aFingerprintID, int aDetectionMS, EEventType aType, bool aResult) {
	int rc;
	char sql[256];

    long long ts = millisecs();
	sprintf(sql,
        "INSERT INTO %s (%s, %s, %s, %s, %s) "  \
        "VALUES (%lld, %d, %d, %d, %d);",
        TABLE_EVENT,
        TABLE_EVENT_TSTAMP,
        TABLE_EVENT_FGP_ID,
        TABLE_EVENT_DETECT,
        TABLE_EVENT_TYPE,
        TABLE_EVENT_RESULT,
        ts,
        aFingerprintID,
        aDetectionMS,
        (int)aType,
        (int)aResult);

	rc = exec(sql, true, EDBAlertError);
    if(rc == SQLITE_OK) {
        req_log_fingerprint(ts, aType, aFingerprintID, aDetectionMS, aResult);
    }
	return rc;
}


int db_get_user_id(char * aEmail) {
    int _id = -1;
    char sql[256];
    sqlite3_stmt * stmt;

    sqlite3_mutex_enter(sqlite3_db_mutex(sDB));

    sprintf(sql,
        "SELECT %s FROM %s WHERE %s = '%s';",
        TABLE_USER_ID,
        TABLE_USER,
        TABLE_USER_EMAIL,
        aEmail);

    sqlite3_prepare(sDB, sql, (int)strlen(sql) + 1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ERROR) {
        fprintf(stderr, "SQL error:  %s\n", sqlite3_errmsg(sDB));
    } else {
        int count = sqlite3_data_count(stmt);
        if(count > 0)
            _id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));

    return _id;
}


uint8_t * db_get_template(int aFingerprintID) {

    const char * b64 = NULL;
    char sql[256];
    sqlite3_stmt * stmt;
    uint8_t * fgp = NULL;
    int checksum = 0;
    bool ok = false;

    sprintf(sql,
        "SELECT %s, %s FROM %s WHERE %s = %d;",
        TABLE_FGP_DATA,
        TABLE_FGP_CHECKSUM,
        TABLE_FGP,
        TABLE_FGP_ID,
        aFingerprintID);

    sqlite3_mutex_enter(sqlite3_db_mutex(sDB));
    sqlite3_prepare(sDB, sql, (int)strlen(sql) + 1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ERROR) {
        fprintf(stderr, "SQL error:  %s\n", sqlite3_errmsg(sDB));
    } else {
        int count = sqlite3_data_count(stmt);
        if(count > 0) {
            b64 = (const char *)sqlite3_column_text(stmt, 0);
            checksum = sqlite3_column_int(stmt, 1);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));

    if(b64 != NULL) {
        size_t template_len = 0;
        fgp = base64_decode(b64, strlen(b64), &template_len);
        if(fgp != NULL) {
            int i;
            int calc_checksum = 0;
            for(i = 0; i < template_len; i++) {
                calc_checksum += fgp[i];
            }
            if(checksum != calc_checksum) {
                free(fgp);
                fgp = NULL;
            }
        }
    }

    return fgp;
}

const char * db_get_user_name(int aUserID, int aFingerprintID, bool aEmail) {
    const char * name = NULL;
    bool valid_params = true;
    char sql[256];
    sqlite3_stmt * stmt;

    if(aUserID >= 0) {
        sprintf(sql,
            "SELECT %s FROM %s WHERE %s = %d;",
            aEmail ? TABLE_USER_EMAIL : TABLE_USER_NICK,
            TABLE_USER,
            TABLE_USER_ID,
            aUserID);

        printf("%s\n", sql);
    }
    else if(aFingerprintID >= 0) {
        sprintf(sql,
            "SELECT %s FROM %s WHERE %s = (SELECT %s FROM %s WHERE %s = %d);",
            aEmail ? TABLE_USER_EMAIL : TABLE_USER_NICK,
            TABLE_USER,
            TABLE_USER_ID,
            TABLE_USER_FGP_UID,
            TABLE_USER_FGP,
            TABLE_USER_FGP_FID,
            aFingerprintID);

        printf("%s\n", sql);
    }
    else {
        valid_params = false;
    }

    if(valid_params) {
        sqlite3_mutex_enter(sqlite3_db_mutex(sDB));
        sqlite3_prepare(sDB, sql, (int)strlen(sql) + 1, &stmt, NULL);
        if (sqlite3_step(stmt) == SQLITE_ERROR) {
            fprintf(stderr, "SQL error:  %s\n", sqlite3_errmsg(sDB));
        } else {
            int count = sqlite3_data_count(stmt);
            if(count > 0) {
                name = (const char *)sqlite3_column_text(stmt, 0);

                printf("%s\n", name);

            }
        }
        sqlite3_finalize(stmt);
        sqlite3_mutex_leave(sqlite3_db_mutex(sDB));
    }
    return name;
}


int db_count(char * aTableName) {
    int rows = -1;
    sqlite3_stmt * stmt;
    char sql[256];

    sqlite3_mutex_enter(sqlite3_db_mutex(sDB));

    sprintf(sql,
        "SELECT COUNT(*) FROM %s;",
        aTableName);
    sqlite3_prepare(sDB, sql, (int)strlen(sql) + 1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ERROR) {
        fprintf(stderr, "SQL error:  %s\n", sqlite3_errmsg(sDB));
    } else {
        rows = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));
    return rows;
}


int db_count_fingerprints() {
    return db_count((char *)TABLE_FGP);
}


int db_count_users() {
    return db_count((char *)TABLE_USER);
}


int db_insert_user(char * aEmail, char * aNick) {
	int rc;
	char sql[256];

	sprintf(sql,
        "INSERT INTO %s (%s, %s) " \
        "VALUES ('%s', '%s');",
        TABLE_USER,
        TABLE_USER_EMAIL,
        TABLE_USER_NICK,
        aEmail,
        aNick);

	rc = exec(sql, true, EDBAlertError);
    if(rc == SQLITE_OK) {
        rc = db_get_user_id(aEmail);
        req_user(rc, aNick, aEmail);
    }
    else {
        rc = -1;
    }

	return rc;
}


//static void * thr_insert_fgp(void * data) {
//    char * sql = (char *)data;
//    printf("exec %s\n", sql);
//   int rc = exec(sql, true, EDBAlertError);
//    free(sql);
//    printf("sql thread done %d\n", rc);
//}



int db_insert_fingerprint(int aUserID, int aFingerprintID, uint8_t * aData, int aDataLength) {
	int rc;

    int checksum = 0;
    int i = 0;
    for(i = 0; i < aDataLength; i++) {
        checksum += aData[i];
    }

//    printf("FGP data (len: %d, chk: %d):\n", aDataLength, checksum);
//    for(i = 0; i < aDataLength; i++) {
//        printf("%.02x ", aData[i]);
//    }
//    printf("\nBase64:\n");

    size_t b64len = 0;
    char * b64 = (char*)base64_encode(aData, aDataLength, &b64len);
    //printf(b64);
    //printf("\nlen=%d\n", b64len);


//    printf("\nUnbase64:\n");
//    size_t unb64len;
//    uint8_t * unb64 = base64_decode(b64, b64len, &unb64len);
//    for(i = 0; i < unb64len; i++) {
//        printf("%.02x ", unb64[i]);

//        if(unb64[i] != aData[i]) {
//            printf("Encode/decode error\n");
//            break;
//        }
//    }
//    printf("\nlen: %d\n", unb64len);


//    size_t datasz = 1 + (aDataLength * 3);
//    char * data =  (char *) malloc(datasz);
//    memset(data, 0, datasz);
//    char buf[3];
//    for(i = 0; i < aDataLength; i++) {
//        sprintf(buf, "%.02x ", aData[i]);
//        strcat(data, buf);
//    }
//    printf("%s\n", data);

    char * sql = (char *) malloc(strlen(b64) + 256);


	// insert fgp
	sprintf(sql,
		"INSERT INTO %s (%s, %s, %s) " \
        "VALUES (%d, '%s', %d);",
        TABLE_FGP,
        TABLE_FGP_ID,
        TABLE_FGP_DATA,
        TABLE_FGP_CHECKSUM,
		aFingerprintID,
        b64,
        checksum);
    rc = exec(sql, true, EDBAlertError);

    //char * msql = (char *)malloc(strlen(sql)+1);
    //strcpy(msql, sql);

    //pthread_t thr;
    //pthread_create(&thr, NULL, thr_insert_fgp, msql);
    //printf("insert fgp w/ %d\n", rc);

    // link user <=> fgp
	sprintf(sql,
		"INSERT INTO %s (%s, %s) " \
        "VALUES (%d, %d);",
        TABLE_USER_FGP,
        TABLE_USER_FGP_FID,
        TABLE_USER_FGP_UID,
		aFingerprintID,
		aUserID);
	rc = exec(sql, true, EDBAlertError);
    //printf("link fgp w/ %d\n", rc);

    if(rc == SQLITE_OK) {
        req_enroll(aUserID, aFingerprintID, b64);
        //printf("req done\n");
    }
    //printf("free...");
    free(sql);
    free(b64);
    //printf("free done\n");
	return rc;
}


int db_delete_user_data(int aUserID, bool aDeleteUser) {

	int rc;
	char sql[256];

	// remove fingerprints
	sprintf(sql,
		"DELETE FROM %s WHERE %s = (SELECT %s FROM %s WHERE %s = %d);",
		TABLE_FGP,
		TABLE_FGP_ID,
		TABLE_USER_FGP_FID,
		TABLE_USER_FGP,
		TABLE_USER_FGP_UID,
		aUserID);
	rc = exec(sql, true, EDBAlertWarning);

	// remove links
	sprintf(sql,
		"DELETE FROM %s WHERE %s = %d;",
		TABLE_USER_FGP,
		TABLE_USER_FGP_UID,
		aUserID);
	rc = exec(sql, true, EDBAlertWarning);

	// remove user
    if(aDeleteUser) {
        sprintf(sql,
            "DELETE FROM %s WHERE %s = %d;",
            TABLE_USER,
            TABLE_USER_ID,
            aUserID);
        rc = exec(sql, true, EDBAlertWarning);
    }
	return rc;
}

