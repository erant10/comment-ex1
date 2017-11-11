// http://www.binarytides.com/server-client-example-c-sockets-linux/

#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write

#include <fcntl.h> // for open
#include <unistd.h> // for close

int sendall(int s, char *buf, int *len) {

    int total = 0; /* how many bytes we've sent */
    int bytesleft = *len; /* how many we have left to send */
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    *len = total; /* return number actually sent here */
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}


int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , read_size, PORT;
    struct sockaddr_in server , client;
    char client_message[2000];

    // init server
    // call the load_users executable


    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Check if PORT was provided
    if (argc == 4) {
        PORT = (int) strtol((argv[3]), NULL, 10);
        if (PORT == 0) {
            printf("Error parsing port.\n");
        }
    } else {
        // Set PORT to default 1337
        PORT = 1337;
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }

    //Listen for clients
    listen(socket_desc , 15);

    // maybe need another while loop here

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    //accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    //Connection established
    puts("Connection accepted");

    //Receive login info from client
    while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 )
    {
        // parse the client_message according to the protocol
        char *username, *password;

        // check if the user exists

        // check if password matches

        // send welcome message

        // recv command from client

        if (loggedin) {
            // Implement the actions here
            if (action == 0) {
                // list_of_files

            } else if( action == 1 ) {
                // delete_file

            } else if( action == 2 ) {
                // add_file

            } else if( action == 3 ) {
                // get_file

            }
        } else {
            // login unsuccessful
            //Send message back to client
            write(client_sock , client_message , strlen(client_message));
        }
    }

    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size < 0) {
        perror("recv failed");
    }

    return 0;
}