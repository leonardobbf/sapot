####################### Makefile ########################
all: sapotclient
sapotclient: SAPoTClient.o main.o 
	gcc -g -o gpc SAPoTClient.o main.o -lpaho-mqtt3c  -Wall
SAPoTClient.o: SAPoTClient.c
	gcc -g -o SAPoTClient.o -c SAPoTClient.c -lpaho-mqtt3c -Wall
main.o: main.c SAPoTClient.h
	gcc -g -o main.o -c main.c -lpaho-mqtt3c -Wall
clean:
	rm -rf *.o
mrproper: clean
	rm -rf ucc
