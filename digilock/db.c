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
#include "base64.h"

static sqlite3 * sDB;

sqlite3 * db_get_database() {
    return sDB;
}


int db_close() {
	int ret = sqlite3_close(sDB);
	if(ret != SQLITE_OK) {
        printf("Can't close database: %s\n", sqlite3_errmsg(sDB));
	}
	else {
        printf("Closed database successfully\n");
	}

	return ret;
}


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
        printf("Can't open database: %s\n", sqlite3_errmsg(sDB));
		return -1;
	} else {
        printf( "Opened database %s successfully\n", aDatabaseFile);
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
         "%s TEXT);",
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
        printf("SQL error:  %s\n", sqlite3_errmsg(sDB));
    } else {
        int count = sqlite3_data_count(stmt);
        if(count > 0)
            _id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));

    return _id;
}


uint8_t * db_get_fingerprint(int aFingerprintID) {

    char * b64 = NULL;
    //uint8_t * blob = NULL;
    int b64_len = 0;
    int fgp_len = 0;
    char sql[256];
    sqlite3_stmt * stmt;
    uint8_t * fgp = NULL;
    int checksum = 0;
  //  bool ok = false;

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
        printf("SQL error:  %s\n", sqlite3_errmsg(sDB));
    } else {
        int count = sqlite3_data_count(stmt);
        if(count > 0) {
            b64 = (char *)sqlite3_column_text(stmt, 0);
            b64_len = sqlite3_column_bytes(stmt, 0);
            //printf("db_get_template b64 %s len %d\n", b64, strlen(b64));
            checksum = sqlite3_column_int(stmt, 1);
            //printf("db_get_template chk %d\n", checksum);
        }
    }

    if(b64 != NULL) {
        
        fgp = unbase64(b64, b64_len /* WTF !!! */, &fgp_len);

        //printf("db_get_template decode %d bytes from %d bytes\n", template_len, strlen(b64));

        //if(fgp != NULL) {
            int i;
            int calc_checksum = 0;
            for(i = 0; i < fgp_len; i++) {
                calc_checksum += fgp[i];
            }

            //printf("db_get_template fgp not null\n");

            if(checksum != calc_checksum) {
                free(fgp);
                fgp = NULL;
                printf("db_get_template bad checksum (%d; %d)\n", checksum, calc_checksum);

            }
        //}
    }

    sqlite3_finalize(stmt);
    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));


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
    }
    else {
        valid_params = false;
    }

    if(valid_params) {
        sqlite3_mutex_enter(sqlite3_db_mutex(sDB));
        sqlite3_prepare(sDB, sql, (int)strlen(sql) + 1, &stmt, NULL);
        if (sqlite3_step(stmt) == SQLITE_ERROR) {
            printf("SQL error:  %s\n", sqlite3_errmsg(sDB));
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
        printf("SQL error:  %s\n", sqlite3_errmsg(sDB));
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


//int db_delete_fingerprints() {
//    int rc;
//    char sql[256];
//    
//    sprintf(sql,
//            "DELETE FROM %s;",
//            TABLE_FGP);
//    
//    rc = exec(sql, true, EDBAlertError);
//    return rc;
//}


int db_insert_fingerprint(int aUserID, int aFingerprintID, uint8_t * aData, int aDataLength) {
	int rc;

    int checksum = 0;
    int i = 0;
    for(i = 0; i < aDataLength; i++) {
        checksum += aData[i];
    }

    int b64len = 0;
    char * b64 = base64(aData, aDataLength, &b64len);
    char * sql = (char *)malloc(b64len + 256);


	// insert or update fgp ?
    bool exists = false;
    sprintf(sql, "SELECT COUNT(*) FROM %s WHERE %s = %d;",
            TABLE_FGP,
            TABLE_FGP_ID,
            aFingerprintID);
    sqlite3_stmt * stmt;
    sqlite3_mutex_enter(sqlite3_db_mutex(sDB));
    sqlite3_prepare(sDB, sql, (int)strlen(sql) + 1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ERROR) {
        printf("SQL error:  %s\n", sqlite3_errmsg(sDB));
    } else {
        exists = !!sqlite3_column_int(stmt, 0);
    }
    sqlite3_mutex_leave(sqlite3_db_mutex(sDB));
    
    if(exists) {
        sprintf(sql,
            "UPDATE %s SET %s = '%s', %s = %d" \
            " WHERE %s = %d;",
            TABLE_FGP,
            TABLE_FGP_DATA,
            b64,
            TABLE_FGP_CHECKSUM,
            checksum,
            TABLE_FGP_ID,
            aFingerprintID);
    }
    else {
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
    }
    rc = exec(sql, true, EDBAlertError);

    // link user <=> fgp
	sprintf(sql,
		"INSERT INTO %s (%s, %s) " \
        "VALUES (%d, %d);",
        TABLE_USER_FGP,
        TABLE_USER_FGP_FID,
        TABLE_USER_FGP_UID,
		aFingerprintID,
		aUserID);
	rc |= exec(sql, true, EDBAlertError);
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

