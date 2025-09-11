#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
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
  
  /*#ifdef DEBUG 
  printf("src is: %s\n", src);
  printf("operation is: %s\n", operation);
  printf("first number is: %d\n", n1);
  printf("second number is: %d\n", n2);
  printf("result is: %d\n", result);
  #endif*/

  return result;
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
  recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));

  char send_buffer[] = "TEXT TCP 1.1 OK\n";
  send_helper(sockfd, send_buffer);

  recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));

  char send_buffer2[1024];
  sprintf(send_buffer2, "%d", client_calc(recv_buffer));
  strcat(send_buffer2, "\n");
  send_helper(sockfd, send_buffer2);

  recv_helper(sockfd, recv_buffer, sizeof(recv_buffer));
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
  if (port < 1000 or port >65535) {
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
    printf("\nERROR: getaddrinfo Failed!\n");
    printf("Returned: %d\n", addrinfo_status);
    return -1;
  }

  #ifdef DEBUG
  printf("getaddrinfo Succeded!\n");
  #endif

  int sockfd;
  for(pInfo = res; pInfo != NULL; pInfo = pInfo->ai_next)
  {
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd != -1)
    {
      break;
    }
    printf("Socket retry");
  }

  if(sockfd == -1)
  {
    printf("\nSocket creation error\n");
    printf("Returned: %d\n", sockfd);
    return -1;
  }

  #ifdef DEBUG
  printf("Socket creation Succeded!\n");
  #endif

  int connect_status = connect(sockfd, pInfo->ai_addr, pInfo->ai_addrlen);
  if(connect_status != 0)
  {
    printf("\nConnection Failed\n");
    printf("Returned: %d\n", connect_status);
    //perror("connect");
    return -1;
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
    }
  }
  else if(strcmp(protocol, "udp") == 0 || strcmp(protocol, "UDP") == 0)
  {
    if(strcmp(pathstring, "text") == 0 || strcmp(pathstring, "TEXT") == 0)
    {
    }
    else if(strcmp(pathstring, "binary") == 0 || strcmp(pathstring, "BINARY") == 0)
    {
    }
  }

  close(sockfd);
  freeaddrinfo(res);
  return 0;
}
