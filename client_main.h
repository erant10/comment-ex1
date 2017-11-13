#ifndef CLIENT_MAIN_H
#define CLIENT_MAIN_H

// handles first interaction with the server and sends a log-in request
int login(int socket);

// handles the list of files operation
int listOfFiles(int socket);

// handles the delete file operation
int deleteFile(int socket, char* filename);

// handles the add file operation
int addFile(int socket, char *path_to_file, char* newfilename);

// handles the get file operation
int getFile(int socket, char* file_name, char* path_to_save);

// notifies the server of ending the connection
int quit(int socket);

// removes '\n' from the end of a string
void removeLineBreak(char* str);


#endif //CLIENT_MAIN_H
