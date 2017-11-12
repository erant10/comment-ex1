#include "main_aux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#define MAX_USERNAME 15
#define MAX_PASSWORD 15
#define NUM_OF_CLIENTS 15
#define MAX_FILESIZE 512
#define LEN_FIELD_SIZE 2        // the number of bytes dedicated to the size of the message
// define OP codes
#define LIST_OF_FILES_OPCODE '1'
#define DELETE_FILE_OPCODE '2'
#define ADDFILE_OPCODE '3'
#define GET_FILE_OPCODE '4'


int sendall(int socket, char *buf, int *len) {
    int total = 0;          // how many bytes we've sent
    int bytesleft = *len;   // how many we have left to send
    int n;

    while(total < *len) {
        n = send(socket, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    *len = total;           // return number actually sent here
    return n == -1 ? -1:0;  // -1 on failure, 0 on success
}

int sendMessage(int socket, char *buf) {
    int msgLen = (int) strlen(buf);

    // send message length in the first two bytes
    char lenBuf[LEN_FIELD_SIZE];
    int lenBufSize = LEN_FIELD_SIZE;
    short lenS = (short) msgLen;
    lenS = htons(lenS);									// change to network order (big endian)
    memcpy(lenBuf, &lenS, LEN_FIELD_SIZE);				// move from short to a char array for sending
    if (sendall(socket, lenBuf, &lenBufSize) == -1) {	// send
        printf("%s\n", strerror(errno));
        printf("Only %d bytes were sent due to the error\n", lenBufSize);
        return 1;
    }

    // send message itself
    if (sendall(socket, buf, &msgLen) == -1) {
        printf("%s\n", strerror(errno));
        printf("Only %d bytes were sent due to the error\n", msgLen);
        return 1;
    }
    return 0;
}

int recvall(int socket, char *buf, int *len) {
    int total = 0;              // how many bytes we've received
    int bytesleft = *len;       // how many bytes we have left to receive
    int n;
    while(total < *len) {
        n = (int) recv(socket, buf + total, (size_t) bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total;               // return number actually received
    return n == -1 ? -1 : 0;    // -1 on failure, 0 on success
}

int recvMessage(int socket, char **buf) {

    // receive message length in the first two bytes
    char lenBuf[LEN_FIELD_SIZE];
    int lenBufSize = LEN_FIELD_SIZE;
    if (recvall(socket, lenBuf, &lenBufSize) == -1) {
        printf("%s\n", strerror(errno));
        printf("Only %d bytes were received due to the error\n", lenBufSize);
        return 1;
    }

    short lenS = 0;
    memcpy(&lenS, lenBuf, LEN_FIELD_SIZE);		// move from char array to a short type
    int lenI = ntohs(lenS);						// change to host's order

    // receive the message itself
    *buf = (char*) malloc(sizeof(char) * lenI + 1); // allocate memory for the message
    if (*buf == NULL) {
        return 1;
    }
    memset(*buf, '\0', lenI + 1);					// initialize the memory
    if (recvall(socket, *buf, &lenI) == -1) {		// receive
        printf("%s\n", strerror(errno));
        printf("Only %d bytes were received due to the error\n", lenI);
        return 1;
    }

    return 0;
}


User* getUsers(char *users_file, int* numOfUsers, char *users_directory) {
    FILE* file;
    char line[MAX_USERNAME + MAX_PASSWORD + 1],
            userName[MAX_USERNAME],
            password[MAX_PASSWORD],
            files_dir[MAX_USERNAME + sizeof(users_directory) + 1],
            *pTab,
            *pNewLine;

    User* res = (User*) malloc(sizeof(User) * NUM_OF_CLIENTS);
    memset(res, 0, sizeof(User) * NUM_OF_CLIENTS);
    int i = 0;

    // open users file
    file = fopen(users_file, "r");
    if (file == NULL) {
        printf("The users file %s couldn't be opened", users_file);
        return NULL;
    }

    // create a user for each line
    while (fgets(line, MAX_USERNAME + MAX_PASSWORD + 1, file) != NULL && i < NUM_OF_CLIENTS) {
        pNewLine = strstr(line, "\n");
        if (pNewLine != NULL) {
            *pNewLine = 0; 				    // replacing '\n' with '\0'
        }

        pTab = strstr(line, "\t");
        if (pTab == NULL) {
            return NULL; 				    // if \t is missing, exit
        }

        strcpy(password, pTab+1);           // retrieve password
        *pTab = 0;                          // replacing \t with '\0'
        strcpy(userName, line);			    // retrieve username
        sprintf(files_dir, sizeof files_dir, "%s/%s", users_directory, userName); // build files directory name

        //create a directory for each user
        struct stat st = {0};
        if (stat(files_dir, &st) == -1) { //if this directory doesn't exist already
            mkdir(files_dir, 0700);
        }

        res[i] = createUser(userName, password);
        i++;
    }

    *numOfUsers = i;
    fclose(file);
    return res;
}

void destroyUsers(User* users, int numOfUsers) {
    int i;
    for (i = 0; i < numOfUsers; i++) {
        // free user's files
        int currNumOfFiles = 0, j = 0;
        while (currNumOfFiles < getNumOfFiles(users[i])) {
            // TODO: delete all files and folders for each user
            currNumOfFiles++;
        }
        free(users[i]);
    }
    free(users);
}

int auth(int socket, User *users, User *authUser, int *userIndex) {

    // get messages containing username and password from client
    char *loginMsgUser = NULL, *loginMsgPass = NULL;
    if (recvMessage(socket, &loginMsgUser) == 1 || recvMessage(socket, &loginMsgPass) == 1) {
        free(loginMsgUser);
        free(loginMsgPass);
        return 1;
    }

    // extract username and password from the received messages
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    if (extract(loginMsgUser, username, "User: ") != 0 || extract(loginMsgPass, password, "Password: ") != 0) { // bad input
        free(loginMsgUser);
        free(loginMsgPass);
        return 1;
    }

    // authenticate
    int i = 0;
    while (i < NUM_OF_CLIENTS && users[i] != NULL) {		// go over all users
        User currUser = users[i];
        if (strcmp(username, getUsername(currUser)) == 0 && strcmp(password, getPassword(currUser)) == 0) {
            *authUser = currUser;	// match found
            *userIndex = i;
            break;
        }
        i++;
    }

    free(loginMsgUser);
    free(loginMsgPass);
    return 0;
}

int handle(int socket, User user, User *users) {
    char* opCode;
    if (recvMessage(socket, &opCode) == 1) {					// receive opcode
        printf("failed to get action request from client\n");
        free(opCode);
        return 1;
    }

    if (*opCode == LIST_OF_FILES_OPCODE) {							// get list of files
        if (getListOfFiles(socket, user) == 1) {
            printf("failed to list files\n");
            free(opCode);
            return 1;
        }
    } else if (*opCode == DELETE_FILE_OPCODE) {					    // delete a file
        if (deleteFile(socket, user) != 0) {
            printf("failed to delete file\n");
            free(opCode);
            return 1;
        }
    } else if (*opCode == ADDFILE_OPCODE) {					        // add a file
        if (addFile(socket, user) != 0) {
            printf("failed to add file\n");
            free(opCode);
            return 1;
        }
    } else if (*opCode == GET_FILE_OPCODE) {						// get a file
        if (getFile(socket, user) != 0) {
            printf("failed to get file\n");
            free(opCode);
            return 1;
        }
    } else {                                					    // error or quit
        free(opCode);
        return 1;
    }
    free(opCode);
    return 0;
}

int quit(int socket, fd_set *master, int *fdmax, int *fdsToUsersMap, int userIndex) {
    // close connection socket
    if (close(socket) == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }
    FD_CLR(socket, master);    // remove from master set
    fdsToUsersMap[userIndex] = 0;

    if (socket == *fdmax) {    // find a new fdmax
        int i, tmpMax = 0;
        for (i = 0; i < *fdmax; i++) {
            if (FD_ISSET(i, master)) {
                if (i > tmpMax) {
                    tmpMax = i;
                }
            }
        }
        *fdmax = tmpMax;
    }
    return 0;
}


int extract(char *fullString, char *extracted, char *prefix) {
    char expectedPrefix[PREFIX_LEN];
    memset(expectedPrefix, '\0', PREFIX_LEN);
    strncpy(expectedPrefix, fullString, strlen(prefix));
    if (strcmp(prefix, expectedPrefix) != 0) {			// check if prefix is correct
        return 1;
    }
    strcpy(extracted, fullString+strlen(prefix));		// extract the rest of the string
    return 0;
}

void closeSockets(int fdmax, fd_set master) {
    int i;
    for (i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &master)) {
            if (close(i) == -1) {
                printf("%s\n", strerror(errno));
            }
        }
    }
}

void closeAndFree(int fdmax, fd_set master, User *users, int numOfUsers, int *fdsToUsersMap) {
    closeSockets(fdmax, master);
    destroyUsers(users, numOfUsers);
    free(fdsToUsersMap);
}

// TODO: add users parent directory to parameters
int getListOfFiles(int socket, User user) {
    // list_of_files
    DIR *dir;
    char *username = getUsername(user);
    struct dirent *ent;
    if ((dir = opendir (username)) != NULL) {
        char *files_buffer, *filename;
        // read all the files into a files_buffer
        while ((ent = readdir (dir)) != NULL) {
            sprintf(filename, "%s\n", ent->d_name);
            sprintf(files_buffer + strlen(files_buffer),filename);  // add file name to list
        }
        // finally close dir
        closedir (dir);

        // Send the file list
        if (sendMessage(socket, files_buffer) != 0) {		// send list of files
            free(files_buffer);
            free(filename);
            free(ent);
            free(username);
            return 1;
        }
    } else {
        /* could not open directory */
        return 1;
    }

    return 0;
}

// TODO: add users parent directory to parameters
int deleteFile(int socket, User user) {
    char* filename;
    if (recvMessage(socket, &filename) != 0) {		// get file name client
        free(filename);
        return 1;
    };

    char *message;
    if( access( filename, F_OK ) != -1 ) {
        // file exists - remove it
        if(remove(filename) == 0) {
            message = "File removed.";
        } else {
            return 1;
        }
    } else {
        // file doesn't exist
        message = "No such file exists!";
    }
    if( send(client_sock , message , strlen(message) , 0) < 0) {
        puts("Failed sending message");
        return 1;
    }
    // Send the file list
    if (sendMessage(socket, message) != 0) {		// send list of files
        free(message);
        free(filename);
        return 1;
    }
    return 0;
}

// TODO: Implement
int addFile(int socket, User user) {
    return 0;
}

// TODO: Implement
int getFile(int socket, User user){
    return 0;
}