//
//  gt511.h
//  digilock
//
//  Created by Olivier on 21/01/2015.
//  Copyright (c) 2015 etincelle. All rights reserved.
//

#ifndef __digilock__gt511__
#define __digilock__gt511__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include "rs232.h"
#include <pthread.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <sys/time.h>
    
#define MAX_FGP_COUNT		2000 // for GT511-C5
// #define MAX_FGP_COUNT		200 // for GT511-C3
    
    typedef enum {
        ECommandNotSet				= 0x00,		// Default value for enum. Scanner will return error if sent this.
        ECommandOpen				= 0x01,		// Open Initialization
        ECommandClose				= 0x02,		// Close Termination
        ECommandUsbInternalCheck	= 0x03,		// UsbInternalCheck Check if the connected USB device is valid
        ECommandChangeEBaudRate		= 0x04,		// ChangeBaudrate Change UART baud rate
        ECommandSetIAPMode			= 0x05,		// SetIAPMode Enter IAP Mode In this mode, FW Upgrade is available
        ECommandCmosLed				= 0x12,		// CmosLed Control CMOS LED
        ECommandGetEnrollCount		= 0x20,		// Get enrolled fingerprint count
        ECommandCheckEnrolled		= 0x21,		// Check whether the specified ID is already enrolled
        ECommandEnrollStart			= 0x22,		// Start an enrollment
        ECommandEnroll1				= 0x23,		// Make 1st template for an enrollment
        ECommandEnroll2				= 0x24,		// Make 2nd template for an enrollment
        ECommandEnroll3				= 0x25,		// Make 3rd template for an enrollment, merge three templates into one template, save merged template to the database
        ECommandIsPressFinger		= 0x26,		// Check if a finger is placed on the sensor
        ECommandDeleteID			= 0x40,		// Delete the fingerprint with the specified ID
        ECommandDeleteAll			= 0x41,		// Delete all fingerprints from the database
        ECommandVerify1_1			= 0x50,		// Verification of the capture fingerprint image with the specified ID
        ECommandIdentify1_N			= 0x51,		// Identification of the capture fingerprint image with the database
        ECommandVerifyTemplate1_1	= 0x52,		// Verification of a fingerprint template with the specified ID
        ECommandIdentifyTemplate1_N	= 0x53,		// Identification of a fingerprint template with the database
        ECommandCaptureFinger		= 0x60,		// Capture a fingerprint image(256x256) from the sensor
        ECommandMakeTemplate		= 0x61,		// Make template for transmission
        ECommandGetImage			= 0x62,		// Download the captured fingerprint image(256x256)
        ECommandGetRawImage			= 0x63,		// Capture & Download raw fingerprint image(320x240)
        ECommandGetTemplate			= 0x70,		// Download the template of the specified ID
        ECommandSetTemplate			= 0x71,		// Upload the template of the specified ID
        ECommandGetDatabaseStart	= 0x72,		// Start database download, obsolete
        ECommandGetDatabaseEnd		= 0x73,		// End database download, obsolete
        ECommandUpgradeFirmware		= 0x80,		// Not supported
        ECommandUpgradeISOCDImage	= 0x81,		// Not supported
    } ECommandCode;

    

    typedef enum {
        NO_ERROR					= 0x0000,	// Default value. no error
        NACK_TIMEOUT				= 0x1001,	// Obsolete, capture timeout
        NACK_INVALID_BAUDRATE		= 0x1002,	// Obsolete, Invalid serial baud rate
        NACK_INVALID_POS			= 0x1003,	// The specified ID is not between 0...(MAX_FGP_COUNT - 1)
        NACK_IS_NOT_USED			= 0x1004,	// The specified ID is not used
        NACK_IS_ALREADY_USED		= 0x1005,	// The specified ID is already used
        NACK_COMM_ERR				= 0x1006,	// Communication Error
        NACK_VERIFY_FAILED			= 0x1007,	// 1:1 Verification Failure
        NACK_IDENTIFY_FAILED		= 0x1008,	// 1:N Identification Failure
        NACK_DB_IS_FULL				= 0x1009,	// The database is full
        NACK_DB_IS_EMPTY			= 0x100A,	// The database is empty
        NACK_TURN_ERR				= 0x100B,	// Obsolete, Invalid order of the enrollment (The order was not as: EnrollStart -> Enroll1 -> Enroll2 -> Enroll3)
        NACK_BAD_FINGER				= 0x100C,	// Too bad fingerprint
        NACK_ENROLL_FAILED			= 0x100D,	// Enrollment Failure
        NACK_IS_NOT_SUPPORTED		= 0x100E,	// The specified command is not supported
        NACK_DEV_ERR				= 0x100F,	// Device Error, especially if Crypto-Chip is trouble
        NACK_CAPTURE_CANCELED		= 0x1010,	// Obsolete, The capturing is canceled
        NACK_INVALID_PARAM			= 0x1011,	// Invalid parameter
        NACK_FINGER_IS_NOT_PRESSED	= 0x1012,	// Finger is not pressed
        INVALID						= 0XFFFF	// Used when parsing fails
    } EErrorCode;
    
    // =========================================================================================================
    // PACKETS
    // =========================================================================================================
#define PACKET_START_CODE_1         0x55
#define PACKET_START_CODE_2         0xAA
#define PACKET_DEVICE_ID            0x01
//#define PACKET_HEADER_LEN           2
//#define PACKET_DEVICE_ID_LEN        2
//#define PACKET_PARAMETER_LEN        4
//#define PACKET_CODE_LEN             2
//#define PACKET_CHECKSUM_LEN         2
#define PACKET_LEN                  12
    
#define DATA_START_CODE_1           0x5A
#define DATA_START_CODE_2           0xA5
#define DATA_DEVICE_ID              0x01
#define DATA_HEADER_LEN             2
#define DATA_DEVICE_ID_LEN          2
#define DATA_CHECKSUM_LEN           2
    
    
//#define DEVICE_INFO_FIRMWARE_LEN    4
//#define DEVICE_INFO_MAX_AREA_LEN    4
//#define DEVICE_INFO_SERIAL_LEN      16

#define RESPONSE_ACK                0x30
#define RESPONSE_NACK               0x31
    
#define RETCODE_NACK                -1
#define RETCODE_TIMEOUT             -2
#define RETCODE_BAD_CHECKSUM        -3
#define RETCODE_UNKNOWN_ACK         -4
#define RETCODE_INVALID_RESPONSE    -5
    
    
#define TEMPLATE_DATA_LEN       498
#define IMAGE_DATA_LEN          (258 * 202)
#define RAW_IMAGE_DATA_LEN      (160 * 120)
    
#define RESPONSE_TIMEOUT_MS     2000
    
typedef enum {
    PACKET_BYTE_START_CODE_1    = 0,
    PACKET_BYTE_START_CODE_2    = 1,
    PACKET_BYTE_DEVICE_ID       = 2,
    PACKET_BYTE_PARAMETER       = 4,
    PACKET_BYTE_CODE            = 8,
    PACKET_BYTE_CHECKSUM        = 10
} EPacketBytes;
    
typedef enum {
    DATA_BYTE_START_CODE_1 = 0,
    DATA_BYTE_START_CODE_2,
    DATA_BYTE_DEVICE_ID
} EDataBytes;
    
typedef enum {
    DEVINFO_BYTE_MAX_AREA = 0,
    DEVINFO_BYTE_FIRMWARE = 4,
    DEVINFO_BYTE_SERIAL = 8
} EDevInfoBytes;
    
    typedef struct {
        uint8_t         com_port;
        uint8_t         available;
        uint8_t         open;
        uint8_t         debug;
        pthread_mutex_t mutex;
    } GT511;
    
    
    typedef struct {
        uint8_t         start1;
        uint8_t         start2;
        uint16_t        device_id;
        uint32_t        parameter;
        uint16_t        code;
        uint16_t        checksum;
    } sPacketBytes;
    
    typedef struct {
        uint8_t         start1;
        uint8_t         start2;
        uint16_t        device_id;
        uint8_t *       data;
        uint16_t        checksum;
    } sDataBytes;
    
    typedef struct {
        uint32_t        firmware;
        uint32_t        isomax;
        uint8_t         serial[16];
    } sDeviceInfo;

    
    
    
    GT511 *     GT511_Init(char * aComPort, int aDebug);
    void        GT511_Free(GT511 * aDevice);
    void        GT511_SetLED(GT511 * aDevice, uint8_t aOn);
    int         GT511_GetEnrollCount(GT511 * aDevice);
    int         GT511_CheckEnrolled(GT511 * aDevice, int aID);
    void        GT511_EnrollStart(GT511 * aDevice, int aID);
    void        GT511_Enroll1(GT511 * aDevice);
    void        GT511_Enroll2(GT511 * aDevice);
    void        GT511_Enroll3(GT511 * aDevice);
    int         GT511_IsFingerPressed(GT511 * aDevice);
    void        GT511_DeleteID(GT511 * aDevice, int aID);
    void        GT511_DeleteAll(GT511 * aDevice);
    void        GT511_Verify(GT511 * aDevice, int aID);
    int         GT511_Identify(GT511 * aDevice, int * aID);
    void        GT511_VerifyTemplate(GT511 * aDevice, int aID, uint8_t * aTemplateData);
    void        GT511_IdentifyTemplate(GT511 * aDevice, uint8_t * aTemplateData);
    void        GT511_CaptureFinger(GT511 * aDevice, int aHiRes);
    uint8_t *   GT511_MakeTemplate(GT511 * aDevice, int aID);
    uint8_t *   GT511_GetImage(GT511 * aDevice);
    uint8_t *   GT511_GetRawImage(GT511 * aDevice);
    uint8_t *   GT511_GetTemplate(GT511 * aDevice, int aID);
    void        GT511_SetTemplate(GT511 * aDevice, int aID, int aCheckDuplicate);

#ifdef __cplusplus
}
#endif
    
#endif /* defined(__digilock__gt511__) */
