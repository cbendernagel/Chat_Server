CFLAGS = -Wall -Werror
BINS = project

all: $(BINS)

project:
	gcc $(CFLAGS) server.c -o server -lpthread
	gcc $(CFLAGS) client.c -o client
	gcc $(CFLAGS) tserver.c -o tserver
	gcc $(CFLAGS) tclient.c -o tclient
	gcc $(CFLAGS) chat.c -o chat

clean:
	rm -f server
	rm -f client
	rm -f tserver
	rm -f tclient
	rm -f chat