#ifndef USER_H
#define USER_H

typedef struct user* User;

// creates a new user
User createUser(char* username, char* password);

// gets user's username
char* getUsername(User user);

// gets user's password
char* getPassword(User user);

// gets number of files in user's directory
int getNumOfFiles(User user);

// adds i to the number of files in the users directory
void incrementNumOfFiles(User user, int i);

#endif
