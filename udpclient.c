#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>



#ifndef WIN32

#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#else

#define WINVER 0x0501
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>


#endif

#include <fcntl.h>

#include <errno.h>
#include <signal.h>

#define BUFSIZE 4096

int s_send = -1;
    
unsigned char buf[BUFSIZE];
    
int main(int argc, char* argv[]) {
    if (argc>=2 && !strcmp(argv[1], "--version")) {
        printf("udpclient 0.0\n");
        return 0;
    }
    if (argc<5 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-?")) {
        fprintf (stderr, 
        "simple UDP client\n"
        "Algorithm:\n"
        "1. Read input from stdin;\n"
        "2. Send UDP packets (all the same) to destination and listen for replies;\n"
        "3. After the first reply dump it to stdout and exit.\n"
        "Buffer size (maximum input and output sizes) is %d bytes\n"
        "IPv6 is supported when you use respective send_addr\n"
        "\n"
        "Implemented by Vitaly _Vi Shukela in 2013, License=MIT\n"
        "\n"
        "Usage:\n"
        "\tudpclient count interval send_host send_port < input > output\n"
        "Example:\n"
        "\techo eth0 | udpserv 10 25000   1.2.3.4 6767 \n"
        ,BUFSIZE);
        return 1;
    }
    
    struct addrinfo hints;
    struct addrinfo *send_addr;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    
    int count = atoi(argv[1]);
    int usleep_interval = atoi(argv[2]);
    char* send_host = argv[3];
    char* send_port = argv[4];
    
    if (!strcmp(send_host, "NULL")) send_host=NULL;
    if (!strcmp(send_port, "NULL")) send_port=NULL; 
    
    
    int gai_error;
    gai_error=getaddrinfo(send_host,send_port, &hints, &send_addr);
    if (gai_error) { fprintf(stderr, "getaddrinfo 1: %s\n",gai_strerror(gai_error)); return 4; }
    if (!send_addr) { fprintf(stderr, "getaddrinfo returned no addresses\n");   return 6;  }
    if (send_addr->ai_next) {
        fprintf(stderr, "Warning: using only one of addresses retuned by getaddrinfo\n");
    }
    
    
    s_send = socket(send_addr->ai_family, send_addr->ai_socktype, send_addr->ai_protocol);
    if (s_send == -1) { perror("socket"); return 7; }
    {
        int one = 1;
        setsockopt(s_send, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        fcntl(s_send, F_SETFL, O_NONBLOCK);
    }
    if (connect(s_send, send_addr->ai_addr, send_addr->ai_addrlen)) {  perror("bind");  return 5;  }
    
    int data_size;
    data_size = fread(&buf, 1, sizeof buf, stdin);
    fclose(stdin);
    
    int i;
    for (i=0;i<count;++i) {
        int ret = send(s_send, buf, data_size, 0);
        if(ret!=data_size) perror("send");
        
        
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s_send, &rfds);
        struct timeval tv = {usleep_interval/1000000,(usleep_interval%1000000)};
                
        ret = select(s_send+1, &rfds, NULL, NULL, &tv);
        
        if (FD_ISSET(s_send, &rfds)) {
            ret = recv(s_send, buf, sizeof buf, 0);
            if(ret>=0) {
                fwrite(buf, 1, ret, stdout);
                break;
            }
        }
    }
    if(i==count)return 1;
    return 0;
}
