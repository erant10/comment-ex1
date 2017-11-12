// handles loading the users and passwords from the users_file

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int argc , char *argv[])
{
    //open users file
    FILE* fid;
    fid = fopen(argv[1],"r");

    if(fid==NULL) {

    }

    char* ClientPath;
    char ClientName[15];
    char password[15];


    //parse every row in the file
    while(fscanf(fid, "%s	%s", ClientName, password)!=EOF)
    {

        strcpy(ClientPath,argv[2]);
        strcat(ClientPath,"/");
        strcat(ClientPath,ClientName);

        //create a directory for each user
        struct stat st = {0};

        if (stat(ClientPath, &st) == -1) { //if this directory doesn't exist already
            mkdir(ClientPath, 0700);
        }

        //save the password in the directory
        strcat(ClientPath,"/pass.txt");
        FILE* fpass = fopen(ClientPath,"w");
        if(fpass==NULL)
        {

        }
        fprintf(fpass, "%s\n", password);
        fclose(fpass);

    }
    fclose(fid);
}