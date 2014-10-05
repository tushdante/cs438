NAME_OF_EXECUTABLE_FILE = mp3server
NAME_OF_EXECUTABLE_1 = mp3client


COMPILER = g++

all: mp3server mp3client

mp3server : mp3.o libcommon.o
	$(COMPILER) -pthread mp3server.cpp mp3.o libcommon.o -o mp3server

mp3client: mp3.o libcommon.o
	$(COMPILER) -pthread mp3client.cpp mp3.o libcommon.o -o mp3client

mp3.o : mp3.cc 
	$(COMPILER) -I /usr/lib/x86_64-redhat-linux5E/include mp3.cc -c	

mp3client.o : mp3client.cpp 
	$(COMPILER) libcommon.cpp mp3client.cpp

mp3server.o : mp3server.cpp 
	$(COMPILER) libcommon.cpp mp3server.cpp
	
clean:
	-rm *.o
