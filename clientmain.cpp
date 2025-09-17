#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
//#define DEBUG
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "protocol.h"
#include <errno.h>

// Included to get the support library
#include <calcLib.h>

int client_calc(const char* src)
{
  char operation[10];
  int n1;
  int n2;
  int result;

  sscanf(src, "%s %d %d", operation, &n1, &n2);

  if(strcmp(operation, "add") == 0)
  {
    result = n1 + n2;
  }
  else if(strcmp(operation, "sub") == 0)
  {
    result = n1 - n2;
  }
  else if(strcmp(operation, "mul") == 0)
  {
    result = n1 * n2;
  }
  else if(strcmp(operation, "div") == 0)
  {
    result = n1 / n2;
  }

  printf("ASSIGNMENT: %s", src);
  return result;
}

uint32_t client_calc(uint32_t operation, uint32_t n1, uint32_t n2)
{
  if(operation == 1)
  {
    printf("ASSIGNMENT: add %d %d\n", n1, n2);
    return n1 + n2;
  }
  else if(operation == 2)
  {
    printf("ASSIGNMENT: sub %d %d\n", n1, n2);
    return n1 - n2;
  }
  else if(operation == 3)
  {
    printf("ASSIGNMENT: mul %d %d\n", n1, n2);
    return n1 * n2;
  }
  else if(operation == 4)
  {
    printf("ASSIGNMENT: div %d %d\n", n1, n2);
    return n1 / n2;
  }
  else
  {
    printf("ERROR: Incorrect use of client_calc");
    return -1;
  }
  
}

ssize_t send_helper(int sockfd, const char* send_buffer)
{
  ssize_t bytes_sent = send(sockfd, send_buffer, strlen(send_buffer), 0);

  #ifdef DEBUG
  printf("\nBytes sent: %ld\n", bytes_sent);
  printf("CLIENT SENT:\n%s\n", send_buffer);
  #endif

  return bytes_sent;
}

ssize_t recv_helper(int sockfd, char* recv_buffer, size_t bufsize)
{
  ssize_t bytes_recieved = recv(sockfd, recv_buffer, bufsize - 1, 0);
  if(bytes_recieved > 0)
  {
    recv_buffer[bytes_recieved] = '\0';
  }

  #ifdef DEBUG
  printf("\nBytes recieved: %ld\n", bytes_recieved);
  printf("SERVER RESPONSE:\n%s", recv_buffer);
  #endif
  
  return bytes_recieved;
}

void case_tcp_text(int sockfd)
{
  char recv_buffer[1024];
  ssize_t bytes_recieved = recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
  if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }

  if(strstr(recv_buffer, "TEXT TCP 1.1") == NULL)
  {
    printf("ERROR: MISSMATCH PROTOCOL\n");
    return;
  }

  char send_buffer[] = "TEXT TCP 1.1 OK\n";
  send_helper(sockfd, send_buffer);
  bytes_recieved = recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
  if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }

  char send_buffer2[1024];
  int result = client_calc(recv_buffer);
  sprintf(send_buffer2, "%d", result);
  strcat(send_buffer2, "\n");
  send_helper(sockfd, send_buffer2);

  bytes_recieved = recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
  if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }
  printf("%s (myresult=%d)\n", recv_buffer, result);
}

void case_tcp_binary(int sockfd)
{
  char recv_buffer[1024];
  ssize_t bytes_recieved = recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
  if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }

  if(strstr(recv_buffer, "BINARY TCP 1.1") == NULL)
  {
    printf("ERROR: MISSMATCH PROTOCOL\n");
    return;
  }

  char send_buffer[] = "BINARY TCP 1.1 OK\n";
  ssize_t bytes_sent = send_helper(sockfd, send_buffer);

  calcMessage msg;
  calcProtocol pro;
  bytes_recieved = recv(sockfd, &pro, sizeof(pro), 0);

  #ifdef DEBUG
  printf("\nBytes sent: %ld\n", bytes_sent);
  printf("bytes recieved %ld\n", bytes_recieved);
  #endif  

  if(bytes_recieved == 26)
  {
    pro.arith = ntohl(pro.arith);
    pro.inValue1 = ntohl(pro.inValue1);
    pro.inValue2 = ntohl(pro.inValue2);
    pro.inResult = ntohl(pro.inResult);

    #ifdef DEBUG
    printf("airth: %d\n", pro.arith);
    printf("inValue1: %d\n", pro.inValue1);
    printf("inValue2: %d\n", pro.inValue2);
    printf("inResult: %d\n", pro.inResult);
    #endif 
  }
  else if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }
  else
  {
    printf("NOT OK\n");
    printf("ERROR: WRONG SIZE OR INCORRECT PROTOCOL\n");
    return;
  }

  //prepare for sending back
  uint32_t result = client_calc(pro.arith, pro.inValue1, pro.inValue2);
  pro.type = htons(2);
  pro.inResult = htonl(result);
  pro.arith = htonl(pro.arith);
  pro.inValue1 = htonl(pro.inValue1);
  pro.inValue2 = htonl(pro.inValue2);

  ssize_t bytes_sent2 = send(sockfd, &pro, sizeof(pro), 0);
  ssize_t bytes_recieved2 = recv(sockfd, &pro, sizeof(pro), 0);

  #ifdef DEBUG
  printf("\nBytes sent: %ld\n", bytes_sent2);
  printf("bytes recieved %ld\n", bytes_recieved2);
  #endif

  if(bytes_recieved2 == 12)
  {
    memcpy(&msg, &pro, 12);
    if(ntohl(msg.message) == 1)
    {
      printf("OK\n (myresult=%d)\n", result);
    }
    else
    {
      printf("NOT OK\n");
    }
  }
  else if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }
  else
  {
    printf("NOT OK\n");
    printf("ERROR: WRONG SIZE OR INCORRECT PROTOCOL\n");
    return;
  }
}

void case_udp_text(int sockfd)
{
  char recv_buffer[1024];
  char send_buffer[] = "TEXT UDP 1.1\n";
  send_helper(sockfd, send_buffer);

  ssize_t bytes_recieved = recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
  if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }

  char send_buffer2[1024];
  int result = client_calc(recv_buffer);
  sprintf(send_buffer2, "%d", result);
  strcat(send_buffer2, "\n");
  send_helper(sockfd, send_buffer2);

  bytes_recieved = recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
  if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }
  printf("%s (myresult=%d)\n", recv_buffer, result);
}

void case_udp_binary(int sockfd)
{
  calcMessage msg;
  msg.type = htons(22);
  msg.message = htonl(0);
  msg.protocol = htons(17);
  msg.major_version = htons(1);
  msg.minor_version = htons(1);

  calcProtocol pro;
  ssize_t bytes_sent = send(sockfd, &msg, sizeof(msg), 0);
  ssize_t bytes_recieved = recv(sockfd, &pro, sizeof(pro), 0);

  #ifdef DEBUG
  printf("\nBytes sent: %ld\n", bytes_sent);
  printf("bytes recieved %ld\n", bytes_recieved);
  #endif  

  if(bytes_recieved == 26)
  {
    pro.arith = ntohl(pro.arith);
    pro.inValue1 = ntohl(pro.inValue1);
    pro.inValue2 = ntohl(pro.inValue2);
    pro.inResult = ntohl(pro.inResult);

    #ifdef DEBUG
    printf("airth: %d\n", pro.arith);
    printf("inValue1: %d\n", pro.inValue1);
    printf("inValue2: %d\n", pro.inValue2);
    printf("inResult: %d\n", pro.inResult);
    #endif  
  }
  else if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }
  else
  {
    printf("NOT OK\n");
    printf("ERROR: WRONG SIZE OR INCORRECT PROTOCOL\n");
    return;
  }

  //prepare for sending back
  uint32_t result = client_calc(pro.arith, pro.inValue1, pro.inValue2);
  pro.type = htons(2);
  pro.inResult = htonl(result);
  pro.arith = htonl(pro.arith);
  pro.inValue1 = htonl(pro.inValue1);
  pro.inValue2 = htonl(pro.inValue2);

  ssize_t bytes_sent2 = send(sockfd, &pro, sizeof(pro), 0);
  ssize_t bytes_recieved2 = recv(sockfd, &pro, sizeof(pro), 0);

  #ifdef DEBUG
  printf("\nBytes sent: %ld\n", bytes_sent2);
  printf("bytes recieved %ld\n", bytes_recieved2);
  #endif

  if(bytes_recieved2 == 12)
  {
    memcpy(&msg, &pro, 12);
    if(ntohl(msg.message) == 1)
    {
      printf("OK\n (myresult=%d)\n", result);
    }
    else
    {
      printf("NOT OK\n");
    }
  }
  else if(bytes_recieved == -1)
  {
    printf("ERROR: MESSAGE LOST (TIMEOUT)\n");
    return;
  }
  else
  {
    printf("NOT OK\n");
    printf("ERROR: WRONG SIZE OR INCORRECT PROTOCOL\n");
    return;
  }
}

int main(int argc, char *argv[]){
  
  
  
  if (argc < 2) {
    fprintf(stderr, "Usage: %s protocol://server:port/path.\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  

    
  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
    char protocolstring[6], hoststring[2000],portstring[6], pathstring[7];

    char *input = argv[1];
    
    /* Some error checks on string before processing */
    // Check for more than two consequtive slashes '///'.

    if (strstr(input, "///") != NULL ){
      printf("Invalid format: %s.\n", input);
      return 1;
    }
    

    // Find the position of "://"
    char *proto_end = strstr(input, "://");
    if (!proto_end) {
        printf("Invalid format: missing '://'\n");
        return 1;
    }

     // Extract protocol
    size_t proto_len = proto_end - input;
    if (proto_len >= sizeof(protocolstring)) {
        fprintf(stderr, "Error: Protocol string too long\n");
        return 1;
    }
    
    // Copy protocol
    strncpy(protocolstring, input, proto_end - input);
    protocolstring[proto_end - input] = '\0';

    // Move past "://"
    char *host_start = proto_end + 3;

    // Find the position of ":"
    char *port_start = strchr(host_start, ':');
    if (!port_start || port_start == host_start) {
	      printf("Error: Port is missing or ':' is misplaced\n");
        return 1;
    }

    // Extract host
    size_t host_len = port_start - host_start;
    if (host_len >= sizeof(hoststring)) {
        printf("Error: Host string too long\n");
        return 1;
    }
    
    // Copy host
    strncpy(hoststring, host_start, port_start - host_start);
    hoststring[port_start - host_start] = '\0';

        // Find '/' which starts the path
    char *path_start = strchr(host_start, '/');
    if (!path_start || *(path_start + 1) == '\0') {
        fprintf(stderr, "Error: Path is missing or invalid\n");
        return 1;
    }

    // Extract path
    if (strlen(path_start + 1) >= sizeof(pathstring)) {
        fprintf(stderr, "Error: Path string too long\n");
        return 1;
    }
    strcpy(pathstring, path_start + 1);

    // Extract port


    size_t port_len = path_start - port_start - 1;
    if (port_len >= sizeof(portstring)) {
        fprintf(stderr, "Error: Port string too long\n");
        return 1;
    }
    strncpy(portstring, port_start + 1, port_len);
    portstring[port_len] = '\0';

    // Validate port is numeric
    for (size_t i = 0; i < strlen(portstring); ++i) {
        if (portstring[i] < '0' || portstring[i] > '9') {
            fprintf(stderr, "Error: Port must be numeric\n");
            return 1;
        }
    }


    
    char *protocol, *Desthost, *Destport, *Destpath;
    protocol=protocolstring;
    Desthost=hoststring;
    Destport=portstring;
    Destpath=pathstring;
      
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 


    
  /* Do magic */
  int port=atoi(Destport);
  if (port < 1 or port >65535) {
    printf("Error: Port is out of server scope.\n");
    if ( port > 65535 ) {
      printf("Error: Port is not a valid UDP or TCP port.\n");
    }
    return 1;
  }
#ifdef DEBUG 
  printf("Protocol: %s Host %s, port = %d and path = %s.\n",protocol, Desthost,port, Destpath);
#endif

  //variable that will be filled with data
  struct addrinfo *res, *pInfo;

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  if(strcmp(protocol, "tcp") == 0 || strcmp(protocol, "TCP") == 0)
  {
    #ifdef DEBUG
    printf("\nHELLO from TCP\n");
    #endif
    hints.ai_socktype = SOCK_STREAM;
  }
  else if(strcmp(protocol, "udp") == 0 || strcmp(protocol, "UDP") == 0)
  {
    #ifdef DEBUG
    printf("\nHELLO from UDP\n");
    #endif
    hints.ai_socktype = SOCK_DGRAM;
  }

  int addrinfo_status = getaddrinfo(Desthost, Destport, &hints, &res);
  if(addrinfo_status != 0)
  {
    printf("\nERROR: getaddrinfo Failed\n");
    printf("Returned: %d\n", addrinfo_status);
    return EXIT_FAILURE;
  }

  #ifdef DEBUG
  printf("getaddrinfo Succeded!\n");
  #endif

  int sockfd;
  for(pInfo = res; pInfo != NULL; pInfo = pInfo->ai_next)
  {
    sockfd = socket(pInfo->ai_family, pInfo->ai_socktype, pInfo->ai_protocol);
    if(sockfd != -1)
    {
      break;
    }
    #ifdef DEBUG
    printf("Socket retry");
    #endif
  }

  if(sockfd == -1)
  {
    printf("\nERROR: Socket creation Failed\n");
    printf("Returned: %d\n", sockfd);
    return EXIT_FAILURE;
  }

  //Set options for socket
  timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  #ifdef DEBUG
  printf("Socket creation Succeded!\n");
  #endif

  int connect_status = connect(sockfd, pInfo->ai_addr, pInfo->ai_addrlen);
  if(connect_status != 0)
  {
    printf("\nERROR: RESOLVE ISSUE\n");
    printf("Returned: %d\n", connect_status);
    //perror("connect");
    return EXIT_FAILURE;
  }

  #ifdef DEBUG
  printf("Connection Succeded!\n");
  #endif

  if(strcmp(protocol, "tcp") == 0 || strcmp(protocol, "TCP") == 0)
  {
    if(strcmp(pathstring, "text") == 0 || strcmp(pathstring, "TEXT") == 0)
    {
      case_tcp_text(sockfd);
    }
    else if(strcmp(pathstring, "binary") == 0 || strcmp(pathstring, "BINARY") == 0)
    {
      case_tcp_binary(sockfd);
    }
  }
  else if(strcmp(protocol, "udp") == 0 || strcmp(protocol, "UDP") == 0)
  {
    if(strcmp(pathstring, "text") == 0 || strcmp(pathstring, "TEXT") == 0)
    {
      case_udp_text(sockfd);
    }
    else if(strcmp(pathstring, "binary") == 0 || strcmp(pathstring, "BINARY") == 0)
    {
      case_udp_binary(sockfd);
    }
  }

  close(sockfd);
  freeaddrinfo(res);
  return 0;
}
