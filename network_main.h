#ifndef NETWORK_MAIN_H
#define NETWORK_MAIN_H
#include "User.h"
#include <netinet/in.h>

// sends entire message
int sendall(int socket, char *buf, int *len);

// wrapper function for sending a message. adds protocol implementation
int sendMessage(int socket, char *buf);

// receives entire message
int recvall(int socket, char *buf, int *len);

// wrapper function for receiving a message. adds protocol implementation
int recvMessage(int socket, char **buf);

// creates users based on the input users file and creates a directory for each user
User* getUsers(char *users_file, int* numOfUsers, char *users_directory);

// free memory allocated for the users
void destroyUsers(User* users, int numOfUsers);

// authenticate the client's username and password
int auth(int socket, User *users, User *authUser);

// handles the client's required operation
int handle(int socket, User user, User *users);

// sends a list of all files in the users directory
int getListOfFiles(int socket, User user, char *users_directory);

// gets a filename from client and deletes this file
int deleteFile(int socket, User user, char *users_directory);

// gets a file from the client and stores it in that clients directory
int addFile(int socket, User user, char *users_directory);

// get a file id from the client and sends that file
int getFile(int socket, User user, char *users_directory);

// extracts from fullString the substring that follows prefix, and stores it in extracted
int extract(char *fullString, char *extracted, char *prefix);

#endif
