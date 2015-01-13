all: main.o intercom.o FPS_GT511Linux.o rs232.o db.o req.o scanner.o lcd_i2c.o iniparser.o dictionary.o
	g++ -g -lsqlite3 -lpthread -lcurl -lwiringPi \
	main.cpp intercom.cpp FPS_GT511Linux.cpp rs232.c db.c req.c scanner.cpp lcd_i2c.c iniparser.c dictionary.c -o digilock
clean:
	rm *.o digilock
