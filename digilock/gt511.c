//
//  gt511.c
//  digilock
//
//  Created by Olivier on 21/01/2015.
//  Copyright (c) 2015 etincelle. All rights reserved.
//

#include "gt511.h"

#define BYTE(v, n)     (uint8_t)((v >> (8 * n)) & 0xff)
static char sSerialMode[] = {'8', 'N', '1', 0};


static long long millisecs() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}

// =======================================================================================================
// PACKET MANAGEMENT
// =======================================================================================================


void PAK_Debug(uint8_t * aBytes, uint16_t aLength) {
    for(int i = 0; i < aLength; i++)
        printf("[%.4d]\t%.2X\n", i, aBytes[i]);
    printf("\n");
}



int PAK_SendCommand(GT511 * aDevice, ECommandCode aCommand, uint32_t aParameter) {
    
    // packet is little endian !
    uint8_t pak[PACKET_LEN];
    uint16_t checksum = 0;
    int retcode;
    
    // start code
    pak[PACKET_BYTE_START_CODE_1] = PACKET_START_CODE_1;
    pak[PACKET_BYTE_START_CODE_2] = PACKET_START_CODE_2;
    
    // device id
    *(uint16_t *)(&pak[PACKET_BYTE_DEVICE_ID]) = (uint16_t)PACKET_DEVICE_ID;
    
    // command parameter
    *(uint32_t *)(&pak[PACKET_BYTE_PARAMETER]) = (uint32_t)aParameter;
    
    // command code
    *(uint16_t *)(&pak[PACKET_BYTE_CODE]) = (uint16_t)aCommand;
   
    // checksum
    for(uint8_t idx = 0; idx < PACKET_BYTE_CHECKSUM; idx++) {
        checksum += pak[idx];
    }
    *(uint16_t *)(&pak[PACKET_BYTE_CHECKSUM]) = checksum;
    
    if(aDevice->debug) {
        printf("PAK_SendCommand will write %d bytes:\n", PACKET_LEN);
        PAK_Debug(pak, PACKET_LEN);
    }
    
    // send
    pthread_mutex_lock(&(aDevice->mutex));
    retcode = RS232_SendBuf(aDevice->com_port, pak, PACKET_LEN);
    pthread_mutex_unlock(&(aDevice->mutex));
    
    // wait a bit for device to process my command TODO: ?????
    usleep(100 * 1000);
    
    return retcode;
}


void PAK_SendData(GT511 * aDevice, uint8_t * aBytes, int aLength) {
    
    int data_packet_len = DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + aLength + DATA_CHECKSUM_LEN;
    uint8_t * data_packet = malloc(data_packet_len * sizeof(uint8_t));
    uint16_t checksum = DATA_START_CODE_1 + DATA_START_CODE_2 + DATA_DEVICE_ID;
    
    // build data packet :
    // header
    data_packet[DATA_BYTE_START_CODE_1] = DATA_START_CODE_1;
    data_packet[DATA_BYTE_START_CODE_2] = DATA_START_CODE_2;
    
    // device ID
    *(uint16_t *)(&data_packet[DATA_BYTE_DEVICE_ID]) = DATA_DEVICE_ID;
    
    // data
    for(int i = 0; i < aLength; i++) {
        data_packet[DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + i] = aBytes[i];
        checksum += aBytes[i];
    }
    
    // checksum
    *(uint16_t *)(data_packet + DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + aLength) = checksum;
    
    long long ms = millisecs();
    pthread_mutex_lock(&(aDevice->mutex));
    RS232_SendBuf(aDevice->com_port, data_packet, data_packet_len);
    
    pthread_mutex_unlock(&(aDevice->mutex));
    printf("PAK_SendData sent %d bytes in %d ms.\n", aLength, (int)(millisecs() - ms));
    
    if (aDevice->debug) {
        printf("FPS - SEND DATA: ");
        PAK_Debug(data_packet, data_packet_len);
    }
    free(data_packet);
}


uint8_t * PAK_ReadBytes(GT511 * aDevice, int aExpectedByteCount) {
    int n, index = 0;
    uint8_t * resp = (uint8_t *)malloc(aExpectedByteCount);
    uint8_t * buffer = (uint8_t *)malloc(aExpectedByteCount);
    long long ms = millisecs();

    // read bytes from serial port
    pthread_mutex_lock(&(aDevice->mutex));
    while(index < aExpectedByteCount) {
        n = RS232_PollComport(aDevice->com_port, buffer, aExpectedByteCount);
        if(n <= aExpectedByteCount && n > 0) {
            memcpy(resp + index, buffer, n);
            index += n;
        }
        else {
            // not enough bytes read, wait a bit
            usleep(10 * 1000);
            if(millisecs() - ms > RESPONSE_TIMEOUT_MS) {
                // reading response took too much time
                printf("PAK_ReadBytes TIMEOUT (read %d/%d bytes) !\n", n, aExpectedByteCount);
                break;
            }
        }
    }
    pthread_mutex_unlock(&(aDevice->mutex));
    free(buffer);
    
    if(index != aExpectedByteCount) {
        free(resp);
        resp = NULL;
    }
    
    return resp;
}


int PAK_ReadResponse(GT511 * aDevice, uint32_t * aParameter) {
    int retcode;
    uint8_t * pak = PAK_ReadBytes(aDevice, PACKET_LEN);
    if(pak != NULL) {
        
        if(aDevice->debug) {
            printf("PAK_ReadResponse did read %d bytes:\n", PACKET_LEN);
            PAK_Debug(pak, PACKET_LEN);
        }
        
        // compute checksum
        uint16_t checksum = 0;
        for(uint8_t idx = 0; idx < PACKET_BYTE_CHECKSUM; idx++) {
            checksum += pak[idx];
        }
        
        if(checksum == *(uint16_t *)(&pak[PACKET_BYTE_CHECKSUM])) {
            // check response format
            uint8_t isvalid = TRUE;
            isvalid &= (pak[PACKET_BYTE_START_CODE_1] == PACKET_START_CODE_1);
            isvalid &= (pak[PACKET_BYTE_START_CODE_2] == PACKET_START_CODE_2);
            isvalid &= (*(uint16_t *)(&pak[PACKET_BYTE_DEVICE_ID]) == PACKET_DEVICE_ID);
           
            if(isvalid) {
                // get return code
                uint32_t parameter = *(uint32_t *)(&pak[PACKET_BYTE_PARAMETER]);
                uint16_t response = *(uint16_t *)(&pak[PACKET_BYTE_CODE]);
                if (aParameter != NULL) {
                    *aParameter = parameter;
                }
                if(response == RESPONSE_ACK) {
                    // packet is well-formed
                    retcode = 0;
                }
                else if(response == RESPONSE_NACK) {
                    retcode = RESPONSE_NACK;
                }
                else {
                    // unknown return code
                    retcode = RETCODE_UNKNOWN_ACK;
                }
            }
            else {
                retcode = RETCODE_INVALID_RESPONSE;
            }
        }
        else {
            retcode = RETCODE_BAD_CHECKSUM;
        }
        free(pak);
    }
    else {
        retcode = RETCODE_TIMEOUT;
    }
    return retcode;
}


int PAK_ReadData(GT511 * aDevice, uint8_t * aData, uint16_t aExpectedByteCount) {
    
    // request a byte array :
    // taille du header 0x5A 0xA5: 2 bytes +
    // taille du device ID: 2 bytes +
    // taille des donnees : aExpectedByteCount bytes +
    // taille du checksum : 2 bytes =>
    // total aExpectedByteCount + 6 bytes
    long long ms = millisecs();
    int data_checksum_index = DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + aExpectedByteCount;
    int data_packet_len = data_checksum_index + DATA_CHECKSUM_LEN;
    
    uint8_t * data_packet = PAK_ReadBytes(aDevice, data_packet_len);
    
    uint8_t header1 = data_packet[DATA_BYTE_START_CODE_1]; // should be 0x5a
    uint8_t header2 = data_packet[DATA_BYTE_START_CODE_2]; // should be 0xa5
    uint16_t deviceID = *(uint16_t *)(&data_packet[DATA_HEADER_LEN]); // device ID, should be always 0x01
    
    if(header1 != DATA_START_CODE_1 || header2 != DATA_START_CODE_2) {
        // bad header
        printf("PAK_ReadData failed : bad headers.\n");
        return -1;
    }
    
    // recup du flux de data et calcul du checksum en même temps
    uint16_t checksum = header1 + header2 + deviceID; // should be 256
    for(int i = 0; i < aExpectedByteCount; i++) {
        aData[i] = data_packet[i + DATA_HEADER_LEN + DATA_DEVICE_ID_LEN];
        checksum += aData[i];
    }
    
    // recuperation du checksum renvoyé par le device (2 derniers bytes) pour vérification
    uint16_t device_checksum = *(uint16_t *)(&data_packet[data_checksum_index]);
    
    if(checksum != device_checksum ) {
        printf("PAK_ReadData failed : bad checksum.\n");
        return -1;
    }

    if(aDevice->debug) {
        printf("PAK_ReadData returned %d bytes in %d ms:\n", aExpectedByteCount, (int)(millisecs() - ms));
        PAK_Debug(aData, aExpectedByteCount);
    }
    
    free(data_packet);
    
    return 0;
}



// =======================================================================================================
// DEVICE MANAGEMENT
// =======================================================================================================
GT511 * GT511_Init(char * aComPort, int aDebug) {
    // init mem
    int port = RS232_OpenComport(aComPort, 9600, sSerialMode);
    GT511 * device = NULL;
    if(port >= 0) {
        device = (GT511 *) malloc(sizeof(GT511));
        device->com_port = port;
        device->debug = aDebug;
        pthread_mutex_init(&(device->mutex), NULL);
        
        // open device
        PAK_SendCommand(device, ECommandOpen, device->debug);
        PAK_ReadResponse(device, NULL);
        if(device->debug) {
            sDeviceInfo devinfo;
            PAK_ReadData(device, (uint8_t *)&devinfo, sizeof(sDeviceInfo));
            printf("GT511_Init device info:\n");
            printf("Firmware v%.8X\n", devinfo.firmware);
            printf("Iso Max Area Size %.8X\n", devinfo.isomax / 1024);
            printf("Serial Number ");
            uint8_t * serial = devinfo.serial;
            for (uint8_t idx = 0; idx < 16; idx++) {
                printf("%.2X%s", serial[idx], idx == 7 ? "-" : "");
            }
            printf("\n");
        }
    }
    return device;
}


void GT511_Free(GT511 * aDevice) {
    // send close command
    PAK_SendCommand(aDevice, ECommandClose, 0);
    PAK_ReadResponse(aDevice, NULL);
    free(aDevice);
    aDevice = NULL;
}


void GT511_SetLED(GT511 * aDevice, uint8_t aOn) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandCmosLed, !!aOn);
    
    // get response
    if(PAK_ReadResponse(aDevice, NULL)) {
        // TODO;
    }
}


int GT511_GetEnrollCount(GT511 * aDevice) {
    PAK_SendCommand(aDevice, ECommandGetEnrollCount, 0);
    uint32_t count;
    if(PAK_ReadResponse(aDevice, &count) != 0) {
        count *= -1;
    }
    return count;
}


int GT511_CheckEnrolled(GT511 * aDevice, int aID) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandCheckEnrolled, aID);
    
    // get response
    uint32_t parameter;
    int enrolled = (PAK_ReadResponse(aDevice, &parameter) == 0);
    if(!enrolled) {
        // TODO
    }
    
    return enrolled;
}


void GT511_EnrollStart(GT511 * aDevice, int aID) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandEnrollStart, aID);
    
    // get response
    uint32_t parameter;
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
    }
}


static void GT511_Enroll(GT511 * aDevice, int aCommand) {
    // build + send command
    PAK_SendCommand(aDevice, aCommand, 0);
    
    // get response
    uint32_t parameter;
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
    }
}
void GT511_Enroll1(GT511 * aDevice) {
    GT511_Enroll(aDevice, ECommandEnroll1);
}
void GT511_Enroll2(GT511 * aDevice) {
    GT511_Enroll(aDevice, ECommandEnroll2);
}
void GT511_Enroll3(GT511 * aDevice) {
    GT511_Enroll(aDevice, ECommandEnroll3);
}


int GT511_IsFingerPressed(GT511 * aDevice) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandIsPressFinger, 0);
    
    // get response
    uint32_t pressed = FALSE;
    if(PAK_ReadResponse(aDevice, &pressed)) {
        // TODO:

    }
    else {
        pressed = (pressed == 0);
    }
    return pressed;
}


void GT511_DeleteID(GT511 * aDevice, int aID) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandDeleteID, aID);
    
    // get response
    if(PAK_ReadResponse(aDevice, NULL)) {
        // TODO:
    }
}


void GT511_DeleteAll(GT511 * aDevice) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandDeleteAll, 0);
    
    // get response
    if(PAK_ReadResponse(aDevice, NULL)) {
        // TODO:
    }
}


void GT511_Verify(GT511 * aDevice, int aID) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandVerify1_1, aID);
    
    // get response
    uint32_t parameter;
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
    }
}


int GT511_Identify(GT511 * aDevice, int * aID) {
    int retcode = 0;
    
    // build + send command
    PAK_SendCommand(aDevice, ECommandIdentify1_N, 0);
    
    // get response
    uint32_t parameter;
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
        retcode = -1;
        switch (parameter) {
            case NACK_BAD_FINGER:
    
                break;
                // etc...
            default:
                break;
        }
    }
    else {
        *aID = parameter;
    }
    return retcode;
}


void GT511_VerifyTemplate(GT511 * aDevice, int aID, uint8_t * aTemplateData) {
    // TODO
}


void GT511_IdentifyTemplate(GT511 * aDevice, uint8_t * aTemplateData) {
    // TODO
}


void GT511_CaptureFinger(GT511 * aDevice, int aHiRes) {
    // build + send command
    PAK_SendCommand(aDevice, ECommandCaptureFinger, aHiRes);
    
    // get response
    uint32_t parameter;
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
    }
}


static uint8_t * GT511_GetData(GT511 * aDevice, uint16_t aCommand, uint32_t aParameter, uint16_t aLen) {
    uint8_t * buffer = NULL;
    
    // build + send command
    PAK_SendCommand(aDevice, aCommand, aParameter);
    
    // get response
    uint32_t parameter;
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
    }
    else {
        buffer = (uint8_t *)malloc(aLen * sizeof(uint8_t));
        if(PAK_ReadData(aDevice, buffer, aLen)) {
            free(buffer);
            buffer = NULL;
        }
    }
    return buffer;
}


uint8_t * GT511_MakeTemplate(GT511 * aDevice, int aID) {
    return GT511_GetData(aDevice, ECommandMakeTemplate, aID, TEMPLATE_DATA_LEN);
}


uint8_t * GT511_GetImage(GT511 * aDevice) {
    return GT511_GetData(aDevice, ECommandGetImage, 0, IMAGE_DATA_LEN);
}


uint8_t * GT511_GetRawImage(GT511 * aDevice) {
    return GT511_GetData(aDevice, ECommandGetRawImage, 0, RAW_IMAGE_DATA_LEN);
}


uint8_t * GT511_GetTemplate(GT511 * aDevice, int aID) {
    return GT511_GetData(aDevice, ECommandGetTemplate, aID, TEMPLATE_DATA_LEN);
}


void GT511_SetTemplate(GT511 * aDevice, int aID, int aCheckDuplicate) {
    uint32_t parameter = (aCheckDuplicate << 16) + aID;
    
    // build + send command
    PAK_SendCommand(aDevice, ECommandSetTemplate, parameter);

    // get response
    if(PAK_ReadResponse(aDevice, &parameter)) {
        // TODO
    }
    else {
        uint8_t * buffer = (uint8_t *)malloc(TEMPLATE_DATA_LEN * sizeof(uint8_t));
        PAK_SendData(aDevice, buffer, TEMPLATE_DATA_LEN);
        if(PAK_ReadResponse(aDevice, &parameter)) {
            // TODO
        }
        free(buffer);
    }
}


