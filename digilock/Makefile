all: main.o FPS_GT511Linux.o rs232.o db.o req.o scanner.o
	g++ -lsqlite3 -lpthread -lcurl main.cpp FPS_GT511Linux.cpp rs232.c db.c req.c scanner.cpp -o digilock
clean:
	rm *.o digilock
