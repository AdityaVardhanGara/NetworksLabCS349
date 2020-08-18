// gcc server.c -o server
// ./server <port>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <fcntl.h>


 
/*for getting file size using stat()*/
#include <sys/stat.h>
 
/*for sendfile()*/
#include <sys/sendfile.h>

int main(int argc, char *argv[])
{
    
    int listensocket = 0, connsocket = 0,k;
    struct sockaddr_in ServAdrss , clnt; 

    char Send_buff[1025];
    time_t ticks; 

    listensocket = socket(AF_INET, SOCK_STREAM, 0);         // create socket 
    
    memset(&ServAdrss, '0', sizeof(ServAdrss));
    memset(Send_buff, '0', sizeof(Send_buff)); 

    ServAdrss.sin_family = AF_INET;
    ServAdrss.sin_addr.s_addr = htonl(INADDR_ANY);              // ip address
    ServAdrss.sin_port = htons(atoi(argv[1]));                      // port which in input 

    k = bind(listensocket, (struct sockaddr*)&ServAdrss, sizeof(ServAdrss));        


    k = listen(listensocket, 10); 

    

    connsocket = accept(listensocket, (struct sockaddr*)NULL, NULL);        // accept the connection of clnt 
    // printf("connected to the clnt\n");
    char buf[100],cmdln[5],File_Name[20],ext[20],lscmdln[20];        // defining variables 
    int size,i,filehandle;
    struct stat obj;
    int already_exists = 0;
    int choice_overwrite = 1;
    char *pos;
    // while loop start for all opreations 
    while(1)                    
    {   
        
        recv(connsocket,buf,100,0);
        sscanf(buf,"%s",cmdln);               // get cmdln with file option 
//----------- for put cmdln -------------------------//

        if(!strcmp(cmdln,"put"))
        {
            int c = 0, len;
            char *f;
            
            sscanf(buf+strlen(cmdln), "%s", File_Name);        // store File_Name in var 
            i = 1;
            // check file already exits or not 

            if( access( File_Name, F_OK ) != -1 )
            {
                already_exists = 1;
                send(connsocket,&already_exists,sizeof(int),0);              // exits 
            } 
            else 
            {
                already_exists = 0;
                 send(connsocket,&already_exists,sizeof(int),0);             // not exits 
            }
            recv(connsocket,&choice_overwrite,sizeof(int),0);               // recv overwrite choice 

            // case of overwrite 
            if(already_exists==1 && choice_overwrite == 1)
            {
                filehandle = open(File_Name, O_WRONLY | O_CREAT | O_TRUNC, 644);         // clear all the file data 
                recv(connsocket, &size, sizeof(int), 0);
                f = malloc(size);
                recv(connsocket, f, size, 0);               // recv full file data 
                c = write(filehandle, f, size);             // write data in file 
                close(filehandle);              // close file 
                send(connsocket, &c, sizeof(int), 0);           // send status 

            }
            else if(already_exists == 0 && choice_overwrite == 1)            // creating the new file 
            {
                filehandle = open(File_Name, O_CREAT | O_EXCL | O_WRONLY, 0666);   // open file 
                recv(connsocket, &size, sizeof(int), 0);
                f = malloc(size);
                recv(connsocket, f, size, 0);
                c = write(filehandle, f, size);
                close(filehandle);
                send(connsocket, &c, sizeof(int), 0);
            }
            
            
        } // ending of put option

//------------------get option ----------------------------//
        else if(!strcmp(cmdln,"get"))
        {
            sscanf(buf, "%s%s", File_Name, File_Name);
            stat(File_Name, &obj);
            filehandle = open(File_Name, O_RDONLY);          // open file with read only option 
            size = obj.st_size;
            if(filehandle == -1)
                 size = 0;
            send(connsocket, &size, sizeof(int), 0);       // sending the size of file 
            if(size==0)
                continue;
            recv(connsocket,&choice_overwrite,sizeof(int),0);   // recv over write choice 
            if(size && choice_overwrite == 1)
                sendfile(connsocket, filehandle, NULL, size);       // sending the file 
      
        }// ending the get option 

//--------------------quit cmdln----------------------------------------//
        else if(!strcmp(cmdln, "quit"))
        {
            printf("FTP server quitting..\n");
            i = 1;
            send(connsocket, &i, sizeof(int), 0);           // closing the server 
            exit(0);
        }//ending of quit option 
//--------------------close client----------------------------------------//
        else if(!strcmp(cmdln, "close"))
        {
            printf("FTP client quitting..\n");
            i = 1;
            send(connsocket, &i, sizeof(int), 0);           // closing the server 
            k = bind(listensocket, (struct sockaddr*)&ServAdrss, sizeof(ServAdrss));        
    k = listen(listensocket, 10); 
    connsocket = accept(listensocket, (struct sockaddr*)NULL, NULL); continue;
        }//ending of quit option 

//---------------------mget option ---------------------------------//
        else if(!strcmp(cmdln,"mget"))
        {
            sscanf(buf,"%s%s",ext,ext);         // get the extension 
            printf("%s\n",ext );
            strcpy(lscmdln,"ls *.");
            strcat(lscmdln,ext);
            strcat(lscmdln,"> filelist.txt");         // run the cmdln and store the filelist in filelist.txt 
            system(lscmdln);

            char *line = NULL;
            size_t len = 0;
            ssize_t read;

            FILE *fp = fopen("filelist.txt","r");           // open the list of files 
            int num_lines = 0;
            int ch;
            while(!feof(fp))                    // count the number of files
            {
                ch = fgetc(fp);
                if(ch == '\n')
                {
                    num_lines++;
                }
            }// end of while 

            // printf("%d\n",num_lines );
            fclose(fp);                 // closing 
            fp = fopen("filelist.txt","r");                 // reopen the file list 
            send(connsocket,&num_lines,sizeof(int),0);              // sending the number of files 

            while((read=getline(&line,&len,fp)) != -1)              // sending all files in while loop 
            {
                if ((pos=strchr(line, '\n')) != NULL)
                    *pos = '\0';
                strcpy(File_Name,line);
                send(connsocket,File_Name,20,0);                 // sending the cmdln 
                stat(line, &obj);
                filehandle = open(line, O_RDONLY);              // open the file only read choice 
                size = obj.st_size;
                if(filehandle == -1)
                     size = 0;
                send(connsocket, &size, sizeof(int), 0);                // send the size 
                recv(connsocket,&choice_overwrite,sizeof(int),0);           // recv overwrite choice 
                if(size && choice_overwrite == 1)
                    sendfile(connsocket, filehandle, NULL, size);               // finally send the file 

            }// end of while loop

            fclose(fp);
            remove("filelist.txt"); 

        }// end of mget option 

    }// end of outer while loop 
}// end of main function 
