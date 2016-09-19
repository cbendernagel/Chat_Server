#include "header.h"

int fd;
fd_set read_set;
fd_set ready_set;
char* messageBuffer;
bool chatPartnerStillConnected = true;

int main(int argc, char **argv) {
	fd = atoi(argv[1]);
	//printf("descriptor: %d\n", fd);
	messageBuffer = calloc(32, 32);

	multiplexChatPartnerAndInput();
	//sleep(10);
	free(messageBuffer);
}

void multiplexChatPartnerAndInput() {
	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(fd, &read_set);
	while(1) {
		ready_set = read_set;
		select(fd+1, &ready_set, NULL, NULL, NULL);
		// USER IS TRYING TO SEND
		if(FD_ISSET(STDIN_FILENO, &ready_set)) {
			if (chatPartnerStillConnected) {
				memset(messageBuffer, '\0', 1024);
				read(0, messageBuffer, 1024);
				//write(1, messageBuffer, 1024);
				if (strcmp(messageBuffer, "/close\n") == 0) {
					//write(1, "pid\n", 4);
					send(fd, "REQPID", 6, 0);
					char* pidBuffer[15];
					char* pidBufferStart = pidBuffer[0];
					recv(fd, pidBuffer, 15, 0);
					int clientpid = atoi(pidBufferStart);
					//write(1, "pid\n", 4);
					kill(clientpid, SIGCHLD);
		 			// kill(clientpid, SIGCHLD);
				}	
				else {
					// clear out stdin
					// int index = 0;
					// while (index < strlen(messageBuffer)) {
					// 	write(1, "\b \b", 3);
					// 	index++;
					// }
					//fflush(stdin);

					// lseek(1, 0, SEEK_END);
					// lseek(1, -1, SEEK_CUR);
					// char character[1];
					// bool stillClearingOut = true;
					// while (stillClearingOut){
					// 	lseek(1, -1, SEEK_CUR);
					// 	read(1, character, 1);

					// 	if(*character == '\n'){
					// 		stillClearingOut = false;
					// 	}
					// 	else {
					// 		write(1, "\0", 1);
					// 		lseek(1, -1, SEEK_CUR);
					// 	}
					// }
					// write(1, "wow", 3);
					// write(1, "[2K", 27);
					write(1, "\n", 1);
					write(1, "\x1b[2A", 7);
					write(1, "\r", 1);
					write(1, "\0\0", 2);
					write(1, "\r", 1);
					write(fd, messageBuffer, 1024);
				}
			}
			// if chatPartner is no longer connected we want to close on any attempted typing to chatPartner
			else {
				send(fd, "REQPID", 6, 0);
				char* pidBuffer[15];
				char* pidBufferStart = pidBuffer[0];
				recv(fd, pidBuffer, 15, 0);
				int clientpid = atoi(pidBufferStart);
				kill(clientpid, SIGCHLD);
			}
		}
		// SOMEONE ELSE IS TRYING TO SEND TO USER 
		if(FD_ISSET(fd, &ready_set)) {
			memset(messageBuffer, '\0', 1024);
			char receivingBuffer[1024];
			memset(receivingBuffer, '\0', 1024);

			read(fd, receivingBuffer, 1024);
			char* dupReceivingBuffer = malloc(1024);
			memset(dupReceivingBuffer, '\0', 1024);
			memcpy(dupReceivingBuffer, receivingBuffer, 1024);
			char* firstWord = strtok(dupReceivingBuffer, " ");
			// client wrote UOFF to us
			if (strcmp(receivingBuffer, "UOFF") == 0) {
				chatPartnerStillConnected = false;
				strcat(messageBuffer, "Chat partner has disconnected.");
				write(1, messageBuffer, 1024);
			}
			// // chat partner wrote UOFF with added u to differentiate it from a client "UOFF" alert
			// else if (strcmp(receivingBuffer, "/UOFF from clientu") == 0) {
			// 	strcat(messageBuffer, ">/UOFF from client");
			// 	write(1, messageBuffer, 1024);
			// }
			// else if (strcmp(firstWord, "FMSc") == 0) { // First message sent (from client)
			// 	strcat(messageBuffer, "<");
			// 	strcat(messageBuffer, receivingBuffer+5);
			// 	strcat(messageBuffer, "\n");
			// 	write(1, messageBuffer, 1024);
			// }
			// else if (strcmp(firstWord, "RECVPID") == 0) {
			// 	write(1, "wedidit\n", 8);
			// }
			// chat partner wrote a normal message
			// else {
			// 	strcat(messageBuffer, ">");
			// 	strcat(messageBuffer, receivingBuffer);
			// 	strcat(messageBuffer, "\n");
			// 	write(1, messageBuffer, 1024);
			// }
			else if (strcmp(firstWord, "WRECV") == 0) {
				strcat(messageBuffer, ">");
				strcat(messageBuffer, receivingBuffer+5);
				strcat(messageBuffer, "\n");
				write(1, messageBuffer, strlen(receivingBuffer) - 5);
			}
			else if (strcmp(firstWord, "WSENT") == 0){
				strcat(messageBuffer, "<");
				strcat(messageBuffer, receivingBuffer+5);
				strcat(messageBuffer, "\n");
				write(1, messageBuffer, strlen(receivingBuffer) - 5);
			}
			free(dupReceivingBuffer);
		}
		sleep(1);
	}
}