////////////////////////////////
// FTP TCP Server Application //
// Skyler Hughes Apr 25       //
////////////////////////////////


#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

volatile short SIGINTERRUPT_FLAG = 0;

//Handles the signal interrupt by setting global flag
void interrupt_handler(int signum){
    printf("\n[RECIEVED SIGINTERRUPT TERMINATING...]\n");
    SIGINTERRUPT_FLAG = 1;
    exit(0);
}
//Checks for signal interrupt then closes the network socket
void cleanup_parent(int *network_socket)
{
    if(SIGINTERRUPT_FLAG){
        close(*network_socket);
        exit(0);
    }
}
//*Handles the sending of a file to a given client socket
void send_client_file(int *client_socket, char *filename){

    FILE *fp = fopen(filename, "r");

    if(!fp){
        printf("File Could Not be Opened\n");

        //Sending Client Error MSG on FOPEN Fail
        char fopen_err[256] = {0};
        char *temp = "[ERR] File was found but could not be opened. ERR:FOPEN";
        strncpy(fopen_err, temp, sizeof(temp));

        if(send(*client_socket, fopen_err, sizeof(fopen_err), 0) == -1){
                perror("Send Failed\n");
        }

    }

    //Reading File line by line, sending each line to client
    char linebuf[256] = {0};

    while(fgets(linebuf, sizeof(linebuf), fp) != NULL){

        //Send Client Buffer
        if(send(*client_socket, linebuf, sizeof(linebuf), 0) == -1){
                perror("Send Failed\n");
        }
    }



    fclose(fp);
}
//Contains the main loop of the progam and all client handling
//Catches the ctrl+c sig interrupt to close the network socket
void handle_clients(int *network_socket)
{

    //Client Sockaddr information
    struct sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_addr.s_addr = INADDR_ANY; //Initialize to 0.0.0.0

    int addr_size = sizeof(struct sockaddr_in);

    for(;;){

        //SIGINT CHECK
        cleanup_parent(network_socket);

        //Create Client Socket(struct sockaddr *)&client_address
        int client_socket = accept(*network_socket, (struct sockaddr *)&client_address, &addr_size);
        if(!client_socket){
            perror("Accept Failed\n");
            break;
        }else{
            printf("Got a client: %s port: %d\n", inet_ntoa(client_address.sin_addr), (int) ntohs(client_address.sin_port));

            //Create Child
            int pid = fork();
            //Fork Failed
            if (pid == -1){
                perror("[ERROR]: Fork Failed on Client Connect");

            //Parent Actions
            }else if(pid > 0){
                signal(SIGCHLD,SIG_IGN); //To prevent zombies we ignore the SIGCHILD, which will cause the child to be deleted from the process table
                close(client_socket);    //The child now owns the socket, we dont want to keep a copy of it open to the parent
                signal(SIGINT, interrupt_handler); //Parent Catches Cntrl-C so the server can close the original network socket
                cleanup_parent(network_socket); //Check for SIGINT and close network socket
                continue;                //Continue Processing More Children
            }else if (pid == 0){

                char filename[256];
                if(recv(client_socket, &filename, sizeof(filename), 0) == -1){
                    perror("Recv failed on client connect");
                }else{
                    
                    //Checking if file exists
                    if(access(filename, F_OK) != -1){
                        send_client_file(&client_socket, filename);
                    }else{
                        //Sending File not found error
                        char file_error[256] = {0};
                        sprintf(file_error,"File: %s not found\n", filename);
                        // strncpy(file_error, temp, strlen(temp));
                        if(send(client_socket, file_error, sizeof(file_error), 0) == -1){
                            perror("Send Failed\n");
                        }
                    }            
                }
                
                //Need to Close the client socket and kill the child
                close(client_socket);
                exit(0);
            }
        }
    }
}

int main(int argc, char **argv){

    int port;
    //Handling command line arguments
    if(argc != 2){
        printf("Invalid Arguments\nUsage: tcpserver port\n");
        exit(1);
    }else{
        port = atoi(argv[1]);
    }

    printf("Starting Server...\n");

    //Creating a Socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(network_socket == -1){
        perror("Socket Failed: \n");
    }

    //Configuring resability in network socet port num
    //&(int){1} Is just handling the type casts 
    setsockopt(network_socket, SOCK_STREAM, (SO_REUSEPORT | SO_REUSEADDR), &(int){1}, sizeof(int));
    
    //Specifying an address and port using sockaddr_in struct from socket.h
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY; //Same as 0.0.0.0 for localhost

    //Bind the server to the address and port
    if(bind(network_socket, (struct sockaddr *)&server_address, sizeof(server_address))){
        perror("Bind Failed\n");
    }

    if(listen(network_socket, 5)) //Can handle 5 connections waiting to connect to this socket
    {
        perror("Listen Failed\n");
    }

    //Client Handling
    handle_clients(&network_socket);

    return 0;
}

