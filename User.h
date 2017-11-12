#ifndef USER_H
#define USER_H

typedef struct user* User;

// creates a new user
User createUser(char* username, char* password);

// gets user's username
char* getUsername(User user);

// gets user's password
char* getPassword(User user);

// gets number of files in user's folder
int getNumOfFiles(User user);


#endif
