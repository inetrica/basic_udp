#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "util.c"

#define MAX_LEN 50010
#define NANO 1000000000

/*
 * if user runs program without specifying all args, print msg
 */
void usage() {
    fprintf(stderr, "Usage: blastee -p <port> -c <echo>\n");
    exit(1);
}

/*
 * use to track metadata on data packets received
 */
struct summary {
    uint num;               //number of packets received
    uint totalBytes;        //number of bytes received
};

/*
 * get/process args from cmd line when program is run
 */
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

//returns 0 if Data packet, 1 if End packet, -1 if something else
void decodePrint(char packet[], int port, char ipaddr[], struct timespec ts){
    //0 = data
    //1 = seq no
    //5 = len
    //9 = data
    char type;
    uint seq, len;
    char *data = NULL;
    type = packet[0];  //first byte in packet specifies type of packet

    memcpy(&seq, packet + 1, sizeof(seq)); //4 bytes at index 1 is sequence number
    memcpy(&len, packet + 5, sizeof(len)); //4 bytes at index 5 is length of packet

    data = (packet + 9); //bytes from index 9 is data

    //get time info for formatting
    struct tm * tmf = localtime(&ts.tv_sec);
    int millisec = ts.tv_nsec/1000000;

    fprintf(stdout, "ip addr = %s\nport = %d\nlen = %u\nseq no = %u\ntime: ", ipaddr, port, len, seq);
    //print time
    fprintf(stdout, "%02d:%02d:%02d.%03d\n", tmf->tm_hour, tmf->tm_min, tmf->tm_sec, millisec);
    fprintf(stdout, "data: ");
    //print first 4 data chars
    int i;
    for(i = 0; i < 4; i++){
        fprintf(stdout, "%c", packet[9+i]);
    } 
    fprintf(stdout, "\n\n");

}

/*
 * tike timespec struct and return time in terms of seconds
 */
double
timeInSec(struct timespec ts){
    double secn = ts.tv_nsec/(NANO*1.0);
    return ts.tv_sec + secn;
}

/*
 * print a summary of data received
 */
void 
printSummary(struct summary s, struct timespec ts){
    double duration = timeInSec(ts);
    double avgpps = s.num/duration;
    double avgbps = s.totalBytes/duration;
    fprintf(stdout, "%u packets\n%u bytes\n%G Packets/sec\n%G Bytes/sec\nduration: %G\n", s.num, s.totalBytes, avgpps, avgbps, duration);
}


int main(int argc, char *argv[]){

    //port number, echo bool, socket s
    int port, echo;
    int s;
    char ipaddr[INET_ADDRSTRLEN];

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

    //determine time spent receiving
    int startedReceiving = 0;
    struct timespec first;
    struct timespec last;

    struct timeval to; //timeval for socket timeout on select 
    fd_set sockets;


    getargs(&port,&echo, argc, argv);   

    //create socket
    if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        err("Error creating socket\n");
    }


    //bind socket
    this_addr.sin_family = AF_INET;
    this_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this_addr.sin_port = htons(port);

    if(bind(s, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0){
        err("Error binding socket\n");
    }

    struct summary summ;
    summ.num = 0;
    summ.totalBytes = 0;

    int sel; //return value from select()
    while(1){
        //reset timeout to 5 seconds
        to.tv_sec = 5;
        to.tv_usec = 0;

        if(!startedReceiving){
            to.tv_sec = 120; //2 minute timeout if we havent started receiving yet
        }

        FD_ZERO(&sockets);
        FD_SET(s, &sockets);

        sel = select(FD_SETSIZE, &sockets, NULL, NULL, &to);
        if(sel == 0){//timed out
            err("Timed out\n");
        }


        rec_size = recvfrom(s, buffer, MAX_LEN, 0, 
                (struct sockaddr *) &that_addr, &sockadd_sz);
        clock_gettime(CLOCK_REALTIME, &last);
        
        if(rec_size > 0){
            if(buffer[0] == 'D'){
                if(startedReceiving == 0){
                    first = last;
                    startedReceiving = 1;
                }
                summ.num++;
                summ.totalBytes += rec_size;

                buffer[rec_size] = '\0';//append null


                /*
                 * find ip addr of sender
                 */
                if(inet_ntop(AF_INET, &(that_addr.sin_addr), ipaddr, INET_ADDRSTRLEN) == NULL){
                    fprintf(stdout, "error determining ip addr for this packet");
                }

                //print packet info, data
                decodePrint(buffer, port, ipaddr, last);

                if(echo){ //send a response, simply change packet type
                    buffer[0] = 'C';

                    if(sendto(s, buffer, rec_size, 0, 
                        (struct sockaddr *) &that_addr, sockadd_sz) < 0){
                        fprintf(stdout, "error sending packet\n");   
                    }
                }   
            } else if(buffer[0] == 'E'){//sender done sending
                break;
            } else err("invalid packet type\n");
            
        }
    }


    clock_gettime(CLOCK_REALTIME, &last);
    struct timespec recvTime = subtract(first, last);

    printSummary(summ, recvTime);


    exit(0);

}
