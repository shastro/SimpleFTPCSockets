////////////////////////////////
// FTP TCP Client Application //
// Skyler Hughes Apr 25       //
////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main( int argc, char **argv){


    char s_filename[256] = {0};
    char *s_ipaddr;
    char *s_port;

    int port;

    //Handle command line arguments
    if (argc != 4){
        printf("Incorrect Arguments:\nUsage:  tcpclient filename ipaddress port\n");
        exit(1);
    }else{
        strncpy(s_filename, argv[1], strlen(argv[1]));
        s_ipaddr   = argv[2];
        s_port     = argv[3];
        port = atoi(s_port);
    }

    //Creating a Socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(!network_socket){
        perror("Socket Creation Failed\n");
        exit(1);
    }

    //Specifying an address and port using sockaddr_in struct from socket.h
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; //AF_INET is ipv4
    server_address.sin_port = htons(port); //Converts port to appropriate format
    server_address.sin_addr.s_addr = inet_addr(s_ipaddr); //Same as 0.0.0.0 for localhost

    int connection_status = connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if(connection_status == -1){
        printf("There was an error making a connection to the remote socket \n\n");

    }


    //Send Filename
    if(send(network_socket, s_filename, sizeof(s_filename), 0) == -1){
        perror("Send Failed\n");
    }


    //Recieve Text Data Back
    char serv_resp[256];

    int retval;
    while (retval){
        retval = recv(network_socket, &serv_resp, sizeof(serv_resp), 0);
        
        if(retval == -1){
            perror("Peer Connection Error on Data Send\n");
            close(network_socket);
            exit(1);
        }else if(retval == 0){
            // printf("Server Finished Sending Data...Quiting\n");
            //Server finished sending data
            break;
        }else{
            printf("%s", serv_resp);
        }

    }


    close(network_socket);


    return 0;
}