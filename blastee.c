#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void usage() {
    fprintf(stderr, "Usage: blastee -p <port> -c <echo>\n");
    exit(1);
}

void invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
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

    int port, echo;
    getargs(&port,&echo, argc, argv);   

    fprintf(stdout, "port = %d\necho = %d\n",
            port, echo);

    exit(0);

}
