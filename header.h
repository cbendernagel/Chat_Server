#define _GNU_SOURCE
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>

//DEFINES
//#define _GNU_SOURCE

// ERROR CODES FOR USE WITH ERR VERB WHEN SERVER IS SENDING TO CLIENT OVER SOCKET
// EXAMPLE "ERR 00"
// 00 - USER WITH THAT NAME ALREADY EXISTS (CREATING NEW USER)
// 01 - USER NOT AVAILABLE FOR CHATTING | EITHER DOESN'T EXIST OR ISN'T CONNECTED (WHEN TRYING TO SEND A MESSAGE)


// SERVER STRUCTS //
struct client {
  time_t login_time;
	char username[20];
	char ip_addr[20];
  int socket; 
	int fd;
	struct client *next_client;
};

struct account {
	char name[25];
	char password[25];
	struct account *next_account;
};

typedef struct client client;
typedef struct account account;

account* account_head;
client* client_head;

// CLIENT STRUCTS //
struct chat {
  struct chat* nextChat;
  char chattingWithUsername[20];
  int fd;
  pid_t pid;
  char** argv;
  bool disabled;
};

typedef struct chat chat;

chat* chat_head = NULL;


///////////////////////////// CHARLIE'S METHOD DECLARATIONS ///////////////////////////////
////////////////////// METHODS THAT WILL ONLY BE USED IN SERVER.C ////////////////////////
void addAccount(account* new_account);
void addClientToList(client* new_client);
bool checkUsername(char* name);
void clearBuffer();
void clientDisconnected(client* dClient);
client* getClientByName(char* username);
void* handleConnectedClient(void* fd);
void handleServerCommand();
void* loginUser(void* fd);
void logoutUser(client* client);
bool newAccount(char* name);
void printAccounts();
void readAccounts(char* file);
void sendUOFF(char* name);
void sigHandler();
void shutdownServer();
void usage();
void verbose(bool input);
void writeAccounts();
bool isCorrectPassword(char* user, char* password);

///////////////////////////// KONRAD'S METHOD DECLARATIONS ///////////////////////////////
////////////////////// METHODS THAT WILL ONLY BE USED IN CLIENT.C ////////////////////////
int attemptServerLogin(char* pname);
void multiplexServerAndInput();
void handleClientCommand();
void handleServerInitiatedComm();
void logoutCommand();
void helpCommand();
void listuCommand(int numArgs, char** args);
void timeCommand(int numArgs, char** args);
void chatCommand(int numArgs, char** args);
void genSecondsToHoursString(char* messageBuffer, int totalSeconds);
// void createNewChat(char** args);
chat* createNewChatStruct(int fd, char **args);
void openChat(chat* chatToOpen, int parentfd, int childfd);
chat* findActiveChat();
void handleSendingChatMSG(chat* activeChat);
void handleReceivedChatMSG(char** args);
void handleReceivedUOFF(int numArgs, char** args);
chat* findChatByUsername(char* userName);
chat* findChatByPID(pid_t pid);
void addChatToList(chat* chatToAdd);
void removeChatFromList(chat* chatToRemove);
void chatStructResourceCleanup(chat* chatBeingRemoved);
void exitMaintenenceTasks();
void reapKilledChatProcesses();
void helpUsage();
bool isValidPassword(char* password);

///////////////////////////// KONRAD'S METHOD DECLARATIONS ///////////////////////////////
////////////////////// METHODS THAT WILL ONLY BE USED IN CHAT.C ////////////////////////
void multiplexChatPartnerAndInput();


///////////////////////////// KONRAD'S METHOD DEFINITIONS ////////////////////////////////
///////////////// METHODS THAT WILL BE USED IN BOTH CLIENT.C AND SERVER.C /////////////////
// LIST OF METHODS FOR EASY REFERENCE:
// char* getNextArg(char* buffer);
// int lenLongestArg(char* buffer);
// int getNumArgs(char* buffer);
// char* genAndFillArgsBuffer(char* buffer, int numArgs, int lenLongestArg);
// char** genAndFillArgsArray(char* messageBuffer);
// bool isCharADigit(char* character);
// bool isArgANumber(char* arg)

char* getNextArg(char* buffer) {
  return strtok(buffer, " ");
}

int lenLongestArg(char* buffer) {
  char bufferDup[1024];
  memset(bufferDup, '\0', 1024);
  memcpy(bufferDup, buffer, 1024);

  int lenLongestArg = 0;
  char* currentArg = getNextArg(bufferDup);
  if (currentArg != NULL) {
    if (strlen(currentArg) > lenLongestArg) {
      lenLongestArg = strlen(currentArg);
    }
    while((currentArg = getNextArg(NULL)) != NULL) {
      //printf("%d\n", (int)strlen(currentSysArg));
      if (strlen(currentArg) > lenLongestArg) {
        lenLongestArg = strlen(currentArg);
      }
    }
    return lenLongestArg;
  }
  else {
    return -1;
  }
}

int getNumArgs(char* buffer) {
  char bufferDup[1024];
  memset(bufferDup, '\0', 1024);
  memcpy(bufferDup, buffer, 1024);

  int numArgs = 0;
  char* currentArg = getNextArg(bufferDup);
  if (currentArg != NULL) {
    numArgs++;
    while((currentArg = getNextArg(NULL)) != NULL) {
      numArgs++;
    }
    return numArgs;
  }
  else {
    return -1;
  }
}

char* genAndFillArgsBuffer(char* buffer, int numArgs, int lenLongestArg) {
    void* newLineLocation = strchr(buffer, '\n');
    if (newLineLocation != NULL) {
        *((char*)newLineLocation) = '\0';
    }
    char bufferDup[1024];
    memset(bufferDup, '\0', 1024);
    memcpy(bufferDup, buffer, 1024);

    // add 1 to length so each element is null terminated
    lenLongestArg++;
    // equivalent of sysArgs[numSysArgs][lenLongestSysArg]
    char* args = (char *)calloc(numArgs, (sizeof(char) * lenLongestArg));
    int cursor = 0;
    char* currentArg = getNextArg(bufferDup);
    strcpy(args + cursor, currentArg); 
    cursor += lenLongestArg;
    while((currentArg = getNextArg(NULL)) != NULL) {
      strcpy(args + cursor, currentArg); 
      cursor += lenLongestArg;
    }
    return args;
}

char** genAndFillArgsArray(char* messageBuffer) {
  
  int numArgs = getNumArgs(messageBuffer);
  int longestArg = lenLongestArg(messageBuffer);
  char* argsBuffer = genAndFillArgsBuffer(messageBuffer, numArgs, longestArg);
  // now to assemble argv[]
  char** args = (char**)calloc(numArgs + 1, sizeof(char*));

  int cursor = 0;
  for (int i = 0; i < numArgs; i++) {
    args[i] = argsBuffer + cursor;
    cursor += (longestArg+1);
    if(strcmp(args[i], "\n") == 0) {
      //printf("about to print newline\n");
    }
    if (strcmp(args[i], "\r") == 0){
      //printf("about to print slash r\n");
    }
    //printf("%s\n", args[i]);

  }
  // set last pointer in the args array to 0 for NULL (just like argv[argc])
  args[numArgs] = 0;
  return args;
}

bool isCharADigit(char* character) {
  //printf("digit consideration: %s\n", character);
    if ((*character >= '0') && (*character <= '9')) {
        return true;
    }
    else {
        return false;
    }
}

bool isArgANumber(char* arg) {
  //printf("string length of thing passed into isarganumber: %ld\n", strlen(arg));
  char* currentChar = arg;
  while (*currentChar != '\0') {
    if(!isCharADigit(currentChar)){
      return false;
    }
    currentChar++;
  }
  return true;
}


// char* getNextNewline(char* buffer) {
//   return strtok(buffer, "\n");
// }

void findNewLineAndReplaceWithNAK(char* messageBuffer) {
  char* newLineLocation; 
  char replaceSpace[1];
  while((newLineLocation = strchr(messageBuffer, '\n')) != NULL) {
    //printf("we find new newLineLocation\n");
    *replaceSpace = 21;
    *newLineLocation = *replaceSpace;
  }
  //printf("we finished serarching for new line\n");
}

void findNAKAndReplaceWithNewLine(char* messageBuffer) {
  char* NAKLocation; 
  char replaceSpace[1];
  while((NAKLocation = strchr(messageBuffer, 21)) != NULL) {
    //printf("we find NAK newLineLocation\n");
    *replaceSpace = '\n';
    *NAKLocation = *replaceSpace;
  }
  //printf("we finished serarching for NAK line\n");
}


// *replaceSpace = 21;   // 21 is DEC value of NAK
//           *replaceLocation = *replaceSpace;