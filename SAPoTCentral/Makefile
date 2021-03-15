####################### Makefile ########################
all: ucc
ucc: SAPoTCentral.o main.o 
	gcc -o ucc SAPoTCentral.o main.o -lpaho-mqtt3c -lmysqlclient -Wall
SAPoTCentral.o: SAPoTCentral.c
	gcc -o SAPoTCentral.o -c SAPoTCentral.c -lpaho-mqtt3c -lmysqlclient -Wall
main.o: main.c SAPoTCentral.h
	gcc -o main.o -c main.c -lpaho-mqtt3c -lmysqlclient -Wall
clean:
	rm -rf *.o
mrproper: clean
	rm -rf ucc
