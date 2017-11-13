#include "client_main.h"
#include "network_main.h"

#define LOGIN_SUCCESS "connected"

int login(int socket) {
    char* serverReply = NULL;
    char user[MAX_USERNAME];
    char password[MAX_PASSWORD];

    // try to log in
    fgets(user, MAX_USERNAME + 6, stdin);			// get username from user
    removeLineBreak(user);
    fgets(password, MAX_PASSWORD + 10, stdin);		// get password from user
    removeLineBreak(password);
    if (sendMessage(socket, user) != 0) { 			// send username to server
        return 1;
    }
    if (sendMessage(socket, password) != 0) { 		// send password to server
        return 1;
    }
    if (recvMessage(socket, &serverReply) != 0) {	// receive log in confirmation
        return 1;
    }
    if (strcmp(serverReply, LOGIN_SUCCESS) != 0) {	// not authorized
        return 1;
    }
    printf("%s\n", serverReply);
    return 0;
}

int listOfFiles(int socket) {
    char* serverReply = NULL;
    char opCode = LIST_OF_FILES_OPCODE;

    if (sendMessage(socket, &opCode) != 0) {		// send opcode '1'
        return 1;
    }
    if (recvMessage(socket, &serverReply) != 0) {	// receive list of files from server
        return 1;
    }
    printf(serverReply);
    return 0;
}

int deleteFile(int socket, char* filename) {
    char* serverReply = NULL;
    char opCode = DELETE_FILE_OPCODE;

    if (sendMessage(socket, &opCode) != 0) {		// send opcode '2'
        return 1;
    }

    if (sendMessage(socket, filename) != 0) {		// send file name
        return 1;
    }
    if (recvMessage(socket, &serverReply) != 0) {		// receive response
        return 1;
    }
    printf(serverReply);
    return 0;
}

int addFile(int socket, char *path_to_file, char* newfilename) {
    char* serverReply = NULL;
    char opCode = ADDFILE_OPCODE;
    FILE *fp;

    if (sendMessage(socket, &opCode) != 0) {		// send opcode '3'
        return 1;
    }

    fp = fopen(path_to_file, "rb");
    if (fp == NULL) {
        printf("error opening file %s.\n", path_to_file);
        return 1;
    }

    // read file content into buffer
    char *content_buffer = malloc(MAX_FILESIZE + 1);
    fread(content_buffer, MAX_FILESIZE, 1, fp);
    content_buffer[content_buffer] = 0;

    // send file name
    if (sendMessage(socket, newfilename) != 0) {
        free(content_buffer);
        return 1;
    }
    // send file content
    if (sendMessage(socket, content_buffer) != 0) {
        free(content_buffer);
        return 1;
    }
    if (fclose(fp) == 0) {
        printf("error closing file %s.\n", path_to_file);
        return 1;
    }
    return 0;
}

int getFile(int socket, char* file_name, char* path_to_save) {
    char* serverReply = NULL;
    char opCode = GET_FILE_OPCODE;

    if (sendMessage(socket, &opCode) != 0) {		// send opcode '4'
        return 1;
    }

    char file_name[MAX_FILENAME], file_content[MAX_FILESIZE];
    int file_size;
    FILE *fd;

    if (recvMessage(socket, &file_name) != 0) {		// get file name
        return 1;
    };

    if (recvMessage(socket, &file_content) != 0) {		// get file content
        return 1;
    };
    file_size = atoi(file_content);

    fd = fopen(path_to_save, "w");
    if (fd == NULL) {
        printf(stderr, "Failed to open file: %s\n", strerror(errno));
        return 1;
    }
    fwrite(file_content, sizeof(char), file_size, fd);
    if (fclose(fp) == 0) {
        printf("error closing file %s.\n", path_to_file);
        return 1;
    }

    return 0;
}

int quit(int socket) {
    char opCode = QUIT_OPCODE;
    if (sendMessage(socket, &opCode) != 0) {		// send opcode '5'
        return 1;
    }
    return 0;
}

void removeLineBreak(char* str) {

}