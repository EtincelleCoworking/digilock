/* 
    FPS_GT511C3.cpp v1.0 - Library for controlling the GT-511Cxx Finger Print Scanner (FPS)
	Created by Josh Hawley, July 23rd 2013
	Licensed for non-commercial use, must include this license message
	basically, Feel free to hack away at it, but just give me credit for my work =)
	TLDR; Wil Wheaton's Law
*/

//
//  FPS_GT511Linux.cpp
//  digilock
//
//  Modified by Olivier Huguenot on 26/12/2014.
//  Copyright (c) 2014 Etincelle Coworking. All rights reserved.
//


#include "FPS_GT511Linux.h"
#include "bitmap.h"
#ifdef __APPLE__
#  include <sys/time.h>
#else
#  include <time.h>
#endif
#include "win32code.h"

long long millisecs() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}


// returns the 12 bytes of the generated command packet
// remember to call delete on the returned array
byte* Command_Packet::GetPacketBytes()
{
	byte* packetbytes= new byte[12];
	
	// update command before calculating checksum (important!)
	word cmd = Command;
	command[0] = GetLowByte(cmd);
	command[1] = GetHighByte(cmd);

	word checksum = _CalculateChecksum();

	packetbytes[0] = COMMAND_START_CODE_1;
	packetbytes[1] = COMMAND_START_CODE_2;
	packetbytes[2] = COMMAND_DEVICE_ID_1;
	packetbytes[3] = COMMAND_DEVICE_ID_2;
	packetbytes[4] = Parameter[0];
	packetbytes[5] = Parameter[1];
	packetbytes[6] = Parameter[2];
	packetbytes[7] = Parameter[3];
	packetbytes[8] = command[0];
	packetbytes[9] = command[1];
	packetbytes[10] = GetLowByte(checksum);
	packetbytes[11] = GetHighByte(checksum);

	return packetbytes;
}

// Converts the int to bytes and puts them into the paramter array
void Command_Packet::ParameterFromInt(int i)
{
	Parameter[0] = (i & 0x000000ff);
	Parameter[1] = (i & 0x0000ff00) >> 8;
	Parameter[2] = (i & 0x00ff0000) >> 16;
	Parameter[3] = (i & 0xff000000) >> 24;
}

// Returns the high byte from a word
byte Command_Packet::GetHighByte(word w)
{
	return (byte)(w >> 8) & 0x00FF;
}

// Returns the low byte from a word
byte Command_Packet::GetLowByte(word w)
{
	return (byte)w & 0x00FF;
}

word Command_Packet::_CalculateChecksum()
{
	word w = 0;
	w += COMMAND_START_CODE_1;
	w += COMMAND_START_CODE_2;
	w += COMMAND_DEVICE_ID_1;
	w += COMMAND_DEVICE_ID_2;
	w += Parameter[0];
	w += Parameter[1];
	w += Parameter[2];
	w += Parameter[3];
	w += command[0];
	w += command[1];

	return w;
}

Command_Packet::Command_Packet()
{
};



// creates and parses a response packet from the finger print scanner
Response_Packet::Response_Packet(byte* buffer, bool UseSerialDebug)
{
	CheckParsing(buffer[0], COMMAND_START_CODE_1, COMMAND_START_CODE_1, "COMMAND_START_CODE_1", UseSerialDebug);
	CheckParsing(buffer[1], COMMAND_START_CODE_2, COMMAND_START_CODE_2, "COMMAND_START_CODE_2", UseSerialDebug);
	CheckParsing(buffer[2], COMMAND_DEVICE_ID_1, COMMAND_DEVICE_ID_1, "COMMAND_DEVICE_ID_1", UseSerialDebug);
	CheckParsing(buffer[3], COMMAND_DEVICE_ID_2, COMMAND_DEVICE_ID_2, "COMMAND_DEVICE_ID_2", UseSerialDebug);
    CheckParsing(buffer[8], Command_Packet::Commands::Ack, Command_Packet::Commands::Nack, "AckNak_LOW", UseSerialDebug);
	if (buffer[8] == Command_Packet::Commands::Ack) ACK = true; else ACK = false;
	CheckParsing(buffer[9], 0x00, 0x00, "AckNak_HIGH", UseSerialDebug);

	word checksum = CalculateChecksum(buffer, 10);
	byte checksum_low = GetLowByte(checksum);
	byte checksum_high = GetHighByte(checksum);
	CheckParsing(buffer[10], checksum_low, checksum_low, "Checksum_LOW", UseSerialDebug);
	CheckParsing(buffer[11], checksum_high, checksum_high, "Checksum_HIGH", UseSerialDebug);
	
	Error = ErrorCodes::ParseFromBytes(buffer[5], buffer[4]);

	ParameterBytes[0] = buffer[4];
	ParameterBytes[1] = buffer[5];
	ParameterBytes[2] = buffer[6];
	ParameterBytes[3] = buffer[7];
	ResponseBytes[0]=buffer[8];
	ResponseBytes[1]=buffer[9];
	for (int i=0; i < 12; i++)
	{
		RawBytes[i]=buffer[i];
	}
}

// parses bytes into one of the possible errors from the finger print scanner
Response_Packet::ErrorCodes::Errors_Enum Response_Packet::ErrorCodes::ParseFromBytes(byte high, byte low)
{
	Errors_Enum e = INVALID;
	if (high == 0x00)
	{
	}
	if (high == 0x10)
	{
		switch(low)
		{
			case 0x00: e = NO_ERROR; break;
			case 0x01: e = NACK_TIMEOUT; break;
			case 0x02: e = NACK_INVALID_BAUDRATE; break;		
			case 0x03: e = NACK_INVALID_POS; break;			
			case 0x04: e = NACK_IS_NOT_USED; break;		
			case 0x05: e = NACK_IS_ALREADY_USED; break;
			case 0x06: e = NACK_COMM_ERR; break;
			case 0x07: e = NACK_VERIFY_FAILED; break;
			case 0x08: e = NACK_IDENTIFY_FAILED; break;
			case 0x09: e = NACK_DB_IS_FULL; break;
			case 0x0A: e = NACK_DB_IS_EMPTY; break;
			case 0x0B: e = NACK_TURN_ERR; break;
			case 0x0C: e = NACK_BAD_FINGER; break;
			case 0x0D: e = NACK_ENROLL_FAILED; break;
			case 0x0E: e = NACK_IS_NOT_SUPPORTED; break;
			case 0x0F: e = NACK_DEV_ERR; break;
			case 0x10: e = NACK_CAPTURE_CANCELED; break;
			case 0x11: e = NACK_INVALID_PARAM; break;
			case 0x12: e = NACK_FINGER_IS_NOT_PRESSED; break;
		}
	}
	return e;
}

// Gets an int from the parameter bytes
int Response_Packet::IntFromParameter()
{
	int retval = 0;
	retval = (retval << 8) + ParameterBytes[3];
	retval = (retval << 8) + ParameterBytes[2];
	retval = (retval << 8) + ParameterBytes[1];
	retval = (retval << 8) + ParameterBytes[0];
	return retval;
}

// calculates the checksum from the bytes in the packet
word Response_Packet::CalculateChecksum(byte* buffer, int length)
{
	word checksum = 0;
	for (int i=0; i<length; i++)
	{
		checksum +=buffer[i];
	}
	return checksum;
}

// Returns the high byte from a word
byte Response_Packet::GetHighByte(word w)
{
	return (byte)(w>>8)&0x00FF;
}

// Returns the low byte from a word
byte Response_Packet::GetLowByte(word w)
{
	return (byte)w&0x00FF;
}

// checks to see if the byte is the proper value, and logs it to the serial channel if not
bool Response_Packet::CheckParsing(byte b, byte propervalue, byte alternatevalue, const char* varname, bool UseSerialDebug)
{
	bool retval = (b != propervalue) && (b != alternatevalue);
	if ((UseSerialDebug) && (retval))
	{
        //~ Serial.print("Response_Packet parsing error ");
        //~ Serial.print(varname);
        //~ Serial.print(" ");
        //~ Serial.print(propervalue, HEX);
        //~ Serial.print(" || ");
        //~ Serial.print(alternatevalue, HEX);
        //~ Serial.print(" != ");
        //~ Serial.println(b, HEX);

		printf("Response_Packet parsing error ");
		printf("%s", varname);
		printf(" ");
		printf("%x", propervalue);
		printf(" || ");
		printf("%x", alternatevalue);
		printf(" != ");
		printf("%x\n", b);
	}
	return retval;
}
// #pragma endregion

// #pragma region -= Data_Packet =-
//void Data_Packet::StartNewPacket()
//{
//	Data_Packet::NextPacketID = 0;
//	Data_Packet::CheckSum = 0;
//}
// #pragma endregion

// #pragma region -= FPS_GT511 Definitions =-

// #pragma region -= Constructor/Destructor =-
// Creates a new object to interface with the fingerprint scanner


#define RESPONSE_LEN (12)

FPS_GT511::FPS_GT511(int port, int baud, const char * mode) {
	if(RS232_OpenComport(port, baud, mode)) {
		printf("Cannot open Com Port\n");
        _available = false;
	}
	else {
		_com_port = port;
		_baud_rate = baud;
		_mode = mode;
        _available = true;
        pthread_mutex_init(& _mutex , NULL);
    }
    _open = false;
}


// destructor
FPS_GT511::~FPS_GT511()
{
	RS232_CloseComport(_com_port);
    //~ _serial.~SoftwareSerial();
}
// #pragma endregion


// #pragma region -= Device Commands =-
//Initialises the device and gets ready for commands
void FPS_GT511::Open()
{
    if(!_available) {
        printf("Device is not available.\n");
        return;
    }
    
    if(_open) {
        printf("Device already opened.\n");
        return;
    }
        
    
	if (UseSerialDebug) 
        printf("FPS - Open\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Open;
	cp->ParameterFromInt(0x0);
	byte * packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet * rp = GetResponse();
    _open = true;
    
    delete cp;
	delete rp;
	delete[] packetbytes;
    return;
}

// According to the DataSheet, this does nothing... 
// Implemented it for completeness.
void FPS_GT511::Close()
{
    if(!_open) {
        printf("Device already closed.\n");
        return;
    }
	if (UseSerialDebug)
        //~ Serial.println("FPS - Close");
        printf("FPS - Close\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Close;
	cp->ParameterFromInt(0x00);
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
    _open = false;
	delete rp;
	delete[] packetbytes;
};

// Turns on or off the LED backlight
// Parameter: true turns on the backlight, false turns it off
// Returns: True if successful, false if not
bool FPS_GT511::SetLED(bool on)
{
    if(!_available)
        return false;
    
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::CmosLed;
	if (on)
	{
		if (UseSerialDebug) 
            printf("FPS - LED on\n");
		cp->Parameter[0] = 0x01;
	}
	else
	{
		if (UseSerialDebug) 
            printf("FPS - LED off\n");
		cp->Parameter[0] = 0x00;
	}
	cp->Parameter[1] = 0x00;
	cp->Parameter[2] = 0x00;
	cp->Parameter[3] = 0x00;
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	bool retval = true;
	if (rp->ACK == false) retval = false;

    delete rp;
	delete[] packetbytes;
	delete cp;
	return retval;
}

// Changes the baud rate of the connection
// Parameter: 9600, 19200, 38400, 57600, 115200
// Returns: True if success, false if invalid baud
// NOTE: Untested (don't have a logic level changer and a voltage divider is too slow)
bool FPS_GT511::ChangeBaudRate(int baud)
{
	if ((baud == 9600) || (baud == 19200) || (baud == 38400) || (baud == 57600) || (baud == 115200))
	{

		if (UseSerialDebug) 
            //~ Serial.println("FPS - ChangeBaudRate");
            printf("FPS - ChangeBaudRate\n");
		Command_Packet* cp = new Command_Packet();
		cp->Command = Command_Packet::Commands::Open;
		cp->ParameterFromInt(baud);
		byte* packetbytes = cp->GetPacketBytes();
		SendCommand(packetbytes, COMMAND_PACKET_LEN);
		Response_Packet* rp = GetResponse();
		bool retval = rp->ACK;
		if (retval) 
		{
			//_serial.end();
			RS232_CloseComport(_com_port);
			//_serial.begin(baud);			
			if(RS232_OpenComport(_com_port, baud, _mode)) {
				printf("Cannot open Com Port\n");
			}
			else {
				_baud_rate = baud;
			}
		}
		delete rp;
		delete[] packetbytes;
		return retval;
	}
	return false;
}

// Gets the number of enrolled fingerprints
// Return: The total number of enrolled fingerprints
int FPS_GT511::GetEnrollCount()
{
    if(!_available)
        return -1;
    
	if (UseSerialDebug) 
        printf("FPS - GetEnrolledCount\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::GetEnrollCount;
    cp->ParameterFromInt(0x0);
	byte * packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet * rp = GetResponse();

	int retval = rp->IntFromParameter();
    delete cp;
	delete rp;
	delete[] packetbytes;
	return retval;
}

// checks to see if the ID number is in use or not
// Parameter: 0...(MAX_FGP_COUNT - 1)
// Return: True if the ID number is enrolled, false if not
bool FPS_GT511::CheckEnrolled(int id)
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - CheckEnrolled");
        printf("FPS - CheckEnrolled\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::CheckEnrolled;
	cp->ParameterFromInt(id);
	byte* packetbytes = cp->GetPacketBytes();
	delete cp;
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	delete[] packetbytes;
	Response_Packet* rp = GetResponse();
	bool retval = false;
	retval = rp->ACK;
	delete rp;
	return retval;
}

// Starts the Enrollment Process
// Parameter: 0...(MAX_FGP_COUNT - 1)
// Return:
//	0 - ACK
//	1 - Database is full
//	2 - Invalid Position
//	3 - Position(ID) is already used
int FPS_GT511::EnrollStart(int id)
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - EnrollStart");
        printf("FPS - EnrollStart\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::EnrollStart;
	cp->ParameterFromInt(id);
	byte* packetbytes = cp->GetPacketBytes();
	delete cp;
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	delete[] packetbytes;
	Response_Packet* rp = GetResponse();
	int retval = 0;
	if (rp->ACK == false)
	{
		if (rp->Error == Response_Packet::ErrorCodes::NACK_DB_IS_FULL) retval = 1;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_INVALID_POS) retval = 2;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_IS_ALREADY_USED) retval = 3;
	}
	delete rp;
	return retval;
}

// Gets the first scan of an enrollment
// Return: 
//	0 - ACK
//	1 - Enroll Failed
//	2 - Bad finger
//	3 - ID in use
int FPS_GT511::Enroll1()
{
    int ret = 0;
	if (UseSerialDebug) 
        //~ Serial.println("FPS - Enroll1");
        printf("FPS - Enroll1\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Enroll1;
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes);
    delete cp;
	delete[] packetbytes;
	Response_Packet* rp = GetResponse();
	int retval = rp->IntFromParameter();
	if (retval < MAX_FGP_COUNT) retval = 3; else retval = 0;
	if (rp->ACK == false)
	{
		if (rp->Error == Response_Packet::ErrorCodes::NACK_ENROLL_FAILED) retval = 1;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_BAD_FINGER) retval = 2;
	}
    ret = rp->ACK ? 0 : retval;
    delete rp;
    return ret;
}

// Gets the Second scan of an enrollment
// Return: 
//	0 - ACK
//	1 - Enroll Failed
//	2 - Bad finger
//	3 - ID in use
int FPS_GT511::Enroll2()
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - Enroll2");
        printf("FPS - Enroll2\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Enroll2;
	byte* packetbytes = cp->GetPacketBytes();
	delete cp;
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	delete[] packetbytes;
	Response_Packet* rp = GetResponse();
	int retval = rp->IntFromParameter();
	if (retval < MAX_FGP_COUNT) retval = 3; else retval = 0;
	if (rp->ACK == false)
	{
		if (rp->Error == Response_Packet::ErrorCodes::NACK_ENROLL_FAILED) retval = 1;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_BAD_FINGER) retval = 2;
	}
	if (rp->ACK)
        retval = 0;
    delete rp;
    return  retval;
}

// Gets the Third scan of an enrollment
// Finishes Enrollment
// Return: 
//	0 - ACK
//	1 - Enroll Failed
//	2 - Bad finger
//	3 - ID in use
int FPS_GT511::Enroll3()
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - Enroll3");
        printf("FPS - Enroll3\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Enroll3;
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
    
    usleep(100 * 1000);
    
	Response_Packet* rp = GetResponse();
	int retval = rp->IntFromParameter();
	if (rp->ACK == false)
	{
		if (rp->Error == Response_Packet::ErrorCodes::NACK_ENROLL_FAILED) retval = 1;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_BAD_FINGER) retval = 2;
        if (retval < MAX_FGP_COUNT) retval = 3;
	}
    else {
        retval = 0;
    }
    delete cp;
    delete[] packetbytes;
    delete rp;
    return retval;
}

// Checks to see if a finger is pressed on the FPS
// Return: true if finger pressed, false if not
bool FPS_GT511::IsPressFinger()
{
    if(!_available)
        return false;
    
	if (UseSerialDebug) 
        //~ Serial.println("FPS - IsPressFinger");
        printf("FPS - IsPressFinger\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::IsPressFinger;
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	bool retval = false;
	int pval = rp->ParameterBytes[0];
	pval += rp->ParameterBytes[1];
	pval += rp->ParameterBytes[2];
	pval += rp->ParameterBytes[3];
	if (pval == 0) retval = true;
	delete rp;
	delete[] packetbytes;
	delete cp;
	return retval;
}

// Deletes the specified ID (enrollment) from the database
// Parameter: 0...(MAX_FGP_COUNT - 1) (id number to be deleted)
// Returns: true if successful, false if position invalid
bool FPS_GT511::DeleteID(int id)
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - DeleteID");
        printf("FPS - DeleteID\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::DeleteID;
	cp->ParameterFromInt(id);
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	bool retval = rp->ACK;
	delete rp;
	delete[] packetbytes;
	delete cp;
	return retval;
}

// Deletes all IDs (enrollments) from the database
// Returns: true if successful, false if db is empty
bool FPS_GT511::DeleteAll()
{
	if (UseSerialDebug) 
        printf("FPS - DeleteAll\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::DeleteAll;
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	bool retval = rp->ACK;
	delete rp;
	delete[] packetbytes;
	delete cp;
	return retval;
}

// Checks the currently pressed finger against a specific ID
// Parameter: 0...(MAX_FGP_COUNT - 1) (id number to be checked)
// Returns:
//	0 - Verified OK (the correct finger)
//	1 - Invalid Position
//	2 - ID is not in use
//	3 - Verified FALSE (not the correct finger)
int FPS_GT511::Verify1_1(int id)
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - Verify1_1");
        printf("FPS - Verify1_1\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Verify1_1;
	cp->ParameterFromInt(id);
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	int retval = 0;
	if (rp->ACK == false)
	{
		if (rp->Error == Response_Packet::ErrorCodes::NACK_INVALID_POS) retval = 1;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_IS_NOT_USED) retval = 2;
		if (rp->Error == Response_Packet::ErrorCodes::NACK_VERIFY_FAILED) retval = 3;
	}
	delete rp;
	delete[] packetbytes;
	delete cp;
	return retval;
}

// Checks the currently pressed finger against all enrolled fingerprints
// Returns:
//	0...(MAX_FGP_COUNT - 1): Verified against the specified ID (found, and here is the ID number)
//	MAX_FGP_COUNT: Failed to find the fingerprint in the database
int FPS_GT511::Identify1_N()
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - Identify1_N");
        printf("FPS - Identify1_N\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::Identify1_N;
	byte* packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	int retval = rp->IntFromParameter();
	if (retval > MAX_FGP_COUNT) retval = MAX_FGP_COUNT;
	delete rp;
	delete[] packetbytes;
	delete cp;
    return retval;
}

// Captures the currently pressed finger into onboard ram use this prior to other commands
// Parameter: true for high quality image(slower), false for low quality image (faster)
// Generally, use high quality for enrollment, and low quality for verification/identification
// Returns: True if ok, false if no finger pressed
bool FPS_GT511::CaptureFinger(bool highquality)
{
	if (UseSerialDebug) 
        //~ Serial.println("FPS - CaptureFinger");
        printf("FPS - CaptureFinger\n");
	Command_Packet* cp = new Command_Packet();
	cp->Command = Command_Packet::Commands::CaptureFinger;
    cp->ParameterFromInt(!!highquality);
    byte * packetbytes = cp->GetPacketBytes();
	SendCommand(packetbytes, COMMAND_PACKET_LEN);
	Response_Packet* rp = GetResponse();
	bool retval = rp->ACK;
	delete rp;
	delete[] packetbytes;
	delete cp;
	return retval;
}


// Gets an image that is 258x202 (52116 bytes) and returns it in 407 Data_Packets
// Use StartDataDownload, and then GetNextDataPacket until done
// Returns: True (device confirming download starting)
int FPS_GT511::GetImage(byte * aBuffer)
{
    int ret = -1;
    
    if (UseSerialDebug)
        printf("FPS - GetImage\n");
    Command_Packet* cp = new Command_Packet();
    cp->Command = Command_Packet::Commands::GetImage;
    byte * packetbytes = cp->GetPacketBytes();
    SendCommand(packetbytes);
    
    usleep(100*1000);
    
    Response_Packet* rp = GetResponse();
    if(rp->ACK) {
        ret = GetData(aBuffer, IMAGE_DATA_LEN);
    }
    else {
        // NACK_INVALID_POS
        // NACK IS NOT USED
    }
    
    return ret;
}



int FPS_GT511::SetTemplate(byte * aTemplateBytes, int aFingerprintID, bool duplicateCheck) {
    int ret = 0;
    
    //return oem_add_template(_com_port, aTemplateBytes, ((duplicateCheck << 16) & 0xFF) | aFingerprintID);
    
    // send command
    if (UseSerialDebug)
        printf("FPS - SetTemplate\n");
    
    Command_Packet* cp = new Command_Packet();
    cp->Command = Command_Packet::Commands::SetTemplate;
    cp->ParameterFromInt((duplicateCheck << 16) | aFingerprintID); // (Param LOWORD=ID, and if Parameter HIWORD is non-zero, fingerprint duplication check will not be performed )
    byte * packetbytes = cp->GetPacketBytes();
    SendCommand(packetbytes);
    
    usleep(100 * 1000);
    
    // get response
    Response_Packet* rp = GetResponse();
    if(rp->ACK) {
        // send data
        usleep(100 * 1000);
        SendData(aTemplateBytes, TEMPLATE_DATA_LEN);
        usleep(100 * 1000);
        
        // get response
        delete rp;
        rp = GetResponse();
        if(rp->ACK) {
            ret = 0;
        }
        else {
            ret = rp->IntFromParameter();
        }
    }
    else {
        ret = rp->IntFromParameter();
    }

    delete cp;
    delete rp;
    return ret;
}


// Gets an image that is qvga 160x120 (19200 bytes) and returns it in 150 Data_Packets
// Use StartDataDownload, and then GetNextDataPacket until done
// Returns: True (device confirming download starting)
	// Not implemented due to memory restrictions on the arduino
	// may revisit this if I find a need for it
int FPS_GT511::GetRawImage(byte * aBuffer)
{
    int ret = -1;
    
    if (UseSerialDebug)
        printf("FPS - GetImage\n");
    Command_Packet* cp = new Command_Packet();
    cp->Command = Command_Packet::Commands::GetImage;
    byte * packetbytes = cp->GetPacketBytes();
    SendCommand(packetbytes);
    
    usleep(100*1000);
    
    Response_Packet* rp = GetResponse();
    if(rp->ACK) {
        ret = GetData(aBuffer, RAW_IMAGE_DATA_LEN);
    }
    else {
        // NACK_INVALID_POS
        // NACK IS NOT USED
    }
    return ret;
}



int FPS_GT511::MakeTemplate(int fgpid)
{
    int retcode = 0;
    
    if (UseSerialDebug)
        printf("FPS - GetTemplate\n");
    Command_Packet* cp = new Command_Packet();
    cp->Command = Command_Packet::Commands::MakeTemplate;
    cp->ParameterFromInt(fgpid);
    
    byte* packetbytes = cp->GetPacketBytes();
    SendCommand(packetbytes);
    
    
#define data_len 498
    
    usleep(100*1000);
    
    Response_Packet* rp = GetResponse();
    
    if(rp->ACK) {
        struct {
            byte startCode1;
            byte startCode2;
            word deviceID;
            byte data[data_len];
            word checksum;
        } dp;
        
        byte * resp = GetResponse(sizeof(dp));
        dp.startCode1 = resp[0];
        dp.startCode2 = resp[1];
        dp.deviceID = (resp[3] << 8) + resp[2];
        
        for(int i = 0; i < data_len; i++)
            dp.data[i] = resp[data_len + 2];
        
        dp.checksum = (resp[data_len+5] << 8) + resp[data_len+4];
    }
    else {
        // NACK_INVALID_POS
        // NACK IS NOT USED
    }
    
    
    return retcode;
}


// Gets a template from the fps (498 bytes)
// the input buffer must be allocated before call, and freed after use
int FPS_GT511::GetTemplate(int fgpid, byte * aBuffer)
{
    // envoyer 12 bytes pour la commande
    int ret = -1;
    
    if (UseSerialDebug)
        printf("FPS - GetTemplate\n");
    Command_Packet* cp = new Command_Packet();
    cp->Command = Command_Packet::Commands::GetTemplate;
    cp->ParameterFromInt(fgpid);

    byte* packetbytes = cp->GetPacketBytes();
    SendCommand(packetbytes);
    
    
    usleep(50 * 1000);
    
    // recevoir 12 bytes pour la réponse
    Response_Packet* rp = GetResponse();
    
    if(rp->ACK) {
    
        ret = GetData(aBuffer, TEMPLATE_DATA_LEN);

//        // demander un flux de bytes de taille :
//        // taille du header 0x5A 0xA5: 2 bytes +
//        // taille du device ID: 2 bytes +
//        // taille du template : 498 bytes +
//        // taille du checksum : 2 bytes =>
//        // total 504 bytes
//        byte * resp = GetResponse(DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + TEMPLATE_DATA_LEN + DATA_CHECKSUM_LEN);
//        uint8_t header1 = resp[0]; // should be 0x5a
//        uint8_t header2 = resp[1]; // should be 0xa5
//        uint16_t deviceID = *(uint16_t *)(&resp[DATA_HEADER_LEN]); // barbu ! device ID, should be always 0x01
//        
//        // recup du flux de data et calcul du checksum en même temps
//        uint16_t checksum = header1 + header2 + deviceID; // should be 256
//        for(int i = 0; i < TEMPLATE_DATA_LEN; i++) {
//            tmp[i] = resp[i + DATA_HEADER_LEN + DATA_DEVICE_ID_LEN];
//            checksum += tmp[i];
//        }
//
//        // recuperation du checksum renvoyé par le device (2 derniers bytes) pour vérification
//        uint16_t device_checksum = *(uint16_t *)(&resp[DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + TEMPLATE_DATA_LEN]);
//
//        retcode = (checksum != device_checksum) ? -1 : TEMPLATE_DATA_LEN;
    }
    else {
        // NACK_INVALID_POS
        // NACK IS NOT USED
    }

    
    return ret;
}



// Uploads a template to the fps 
// Parameter: the template (498 bytes)
// Parameter: the ID number to upload
// Parameter: Check for duplicate fingerprints already on fps
// Returns: 
//	0...(MAX_FGP_COUNT - 1) - ID duplicated
//	MAX_FGP_COUNT - Uploaded ok (no duplicate if enabled)
//	(MAX_FGP_COUNT + 1) - Invalid position
//	(MAX_FGP_COUNT + 2) - Communications error
//	(MAX_FGP_COUNT + 3) - Device error
	// Not implemented due to memory restrictions on the arduino
	// may revisit this if I find a need for it
//int FPS_GT511::SetTemplate(byte* tmplt, int id, bool duplicateCheck)
//{
	// Not implemented due to memory restrictions on the arduino
	// may revisit this if I find a need for it
	//return -1;
//}

// resets the Data_Packet class, and gets ready to download
	// Not implemented due to memory restrictions on the arduino
	// may revisit this if I find a need for it
void FPS_GT511::StartDataDownload()
{

    
    
}

// Returns the next data packet 
	// Not implemented due to memory restrictions on the arduino
	// may revisit this if I find a need for it
//Data_Packet GetNextDataPacket()
//{
//	return 0;
//}

// Commands that are not implemented (and why)
// VerifyTemplate1_1 - Couldn't find a good reason to implement this on an arduino
// IdentifyTemplate1_N - Couldn't find a good reason to implement this on an arduino
// MakeTemplate - Couldn't find a good reason to implement this on an arduino
// UsbInternalCheck - not implemented - Not valid config for arduino
// GetDatabaseStart - historical command, no longer supported
// GetDatabaseEnd - historical command, no longer supported
// UpgradeFirmware - Data sheet says not supported
// UpgradeISOCDImage - Data sheet says not supported
// SetIAPMode - for upgrading firmware (which data sheet says is not supported)
// Ack and Nack	are listed as commands for some unknown reason... not implemented
// #pragma endregion


// #pragma region -= Private Methods =-
// Sends the command to the software serial channel

void FPS_GT511::SendCommand(byte cmd[]) {
    SendCommand(cmd, COMMAND_PACKET_LEN);
}


void FPS_GT511::SendCommand(byte cmd[], int length)
{
    pthread_mutex_lock(&_mutex);
    
	//~ _serial.write(cmd, length);
	for(int i = 0; i < length; i++)
		RS232_SendByte(_com_port, cmd[i]);
	
    pthread_mutex_unlock(&_mutex);
    
    
    if (UseSerialDebug) {
        printf("FPS - SEND: ");
		DebugBytes(cmd, length);
        printf("\n");
	}
}


//build a data packet from given raw bytes and sends it to the chip.
void FPS_GT511::SendData(uint8_t aRawBytes[], int aLength)
{
    
    int data_packet_len = DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + aLength + DATA_CHECKSUM_LEN;
    uint8_t * data_packet = new uint8_t[data_packet_len];
    uint16_t checksum = DATA_START_CODE_1 + DATA_START_CODE_2 + DATA_DEVICE_ID_1 + DATA_DEVICE_ID_2;
    
    // build data packet :
    // header
    data_packet[0] = DATA_START_CODE_1;
    data_packet[1] = DATA_START_CODE_2;
    
    // device ID
    data_packet[2] = DATA_DEVICE_ID_1;
    data_packet[3] = DATA_DEVICE_ID_2;

    // data
    for(int i = 0; i < aLength; i++) {
        data_packet[DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + i] = aRawBytes[i];
        checksum += aRawBytes[i];
    }
    
    // checksum
    *(uint16_t *)(data_packet + DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + aLength) = checksum;
    
    long long ms = millisecs();
    pthread_mutex_lock(&_mutex);
//    for(int i = 0; i < aLength; i++)
//        RS232_SendByte(_com_port, data_packet[i]);

    RS232_SendBuf(_com_port, data_packet, data_packet_len);
    
    pthread_mutex_unlock(&_mutex);
    printf("SendData sent %d bytes in %d ms.\n", aLength, (int)(millisecs() - ms));
    
    if (UseSerialDebug) {
        printf("FPS - SEND DATA: ");
        DebugBytes(data_packet, data_packet_len);
        printf("\n");
    }
    delete[] data_packet;
}

// Gets the response to the command from the software serial channel (and waits for it)
Response_Packet * FPS_GT511::GetResponse() {
    int n;
    byte * resp = new byte[RESPONSE_LEN];
    byte * buffer = new byte[RESPONSE_LEN];
    Response_Packet * rp = NULL;
    
    long long ms = millisecs();
    
    pthread_mutex_lock(&_mutex);
    int index = 0;
    while(index < RESPONSE_LEN) {
        n = RS232_PollComport(_com_port, buffer, RESPONSE_LEN);
        if(n <= RESPONSE_LEN && n > 0) {
            if(UseSerialDebug)
                printf("RS232_PollComport read %d bytes.\n", n);
            memcpy(resp + index, buffer, n);
            index += n;
        }
        else {
            usleep(10 * 1000);
            if(millisecs() - ms > RESPONSE_TIMEOUT_MS) {
                printf("RS232_PollComport TIMEOUT :(\n");
                break;
            }
        }
    }
    pthread_mutex_unlock(&_mutex);

    rp = new Response_Packet(resp, UseSerialDebug);
    if (UseSerialDebug)	{
        printf("FPS - RECV: ");
        DebugBytes(rp->RawBytes, 12);
        printf("\n\n");
    }
    
    delete[] resp;
    delete[] buffer;

	return rp;
}


byte * FPS_GT511::GetResponse(int aExpectedByteCount) {
    bool done = false;
    int n;
    byte * resp = new byte[aExpectedByteCount];
    byte * buffer = new byte[aExpectedByteCount];

    pthread_mutex_lock(&_mutex);
    
    int index = 0;
    while(done == false) {
        n = RS232_PollComport(_com_port, buffer, aExpectedByteCount);
        if(n == aExpectedByteCount) {
            if(UseSerialDebug)
                printf("FULL - RS232_PollComport read %d bytes.\n", n);
            done = true;
            memcpy(resp, buffer, aExpectedByteCount);
        }
        else if(n < aExpectedByteCount && n > 0) {
            if(UseSerialDebug)
                printf("PARTIAL RS232_PollComport read %d bytes.\n", n);
            memcpy(resp + index, buffer, n);
            index += n;
            if(index == aExpectedByteCount)
                done = true;
        }
        else {
            usleep(10 * 1000);
        }
    }
    
    pthread_mutex_unlock(&_mutex);
    return resp;
}



// get a data packet.
int FPS_GT511::GetData(byte * aBuffer, int aExpectedByteCount) {
    
    // demander un flux de bytes de taille :
    // taille du header 0x5A 0xA5: 2 bytes +
    // taille du device ID: 2 bytes +
    // taille des donnees : xxx bytes +
    // taille du checksum : 2 bytes =>
    // total xxx + 6 bytes
    long long ms = millisecs();
    int data_packet_len = DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + aExpectedByteCount + DATA_CHECKSUM_LEN;
    byte * data_packet = GetResponse(data_packet_len);
    
    //byte * data = new byte[aExpectedByteCount];
    uint8_t header1 = data_packet[0]; // should be 0x5a
    uint8_t header2 = data_packet[1]; // should be 0xa5
    uint16_t deviceID = *(uint16_t *)(&data_packet[DATA_HEADER_LEN]); // device ID, should be always 0x01
    
    if(header1 != DATA_START_CODE_1 || header2 != DATA_START_CODE_2) {
        // bad header
        printf("GetData failed : bad headers.\n");
        return -1;
    }
    
    // recup du flux de data et calcul du checksum en même temps
    uint16_t checksum = header1 + header2 + deviceID; // should be 256
    for(int i = 0; i < TEMPLATE_DATA_LEN; i++) {
        aBuffer[i] = data_packet[i + DATA_HEADER_LEN + DATA_DEVICE_ID_LEN];
        checksum += aBuffer[i];
    }
    
    // recuperation du checksum renvoyé par le device (2 derniers bytes) pour vérification
    uint16_t device_checksum = *(uint16_t *)(&data_packet[DATA_HEADER_LEN + DATA_DEVICE_ID_LEN + TEMPLATE_DATA_LEN]);
    
    if(checksum != device_checksum ) {
        printf("GetData failed : bad checksum.\n");
        return -1;
    }
    
    if (UseSerialDebug) {
        printf("FPS - GET DATA: ");
        DebugBytes(data_packet, data_packet_len);
        printf("\n");
    }

    printf("GetData returned %d bytes in %d ms.\n", aExpectedByteCount, (int)(millisecs() - ms));
    return 0;
}


// sends the bye aray to the serial debugger in our hex format EX: "00 AF FF 10 00 13"
void FPS_GT511::DebugBytes(byte data[], int length) {
    for(int i = 0; i < length; i++)
        printf("%.2X ", data[i]);
    printf("\n");
}

// sends a byte to the serial debugger in the hex format we want EX "0F"
//void FPS_GT511::serialPrintHex(byte data)
//{
//  char tmp[16];
//  sprintf(tmp, "%.2X",data); 
//  printf("%s", tmp);
//}

