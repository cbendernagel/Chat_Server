#include "header.h"

char messageBuffer[1024];
int listenfd;
int optval = 1;
struct sockaddr_in serverInfo;
struct sockaddr_in clientInfo;
int connfd;
fd_set read_set;
fd_set ready_set;

int main(int argc, char **argv) {

	//Initialize our client_head to null
	client_head = NULL;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(9001);
	serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listenfd, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
	listen(listenfd, 1024);

	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(listenfd, &read_set);

	while(1) {
		ready_set = read_set;
		select(listenfd+1, &ready_set, NULL, NULL, NULL);
		
		// STDIN SERVER COMMANDS
		if (FD_ISSET(STDIN_FILENO, &ready_set)) {
			// we handle possible user server commands
			// for now might be just listening for /shutdown
		}
		// NEW CLIENT KNOCKING ON DOOR
		else if (FD_ISSET(listenfd, &ready_set)) {
			int length = sizeof(clientInfo);
			printf("make it to before accept\n");
			connfd = accept(listenfd, (struct sockaddr*)&clientInfo, (unsigned int*)&length);
			recv(connfd, messageBuffer, 1024, 0);
			printf("client typed to us: %s\n", messageBuffer);
			//send(connfd, "BYE", 6, 0);
		}
		// AN EXISTING CLIENT IS MESSAGING US
		else {

		}
	}

} // end of main



// #include "header.h"

// char messageBuffer[1024];
// char newMessageBuffer[] = "HEY KONRAD";
// int listenfd;
// int optval = 1;
// struct sockaddr_in serverInfo;
// struct sockaddr_in clientInfo;
// int connfd;

// int main(int argc, char **argv) {
// 	listenfd = socket(AF_INET, SOCK_STREAM, 0);
// 	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
// 	serverInfo.sin_family = AF_INET;
// 	serverInfo.sin_port = htons(9001);
// 	serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
// 	bind(listenfd, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
// 	listen(listenfd, 1024);

// 	int length = sizeof(clientInfo);
// 	printf("We make it to before accept call\n");
// 	connfd = accept(listenfd, (struct sockaddr*)&clientInfo, (unsigned int*)&length);
// 	send(connfd, newMessageBuffer, 11, 0);
// 	printf("| 1 | SENT GREETING TO KONRAD\n");
// 	recv(connfd, messageBuffer, 1024, 0);
// 	if (strcmp(messageBuffer, "HEY CHARLIE") == 0) {
// 		printf("| 4 | HEY CHARLIE GREETING RECEIVED\n");
// 	}
// 	printf("Finished!\n");
// 	// int count = 1;
// 	// while (count < 100) {
// 	// 	count++;
// 	// 	printf("listening: %d\n", count);
// 	// 	sleep(1);
// 	// }
// }