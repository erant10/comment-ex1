#include <arpa/inet.h> //inet_addr
#include <fcntl.h> // for open
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>		// for close
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "network_main.h"

#define DEFAULT_PORT "1337"
#define DEFAULT_HOSTNAME "localhost"
#define MAX_OP_LEN 20
#define MAX_ARG_LEN 500
// OP names
#define LIST_OF_FILES "list_of_files"
#define DELETE_FILE "delete_file"
#define ADD_FILE "add_file"
#define GET_FILE "get_file"
#define QUIT "quit"

int main(int argc , char *argv[])
{


    // handle command line arguments
    char* port = DEFAULT_PORT;
    char* hostname = DEFAULT_HOSTNAME;

    if (argc > 3) {						// too many arguments
        return 1;
    }
    else if(argc > 1) {
        hostname = argv[1];
        if (argc == 3) {
            port = argv[2];
        }
    }

    // connection parameters and server's address
    struct addrinfo hints, *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(hostname, port, &hints, &serverInfo);

    // create socket
    int sock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (sock == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }
    freeaddrinfo(serverInfo);

    char *serverReply;
    // receive greeting from server
    if (recvMessage(sock, &serverReply) != 0) {
        return 1;
    }
    printf("%s\n", serverReply);

    // log in to mail server
    if (login(sock) == 1) {
        if (close(sock) == -1) {
            printf("%s\n", strerror(errno));
        }
        return 1;
    }
    char userInput[MAX_OP_LEN + 2 + 2*MAX_ARG_LEN],
            op[MAX_OP_LEN],
            arg1[MAX_ARG_LEN],
            arg2[MAX_ARG_LEN];

    while(1) {
        // clear commands
        memset(op, 0, MAX_OP_LEN);
        memset(arg1, 0, MAX_ARG_LEN);
        memset(arg2, 0, MAX_ARG_LEN);

        // get desired operation from user
        fgets(userInput, MAX_OP_LEN + 2 + 2*MAX_ARG_LEN, stdin);

        // parse input
        strcpy(op, strtok(userInput , " "));
        strcpy(arg1, strtok(NULL, " "));
        strcpy(arg2 , strtok(NULL, " "));

        // match op name
        if (strcmp(op, LIST_OF_FILES) == 0) {
            if (listOfFiles(sock) == 1) {
                printf("error in getting list of files\n");
                if (close(sock) == -1) {
                    printf("%s\n", strerror(errno));
                }
                return 1;
            }
        } else if (strcmp(op, DELETE_FILE) == 0) {
            if (arg1 == NULL) {
                printf("filename was not provided");
            } else if (deleteFile(sock, arg1) == 1) {
                printf("error in deleting file\n");
                if (close(sock) == -1) {
                    printf("%s\n", strerror(errno));
                }
                return 1;
            }
        } else if (strcmp(op, ADD_FILE) == 0) {
            if (arg1 == NULL || arg2 == NULL) {
                printf("path to file and/or new file name were not provided");
            } else if (addFile(sock, arg1, arg2) == 1) {
                printf("error in adding file\n");
                if (close(sock) == -1) {
                    printf("%s\n", strerror(errno));
                }
                return 1;
            }
        } else if (strcmp(op, GET_FILE) == 0) {
            if (arg1 == NULL || arg2 == NULL) {
                printf("path to save and/or file name were not provided");
            } else if (getFile(sock, arg1, arg2) == 1) {
                printf("error in gettting file\n");
                if (close(sock) == -1) {
                    printf("%s\n", strerror(errno));
                }
                return 1;
            }
        } else if (strcmp(op, QUIT) == 0) {
            if (quit(sock) == 1) {
                printf("error in quitting\n");
                if (close(sock) == -1) {
                    printf("%s\n", strerror(errno));
                }
                return 1;
            }
            break;
        }
        // invalid op
        break;
    }

    // close socket
    if (close(sock) == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    return 0;
}
