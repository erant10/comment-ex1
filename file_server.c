#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <unistd.h>    //write
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <netinet/in.h>
#include "network_main.h"
#define DEFAULT_PORT 1337
#define MAX_USERS 15
#define NUM_OF_CLIENTS 15
#define MAX_PATH_LEN 500
#define GREETING_MSG "Welcome! Please log in."
#define WELCOME_MSG "Hi %s, you have %d files stored.\n"


int main(int argc , char *argv[])
{

    // handle command line arguments
    int port;
    char *users_file;
    char dir_path[MAX_PATH_LEN];

    if (argc < 2) {								// too few arguments
        printf("Missing users file's path.\n");
        return 1;
    } else if (argc == 4) {						// port was provided
        port = atoi(argv[3]);
        if (port == 0) {
            printf("Error parsing port.\n");
            return 1;
        }
    } else if (argc == 3) {						// port was not provided
        port = DEFAULT_PORT;
    } else {
        return 1;
    }
    users_file = argv[1];
    users_file[sizeof(users_file)] = 0;
    dir_path = argv[2];

    // initialization
    int l_sock;                                       // listening socket
    int numOfUsers;
    struct sockaddr_in myaddr, client_addr;
    unsigned int sin_size = sizeof(struct sockaddr_in);
    char* greeting = GREETING_MSG;						// greeting message to show client

    User* users = getUsers(users_file, &numOfUsers, dir_path);	// creates users and their directories based on the supplied users file
    if (users == NULL) {
        return 1;
    }

    //create listening socket
    l_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (l_sock == -1) {
        printf("%s\n", strerror(errno));
        destroyUsers(users, numOfUsers);
        return 1;
    }

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    // Bind the socket
    if (bind(l_sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        printf("%s\n", strerror(errno));
        if (close(l_sock) == -1) {
            printf("%s\n", strerror(errno));
        }
        destroyUsers(users, numOfUsers);
        return 1;
    }

    // start listening for clients
    if (listen(l_sock, MAX_USERS) == -1) {
        printf("%s\n", strerror(errno));
        if (close(l_sock) == -1) {
            printf("%s\n", strerror(errno));
        }
        destroyUsers(users, numOfUsers);
        return 1;
    }


    // keep accepting connections one at a time
    while(1){

        int c_sock;
        // handle new connection
        c_sock = accept(l_sock, (struct sockaddr*) &client_addr, &sin_size);  // create connection socket
        if (c_sock == -1) {
            printf("%s\n", strerror(errno));
        }

        // greet client
        if (sendMessage(c_sock, greeting) != 0) {
            if (close(c_sock) == -1) {
                printf("%s\n", strerror(errno));
                return 1;
            }
        }

        // authenticate client
        User user = NULL;
        if (auth(c_sock, users, &user) == 1 || user == NULL) {	            // failed log in
            printf("error in authentication\n");
            sendMessage(c_sock, "Invalid username or password\n");
            if (close(c_sock) == -1) {
                printf("%s\n", strerror(errno));
                return 1;
            }
        }

        // successful log in
        sendMessage(c_sock, "connected\n");
        if (close(c_sock) == -1) {
            printf("%s\n", strerror(errno));
            return 1;
        }
        char *username = getUsername(user);
        int numOfFiles = getNumOfFiles(user);
        char initial_msg[sizeof(WELCOME_MSG)+sizeof(username)+2];
        // Send the user a welcome message
        sprintf(initial_msg, sizeof initial_msg, WELCOME_MSG, username, numOfFiles);
        if (sendMessage(c_sock, initial_msg) != 0) {
            if (close(c_sock) == -1) {
                printf("%s\n", strerror(errno));
                return 1;
            }
        }

        // keep handling the client's requests until error or quit
        while(handle(c_sock, user, users, dir_path) == 0) {
            // keep going
        }
        // error or quit - close connection socket
        if (close(c_sock) == -1) {
            printf("%s\n", strerror(errno));
            return 1;
        }

    }

    return 0;
}