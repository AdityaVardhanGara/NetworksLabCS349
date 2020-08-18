// gcc client.c -o client
// ./client <ip_add> <port>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <sys/stat.h>
 
#include <sys/sendfile.h>
#include <fcntl.h>


int mysocket = 0, n = 0;
    char recieve_buff[1024];
	
    int option;
    char option_str[1000];
    char Filenm[20],buf[100],external[20],cmdln[20];
    FILE *fp;
    int HandleFile;
    struct stat obj;
    int size,status,i=1;
    char *f;
    int already_exits=0;
    int overwirte_option = 1;
    char *pos;
    int num_lines;
    struct sockaddr_in ServerAdrss; 

int main(int argc, char *argv[])
{
    
    if(argc != 3)
    {
        printf("Incorrect input \n");		// checking argument 
        return 1;
    } 

    memset(recieve_buff, '0',sizeof(recieve_buff));
    if((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");			// socket create
        return 1;
    } 

    memset(&ServerAdrss, '0', sizeof(ServerAdrss)); 			//assigning  server address

    ServerAdrss.sin_family = AF_INET;			//protocol
    ServerAdrss.sin_port = htons(atoi(argv[2])); 				// assigning port

    if(inet_pton(AF_INET, argv[1], &ServerAdrss.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(mysocket, (struct sockaddr *)&ServerAdrss, sizeof(ServerAdrss)) < 0)			// connect to the server
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 



    while(1)
    {
        printf("Enter a option:\n1- put\n2- get\n 3- mput\n 4-mget\n 5-quit both\n 6-close client\n");

        fgets(option_str,1000,stdin);
        option_str[strlen(option_str)-1] = '\0';
        if(strlen(option_str)>1)
        {
        	printf("error\n");
        	continue;
        }
        option = atoi(option_str);
        

        //--------put file in server----------------//
           if(option ==1)														
                putfn();
                
             
//----------------get file from server-------------------//
             else if(option ==2)	
                fun2();
                               
//------------------mput server ----------------------------------//
            else if(option ==3)	
                fun4();
                
//----------------mget server------------------------------------//
             else if(option ==4)	
             fun5();
//------------------quit the server-------------------------//
             else if(option ==5)	
                fun3();	// faild to quit                
               else if(option ==6)
		funclose(); 

//------------- default option --------------------//
            else
            	printf("choose the vaild option\n");
            	


    }// end of while 

    return 0;
}// end of main
int fun5()
{
   printf("enter the extension , you want to get from server:\n");				
                scanf("%s",external);									// take input the files externalension 
                strcpy(buf,"mget ");
                strcat(buf,external);
                send(mysocket,buf,100,0);									// sending buffer with option and externalension 
                recv(mysocket,&num_lines,sizeof(int),0);					// get number of files

                while(num_lines > 0)
                {


                    recv(mysocket,Filenm,20,0);							// recv file name 
                    recv(mysocket, &size, sizeof(int), 0);				// recv the size of file 
                    if(!size)
                    {
                        printf("No such file on the remote directory\n\n");				// error handling 
                        return 0;
                    }

                    if( access( Filenm, F_OK ) != -1 ) 				// checking if already exits or not 
                    {
                        already_exits = 1;
                        printf("%s file already exits in client 1. overwirte 2.NO overwirte\n",Filenm);
                        scanf("%d",&overwirte_option);					// taking overwirte option 
                    }
                    send(mysocket,&overwirte_option,sizeof(int),0);			// sending overwrite option 

                    if(already_exits==1 && overwirte_option == 1)				// option according to the option
                    {
                        HandleFile = open(Filenm, O_WRONLY | O_CREAT | O_TRUNC, 644);				// clear all the file
                        f = malloc(size);
                        recv(mysocket, f, size, 0);
                        write(HandleFile, f, size);								// send file 
                        close(HandleFile);
                    }
                    else if( already_exits ==0 && overwirte_option == 1)
                    {
                        HandleFile = open(Filenm, O_CREAT | O_EXCL | O_WRONLY, 0666);				// open new file 
                        f = malloc(size);
                        recv(mysocket, f, size, 0);
                        write(HandleFile, f, size);
                        close(HandleFile);
                    }
                    overwirte_option = 1;
                    already_exits = 0;
                    num_lines--; 
                }
}
int fun4()
{
printf("enter the extension , you want to put in server:\n");
                scanf("%s",external);									//  take the extension 
                							
                strcpy(cmdln,"ls *.");
                strcat(cmdln,external);
                strcat(cmdln," > temp.txt");						// store all file list 
                // printf("%s\n",cmdln);
                system(cmdln);


                char *line = NULL;                      //intilize file var
                size_t len =0;
                ssize_t read;
                FILE *fp = fopen("temp.txt","r");
                while ((read = getline(&line, &len, fp)) != -1)             // read input
                {   
                    if ((pos=strchr(line, '\n')) != NULL)
                        *pos = '\0';

                    HandleFile = open(line,O_RDONLY);				// open files line wise 
                    strcpy(buf,"put ");
                    strcat(buf,line);

                    send(mysocket,buf,100,0);
                    recv(mysocket,&already_exits,sizeof(int),0);
                    if(already_exits){
                        printf("%s file already exits in server 1. overwirte 2.NO overwirte\n",line); // overwrite option for that particular file
                        scanf("%d",&overwirte_option);
                    }
                    send(mysocket,&overwirte_option,sizeof(int),0);			// sending overwrite option 
                    if(overwirte_option==1)
                    {
                        stat(line, &obj);
                        size = obj.st_size;
                        send(mysocket, &size, sizeof(int), 0);
                        sendfile(mysocket, HandleFile, NULL, size);
                        recv(mysocket, &status, sizeof(int), 0);
                        if(status)											// status 
                            printf("%s stored successfully\n",line);
                        else		
                            printf("%s failed to be stored to remote machine\n",line); 
                    }
                    overwirte_option = 1;						// re-assign overwrite option 
                } // end of while 
                fclose(fp);			// close the file 
                remove("temp.txt");
}
int fun2()
{
printf("Enter Filenm to get: ");
                scanf("%s", Filenm);
                strcpy(buf, "get ");
                strcat(buf, Filenm);

                send(mysocket, buf, 100, 0);				//send the get cmdln with file name

                recv(mysocket, &size, sizeof(int), 0);
                if(!size)
                {
                    printf("No such file on the remote directory\n\n");			//file doesn't exits
                    return 0;
                }

                if( access( Filenm, F_OK ) != -1 ) 
                {
                    already_exits = 1;
                    printf("same name file already exits in client 1. overwirte 2.NO overwirte\n");		// file already exits 
                    scanf("%d",&overwirte_option);
                }
                send(mysocket,&overwirte_option,sizeof(int),0);

                if(already_exits==1 && overwirte_option == 1)
                {
                    HandleFile = open(Filenm, O_WRONLY | O_CREAT | O_TRUNC, 644);		// open file with all clear data 
                    f = malloc(size);
                    recv(mysocket, f, size, 0);
                    write(HandleFile, f, size);
                    close(HandleFile);
                }
                else if( already_exits ==0 && overwirte_option == 1)
                {
                    HandleFile = open(Filenm, O_CREAT | O_EXCL | O_WRONLY, 0666);		// open new file 
                    f = malloc(size);
                    recv(mysocket, f, size, 0);
                    write(HandleFile, f, size);
                    close(HandleFile);
                }               
}
int fun3()
{
strcpy(buf, "quit");
                send(mysocket, buf, 100, 0);					// sending quit cmdln for closing both server and client
                recv(mysocket, &status, 100, 0);
                if(status)
                {
                    printf("Server closed\nQuitting..\n");
                    exit(0);
                }
                printf("Server failed to close connection\n");	

}

int putfn()
{
printf("enter the Filenm to put in server\n");
                scanf("%s",Filenm);							// read the file name 
                if( access( Filenm, F_OK ) == -1 )
	            {
	                printf(" %s does not exits in client side \n",Filenm );
	                return 0;
	            } 
                HandleFile = open(Filenm,O_RDONLY);
                strcpy(buf,"put ");
                strcat(buf,Filenm);
                send(mysocket,buf,100,0);						// send put cmdln with Filenm
                recv(mysocket,&already_exits,sizeof(int),0);
                if(already_exits){
                 
                }
                send(mysocket,&overwirte_option,sizeof(int),0);			// sending overwrite option 
                if(overwirte_option==1)
                {
                    stat(Filenm, &obj);
                    size = obj.st_size;
                    send(mysocket, &size, sizeof(int), 0);
                    sendfile(mysocket, HandleFile, NULL, size);				// sending file 
                    recv(mysocket, &status, sizeof(int), 0);
                    if(status)
                        printf("%s File stored successfully\n",Filenm);							//status 
                    else
                        printf("%s File failed to be stored to remote machine\n" , Filenm); 
                }
	return status;
}
int funclose()
{


strcpy(buf, "close");
                send(mysocket, buf, 100, 0);					// sending quit cmdln for closing client
                recv(mysocket, &status, 100, 0);
                if(status)
                {
                    printf("client closed\nQuitting..\n");
                    exit(0);
                }
                printf("client failed to close connection\n");	

}
