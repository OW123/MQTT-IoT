CC = gcc
CFLAGS = -g 
HEADERS = -pthread

all: serverMQTT clientMQTT

serverMQTT: serverMQTT.c
	$(CC) $(CFLAGS) $(HEADERS) serverMQTT.c frame_constructor.c -o serverMQTT

clientMQTT: clientMQTT.c
	$(CC) $(CFLAGS) $(HEADERS) clientMQTT.c frame_constructor.c -o clientMQTT  

clean:
	rm -f serverMQTT clientMQTT

fresh:
	make clean 
	make all