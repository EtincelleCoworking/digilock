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


#ifndef _DB_H_
#define _DB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite3.h"
#include "req.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum EEventType {
	EEventTypeEntry = 1,
	EEventTypeExit,
    EEventTypeEnroll,
    EEventTypeReboot,
	EEventTypePowerOff,
	EEventTypeIntercom1,
	EEventTypeIntercom2,
	// ...
	EEventTypeLast
} EEventType;

typedef enum EDBAlert {
	EDBAlertNone = 0,
	EDBAlertVerbose,
	EDBAlertWarning,
	EDBAlertError,
	EDBAlertFatal
} EDBAlert;

#define DATABASE_FILE		"digilock.sqlite"

#define TABLE_USER			"User"
#define TABLE_USER_ID		"id"
//#define TABLE_USER_FNAME	"fname"
//#define TABLE_USER_LNAME	"lname"
#define TABLE_USER_EMAIL    "email"

#define TABLE_FGP			"Fingerprint"
#define TABLE_FGP_ID		"id"
#define TABLE_FGP_DATA		"data"
#define TABLE_FGP_CHECKSUM	"checksum"

#define TABLE_USER_FGP		"UserFingerprint"
#define TABLE_USER_FGP_FID	"fgp_id"
#define TABLE_USER_FGP_UID	"user_id"

#define TABLE_EVENT			"Event"
#define TABLE_EVENT_ID		"id"
#define TABLE_EVENT_TSTAMP	"timestamp"
#define TABLE_EVENT_FGP_ID	"fgp_id"
#define TABLE_EVENT_TYPE	"type"
#define TABLE_EVENT_RESULT  "result"


sqlite3 * db_get_database();
int db_close();
int db_open();
int db_insert_event(int aFingerprintID, EEventType aType, bool aResult);
int db_insert_user(char * aEmail);
int db_count_users();
int db_get_user_id(char * aEmail);
int db_insert_fingerprint(int aUserID, int aFingerprintID, uint8_t * aData);
int db_delete_user_data(int aUserID, bool aDeleteUser);
    int db_drop_tables();
    
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
