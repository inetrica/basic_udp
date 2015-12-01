#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_LEN 50010
#define MIN_CHAR 33
#define MAX_CHAR 126

void 
usage() {
    fprintf(stderr, "Usage: blaster -s <hostname> -p <port> -r <rate> "
    "-n <num packets> -q <seq_no> -l <length> -c <echo>\n");
    exit(1);
}

void 
invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
    exit(1);
}

void 
err(char* msg){
    fprintf(stderr, msg);
    exit(1);
}

unsigned int 
getUint(char* option, char* arg){
    unsigned long tmp = 0;
    errno = 0;
    tmp = strtoul(arg, NULL, 10);
    if(errno != 0 || tmp > UINT_MAX){
        invalidRange(option);
    }

    return (uint) tmp;

}

void 
getargs(char **hostname, int *port, int *rate, int *num, 
        uint *seq_no, uint *length, int *echo, int argc, char *argv[]){
    int a = 0;
    int c = 0;
    while((a = getopt(argc, argv, "s:p:r:n:q:l:c:")) != -1){
        c++;
        switch(a){
            case 's':
                *hostname = strdup(optarg);
                break;
            case 'p':
                *port = atoi(optarg);
                if(*port <= 1024 || 65536 <= *port) invalidRange("port");
                break;
            case 'r':
                *rate = atoi(optarg);
                //assuming rate must be greater than 0
                if(*rate <= 0) invalidRange("rate");
                break;
            case 'n':
                *num = atoi(optarg);
                if(*num < 0) invalidRange("num");
                break;
            case 'q':
                *seq_no = getUint("seq_no", optarg);
                //if(*seq_no < 0) invalidRange("seq_no");
                break;
            case 'l':
                *length = getUint("length", optarg);
                if(MAX_LEN <= *length) invalidRange("length");
                break;
            case 'c':
                *echo = atoi(optarg);
                if(*echo != 0 && *echo != 1) invalidRange("echo");
                break;
            default:
                usage();
        }
    }
    if(c != 7) usage();

}

/*
 * calculate amount of time we'll need to sleep in order to match 'rate'
 * where rate is pkts/sec
 * */
struct timespec 
calcSleepTime(int rate){
    const uint nsps = 1000000000; //1000000000 nanoseconds per second
    struct timespec tspec;
    long nanosecSleep = nsps/rate;
    int sec = nanosecSleep/nsps;
    tspec.tv_sec = sec;
    tspec.tv_nsec = nanosecSleep%nsps;
    return tspec;
}

void 
createPacket(uint seq, uint len, char data[], int type){
    char pktType;
    int pos = 0;
    switch(type){
    case 0:
        pktType = 'D';
        break;
    case 1:
        pktType = 'E';
        break;
    case 2:
        pktType = 'C';
        break;
    default:
        err("Invalid packet type while trying to create packet");
    }

    //indicate packet type
    //fprintf(stdout, "size of pktType char = %d\n", (int) sizeof(pktType));
    *(data) = pktType;
    //fprintf(stdout, "seq %u and len %u\n", seq, len);
    
    if(sizeof(seq) != 4 || sizeof(len) != 4){
        err("sizeof(uint) is not 4?\n");
    }
    memcpy(data+1, &seq, sizeof(seq));
    memcpy(data+5, &len, sizeof(len));

    int char_diff = MAX_CHAR - MIN_CHAR;

    int i;
    pos = 9;
    for(i = 0; i < len; i++){
        int dval = (seq + i) % char_diff;
        data[pos] = (char) (MIN_CHAR + dval);
        pos++;
    }

    fprintf(stdout, "%u\n", seq);
    for(i = 9; i < 13; i++){
        fprintf(stdout, "%c", data[i]);
    }
    fprintf(stdout, "\n");

    data[pos] = '\0';
    return;

}

void
decodeEcho(char packet[]){
    //make sure packet received was an echo...
    if(packet[0] != 'C'){
        fprintf(stdout, "received a non-echo packet\n");
        return;
    }
    uint seq, len;
    char *data = NULL;
    int i = 0;
    
    memcpy(&seq, packet + 1, sizeof(seq));
    memcpy(&len, packet + 5, sizeof(len));

    data = (packet + 9);

    fprintf(stdout, "echo %u\n", seq);
    for(i = 9; i < 13; i++){
        fprintf(stdout, "%c", packet[i]);
    }
    fprintf(stdout, "\n");

}

int 
main(int argc, char *argv[]){

    //arg values
    char* hostName = NULL;
    int port, rate, numPkts, echo;
    uint seq_no, len;
    getargs(&hostName, &port, &rate, &numPkts, &seq_no, &len, &echo, argc, argv);   

    //timespec struct for nanosleep to control rate
    struct timespec ts;

    //socket vars
    //s socket, socket address structs
    int s;
    struct sockaddr_in this_addr;
    struct sockaddr_in that_addr;
    socklen_t sockadd_sz = sizeof(that_addr);

    
    //number of bytes received on return from recvfrom()
    //+ buffer holding data received
    int rec_size;
    char buffer[MAX_LEN];
    char recvBuff[MAX_LEN];

    //host entry
    struct hostent *he;


    fprintf(stdout, "hostName = %s\nport = %d\nrate = %d\nnum = %d\nseq = %u\nlen = %u\necho = %d\n",
            hostName, port, rate, numPkts, seq_no, len, echo);

    /*
     * calculate how long we need to sleep per packet in for loop
     */
    ts = calcSleepTime(rate);
    fprintf(stdout, "sleep rate is sec = %d, nano = %ld\n", (int) ts.tv_sec, ts.tv_nsec);


    /*
     * setup socket
     */
    if((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        err("error creating socket\n");       
    }

    this_addr.sin_family = AF_INET;
    this_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this_addr.sin_port = htons(0);

    if(echo && bind(s, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0){
        err("error binding socket\n");
    }

    /*
     * get host addr
     */
    he = gethostbyname(hostName);
    if(he == NULL){
        err("error finding host\n");
    }

    that_addr.sin_family = AF_INET;
    //that_addr.sin_addr = he->h_addr_list[0];
    memcpy(&(that_addr.sin_addr), *(he->h_addr_list), he->h_length);
    that_addr.sin_port = htons(port);

    

    /*
     * for each packet, send
     */
    int i, type;
    type = 0;
    for(i = 0; i < numPkts; i++){
        if(i == numPkts -1) type = 1;//end packet
        fprintf(stdout, "send a packet\n");
        //create packet
        createPacket(seq_no, len, buffer, type);
        //fprintf(stdout, "have packet\n%c %u %u %s\n", buffer[0], (uint) buffer[1], (uint) buffer[5], buffer+9);
        //send the packet

        if(sendto(s, buffer, len, 0, 
                    (struct sockaddr *) &that_addr, sockadd_sz) < 0){
            err("error sending packet\n");   
        }

        if(echo){
            rec_size = recvfrom(s, recvBuff, MAX_LEN, 0, (struct sockaddr *) &that_addr, &sockadd_sz);
            if(rec_size > 0){
                recvBuff[rec_size] = '\0';
                decodeEcho(recvBuff);
            }
        }
        //increment seq_no
        seq_no++;

        //sleep to match rate
        nanosleep(&ts, NULL);
    }
    

    exit(0);

}
