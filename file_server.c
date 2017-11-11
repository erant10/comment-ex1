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
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    // keep accepting connections
    while(1){

        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0) {
            perror("accept failed");
            return 1;
        }
        //Connection established
        puts("Connection accepted");

        char *client_login_info;
        char *username, *password;
        //Keep receiving login info from client until the login is successful
        while( !successful_login ) {
            if ((read_size = recv(client_sock, client_login_info, 2000, 0)) > 0) {
                // parse the client_login_info according to the protocol
                username = "";
                password = "";

                // check if the user exists
                DIR *dir = opendir(username);
                if (dir) {
                    // Directory exists - check if password matches
                    if (password_match) {
                        successful_login = 1;
                    }
                    // closedir when done
                    closedir(dir);
                } else if (ENOENT != errno) {
                    // opendir() failed for some other reason.
                    perror("opendir failed");
                    return 1;
                }
                if(!successful_login) {
                    //Send failed login message
                    char *error_message = "Invalid username or password";
                    if( send(client_sock , error_message , strlen(error_message) , 0) < 0)
                    {
                        puts("Failed sending message");
                        return 1;
                    }
                }
            }
            if (read_size == 0) {
                puts("Client disconnected");
                fflush(stdout);
            } else if (read_size < 0) {
                perror("recv failed");
            }
        }
        // login successful.
        // count how many files the user has
        int file_count;

        // send welcome message
        char *welcome_message;
        sprintf(welcome_message, "Welcome %s, You have %d files.", file_count);;
        if( send(client_sock , welcome_message , strlen(welcome_message) , 0) < 0) {
            puts("Failed sending message");
            return 1;
        }
        // start listening for commands
        char *client_request;
        while ((read_size = recv(client_sock, client_request, 2000, 0)) > 0) {
            // read action type from client_request
            int action;

            switch(action) {
                case 0 :
                    // list_of_files
                    DIR *dir;
                    struct dirent *ent;
                    if ((dir = opendir (username)) != NULL) {
                        char *files_buffer;
                        char *filename;
                        // read all the files into a files_buffer
                        while ((ent = readdir (dir)) != NULL) {
                            sprintf(filename, "%s\n", ent->d_name);
                            sprintf(files_buffer + strlen(files_buffer),filename);
                        }
                        // finally close dir
                        closedir (dir);

                        // Send the file list
                        if( send(client_sock , files_buffer , strlen(files_buffer) , 0) < 0) {
                            puts("Failed sending message");
                            return 1;
                        }
                    } else {
                        /* could not open directory */
                        perror ("");
                        return EXIT_FAILURE;
                    }
                    break;
                case 1 :
                    // delete_file
                    char *filename, *message;
                    if( access( filename, F_OK ) != -1 ) {
                        // file exists - remove it
                        if(remove(filename) == 0) {
                            message = "File removed.";
                        } else {
                            printf("Error: unable to delete the file");
                        }
                    } else {
                        // file doesn't exist
                        message = "No such file exists!";
                    }
                    if( send(client_sock , message , strlen(message) , 0) < 0) {
                        puts("Failed sending message");
                        return 1;
                    }
                    break;
                case 2 :
                    // add_file
                    break;
                case 3 :
                    // get_file
                    break;
                default :
                    // quit
                    printf("quit");
            }
        }
        if (read_size == 0) {
            puts("Client disconnected");
            fflush(stdout);
        } else if (read_size < 0) {
            perror("recv failed");
        }

    }

    return 0;
}