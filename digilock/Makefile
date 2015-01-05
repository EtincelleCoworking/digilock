all: enroll idfinger blink hello

hello: main.o FPS_GT511Linux.o rs232.o
	g++ -DTARGET_HELLO main.cpp FPS_GT511Linux.cpp rs232.c -o hello

enroll: main.o FPS_GT511Linux.o rs232.o
	g++ -DTARGET_ENROLL main.cpp FPS_GT511Linux.cpp rs232.c -o enroll

idfinger: main.o FPS_GT511Linux.o rs232.o
	g++ -DTARGET_IDFINGER main.cpp FPS_GT511Linux.cpp rs232.c -o idfinger

blink: main.o FPS_GT511Linux.o rs232.o
	g++ -DTARGET_BLINK main.cpp FPS_GT511Linux.cpp rs232.c -o blink
	
clean:
	rm -rf *.o hello enroll idfinger blink
