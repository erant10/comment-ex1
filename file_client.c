#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr

#include <fcntl.h> // for open
#include <unistd.h> // for close

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char *HOST;
    int PORT;


    // Check arguments
    if ( argc > 3) {
        printf("Error in number of arguments (1 or 2 expected)\n");
        exit(-1);
    }

    // Check if a host and/or port was provided
    if (argc > 1) {
        // Set host name
        HOST = argv[1];
        // Set port
        PORT = (int) strtol((argv[2]), NULL, 10);
        if (PORT == 0) {
            printf("Error parsing port.\n");
            exit(-1);
        }
    } else {
        // if they were not provided - set to default values
        HOST = "127.0.0.1";
        PORT = 1337;
    }

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr(HOST);
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    // Successfully connected to server
    char message[1000] , server_reply[2000];
    int action;
    int quit = 0;
    int loggedin = 0;
    char *input;

    // try to login - recieve welcome message from the server
    // puts("Welcome! Please log in.\n");

    scanf("%s" , input);
    while(!loggedin && !quit){
        //Send login info
        if( send(sock , input , strlen(input) , 0) < 0)
        {
            puts("Failed sending login details");
            return 1;
        }
        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }
        // print response
        // puts(server_reply);
        // check the server reply to see if login was successful
        if(successful_login){
            loggedin = 1;
        } else {
            //puts("Invalid username or password. try again.\n");
            scanf("%s" , input);
        }
    }

    //keep communicating with server until quit
    while(!quit)
    {
        char *action_name, *param1, *param2;
        int read = scanf("%s %s %s", action_name, param1, param2);
        action = -1;
        // read the input from the user until the action is valid
        while (action < 0 && !quit) {
            if (strcmp(action_name,"list_of_files") == 0) {
                action = 0;
            } else if (strcmp(action_name,"delete_file") == 0) {
                action = 1;
            } else if(strcmp(action_name,"add_file") == 0) {
                action = 2;
            } else if(strcmp(action_name,"get_file") == 0) {
                action = 3;
            } else if(strcmp(action_name,"quit") == 0) {
                quit = 1;
            } else {
                printf("Sorry, Unknown Action. Try Again: \n");
                scanf("%s" , action_name);
            }
        }
        if (quit) {
            break;
        }
        //Handle Actions Here
        if (action == 0) {
            // list_of_files - send action to server and print the response
            if( send(sock , action , strlen(action) , 0) < 0) {
                puts("Send failed");
                return 1;
            }
            //Receive a reply from the server
            if( recv(sock , server_reply , 2000 , 0) < 0) {
                puts("recv failed");
                break;
            }
            // print response
            puts(server_reply);

        } else if( action == 1 ) {
            // delete_file - send action and the file name to the server
            char buffer = ""; // build the buffer according to the protocol
            if( send(sock , buffer , strlen(buffer) , 0) < 0) {
                puts("Send failed");
                return 1;
            }
            //Receive a reply from the server
            if( recv(sock , server_reply , 2000 , 0) < 0) {
                puts("recv failed");
                break;
            }
            // print response
            puts(server_reply);

        } else if( action == 2 ) {
            // add_file - look for the chosen file and open fd

            // ASK YUVAL - what is the best approach to send the file
            // build the buffer with the file content according to the protocol
                // open the file
                // copy file content into buffer

            // send the buffer to the server
            char buffer = ""; // build the data buffer according to the protocol
            if( send(sock , buffer , strlen(buffer) , 0) < 0) {
                puts("Send failed");
                return 1;
            }
            //Receive a reply from the server
            if( recv(sock , server_reply , 2000 , 0) < 0) {
                puts("recv failed");
                break;
            }
            // print response
            puts(server_reply);

        } else if( action == 3 ) {
            // get_file - send action and the file name to the server
            char buffer = "";
            if( send(sock , buffer , strlen(buffer) , 0) < 0) {
                puts("Send failed");
                return 1;
            }
            //Receive a reply from the server
            if( recv(sock , server_reply , 2000 , 0) < 0) {
                puts("recv failed");
                break;
            }
            // save the file


            // print response
            puts(server_reply);
        }


    }

    close(sock);
    return 0;
}