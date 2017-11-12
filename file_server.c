#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include "network_main.h"
#define DEFAULT_PORT 1337
#define MAX_USERS 15
#define NUM_OF_CLIENTS 15
#define MAX_FILE 512
#define GREETING_MSG "Welcome! Please log in."


int main(int argc , char *argv[])
{

    // handle command line arguments
    int port;
    char* users_file;

    if (argc < 2) {								// too few arguments
        printf("Missing users file's path.\n");
        return 1;
    } else if (argc == 3) {						// port was provided
        port = atoi(argv[2]);
    } else if (argc == 2) {						// port was not provided
        port = DEFAULT_PORT;
    } else {
        return 1;
    }
    users_file = argv[1];

    // initialization
    int lsd, csd;                                       // listening socket & client socket
    int numOfUsers, fdmax;
    struct sockaddr_in myaddr, client_addr;
    unsigned int sin_size = sizeof(struct sockaddr_in);
    char* greeting = GREETING_MSG;						// greeting message to show client

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    User* users = getUsers(users_file, &numOfUsers);	// creates users and their directories based on the supplied users file
    if (users == NULL) {
        return 1;
    }
    int* fdsToUsersMap = (int*) calloc(NUM_OF_CLIENTS, sizeof(int)); // fdsToUsersMap[i] is the fd associated with Users[i]


    //create listening socket
    lsd = socket(PF_INET, SOCK_STREAM, 0);
    if (lsd == -1) {
        printf("%s\n", strerror(errno));
        destroyUsers(users, numOfUsers);
        return 1;
    }
    FD_SET(lsd, &master);
    fdmax = lsd;

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    // Bind the socket
    if (bind(lsd, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        printf("%s\n", strerror(errno));
        if (close(lsd) == -1) {
            printf("%s\n", strerror(errno));
        }
        destroyUsers(users, numOfUsers);
        return 1;
    }

    // start listening for clients
    if (listen(lsd, MAX_USERS) == -1) {
        printf("%s\n", strerror(errno));
        if (close(lsd) == -1) {
            printf("%s\n", strerror(errno));
        }
        destroyUsers(users, numOfUsers);
        return 1;
    }


    // keep accepting connections one at a time
    while(1){
        readfds = master;      // copy master set

        if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1) {
            printf("%s\n", strerror(errno));
            closeAndFree(fdmax, master, users, numOfUsers, fdsToUsersMap);
            return 1;
        }

        int i;
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == lsd) {
                    // handle new connection
                    csd = accept(lsd, (struct sockaddr*) &client_addr, &sin_size);  // create connection socket
                    if (csd == -1) {
                        printf("%s\n", strerror(errno));
                        continue;			                                        // move on to next read-ready socket
                    }

                    // greet client
                    if (sendMessage(csd, greeting) != 0) {
                        if (close(csd) == -1) {
                            printf("%s\n", strerror(errno));
                            closeAndFree(fdmax, master, users, numOfUsers, fdsToUsersMap);
                            return 1;
                        }
                        continue;			                                        // move on to next read-ready socket
                    }

                    // authenticate client
                    User user = NULL;
                    int userIndex = 0;
                    if (auth(csd, users, &user, &userIndex) == 1 || user == NULL) {	            // failed log in
                        printf("error in authentication\n");
                        sendMessage(csd, "Invalid username or password\n");
                        if (close(csd) == -1) {
                            printf("%s\n", strerror(errno));
                            closeAndFree(fdmax, master, users, numOfUsers, fdsToUsersMap);
                            return 1;
                        }
                        continue;			                                        // move on to next read-ready socket
                    }
                    User user = findUserByFd(users, fdsToUsersMap, i, &userIndex);
                    char *username = getUsername(user);
                    int numOfFiles = getNumOfFiles(user);
                    char *initial_msg;
                    // Send the user a welcome message
                    sprintf(initial_msg, sizeof initial_msg, "Hi %s, you have %d files stored.\n", username, numOfFiles);
                    if (sendMessage(csd, initial_msg) != 0) {	            // successful log in
                        if (close(csd) == -1) {
                            printf("%s\n", strerror(errno));
                            closeAndFree(fdmax, master, users, numOfUsers, fdsToUsersMap);
                            return 1;
                        }
                        continue;			                                        // move on to next read-ready socket
                    }

                    FD_SET(csd, &master); // add to master set
                    if (csd > fdmax) {    // keep track of the max
                        fdmax = csd;
                    }
                    fdsToUsersMap[userIndex] = csd;

                } else {
                    // handle client's request
                    int userIndex = 0;
                    User user = findUserByFd(users, fdsToUsersMap, i, &userIndex);
                    // handle the client's requests until quit or error
                    while(handle(i, user, users, fdsToUsersMap, lsd) != 0) { // error or quit
                        if (quit(i, &master, &fdmax, fdsToUsersMap, userIndex) != 0) {
                            printf("%s\n", strerror(errno));
                            closeAndFree(fdmax, master, users, numOfUsers, fdsToUsersMap);
                            return 1;
                        }
                    }
                }
            }
        }

    }

    return 0;
}