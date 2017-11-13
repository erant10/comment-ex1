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
#define PREFIX_LEN 20
#define NUM_OF_CLIENTS 15
#define MAX_FILESIZE 512
#defint MAX_FILENAME 50
#define LEN_FIELD_SIZE 2        // the number of bytes dedicated to the size of the message

// define OP codes
#define LIST_OF_FILES_OPCODE '1'
#define DELETE_FILE_OPCODE '2'
#define ADDFILE_OPCODE '3'
#define GET_FILE_OPCODE '4'
#define QUIT_OPCODE '5'

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
        printf("Only %d bytes were sent\n", lenBufSize);
        return 1;
    }

    // send message itself
    if (sendall(socket, buf, &msgLen) == -1) {
        printf("%s\n", strerror(errno));
        printf("Only %d bytes were sent\n", msgLen);
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
        printf("Only %d bytes were received\n", lenBufSize);
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
        printf("Only %d bytes were received\n", lenI);
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
            currNumOfFiles++;
        }
        free(users[i]);
    }
    free(users);
}

int auth(int socket, User *users, User *authUser) {

    // get messages containing username and password from client
    char *loginMsgUser = NULL, *loginMsgPass = NULL;
    if (recvMessage(socket, &loginMsgUser) == 1 || recvMessage(socket, &loginMsgPass) == 1) {
        return 1;
    }

    // extract username and password from the received messages
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    if (extract(loginMsgUser, username, "User: ") != 0 || extract(loginMsgPass, password, "Password: ") != 0) { // bad input
        return 1;
    }

    // authenticate
    int i = 0;
    while (i < NUM_OF_CLIENTS && users[i] != NULL) {		// go over all users
        User currUser = users[i];
        if (strcmp(username, getUsername(currUser)) == 0 && strcmp(password, getPassword(currUser)) == 0) {
            *authUser = currUser;	// match found
            break;
        }
        i++;
    }

    return 0;
}

int handle(int socket, User user, User *users, char *users_directory) {
    char* opCode;
    if (recvMessage(socket, &opCode) == 1) {					// receive opcode
        printf("failed to get action request from client\n");
        return 1;
    }

    if (*opCode == LIST_OF_FILES_OPCODE) {							// get list of files
        if (getListOfFiles(socket, user, users_directory) == 1) {
            printf("failed to list files\n");
            return 1;
        }
    } else if (*opCode == DELETE_FILE_OPCODE) {					    // delete a file
        if (deleteFile(socket, user, users_directory) != 0) {
            printf("failed to delete file\n");
            return 1;
        }
    } else if (*opCode == ADDFILE_OPCODE) {					        // add a file
        if (addFile(socket, user, users_directory) != 0) {
            printf("failed to add file\n");
            return 1;
        }
    } else if (*opCode == GET_FILE_OPCODE) {						// get a file
        if (getFile(socket, user, users_directory) != 0) {
            printf("failed to get file\n");
            return 1;
        }
    } else {                                					    // error or quit
        return 1;
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

int getListOfFiles(int socket, User user, char *users_directory) {
    // list_of_files
    DIR *dir;
    char *username = getUsername(user);
    char path[sizeof(username)+ sizeof(users_directory)+1];
    sprintf(path, sizeof path, "%s/%s", users_directory, username); // build path to user directory
    int numOfFiles = getNumOfFiles(user);

    struct dirent *ent;
    if ((dir = opendir (path)) != NULL) {
        char file_list[MAX_FILENAME*numOfFiles], filename[MAX_FILENAME];
        // read all the files into a file_list
        while ((ent = readdir (dir)) != NULL) {
            sprintf(filename, ent->d_name);
            sprintf(file_list + strlen(file_list), "%s\n", filename);  // add file name to list of files
        }
        // finally close dir
        closedir (dir);

        // Send the file list
        if (sendMessage(socket, file_list) != 0) {		// send list of files
            return 1;
        }
    } else {
        /* could not open directory */
        return 1;
    }

    return 0;
}

int deleteFile(int socket, User user, char *users_directory) {
    char filename[MAX_FILENAME];
    if (recvMessage(socket, &filename) != 0) {		// get file name client
        return 1;
    };

    char filepath[sizeof(username) + sizeof(users_directory) + sizeof(filename) + 2];
    // build path to file
    sprintf(filepath, sizeof filepath, "%s/%s/%s", users_directory, username,filename);

    char *message;
    if( access( filepath, F_OK ) != -1 ) {
        // file exists - remove it
        if(remove(filepath) == 0) {
            message = "File removed.";
            incrementNumOfFiles(user, -1);
        } else {
            return 1;
        }
    } else {
        // file doesn't exist
        message = "No such file exists!";
    }

    if (sendMessage(socket, message) != 0) {		// send list of files
        return 1;
    }
    return 0;
}

int addFile(int socket, User user, char *users_directory) {
    // first receive the file name
    char file_name[MAX_FILENAME], file_content[MAX_FILESIZE];
    int file_size;
    FILE *fd;
    char *username = getUsername(user);

    if (recvMessage(socket, &file_name) != 0) {		// get file name
        return 1;
    };

    if (recvMessage(socket, &file_content) != 0) {		// get file content
        return 1;
    };
    file_size = atoi(file_content);

    char filepath[sizeof(username) + sizeof(users_directory) + sizeof(file_name) + 2];
    // build path to file
    sprintf(filepath, sizeof filepath, "%s/%s/%s", users_directory, username, file_name);

    fd = fopen(filepath, "w");
    if (fd == NULL) {
        printf(stderr, "Failed to open file: %s\n", strerror(errno));
        return 1;
    }
    fwrite(file_content, sizeof(char), file_size, fd);
    fclose(fd);

    // increment users file count
    incrementNumOfFiles(user, 1);

    // send result message
    char *message = "File added.";
    if (sendMessage(socket, message) != 0) {		// send result
        return 1;
    }

    return 0;
}

int getFile(int socket, User user, char *users_directory) {
    // first receive the file name
    char file_name[MAX_FILENAME], file_content[MAX_FILESIZE];
    int file_size;
    char *username = getUsername(user);
    FILE *fp;

    if (recvMessage(socket, &file_name) != 0) {		// get file name
        return 1;
    };

    char filepath[sizeof(username) + sizeof(users_directory) + sizeof(file_name) + 2];
    // build path to file
    sprintf(filepath, sizeof filepath, "%s/%s/%s", users_directory, username, file_name);

    fp = fopen(filepath, "rb");
    if (fp == NULL) {
        char *message = "No such file exists!";
        sendMessage(socket, message);
        return 1;
    }

    // read file content into buffer
    char *content_buffer = malloc(MAX_FILESIZE + 1);
    fread(content_buffer, MAX_FILESIZE, 1, fp);
    fclose(fp);
    content_buffer[content_buffer] = 0;

    // send file name
    if (sendMessage(socket, file_name) != 0) {
        free(content_buffer);
        return 1;
    }
    // send file content
    if (sendMessage(socket, content_buffer) != 0) {
        free(content_buffer);
        return 1;
    }

    return 0;
}