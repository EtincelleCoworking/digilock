typedef enum ESerialPort {
	ttyS0	= 0,
	ttyS1	,
	ttyS2	,
	ttyS3	,
	ttyS4,
	ttyS5,
	ttyS6,
	ttyS7,
	ttyS8,
	ttyS9,
	ttyS10,
	ttyS11,	
	ttyS12	,
	ttyS13	,
	ttyS14	,
	ttyS15	,
	ttyUSB0	,
	ttyUSB1	,
	ttyUSB2	,
	ttyUSB3	,
	ttyUSB4	,
	ttyUSB5	,
	ttyAMA0	,
	ttyAMA1	,
	ttyACM0	,
	ttyACM1	,
	rfcomm0	,
	rfcomm1	,
	ircomm0	,
	ircomm1	,
	cuau0,
	cuau1,	
	cuau2,
	cuau3,
	cuaU0,
	cuaU1,
	cuaU2,
	cuaU3,
#ifdef __APPLE__
    apple_ttyUSB0,
    apple_ttyUSB1
#endif
} ESerialPort;
