/*
***************************************************************************
*
* Author: Olivier Huguenot
*
* Copyright (C) 2014 Olivier Huguenot
*
***************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
***************************************************************************
*
* This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*
***************************************************************************
*/
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>

static sqlite3 * sDB;

sqlite3 * db_get_database() {
    return sDB;
}


long long db_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}


int db_close() {
	int ret = sqlite3_close(sDB);
	if(ret != SQLITE_OK) {
		fprintf(stderr, "Can't close database: %s\n", sqlite3_errmsg(sDB));
	}
	else {
		fprintf(stdout, "Closed database %s successfully\n", DATABASE_FILE);
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


int alert(char * aSubject, char * aMessage, EDBAlert aLevel) {
	// TODO:
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
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		alert(aSQL, zErrMsg, aAlertLevel);
		sqlite3_free(zErrMsg);
	} else {
		fprintf(stdout, "SQL OK: %s\n", aSQL);
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

int db_open() {
   int rc = -1;
   char sql[256];

	/* Open database */
	rc = sqlite3_open(DATABASE_FILE, &sDB);
    
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(sDB));
		return -1;
	} else {
		fprintf(stdout, "Opened database %s successfully\n", DATABASE_FILE);
	}

	/* Create tables */
	sprintf(sql, 
		 "CREATE TABLE IF NOT EXISTS %s ("  \
         "%s INTEGER PRIMARY KEY AUTOINCREMENT," \
         "%s TEXT NOT NULL);",
         TABLE_USER,
         TABLE_USER_ID,
         TABLE_USER_EMAIL);
	rc |= exec(sql, false, EDBAlertFatal);
	
	sprintf(sql, 
		 "CREATE TABLE IF NOT EXISTS %s ("  \
         "%s INT PRIMARY KEY NOT NULL," \
         "%s INT," \
         "%s BLOB);",
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
         "%s INT);",
         TABLE_EVENT,
         TABLE_EVENT_ID,
         TABLE_EVENT_TSTAMP,
         TABLE_EVENT_FGP_ID,
         TABLE_EVENT_TYPE,
         TABLE_EVENT_RESULT);
	rc |= exec(sql, false, EDBAlertFatal);
	
	return rc;
}


int db_insert_event(int aFingerprintID, EEventType aType, bool aResult) {
	int rc;
	char sql[256];

    long long ts = db_timestamp();
	sprintf(sql,
        "INSERT INTO %s (%s, %s, %s, %s) "  \
        "VALUES (%lld, %d, %d, %d);",
        TABLE_EVENT,
        TABLE_EVENT_TSTAMP,
        TABLE_EVENT_FGP_ID,
        TABLE_EVENT_TYPE,
        TABLE_EVENT_RESULT,
        ts,
        aFingerprintID,
        (int)aType,
        (int)aResult);

	rc = exec(sql, true, EDBAlertError);
    if(rc == SQLITE_OK) {
        req_log(ts, aType, aFingerprintID, aResult);
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


int db_count_users() {
    int rows;
    sqlite3_stmt * stmt;
    char sql[256];

    sqlite3_mutex_enter(sqlite3_db_mutex(sDB));

    sprintf(sql,
        "SELECT COUNT(*) FROM %s;",
        TABLE_USER);
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


//int db_get_users() {
//    int _id = -1, rc = 0;
//    sqlite3_stmt * stmt;
//    char sql[256];
//    sprintf(sql,
//        "SELECT (%s) FROM %s ORDER BY %s;",
//        TABLE_USER_EMAIL,
//        TABLE_USER,
//        TABLE_USER_EMAIL);

//    sqlite3_prepare(db, sql, strlen(sql) + 1, &stmt, NULL);
//    while(sqlite3_step(stmt) == SQLITE_ROW) {
//        char * fname = sqlite3_column_text(stmt, 0);
//        char * lname = sqlite3_column_text(stmt, 0);
//    }
//    sqlite3_finalize(stmt);

//    return _id;
//}


int db_insert_user(char * aEmail) {
	int rc;
	char sql[256];

	sprintf(sql,
        "INSERT INTO %s (%s) " \
        "VALUES ('%s');",
        TABLE_USER,
        TABLE_USER_EMAIL,
        aEmail);

	rc = exec(sql, true, EDBAlertError);
    if(rc == SQLITE_OK) {
        rc = db_get_user_id(aEmail);
        req_user(rc, aEmail);
    }
    else {
        rc = -1;
    }

	return rc;
}


int db_insert_fingerprint(int aUserID, int aFingerprintID, uint16_t aChecksum, uint8_t * aData) {
	int rc;
	char sql[256];

	// insert fgp
	sprintf(sql,
		"INSERT INTO %s (%s, %s, %s) " \
        "VALUES (%d, '%s', %d);",
        TABLE_FGP,
        TABLE_FGP_ID,
        TABLE_FGP_DATA,
        TABLE_FGP_CHECKSUM,
		aFingerprintID,
		"", // TODO: aData,
		aChecksum);
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
	rc = exec(sql, true, EDBAlertError);
    if(rc == SQLITE_OK) {
        req_fgp(aUserID, aFingerprintID, aData);
    }

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

