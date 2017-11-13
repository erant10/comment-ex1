#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "User.h"
#include "Mail.h"
#define MAXFILES 15
#define MAX_USERNAME 25
#define MAX_PASSWORD 25

struct user {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int numOfFiles;
};

User createUser(char* username, char* password) {

    // memory allocation
    User res = (User) malloc(sizeof(*res));
    if (res == NULL) {						// allocation failure
        printf("memory allocation error\n");
        return NULL;
    }

    // fill in user's fields
    strcpy(res->username,username);
    strcpy(res->password, password);
    res->numOfFiles = 0;
    return res;
}

char* getUsername(User user) {
    return user->username;
}

char* getPassword(User user) {
    return user->password;
}

int getNumOfFiles(User user) {
    return user->numOfFiles;
}

void incrementNumOfFiles(User user, int i) {
    user->numOfFiles += i;
};