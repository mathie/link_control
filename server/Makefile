# simple makefile to make server

all: server

clean:
	rm -f *.o server *~ core

# deps
server.o: server.c ../include/cliserv.h ../include/protocol.h server.h

read_config.o: read_config.c server.h

list_fns.o: list_fns.c server.h

process_client.o: process_client.c server.h

process_peer.o: process_peer.c server.h

send_mesage.o: send_message.c server.h

poll_clients.o: poll_clients.c server.h

server:	server.o read_config.o list_fns.o process_client.o process_peer.o \
	send_message.o poll_clients.o ../common/common.a

install: all
	# do nothing yet
