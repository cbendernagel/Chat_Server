#include "header.h"

char messageBuffer[1024];
char newMessageBuffer[] = "HEY CHARLIE";
struct in_addr serverIPStruct;
int serverIP;
int port;
int clientfd;
struct sockaddr_in serverInfo;

int main(int argc, char **argv) {
	printf("ip: %d\n", serverIP);
	port = 9001;
	clientfd = socket(AF_INET, SOCK_STREAM, 0);
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(port);
	serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);		// local host
	printf("new ip: %d\n", serverInfo.sin_addr.s_addr);
	if (connect(clientfd, (const struct sockaddr*)&serverInfo, sizeof(serverInfo)) >= 0) {
		printf("Successful connection!\n");
		recv(clientfd, messageBuffer, 1024, 0);
		if (strcmp(messageBuffer, "HEY KONRAD") == 0) {
			printf("| 2 | HEY KONRAD GREETING RECEIVED\n");
			send(clientfd, newMessageBuffer, 11, 0);
			printf("| 3 | SENT GREETING TO CHARLIE AND FINISHED\n");
		}
		return 0;
	}
	else {
		printf("UNSUCCESSFUL\n");
		return -1;
	}
}


// THIS IS IF WE WANTED OUR CLIENT TO CONNECT TO A SPECIFIC IP INSTEAD OF JUST LOCALHOST
//char hostIPAsString[] = "127.0.0.1";
//inet_aton(hostIPAsString, &serverIPStruct);
// serverIP = serverIPStruct.s_addr;
//serverInfo.sin_addr.s_addr = serverIP;