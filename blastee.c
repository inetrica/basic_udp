#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define MAX_LEN 50000

void usage() {
    fprintf(stderr, "Usage: blastee -p <port> -c <echo>\n");
    exit(1);
}

void invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
    exit(1);
}

void setup_err(char* msg){
    fprintf(stderr, msg);
    exit(1);
}

void getargs(int *port, int *echo, int argc, char *argv[]){
    int a = 0;
    int c = 0;
    while((a = getopt(argc, argv, "p:c:")) != -1){
        c++;
        switch(a){
            case 'p':
                *port = atoi(optarg);
                if(*port <= 1024 || 65536 <= *port) invalidRange("port");
                break;
            case 'c':
                *echo = atoi(optarg);
                if(*echo != 0 && *echo != 1) invalidRange("echo");
                break;
            default:
                usage();
        }
    }
    if(c != 2) usage();

}

int main(int argc, char *argv[]){

    //port number, echo bool, socket s
    int port, echo;
    int s;

    //stockaddr_in struct for bind this
    //sockaddr_in struct for receiving from remote
    //socklen_t for recvfrom()
    struct sockaddr_in this_addr;
    struct sockaddr_in that_addr;
    socklen_t sockadd_sz = sizeof(that_addr);

    //number of bytes received on return from recvfrom()
    //+ buffer holding data received
    int rec_size;
    char buffer[MAX_LEN];


    getargs(&port,&echo, argc, argv);   

    fprintf(stdout, "port = %d\necho = %d\n",
            port, echo);



    //create socket
    if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        setup_err("Error creating socket\n");
    }
    

    //bind socket
    this_addr.sin_family = AF_INET;
    this_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this_addr.sin_port = htons(port);

    if(bind(s, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0){
        setup_err("Error binding socket\n");
    }

    while(1){
        rec_size = recvfrom(s, buffer, MAX_LEN, 0, 
                (struct sockaddr *) &that_addr, &sockadd_sz);
        if(rec_size > 0){
            buffer[rec_size] = '\0';//append null
            fprintf(stdout, "received \"%s\"\n", buffer);
        }
    }


    exit(0);

}
