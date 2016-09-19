#include "header.h"

char* messageBuffer;
struct in_addr serverIPStruct;
int serverIP;
int port;
int clientfd;
char *pname;
char *pip;
char *pport;
fd_set read_set;
fd_set ready_set;
struct sockaddr_in serverInfo;
int rv;
int maxNumfds = 3;
int offset = 100;
int lastClosedfd = -7;

//Complilation flags
bool v_flag = false;
bool c_flag = false;

void sigHandler(int signal, siginfo_t* signalInfo, void* context) {
	// printf("\n");
	// printf("HANDLER________________\n");
	pid_t pid = signalInfo -> si_pid;
	printf("pid: %d\n", pid);
	// find chat by pid
	chat* chatp = findChatByPID(pid);
	// printf("chat has pid: %d\n", chatp -> pid);
	// remove this chat from the list
	// printf("we made it before removeChatFromList\n");
	chatStructResourceCleanup(chatp);
	removeChatFromList(chatp);
	// printf("we made it after removeChatFromList\n");
	// printf("\n");
}

int main(int argc, char **argv) {


	//DISABLE CTRL-C SIGNAL
	signal(SIGINT, exitMaintenenceTasks);

	int opt;

	// printf("args %d\n", argc);

	if(argc < 4){
		fprintf(stderr, "Not enough arugments, exiting.\n");
		return EXIT_FAILURE;
	}

	//Find flag arguments
    while((opt = getopt(argc, argv, "hcv:")) != -1) {
        switch(opt) {
            case 'h':
                /* The help menu was selected */
                helpUsage();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                v_flag = true;
                break;
            case 'c':
            	c_flag = true;
            	break;
            case '?':
                /* Let this case fall down to default;
                 * handled during bad option.*//*
                 */
            default:
                /* A bad option was provided. */
                exit(EXIT_FAILURE);
                break;
        }
    }


	//clientpid = getpid();
	// signal(SIGINT, sigHandler);
	struct sigaction mySigAction;
	mySigAction.sa_sigaction = *sigHandler;
	mySigAction.sa_flags = SA_SIGINFO;

	sigaction(SIGCHLD, &mySigAction, NULL);


	messageBuffer = calloc(32, 32);

	pname = argv[argc - 3];
	pip = argv[argc - 2];
	pport = argv[argc - 1];

	// GET PORT AND IP
	inet_aton(pip, &serverIPStruct);
	port = atoi(pport);
	port = htons(port);
	serverIP = serverIPStruct.s_addr;

	// FILL IN SOCKETADDR SERVER STRUCT
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = port;
	serverInfo.sin_addr.s_addr = serverIP;

	clientfd = socket(AF_INET, SOCK_STREAM, 0);
	// printf("clientfd is %d\n", clientfd);

	rv = attemptServerLogin(pname);
	if (rv == 0) {
		// printf("made it here\n");
		multiplexServerAndInput();
	}
	else {
		printf("Please rerun and try again\n");
		// program ends since login was unsuccessful
	}
	// at very end free messageBuffer
	free(messageBuffer);
}


/////////////////// INDIVIDUAL CHAT WINDOW MUST BE ABLE TO SUPPORT /CLOSE WHICH SHOULD TERMINATE THE CHILD PROCESS (SAME AS CLOSING TERMINAL WINDOW)s


/*
	Return 0 if successful, -1 if unsuccessful
*/
int attemptServerLogin(char* pname){
	// ATTEMPT CONNECTION
	if (connect(clientfd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) >= 0) {
		maxNumfds++;
		printf("Successful connection\n");
		send(clientfd, "WOLFIE \r\n\r\n", 12, 0);
		if(v_flag == true){
			//verbose(false);
			printf("outgoing command: WOLFIE \r\n\r\n\n");
		}
		recv(clientfd, messageBuffer, 1024, 0);
		if(v_flag == true){
			verbose(true);
		}
		if (strcmp(messageBuffer, "EIFLOW \r\n\r\n") == 0) {
			/* SEND "IAM USERNAME" */
			char nameBuffer[100];
			memset(nameBuffer, '\0', 100);
			//NEW USER
			if(c_flag == true){
				strcpy(nameBuffer, "IAMNEW ");
				strcat(nameBuffer, pname);
				strcat(nameBuffer, " \r\n\r\n");
				send(clientfd, nameBuffer, strlen(nameBuffer), 0);
				if(v_flag == true){
					//verbose(false);
					printf("outgoing command: %s\n", nameBuffer);
				}
				memset(messageBuffer, '\0', 1024);
				recv(clientfd, messageBuffer, 1024, 0);
				if(v_flag == true){
					verbose(true);
				}
				char** args = genAndFillArgsArray(messageBuffer);
				//User did not already exist
				if(strcmp(args[0], "HINEW") == 0){
					memset(nameBuffer, '\0', 100);

					//
					char* password = getpass("Enter your password: ");


					// printf("Enter your new password: ");
    	// 			scanf("%s",nameBuffer);
    	// 			char* password = nameBuffer;
					memset(messageBuffer, '\0', 1024);
					// printf("password: %s\n",nameBuffer);
					//printf("Entered password was %s\n", password);

					strcat(messageBuffer, "NEWPASS ");
					strcat(messageBuffer, password);
					strcat(messageBuffer, " \r\n\r\n");
					//printf("message sent: %s\n",messageBuffer);
					send(clientfd, messageBuffer, strlen(messageBuffer), 0);
					if(v_flag == true){
						verbose(false);
					}
					memset(messageBuffer, '\0', 1024);
					recv(clientfd, messageBuffer, 1024, 0);
					if(v_flag == true){
						verbose(true);
					}
					args = genAndFillArgsArray(messageBuffer);
					if(strcmp(args[0], "SSAPWEN") == 0){
						printf("f\n");
						memset(messageBuffer, '\0', 1024);
						recv(clientfd, messageBuffer, 1024, 0);
						//printf("buffer: %s\n", messageBuffer);
						if(v_flag == true){
							verbose(true);
						}
						args = genAndFillArgsArray(messageBuffer);
						if(strcmp(args[0], "HI") == 0){
							printf("d\n");
							memset(messageBuffer, '\0', 1024);
							recv(clientfd, messageBuffer, 1024, 0);
							if(v_flag == true){
								verbose(true);
							}
							printf("Message of the day: %s\n", messageBuffer);
							return 0;
						}
					}
					else if(strcmp(messageBuffer, "ERR 02 BAD PASSWORD") == 0){
						memset(messageBuffer, '\0', 1024);
						recv(clientfd, messageBuffer, 1024, 0);
						if(v_flag == true){
							verbose(true);
						}
						if(strcmp(messageBuffer, "BYE \r\n\r\n") == 0){
							return -1;
						}
					}
				}
				else if (strcmp(messageBuffer, "ERR 00 USER NAME TAKEN \r\n\r\n") == 0) {		// user with the name already exists
					//printf("User with this name already exists \n");
					memset(messageBuffer, '\0', 1024);
					recv(clientfd, messageBuffer, 1024, 0);
					if(v_flag == true){
						verbose(true);
					}
					if(strcmp(messageBuffer, "BYE \r\n\r\n") == 0){
						return -1;
					}
				}
			}
			else if(c_flag == false){
				strcpy(nameBuffer, "IAM ");
				strcat(nameBuffer, pname);
				strcat(nameBuffer, " \r\n\r\n");
				if(v_flag == true){
					//verbose(false);
					printf("outgoing command: %s\n", nameBuffer);
				}
				send(clientfd, nameBuffer, strlen(nameBuffer), 0);
				/* RECEIVE "HI USERNAME" */
				memset(messageBuffer, '\0', 1024);
				memset(nameBuffer, '\0', 100);
				recv(clientfd, messageBuffer, 1024, 0);
				if(v_flag == true){
					verbose(true);
				}
				char** args = genAndFillArgsArray(messageBuffer);
				if(strcmp(args[0], "AUTH") == 0){
					
					char* password = getpass("Enter your password: ");

					// printf("Please enter password: ");
					// scanf("%s", nameBuffer);
					memset(messageBuffer, '\0', 1024);
					strcat(messageBuffer, "PASS ");
					// char* password = nameBuffer;
					strcat(messageBuffer, password);
					strcat(messageBuffer, " \r\n\r\n");
					printf("message sent: %s\n",messageBuffer);
					send(clientfd, messageBuffer, strlen(messageBuffer), 0);
					if(v_flag == true){
						verbose(false);
					}
					memset(messageBuffer, '\0', 1024);
					recv(clientfd, messageBuffer, 1024, 0);
					if(v_flag == true){
						verbose(true);
					}

					//GOOD PASSWORD
					if(strcmp(messageBuffer, "SSAP \r\n\r\n") == 0){
						printf("Please enter password: \n\n");
						memset(messageBuffer, '\0', 1024);
						recv(clientfd, messageBuffer, 1024, 0);
						if(v_flag == true){
							verbose(true);
						}
						args = genAndFillArgsArray(messageBuffer);
						if(strcmp(args[0], "HI") == 0){
							memset(messageBuffer, '\0', 1024);
							recv(clientfd, messageBuffer, 1024, 0);
							if(v_flag == true){
								verbose(true);
							}
							printf("Message of the day: %s\n", messageBuffer);
							return 0;
						}
					}
					//BAD PASSWORD
					else if(strcmp(messageBuffer, "ERR 02 BAD PASSWORD \r\n\r\n") == 0){
						memset(messageBuffer, '\0', 1024);
						recv(clientfd, messageBuffer, 1024, 0);
						if(v_flag == true){
							verbose(true);
						}
						if(strcmp(messageBuffer, "BYE \r\n\r\n") == 0){
							return -1;
						}
					}
				}
				else if(strcmp(messageBuffer, "ERR 01 USER NOT AVAILABLE \r\n\r\n") == 0){
					memset(messageBuffer, '\0', 1024);
					recv(clientfd, messageBuffer, 1024, 0);
					if(v_flag == true){
						verbose(true);
					}
					if(strcmp(messageBuffer, "BYE \r\n\r\n") == 0){
						return -1;
					}
				}else if(strcmp(messageBuffer, "ERR 00 USER NAME TAKEN \r\n\r\n") == 0){
					memset(messageBuffer, '\0', 1024);
					recv(clientfd, messageBuffer, 1024, 0);
					if(v_flag == true){
						verbose(true);
					}
					if(strcmp(messageBuffer, "BYE \r\n\r\n") == 0){
						return -1;
					}
				}
			}
		}
		else {
			printf("EIFLOW not received\n");
			return -1;
		}
	}
	else {
		printf("Failed connection\n");
		return -1;
	}
	return -1;
}

void multiplexServerAndInput() {
	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(clientfd, &read_set);
	while(1) {
		//reapKilledChatProcesses();
		ready_set = read_set;
		// printf("ready_set waiting\n");
		select(maxNumfds, &ready_set, NULL, NULL, NULL);
		// if (FD_ISSET(lastClosedfd, &ready_set)){
		// 	// DO NOTHING, wait for read set to be updated next go around 
		// }
		// else {
			// printf("ready_set triggered\n");
			chat* activeChat = findActiveChat();
			/* USER TYPED IN A CHAT */
			if (activeChat != NULL) {
				// printf("ACTIVE CHAT NOT NULL\n");

				// IF SENDING MSG
				handleSendingChatMSG(activeChat);
			}
			else {
				// printf("we enter into forever hang\n");
				/* USER TYPED IN CLIENT */
				if (FD_ISSET(STDIN_FILENO, &ready_set)) {
					// printf("user is writing to us\n");
					handleClientCommand();
				}
				/* INPUT FROM SERVER */
				else if(FD_ISSET(clientfd, &ready_set)){
					// printf("server is writing to us\n");
					handleServerInitiatedComm();
				}
				
				else {
					// printf("do nothing\n");
				}
				// printf("we leave forever hang\n");
			}
			sleep(1);
		// }
	} // end of while
}


// LOGOUT
// HELP
// LISTU
// TIME
// BYE

// CHAT <TO> <MESSAGE>(initiates a chat) -------------->  convert to format for sending over socket: MSG <TO> <FROM> <MESSAGE>
// 		^^^^^^^^^^ THIS CLIENT FEATURES A USER SENDING A MESSAGE ^^^^^^^^^^^^
// 		POSSIBLE IMMEDIATE SERVER RESPONSES AFTER SENDING:	
//				ERR 01 USER DOES NOT EXIST (SENDING A MESSAGE)
// 				MSG <TO> <FROM> <MESSAGE> WHERE <FROM> IS THIS CLIENT'S USER
void handleClientCommand() {
	// printf("we make it to handle client command\n");
	memset(messageBuffer, '\0', 1024);
  	read(0, messageBuffer, 1024);
  	// printf("do we make it after read?\n");

	int numArgs = getNumArgs(messageBuffer);
  	char** args = genAndFillArgsArray(messageBuffer);

	// printf("%s\n", args[0]);
	// printf("%d\n", numArgs);
	if ((strcmp(args[0], "/logout") == 0) && numArgs == 1){
		logoutCommand();
	}
	else if ((strcmp(args[0], "/help") == 0) && numArgs == 1){
		helpCommand();
	}
	else if ((strcmp(args[0], "/listu") == 0) && numArgs == 1){
		listuCommand(numArgs, args);
	}
	else if ((strcmp(args[0], "/time") == 0) && numArgs == 1){
		timeCommand(numArgs, args);
	}
	else if ((strcmp(args[0], "/chat") == 0) && numArgs >= 4) {
		if (strcmp(pname, args[1]) == 0) {
			// not good
		}
		else if (strcmp(pname, args[2]) != 0) {
			// not good, user should be sender
		}
		else {
			chatCommand(numArgs, args);
		}
	}
	else {
		printf("Command not supported or incorrect number of arguments. Please try again\n");
	}
	// first must free address of args[0] as this is the start address of the argsBuffer generated by genAndFillArgsBuffer() (we calloc'd)
	// then must free address of args as this is the start address of the args char** array generated by genAndFillArgsArray() (we calloc'd)
	free(args[0]);
	free(args);	
}


// MSG (from server) received over socket in format: MSG <TO> <FROM> <MESSAGE> WHERE <TO> IS THIS CLIENT'S USER
// 		^^^^^^^^^^^^ THIS CLIENT FEATURES A USER RECEIVING A MESSAGE ^^^^^^^^^^^^^^^^


// UOFF <NAME> | IS SENT TO ALL CLIENTS WHEN A USER DISCONNECTS

//		vvvvvv .... IF THE DISCONNECTED USER IS ONE THIS CLIENT'S USER WAS SPEAKING WITH .... vvvvvvv
//		PRINT "USER HAS DISCONNECTED" IN RELEVANT CHAT TERMINAL OF RELEVANT CHILD PROCESS DEDICATED TO THAT CHAT
//		ON NEXT KEYPRESS IN THAT CHAT TERMINAL THE XTERM WINDOW SHOULD CLOSE PREVENTING USER FROM ATTEMPTING TO SEND 
//		ANYTHING TO A DISCONNECTED USER	


// BYE
void handleServerInitiatedComm() {
	memset(messageBuffer, '\0', 1024);
  	recv(clientfd, messageBuffer, 1024, 0);
  	//catch server crash
  	if(strlen(messageBuffer) == 0)
  		exitMaintenenceTasks();

  	if(v_flag == true){
		verbose(true);
	}
  	findNewLineAndReplaceWithNAK(messageBuffer);
	int numArgs = getNumArgs(messageBuffer);
  	char** args = genAndFillArgsArray(messageBuffer);
	findNAKAndReplaceWithNewLine(args[numArgs-1]);
	findNAKAndReplaceWithNewLine(messageBuffer);

	// printf("Server is sending me: %s\n", messageBuffer);

  	if (strcmp(messageBuffer, "BYE \r\n\r\n") == 0){
  		// printf("Server typed bye to us, result of server shutdown\n");
  		printf("Server shutdown - Logging off\n");
  		exitMaintenenceTasks();
  	}	
  	else if ((strcmp(args[0], "UOFF") == 0) && (numArgs == 3) && (strcmp(args[numArgs-1], "\r\n\r\n") == 0)) {
  		handleReceivedUOFF(numArgs, args);
  	}
  	else if ((strcmp(args[0], "MSG") == 0) && (numArgs >= 5) && (strcmp(args[numArgs-1], "\r\n\r\n") == 0)) {
  		// printf("RECEIVING A MESSAGE\n");
		sleep(1); // so child process actually finishes doing everything its supposed to 
  		handleReceivedChatMSG(args);
  	}
  	else if (strcmp(messageBuffer, "ERR 01 USER NOT AVAILABLE \r\n\r\n") == 0) {
  		printf("User not available for chatting\n");
  	}
  	else {
  		printf("Failed: Incorrect protocol verb received from server, or proper verb with incorrect number of args/wrong ending\n");
  	}

  	// first must free address of args[0] as this is the start address of the argsBuffer generated by genAndFillArgsBuffer() (we calloc'd)
	// then must free address of args as this is the start address of the args char** array generated by genAndFillArgsArray() (we calloc'd)
	free(args[0]);
	free(args);	
}

void logoutCommand(){
	send(clientfd, "BYE \r\n\r\n", 8, 0);
	if(v_flag == true){
		//verbose(false);
		printf("outgoing command: BYE \r\n\r\n\n");
	}
	memset(messageBuffer, '\0', 1024);
	recv(clientfd, messageBuffer, 1024, 0);
	if(v_flag == true){
		verbose(true);
	}
	if (strcmp(messageBuffer, "BYE \r\n\r\n") == 0) {
		//////////////////////////////////////////// DO CLEAN UP WORK HERE FOR LOGGING CLIENT OUT ////////////////////////////////////
		printf("Logged out successfully\n");
		exitMaintenenceTasks();
		
	}
	else {
		printf("Logout procedure failed: still logged in\n");
	}
}

void helpCommand() {
	printf("ACCEPTED COMMANDS_______\n");
	printf("%-15s%-50s\n", "BYE", "Informs the client the server is shutting down");
	printf("%-15s%-50s\n", "UOFF userName", "Informs the client that a client has logged out");
	printf("%-15s%-50s\n", "MSG", "A client to client message the server passes along to both clients for them to display in the chats");
	printf("%-15s%-50s\n", "BYE", "Starts the process of logging the client out of the server");
}



void listuCommand(int numArgs, char** args) {
	send(clientfd, "LISTU \r\n\r\n", 10, 0);
	if(v_flag == true){
		//verbose(false);
		printf("outgoing command: LISTU \r\n\r\n\n");
	}
	memset(messageBuffer, '\0', 1024);
	recv(clientfd, messageBuffer, 1024, 0);
	if(v_flag == true){
		verbose(true);
	}
	findNewLineAndReplaceWithNAK(messageBuffer);
	numArgs = getNumArgs(messageBuffer);
  	args = genAndFillArgsArray(messageBuffer);

  	int index = 1;
  	while (index <= numArgs -1) {
  		findNAKAndReplaceWithNewLine(args[index]);
  		index++;
  	}
		
	if ((strcmp(args[0], "UTSIL") == 0) && (numArgs >= 3) && (strcmp(args[numArgs-1], "\r\n\r\n") == 0)) {
		// first lets check whole thing
		index = 1;
		bool isValid = true; 
		while((index < numArgs -1) && isValid) {
			// user name
			if (index % 2 == 1) {
				// do nothing all is well
			}
			// \r\n\r\n
			else {
				if (strcmp(args[index], "\r\n") == 0){
					// do nothing , all is well
				}
				else {
					// invalid
					isValid = false;
				}
			}
			index++;
		}
		// then lets print whole thing 
		if (isValid) {
			printf("LIST OF USERS_______\n");
			index = 1;
			while((index < numArgs -1)) {
			// user name
				if (index % 2 == 1) {
					// do nothing all is well
					printf("%s\n", args[index]);
				}
				index++;
			}
		}
		else {
			printf("Incorrect protocol format received from server for list of users\n");
		}
	}
	else {
		printf("listu command failed. Server did not send proper verb or correct number of arguments\n");
	}
	free(args);
}

void timeCommand(int numArgs, char** args){
	send(clientfd, "TIME \r\n\r\n", 9, 0);
	if(v_flag == true){
		//verbose(false);
		printf("outgoing command: TIME \r\n\r\n\n");
	}
	// printf("We sent the time to server\n");
	memset(messageBuffer, '\0', 1024);
	recv(clientfd, messageBuffer, 1024, 0);
	if(v_flag == true){
		verbose(true);
	}
	findNewLineAndReplaceWithNAK(messageBuffer);
	
	numArgs = getNumArgs(messageBuffer);
	args = genAndFillArgsArray(messageBuffer);
	findNAKAndReplaceWithNewLine(args[numArgs-1]);

	// printf("The message we received for time is %s\n", messageBuffer);
	if ((strcmp(args[0], "EMIT") == 0) && (numArgs == 3) && (strcmp(args[numArgs-1], "\r\n\r\n") == 0)) {
		// printf("we make it into the IF YAYAAYAY\n");
		if(isArgANumber(args[1])){
			int seconds = atoi(args[1]);
			memset(messageBuffer, '\0', 1024);
			genSecondsToHoursString(messageBuffer, seconds);
			printf("%s\n", messageBuffer);
		}
		else {
			printf("Time request failed: Second argument returned from server is not a number\n");
		}
	}
	else {
		printf("Time request failed: Server did not send proper verb or correct number of arguments.\n");
	}
}

void chatCommand(int numArgs, char** args) {
	//createNewChat(args);
	// int *fileDescriptors = calloc(2, sizeof(int));
	// socketpair(AF_UNIX, SOCK_STREAM, 0, fileDescriptors);
	// chat* newChatStruct = createNewChatStruct(fileDescriptors[0], args);
	// addChatToList(newChatStruct);
	// openChat(newChatStruct, fileDescriptors[0], fileDescriptors[1]);
	
	// char firstMessageSentFromThisUser[1024];
	// memset(firstMessageSentFromThisUser, '\0', 1024);
	// strcat(firstMessageSentFromThisUser, "FMSc ");

	char typedMessageBuffer[1024];
	memset(typedMessageBuffer, '\0', 1024);
	int cursor = 3;
	while(args[cursor] != NULL) {
		strcat(typedMessageBuffer, args[cursor]);
		strcat(typedMessageBuffer, " ");
		cursor++;
	}
	// strcat(firstMessageSentFromThisUser, typedMessageBuffer);
	//send(fileDescriptors[0], firstMessageSentFromThisUser, 1024, 0);

	memset(messageBuffer, '\0', 1024);
	strcat(messageBuffer, "MSG ");
	strcat(messageBuffer, args[1]);
	strcat(messageBuffer, " ");
	strcat(messageBuffer, args[2]);
	strcat(messageBuffer, " ");
	
	strcat(messageBuffer, typedMessageBuffer);
	strcat(messageBuffer, " ");
	strcat(messageBuffer, "\r\n\r\n");
	// printf("This is what I'm sending to server: %s\n", messageBuffer);
	send(clientfd, messageBuffer, strlen(messageBuffer), 0);
	if(v_flag == true){
		verbose(false);
	}

	//free(fileDescriptors);
}

void genSecondsToHoursString(char* messageBuffer, int totalSeconds){
	int hours = totalSeconds / 3600;
	int remainingSeconds = totalSeconds % 3600;
	int minutes = remainingSeconds / 60;
	int seconds = remainingSeconds % 60;
	sprintf(messageBuffer, "connected for %d hour(s), %d minutes(s), and %d second(s)\n", hours, minutes, seconds);
}

void openChat(chat* chatToOpen, int parentfd, int childfd){
	// printf("\n");
	// printf("openChat START\n");
	pid_t pid;
	pid = fork();
	if (pid == 0) {
		close(parentfd);
		char commandLineBuffer[200];
		memset(commandLineBuffer, '\0', 200);
		char titleBuffer[70];
		memset(titleBuffer, '\0', 70);
		strcat(titleBuffer, chatToOpen -> chattingWithUsername);
		strcat(titleBuffer, "---client:");
		strcat(titleBuffer, pname);
		sprintf(commandLineBuffer, "xterm -geometry 90x35+%d+100 -T %s -e ./chat %d", offset, titleBuffer, childfd);
		char** argv = genAndFillArgsArray(commandLineBuffer);
		execvp(argv[0], argv);
		printf("If we make it here execv failed\n");
	}
	chatToOpen -> pid = pid;
	close(childfd);
	//add parent descriptor to read_set 
	FD_SET(parentfd, &read_set);
	maxNumfds++;
}

chat* createNewChatStruct(int fd, char **args) {
	// printf("\n");
	// printf("createNewChatStruct START\n");
	
	char* receiver = malloc(20);
	char* sender = malloc(20);
	memset(receiver, '\0', 20);
	memset(sender, '\0', 20);
	strcat(receiver, args[1]);
	strcat(sender, args[2]);

	chat* chatp = (chat*)malloc(sizeof(chat));
	memset(chatp, '\0', sizeof(chat));
	char chattingWithUsername[20];
	memset(chattingWithUsername, '\0', 20);

	if(strcmp(pname, receiver) == 0) {
		strcat(chattingWithUsername, sender);
	}
	else {
		strcat(chattingWithUsername, receiver);
	}

	// FIllING IN STRUCT INFO
	chatp -> nextChat = NULL;
	// printf("The chattingWithUserName we're putting into the struct is %s\n", chattingWithUsername);
	strcat(chatp -> chattingWithUsername, chattingWithUsername);
	chatp -> fd = fd;
	chatp -> disabled = false;
	// might not need to be callocing and therefore would not need argv
	// chatp -> argv = argv;
	// PID will be filled in when we fork in openNewChat()
	free(sender);
	free(receiver);
	return chatp;
}

chat* findActiveChat() {
	// printf("START |findActiveChat\n");
	chat* cursor = chat_head;
	while (cursor != NULL) {
		// printf("searching for chat\n");
		if (FD_ISSET(cursor -> fd, &ready_set)){
			// printf("FOUND CHAT\n");
			return cursor;
		}
		cursor = cursor -> nextChat;
	}
	// printf("returning null\n");
	return NULL;
}

void handleSendingChatMSG(chat* activeChat) {
	char chatMessage[1024];
	memset(chatMessage, '\0', 1024);
	memset(messageBuffer, '\0', 1024);
	int fd = activeChat -> fd;

	recv(fd, chatMessage, 1024, 0);
	if (strcmp(chatMessage, "REQPID") == 0) {
		pid_t ourpid = getpid();
		sprintf(messageBuffer, "%d", ourpid);
		send(fd, messageBuffer, strlen(messageBuffer), 0);
	}
	else {
		strcat(messageBuffer, "MSG ");
		// TO
		strcat(messageBuffer, activeChat -> chattingWithUsername);
		strcat(messageBuffer, " ");
		// FROM
		strcat(messageBuffer, pname);
		strcat(messageBuffer, " ");
		// ATTACH MESSAGE
		strcat(messageBuffer, chatMessage);
		strcat(messageBuffer, " ");
		strcat(messageBuffer, "\r\n\r\n");
		send(clientfd, messageBuffer, strlen(messageBuffer), 0);
		if(v_flag == true){
			verbose(false);
		}
	}
}

void handleReceivedChatMSG(char** args) {
	chat* targetChat = NULL;
	bool userIsReciever = false;
	// USER IS RECEIVER
	if (strcmp(pname, args[1]) == 0) {
		// printf("USER IS RECEIVER\n");
		userIsReciever = true;
		// printf("going to try to find chattingwith %s\n", args[2]);
		targetChat = findChatByUsername(args[2]);
	}
	// USER IS SENDER 
	else {
		// printf("USER IS SENDER\n");
		// printf("going to try to find chattingwith %s\n", args[1]);
		targetChat = findChatByUsername(args[1]);
		// do nothing
	}
	// first time receiving message (no pre-existing chat)
	if (targetChat == NULL) {
		// create new chat struct (chat struct is added to end of list) and open chat
		int *fileDescriptors = calloc(2, sizeof(int));
		socketpair(AF_UNIX, SOCK_STREAM, 0, fileDescriptors);
		chat* newChatStruct = createNewChatStruct(fileDescriptors[0], args);
		addChatToList(newChatStruct);
		targetChat = newChatStruct;
		openChat(newChatStruct, fileDescriptors[0], fileDescriptors[1]);
		free(fileDescriptors);
	}
	else {
		// found existing chat
	}

		int fd = targetChat -> fd;

		// have to reassemble typed message
		char typedMessageBuffer[1024];
		memset(typedMessageBuffer, '\0', 1024);

		if(userIsReciever) {
			strcat(typedMessageBuffer, "WRECV ");
		}
		else {
			strcat(typedMessageBuffer, "WSENT ");
		}

		int cursor = 3;
		while(args[cursor] != NULL) {
			strcat(typedMessageBuffer, args[cursor]);
			strcat(typedMessageBuffer, " ");
			cursor++;
		}

		// // special case of other user typing "/UOFF from client". must add "u" to designate this is a message from the user and not client-sent
		// if (strcmp(typedMessageBuffer, "/UOFF from client") == 0) {
		// 	char modifiedMessageBuffer[50];
		// 	memset(modifiedMessageBuffer, '\0', 50);
		// 	strcat(modifiedMessageBuffer, "/UOFF from clientu");
		// 	send(fd, modifiedMessageBuffer, 50, 0);
		// }
		// normal case. just pass along the message without any modifying. 
		
		send(fd, typedMessageBuffer, strlen(typedMessageBuffer), 0);
}

void handleReceivedUOFF(int numArgs, char** args){
	if (numArgs != 3) {
		printf("Incorrect number of args given in Server's UOFF message.\n");
		return;
	}
	// lets check to see if we're chatting with one of the disconnected users
	char* disconnectedUser = malloc(20);
	memset(disconnectedUser, '\0', 20);
	strcat(disconnectedUser, args[1]);
	chat* chatWithUser = findChatByUsername(disconnectedUser);
	if (chatWithUser != NULL) {
		int fd = chatWithUser -> fd;
		// removeChatFromList(chatWithUser);
		// DISABLE INSTEAD OF REMOVING 
		chatWithUser -> disabled = true;
		send(fd, "UOFF", 4, 0);
	}
	free(disconnectedUser);
}

chat* findChatByUsername(char* userName){
	// printf("\n");
	// printf("findChatByUsername START\n");
	// printf("username passed in: %s\n", userName);
	chat* targetChat = NULL;
	chat* cursor = chat_head;
	
	while (cursor != NULL) {
		if ((strcmp(userName, cursor -> chattingWithUsername) == 0) && (cursor -> disabled == false)){
			targetChat = cursor;
		}
		cursor = cursor -> nextChat;
	}
	return targetChat;
}

chat* findChatByPID(pid_t pid) {
	chat* targetChat = NULL;
	chat* cursor = chat_head;
	while (cursor != NULL) {
		if (pid == cursor -> pid) {
			targetChat = cursor;
		}
		cursor = cursor -> nextChat;
	}
	return targetChat;
}

void addChatToList(chat* chatToAdd) {
	// printf("\n");
	if (chat_head == NULL) {
		// printf("we update chat_head\n");
		chat_head = chatToAdd;
	}
	else {
		chat* cursor = chat_head;
		while (cursor -> nextChat != NULL) {
			cursor = cursor -> nextChat;
		}
		cursor -> nextChat = chatToAdd;
		// printf("we added a new tail\n");
	}
}

void removeChatFromList(chat* chatToRemove) {
	// printf("\n");
	// printf("WE REMOVE CHAT FROM LIST\n");
	// printf("WE REMOVE CHAT FROM LIST\n");
	// printf("WE REMOVE CHAT FROM LIST\n");
	// clean up resources associated with chat we're removing
	// printf("START |removeChatFromList\n");
	if (chatToRemove != NULL) {
		
		// chat we're removing is the head 
		if (chat_head == chatToRemove) {
			// printf("CHAT IS HEAD\n");
			if (chat_head -> nextChat == NULL) {
				free(chat_head);
				chat_head = NULL;
				// printf("chat head set to NULL\n");
			}
			else {
				chat_head = chat_head -> nextChat;
				free(chat_head);
				// printf("chat head advanced\n");
			}
		}
		// chat we're removing is the tail
		else if (chatToRemove -> nextChat == NULL) {
			// printf("CHAT IS TAIL\n");
			chat* cursor = chat_head;
			while ((cursor != NULL) && (cursor -> nextChat != chatToRemove)){
				cursor = cursor -> nextChat;
			}
			if (cursor != NULL){
				cursor -> nextChat = NULL;
				free(chatToRemove);
			}
		}
		// chat we're removing is somewhere in between
		else {
			// printf("CHAT IS IN BETWEEN\n");
			chat* cursor = chat_head;
			while ((cursor != NULL) && (cursor -> nextChat != chatToRemove)) {
				cursor = cursor -> nextChat;
			}
			if(cursor != NULL){
				cursor -> nextChat = chatToRemove -> nextChat;
				free(chatToRemove);
			}
		}
	}
	else {
		// do nothing

	}
}












void chatStructResourceCleanup(chat* chatBeingRemoved) {
	lastClosedfd = chatBeingRemoved -> fd;
	FD_CLR(chatBeingRemoved -> fd, &read_set);
	close(chatBeingRemoved -> fd);
}

void exitMaintenenceTasks() {
	// printf("\n");
	// printf("MAINTENENCE___________\n");
	//  kill all chat processes 
	chat* cursor = chat_head;
	while (cursor != chat_head) {
		kill(cursor -> pid, SIGTERM);
		cursor = cursor -> nextChat;
	}
	// reap the killed processes
	reapKilledChatProcesses();
	// free messageBuffer
	free(messageBuffer);
	// close clientfd
	close(clientfd);

	exit(EXIT_SUCCESS);
}

void reapKilledChatProcesses() {
	// printf("\n");
	// printf("REAPING CHILD PROCESSES___________\n");
	int status;
	pid_t pid;
	while(((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) && (errno != ECHILD)) {
		// printf("pid of killed process %d\n", pid);
		sleep(1);

		// find chat by pid
		chat* chatp = findChatByPID(pid);
		if (chatp != NULL) {
			// printf("Chat to remove found by PID\n");
			chatStructResourceCleanup(chatp);
			removeChatFromList(chatp);
		}
		else {
			
		}
		// remove this chat from the list
	}
}



//char hostIPAsString[] = "127.0.0.1";
//inet_aton(hostIPAsString, &serverIPStruct);
// serverIP = serverIPStruct.s_addr;
//serverInfo.sin_addr.s_addr = serverIP;

void verbose(bool input){
	if(input == true)
		printf("incoming command: ");
	else
		printf("outgoing command: ");

	printf("%s",messageBuffer);
}


void helpUsage(){
	printf("USAGE\n\n");
	printf("./client [-hcv] NAME SERVER_IP SERVER_PORT\n");
	printf("%-15s%-50s\n", "-h", "Displays help menu & returns EXIT_SUCCESS");
	printf("%-15s%-50s\n", "-c", "Requests to server to create a new user");
	printf("%-15s%-50s\n", "-v", "Verbose print all incoming and outgoing protocol verbs & content");
	printf("%-15s%-50s\n", "NAME", "This is the username to display while chatting");
	printf("%-15s%-50s\n", "SERVER_IP", "The ipaddress of the server to connect to.");
	printf("%-15s%-50s\n", "SERVER_PORT", "The port to connect to.");
}