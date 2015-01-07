all: main.o io.o FPS_GT511Linux.o rs232.o db.o req.o scanner.o
	g++ -lsqlite3 -lpthread -lcurl -lwiringPi main.cpp io.cpp FPS_GT511Linux.cpp rs232.c db.c req.c scanner.cpp -o digilock
clean:
	rm *.o digilock
