#include "header.h"
#define MAX_INPUT 1024

char* messageBuffer;
char* pport;
char* motd;
char* Afile;
int listenfd;
int optval = 1;
int comm_fd = 50;
int void_fd;
struct sockaddr_in serverInfo;
struct sockaddr_in clientInfo;
int connfd;
fd_set read_set;
fd_set ready_set;
fd_set com_set;
fd_set comm_set;
bool account_file = false;
bool h_flag = false;
bool v_flag = false;

struct timeval timeout = {5,0};

//THREADS
pthread_t commThread;
pthread_t loginThread;


int main(int argc, char **argv) {

	messageBuffer = calloc(32,32);
	account_head = NULL;

	//Checking if we received an account file
	if( access( argv[argc - 1], F_OK ) != -1 ) {
    	// file exists
    	account_file = true;
    	Afile = argv[argc - 1];
    	argc--;
    	readAccounts(argv[argc]);
	}else{
		readAccounts("Account.txt");
	}

	//DISABLE CTRL-C SIGNAL
	signal(SIGINT, sigHandler);

	int opt;

	//Find flag arguments
    while((opt = getopt(argc, argv, "hv:")) != -1) {
        switch(opt) {
            case 'h':
                /* The help menu was selected */
                usage();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                v_flag = true;
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

    if(argc < 3){
		fprintf(stderr, "Not enough arugments, exiting.\n");
		return EXIT_FAILURE;
	}

	pport = argv[argc - 2];
	int port = port = atoi(pport);

	motd = argv[argc - 1];

	//Initialize our client_head to null
	client_head = NULL;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(port);
	serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listenfd, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
	listen(listenfd, 1024);

	FD_ZERO(&read_set);
	FD_ZERO(&com_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(listenfd, &read_set);

	int max_fd = listenfd + 1;

	printf("Currently listening on port %s\n", argv[argc - 2]);

	while(1) {
		ready_set = read_set;
		select(max_fd, &ready_set, NULL, NULL, NULL);
		
		// STDIN SERVER COMMANDS
		if (FD_ISSET(STDIN_FILENO, &ready_set)) {
			//printf("server command\n");
			handleServerCommand();
			// we handle possible user server commands
			// for now might be just listening for /shutdown
		}
		// NEW CLIENT KNOCKING ON DOOR
		else if (FD_ISSET(listenfd, &ready_set)) {
			pthread_create(&loginThread, NULL, loginUser, (void*)&listenfd);
			pthread_setname_np(loginThread, "LOGIN");
		}
		// AN EXISTING CLIENT IS MESSAGING US
		// else {
		// 	printf("we got something from client\n");
		// 	handleConnectedClient();
			
		// }
	}
	free(messageBuffer);

} // end of main


// void handleIncomingMessage(){
// 	mem
// }

bool checkUsername(char* name){
	client* temp_client = client_head;
	while(temp_client != NULL){
		if(strcmp(temp_client -> username, name) == 0){
			return true;
		}
		temp_client = temp_client -> next_client;
	}
	return false;
}

void clearBuffer(){
	memset(messageBuffer, '\0', 1024);
}

void handleServerCommand(){
	clearBuffer();
	read(0, messageBuffer, 1024);

	//SERVER SHUTDOWN
	if(strlen(messageBuffer) == 0){
		shutdownServer();
	}

	int numArgs = getNumArgs(messageBuffer);
	if (numArgs > 2) {
  		printf("Max of 2 arguments supported. Please try again\n");
  		return;
  	}
  	else{
  		char** args = genAndFillArgsArray(messageBuffer);

  		//DISPLAY ALL USERS
  		if(strcmp(args[0], "/users") == 0){
  			printf("USERS:\n");
  			client* temp_client = client_head;
  			while(temp_client != NULL){
  				printf("%s\n", temp_client -> username);
  				temp_client = temp_client -> next_client;
  			}
  		}
  		//DISPLAY USAGE STATEMENT
  		else if(strcmp(args[0], "/help") == 0){
  			printf("ACCEPTED COMMANDS\n");
  			printf("%-15s%-50s\n", "MSG", "Sends a message to a specified user if they exist");
  			printf("%-15s%-50s\n", "TIME", "Sends the client the amount of time they have been connected to server");
  			printf("%-15s%-50s\n", "LISTU", "Sends the client a list of all users currently connected to the server");
  			printf("%-15s%-50s\n", "BYE", "Starts the process of logging the client out of the server");
  		}
  		else if(strcmp(args[0], "/accts") == 0) {
  			printAccounts();
  		}
  		//SHUTDOWN SERVER
  		else if(strcmp(args[0], "/shutdown") == 0){
  			printf("server shutdown\n");
  			shutdownServer();
  		}
  	}
  	clearBuffer();
}

void *handleConnectedClient(void* fd){
	//zero out com_set
	while(1){
		comm_set = com_set;
		int rc = select(comm_fd, &comm_set, NULL, NULL, &timeout);
		//Find out which client is messaging us
		
		if(rc == 0){
			continue;
		}

		client* temp_client = client_head;
		while(temp_client != NULL){
			if(FD_ISSET(temp_client -> fd, &comm_set)){
				break;
			}
			temp_client = temp_client -> next_client;
		}

		clearBuffer();
		recv(temp_client -> fd, messageBuffer, 1024, 0);
		if(v_flag == true){
			verbose(true);
		}
		//We have a new user
		if(temp_client == client_head && strcmp(messageBuffer, "newUser") == 0){
			continue;
		}

		//If the client does not exist, which shouldnt ever happen
		if(temp_client == NULL){
			continue;
		}

		//CLIENT RANDOMLY DISCONNECTED
		if(strlen(messageBuffer) == 0){
			clientDisconnected(temp_client);
			continue;
		}

		if(v_flag == true){
			verbose(true);
		}

		findNewLineAndReplaceWithNAK(messageBuffer);
		int numArgs = getNumArgs(messageBuffer);
  		char** args = genAndFillArgsArray(messageBuffer);
		findNAKAndReplaceWithNewLine(args[numArgs-1]);

		///////////////////////////// RECEIVED MESSAGE FROM CLIENT ///////////////////////////////
		////////////////////// POSSIBLE MESSAGES SENT TO SERVER BY CLIENT ////////////////////////

		//CLIENT SENDS MESSAGE TO ANOTHER CLIENT
		if(strcmp(args[0], "MSG") == 0 && strcmp(args[numArgs - 1], "\r\n\r\n") == 0){
			//they sent too few arguments
			if(numArgs < 5){
				clearBuffer();
				strcat(messageBuffer, "ERR 100 INTERNAL SERVER ERROR \r\n\r\n");
				if(v_flag == true){
						verbose(false);
				}
				clearBuffer();
				continue;
				//senmd error message
			}else{
				client* toClient = getClientByName(args[1]);
				if(toClient == NULL){
					//TO CLIENT DOES NOT EXIST
					clearBuffer();
					strcat(messageBuffer, "ERR 01 USER NOT AVAILABLE \r\n\r\n");
					if(v_flag == true){
						verbose(false);
					}
					send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
					if(v_flag == true){
						verbose(false);
					}
					clearBuffer();
					continue;
				}
				else{
					char typedMessageBuffer[1024];
					memset(typedMessageBuffer, '\0', 1024);
					clearBuffer();
					strcat(messageBuffer, "MSG ");
					strcat(messageBuffer, args[1]);
					strcat(messageBuffer, " ");
					strcat(messageBuffer, args[2]);
					int cursor = 3;
					while(args[cursor] != NULL) {
						strcat(typedMessageBuffer, " ");
						strcat(typedMessageBuffer, args[cursor]);
						cursor++;
					}
					strcat(messageBuffer, typedMessageBuffer);
					if(v_flag == true){
						verbose(false);
					}
					send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
					send(toClient -> fd, messageBuffer, strlen(messageBuffer), 0);
					if(v_flag == true){
						verbose(false);
					}
					clearBuffer();
					free(args[0]);
					free(args);
					continue;
				}
			}
		}
		else if(strcmp(args[0], "TIME") == 0 && numArgs < 3 && strcmp(args[numArgs - 1], "\r\n\r\n") == 0){
			time_t elapsed_time = time(NULL) - temp_client -> login_time;
			char et[15];
			memset(et, '\0', 15);
			sprintf(et, "%ld", elapsed_time);
			clearBuffer();
			strcat(messageBuffer, "EMIT ");
			strcat(messageBuffer, et);
			strcat(messageBuffer, " \r\n\r\n");
			printf("This is the buffer we're sending for time: %s\n", messageBuffer);
			if(v_flag == true){
				verbose(false);
			}
			send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
			clearBuffer();
			free(args[0]);
			free(args);
			continue;
		}
		else if(strcmp(args[0], "LISTU") == 0 && numArgs < 3 && strcmp(args[numArgs - 1], "\r\n\r\n") == 0){
			clearBuffer();

			strcat(messageBuffer, "UTSIL");

			client* usr_client = client_head;

			while(usr_client != NULL){
				strcat(messageBuffer, " ");
				strcat(messageBuffer, usr_client -> username);
				strcat(messageBuffer, " \r\n");
				usr_client = usr_client -> next_client;
			}

			strcat(messageBuffer, "\r\n");
			if(v_flag == true){
						verbose(false);
			}
			send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
			clearBuffer();
			free(args[0]);
			free(args);
		}
		else if(strcmp(args[0], "BYE") == 0 && numArgs < 3 && strcmp(args[numArgs - 1], "\r\n\r\n") == 0){
			logoutUser(temp_client);
			free(args[0]);
			free(args);
		}
		else{
			fprintf(stderr, "Not a valid command.\n");
			clearBuffer();
			strcat(messageBuffer, "ERR 100 INTERNAL SERVER ERROR \r\n\r\n");
			send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
			if(v_flag == true){
				verbose(false);
			}
			clearBuffer();
			free(args[0]);
			free(args);
		}
	}
	return 0;
}

client* getClientByName(char* username){
	client* temp_client = client_head;
	
	while(temp_client != NULL){
		if(strcmp(temp_client -> username, username) == 0){
			break;
		}
		temp_client = temp_client -> next_client;
	}

	return temp_client;
}

void addClientToList(client* new_client){
	if(client_head == NULL && new_client != NULL){
		client_head = new_client;
		//FD_SET(void_fd, &com_set);
		FD_SET(new_client -> fd, &com_set);
		pthread_create(&commThread, NULL, handleConnectedClient, (void*)&void_fd);
		pthread_setname_np(commThread, "COMM");
	}
	else{
		client* temp_client = client_head;
		while(temp_client -> next_client != NULL){
			temp_client = temp_client -> next_client;
		}
		temp_client -> next_client = new_client;
		FD_SET(new_client -> fd, &com_set);
		comm_set = com_set;
		//send(void_fd, "newUser", 7, 0);
	}
}

void sendUOFF(char* name){
	client* temp_client = client_head;
	clearBuffer();
	strcat(messageBuffer, "UOFF ");
	strcat(messageBuffer, name);
	strcat(messageBuffer, " \r\n\r\n");
	while(temp_client != NULL){
		send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
		if(v_flag == true){
			verbose(false);
		}
		temp_client = temp_client -> next_client;
	}
}

void shutdownServer(){
	if(client_head == NULL){
		FD_ZERO(&read_set);
		FD_ZERO(&com_set);
		free(messageBuffer);
		exit(EXIT_SUCCESS);
	}
	client* temp_client = client_head;
	client* prevClient = temp_client;
	clearBuffer();
	strcat(messageBuffer, "BYE \r\n\r\n");

	//Kill comm thread
	//pthread_kill(commThread, 0);
	pthread_cancel(commThread);

	//CLOSE ALL FILE DESCRIPTORS
	while(temp_client != NULL){
		if(v_flag == true){
			verbose(true);
		}
		send(temp_client -> fd, messageBuffer, strlen(messageBuffer), 0);
		close(temp_client -> fd);
		prevClient = temp_client;
		temp_client = temp_client -> next_client;
		free(prevClient);
	}

	writeAccounts();

	//zero out read sets
	FD_ZERO(&read_set);
	FD_ZERO(&com_set);

	//FREE MEMORY
	free(messageBuffer);


	exit(EXIT_SUCCESS);
}

void logoutUser(client* logout_client){
	bool killThread = false;
	client* prevClient = client_head;
	FD_CLR(logout_client -> fd, &com_set);
	if(client_head == logout_client){
	  	if(client_head -> next_client == NULL){
	  		client_head = NULL;
	  		killThread = true;
	  	}
	  	else{
	  		client_head = logout_client -> next_client;
	  	}
	}
	else{
	  	while(prevClient -> next_client != logout_client)
			prevClient = prevClient -> next_client;
	  	prevClient -> next_client = logout_client -> next_client;
	}
	char* name = logout_client -> username;
	clearBuffer();
	strcat(messageBuffer, "BYE \r\n\r\n");
	if(v_flag == true){
		verbose(false);
	}
	send(logout_client -> fd, messageBuffer, strlen(messageBuffer), 0);
	clearBuffer();
	//close fd
	close(logout_client -> fd);
	//free the memory
	free(logout_client);

	//send UOFF message to all other clients
	sendUOFF(name);
	if(killThread == true){
		printf("thread killed");
		pthread_exit(&commThread);
		pthread_cancel(commThread);
	}
}

void clientDisconnected(client* dClient){
	client* prevClient = client_head;
	bool killThread = false;
	FD_CLR(dClient -> fd, &read_set);
	if(client_head == dClient){
	  	if(client_head -> next_client == NULL){
	  		client_head = NULL;
	  		killThread = true;
	  	}
	  	else{
	  		client_head = dClient -> next_client;
	  	}
	}
	else{
	  	while(prevClient -> next_client != dClient)
			prevClient = prevClient -> next_client;
	  	prevClient -> next_client = dClient -> next_client;
	}
	char* name = dClient -> username;
	clearBuffer();
	//close fd
	close(dClient -> fd);
	//free the memory
	free(dClient);

	//send UOFF message to all other clients
	sendUOFF(name);

	if(killThread == true){
		pthread_exit(&commThread);
		pthread_cancel(commThread);
	}

}

void* loginUser(void* fd){
	int length = sizeof(clientInfo);
	connfd = accept(listenfd, (struct sockaddr*)&clientInfo, (unsigned int*)&length);
	memset(messageBuffer, '\0', 1024);
	recv(connfd,  messageBuffer, 1024, 0);
	if(v_flag == true){
		verbose(true);
	}
	char** args = genAndFillArgsArray(messageBuffer);
	//They're asking to start chatting
	if(strcmp(args[0], "WOLFIE") == 0){
		memset(messageBuffer, '\0', 1024);
		strcat(messageBuffer, "EIFLOW \r\n\r\n");
		send(connfd, messageBuffer, strlen(messageBuffer), 0);
		memset(messageBuffer, '\0', 1024);
		recv(connfd, messageBuffer, 1024, 0);
		if(v_flag == true){
			verbose(true);
		}
		args = genAndFillArgsArray(messageBuffer);
		char* user;
		//OLD USER
		if(strcmp(args[0], "IAM") == 0){
			user = args[1];
			//User Exists
			clearBuffer();
			if(newAccount(args[1]) == false){
				printf("here\n");
				//user is not already logged in
				if(checkUsername(args[1]) == false){
					printf("here\n");
					strcat(messageBuffer, "AUTH ");
					strcat(messageBuffer, args[1]);
					strcat(messageBuffer, " \r\n\r\n");
					send(connfd, messageBuffer, strlen(messageBuffer), 0);
					clearBuffer();
					recv(connfd, messageBuffer, 1024, 0);
					if(v_flag == true){
						verbose(true);
					}	
					args = genAndFillArgsArray(messageBuffer);
					//got right protocol back
					if(strcmp(args[0], "PASS") == 0){
						printf("user: %s\n",user);
						//user provided proper password
						if(isCorrectPassword(user,args[1]) == true){
							clearBuffer();
							strcat(messageBuffer, "SSAP \r\n\r\n");
							send(connfd, messageBuffer, strlen(messageBuffer), 0);
							if(v_flag == true){
								verbose(false);
							}
							//printf("here\n");
							clearBuffer();
							sleep(2);
							strcat(messageBuffer, "HI ");
							strcat(messageBuffer, user);
							strcat(messageBuffer, " \r\n\r\n");
							send(connfd, messageBuffer, strlen(messageBuffer), 0);
							if(v_flag == true){
								verbose(false);
							}
							clearBuffer();
						}
						//incorrect password
						else{
							strcat(messageBuffer, "ERR 02 BAD PASSWORD \r\n\r\n");
							send(connfd, messageBuffer, strlen(messageBuffer), 0);
							if(v_flag == true){
								verbose(false);
							}
							clearBuffer();
							strcat(messageBuffer, "BYE \r\n\r\n");
							send(connfd, messageBuffer, strlen(messageBuffer), 0);
							if(v_flag == true){
								verbose(false);
							}
							clearBuffer();
							close(connfd);
							goto killThread;
						}
					}
				}
				//user already logged in
				else{
					strcat(messageBuffer, "ERR 00 USER NAME TAKEN \r\n\r\n");
					send(connfd, messageBuffer, strlen(messageBuffer), 0);
					if(v_flag == true){
						verbose(false);
					}
					clearBuffer();
					strcat(messageBuffer, "BYE \r\n\r\n");
					send(connfd, messageBuffer, strlen(messageBuffer), 0);
					if(v_flag == true){
						verbose(false);
					}
					clearBuffer();
					close(connfd);
					goto killThread;
				}
			}
			//User does not exist
			else{
				strcat(messageBuffer, "ERR 01 USER NOT AVAILABLE \r\n\r\n");
				send(connfd, messageBuffer, strlen(messageBuffer), 0);
				if(v_flag == true){
					verbose(false);
				}
				clearBuffer();
				strcat(messageBuffer, "BYE \r\n\r\n");
				send(connfd, messageBuffer, strlen(messageBuffer), 0);
				if(v_flag == true){
					verbose(false);
				}
				clearBuffer();
				close(connfd);
				goto killThread;
			}
		}
		//NEW USER
		else if(strcmp(args[0], "IAMNEW") == 0){
			user = args[1];
			//This account does not already exist
			if(newAccount(args[1]) == true){
				clearBuffer();
				strcat(messageBuffer, "HINEW ");
				strcat(messageBuffer, args[1]);
				strcat(messageBuffer, " \r\n\r\n");
				send(connfd, messageBuffer, strlen(messageBuffer), 0);
				if(v_flag == true){
					verbose(false);
				}
				clearBuffer();
				recv(connfd, messageBuffer, 1024, 0);
				if(v_flag == true){
					verbose(true);
				}	
				args = genAndFillArgsArray(messageBuffer);
				//new password came
				if(strcmp(args[0], "NEWPASS") == 0){
					printf("password %s\n", args[1]);
					if(isValidPassword(args[1]) == true){
						printf("valid password\n");
						account* new_account = malloc(sizeof(account));
						strcpy(new_account -> name, user);
						strcpy(new_account -> password, args[1]);
						new_account -> next_account = NULL;
						addAccount(new_account);
						clearBuffer();
						strcat(messageBuffer, "SSAPWEN \r\n\r\n");
						send(connfd, messageBuffer, strlen(messageBuffer), 0);
						if(v_flag == true){
							verbose(false);
						}
						clearBuffer();
						sleep(2);
						strcat(messageBuffer, "HI ");
						strcat(messageBuffer, user);
						strcat(messageBuffer, " \r\n\r\n");
						send(connfd, messageBuffer, strlen(messageBuffer), 0);
						if(v_flag == true){
							verbose(false);
						}
						sleep(2);
					}
					else{
						clearBuffer();
						strcat(messageBuffer, "ERR 02 BAD PASSWORD \r\n\r\n");
						send(connfd, messageBuffer, strlen(messageBuffer), 0);
						if(v_flag == true){
							verbose(false);
						}	
						clearBuffer();
						strcat(messageBuffer, "BYE \r\n\r\n");
						send(connfd, messageBuffer, strlen(messageBuffer), 0);
						if(v_flag == true){
							verbose(false);
						}
						close(connfd);
						goto killThread;
					}
				}
				else{
					goto BadLog;
				}
			}
			//This user already exists
			else{
				clearBuffer();
				strcat(messageBuffer, "ERR 00 USER NAME TAKEN \r\n\r\n");
				send(connfd, messageBuffer, strlen(messageBuffer), 0);
				if(v_flag == true){
					verbose(false);
				}
				clearBuffer();
				strcat(messageBuffer, "BYE \r\n\r\n");
				send(connfd, messageBuffer, strlen(messageBuffer), 0);
				if(v_flag == true){
					verbose(false);
				}	
				close(connfd);
				goto killThread;
			}
		}
		else{
BadLog:
			clearBuffer();
			strcat(messageBuffer, "ERR 100 INTERNAL SERVER ERROR \r\n\r\n");
			send(connfd, messageBuffer, strlen(messageBuffer), 0);
			if(v_flag == true){
				verbose(false);
			}
			clearBuffer();
			strcat(messageBuffer, "BYE \r\n\r\n");
			send(connfd, messageBuffer, strlen(messageBuffer), 0);
			if(v_flag == true){
				verbose(false);
			}	
			close(connfd);
			clearBuffer();
			goto killThread;
		}

		//printf("here:\n");
		client* new_client = malloc(sizeof(client));

		strcat(new_client -> username, user);
		new_client -> fd = connfd;
		new_client -> next_client = NULL;

		//addClient(new_client);
		

		clearBuffer();
		strcat(messageBuffer, "MOTD ");
		strcat(messageBuffer, motd);
		strcat(messageBuffer, " \r\n\r\n");
		send(connfd, messageBuffer, strlen(messageBuffer), 0);
		if(v_flag == true){
			verbose(false);
		}

		new_client -> login_time = time(NULL);

		//Login complete!
		clearBuffer();
		addClientToList(new_client);
	}
killThread:
	free(args[0]);
	free(args);
	pthread_kill(loginThread, 0);
	return 0;
}
void usage(){
	printf("USAGE\n\n");
	printf("./server.o [-h|-v] PORT_NUMBER MOTD\n");
	printf("%-15s%-50s\n", "-h", "Displays help menu & returns EXIT_SUCCESS");
	printf("%-15s%-50s\n", "-v", "Verbose print all incoming and outgoing protocol verbs & content");
	printf("%-15s%-50s\n", "PORT_NUMBER", "Port number to listen on.");
	printf("%-15s%-50s\n", "MTOD", "Message to display to the client when they connect.");
}

void sigHandler(){
	printf("\n");
	shutdownServer();
}

void verbose(bool input){
	if(input == true)
		printf("incoming command: ");
	else
		printf("outgoing command: ");

	printf("%s",messageBuffer);
}

void readAccounts(char* file){
	//printf("parsing file\n");
	FILE* fp;
	fp = fopen(file, "r");

	//could not open file
	if(fp == NULL)
		exit(EXIT_FAILURE);

	//printf("here\n");
	clearBuffer();
	while(fgets(messageBuffer, 1024, fp) != NULL){
		//printf("line: %s\n",messageBuffer);
		char** args = genAndFillArgsArray(messageBuffer);
		//printf("username: %s\n password: %s\n", args[0], args[1]);
		account* new_account = malloc(sizeof(account));

		//add username and password
		strcat(new_account -> name, args[0]);
		//password = unhash(args[1]);
		strcat(new_account -> password, args[1]);
		new_account -> next_account = NULL;

		addAccount(new_account);
		clearBuffer();
		free(args[0]);
		free(args);
	}

	fclose(fp);
}

void addAccount(account* new_account){
	account* temp_account = account_head;

	if(account_head == NULL){
		account_head = new_account;
	}else{
		while(temp_account -> next_account != NULL)
			temp_account = temp_account -> next_account;
		temp_account -> next_account = new_account;
	}
}

void printAccounts(){
	account* temp_account = account_head;
	if(account_head == NULL)
		printf("There are no accounts to display\n");
	else{
		printf("Accounts\n");
		printf("%-25s%-25s\n", "Username", "Password");
		while(temp_account != NULL){
			printf("%-25s%-25s\n", temp_account -> name, temp_account -> password);
			temp_account = temp_account -> next_account;
		}
	}
}

void writeAccounts(){
	char* outFile;
	if(account_file == true)
		outFile = Afile;
	else
		outFile = "Account.txt";

	FILE* fp = fopen(outFile, "w");

	if(fp == NULL){
		fprintf(stderr, "ERR 100 CANNOT WRITE ACCOUNTS TO FILE");
	}else{
		account* temp_account = account_head;
		account* prevAccount = temp_account;

		//write each accounts creds to file
		while(temp_account != NULL){
			//writes user and password to file
			//char* has
			fprintf(fp, "%s %s\n", temp_account -> name, temp_account -> password);
			prevAccount = temp_account;
			temp_account = temp_account -> next_account;
			free(prevAccount);
		}
		fclose(fp);
	}
}

bool newAccount(char* name){
	account* temp_account = account_head;
	while(temp_account != NULL){
		if(strcmp(temp_account -> name, name) == 0)
			return false;
		temp_account = temp_account -> next_account;
	}

	return true;
}

bool isValidPassword(char* password){
	bool containsUppercase = false; 
	bool containsSymbol = false;
	bool containsNumber = false;

	if(strlen(password) < 5) {
		return false;
	}

	char* currentPosition = password;
	int length = strlen(password);
	for (int i = 0; i < length; i++) {
		// possibility to end early before searching through the entire password
		if(containsUppercase && containsSymbol && containsNumber) {
			return true;
		}
		char currentChar = *currentPosition;
		// UPPERCASE 
		if (currentChar >= 'A' && currentChar <= 'Z'){
			//printf("uppercase found\n");
			containsUppercase = true;
		}
		// SYMBOL 
		else if (((currentChar >= '!') && (currentChar <= '/')) || (((currentChar >= ':') && (currentChar <= '@'))) 
			|| ((currentChar >= '[') && (currentChar <= 96)) || ((currentChar >= '{') && (currentChar <= '~'))) {
			//printf("symbol found\n");
			containsSymbol = true;
		}
		// NUMBER
		else if (currentChar >= '0' && currentChar <= '9'){
			//printf("number found\n");
			containsNumber = true;
		}
		else {
			// do nothing
		}
		currentPosition++;
	} // for loop end

	if(containsUppercase && containsSymbol && containsNumber) {
		return true;
	}
	else {
		return false;
	}
}

bool isCorrectPassword(char* user, char* password){
	account* temp_account = account_head;
	while(temp_account != NULL){

		if((strcmp(temp_account -> name, user) == 0)){
			if(strcmp(temp_account -> password, password) == 0){
				return true;
			}
		} 
		
		temp_account = temp_account -> next_account;
	}

	return false;
}