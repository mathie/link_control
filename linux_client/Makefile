# linux_client

LDFLAGS += -lpthread
all: client

install: all
	# do nothing for now

clean:
	rm -f client *.o *~

client: client.o list_fns.o read_config.o listen_status.o ../common/common.a
