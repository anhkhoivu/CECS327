/**
    C++ client example using sockets.
    This programs can be compiled in linux and with minor modification in 
	   mac (mainly on the name of the headers)
    Windows requires extra lines of code and different headers
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
...
WSADATA wsaData;
iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
...
*/
#include <iostream>    //cout
#include <string>
#include <stdio.h> //printf
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_LENGTH 2048
#define WAITING_TIME 1000

/**
*   Creates a connection to the server via a socket and returns that socket
*
*   @param host The ip address or host name of the server
*   @param port The port used to access the server
*/
int create_connection(std::string host, int port)
{
    int s;
    struct sockaddr_in saddr;
    //Sets sin_zero to 0
    memset(&saddr,0, sizeof(sockaddr));
    //Returns a socket descriptor
    s = socket(AF_INET,SOCK_STREAM,0);
    //Is either PF_INET or AF_INET
    saddr.sin_family=AF_INET;
    //host to network short: Changes host byte order to network byte order and stores to sin_port
    saddr.sin_port= htons(port);
    //Each num holds a portion of the ip address
    int a1,a2,a3,a4;
    //Stores the ip into each variable, returns the number of arguments successfully filled
    if (sscanf(host.c_str(), "%d.%d.%d.%d", &a1, &a2, &a3, &a4 ) == 4)
    {
        std::cout << "by ip";
        //Stores an integer value suitable for use as an Internet address
        saddr.sin_addr.s_addr =  inet_addr(host.c_str());
    }
    else {
    	//If the host string was not split and stored successfully, then it is not an ip addr
        std::cout << "by name";
        hostent *record = gethostbyname(host.c_str());
        in_addr *addressptr = (in_addr *)record->h_addr;
        saddr.sin_addr = *addressptr;
    }
    //connects the socket descriptor to the remote server we want
    if(connect(s,(struct sockaddr *)&saddr,sizeof(struct sockaddr))==-1)
    {
        perror("connection fail");
        exit(1);
        return -1;
    }
    return s;
}
/**
*   Sends data to the specified socket
*
*   @param sock The socket / remote server we want to send data to
*   @param message The message to be sent
*/
int request(int sock, std::string message)
{
    return send(sock, message.c_str(), message.size(), 0);
}
/**
*   Gets the servers response from the indicated socket
*
*   @param s The socket to be read from
*/
std::string reply(int s)
{
    std::string strReply;
    int count;
    char buffer[BUFFER_LENGTH+1];
    
    usleep(WAITING_TIME);
    //Continously receive data until 
    do {
    	//Stores the length of the message into count, message is stored in buffer
        count = recv(s, buffer, BUFFER_LENGTH, 0);
        //Add terminte char to the end of buffer message
        buffer[count] = '\0';
        //Store buffer data into strReply
        strReply += buffer;
    }while (count ==  BUFFER_LENGTH);
    return strReply;
}
/**
*   Requests a reply from the server after sending a message
*
*   @param s The sockete / remote server being connected to
*   @param message The message to be sent
*/
std::string request_reply(int s, std::string message)
{
	//If the message is sent successfully, receive server response
	if (request(s, message) > 0)
    {
    	return reply(s);
	}
	return "";
}

int cleanString(std::string reply)
{
	
}


int main(int argc , char *argv[])
{
    int sockpi;
    std::string strReply;
    
    //TODO  arg[1] can be a dns or an IP address.
    if (argc > 2)
        sockpi = create_connection(argv[1], atoi(argv[2]));
    if (argc == 2)
        sockpi = create_connection(argv[1], 21);
    else
        sockpi = create_connection("130.179.16.134", 21);
    strReply = reply(sockpi);
    std::cout << strReply  << std::endl;
    
    
    strReply = request_reply(sockpi, "USER anonymous\r\n");
    //TODO parse srtReply to obtain the status. 
	// Let the system act according to the status and display
    // friendly message to the user 
	// You can see the ouput using std::cout << strReply  << std::endl;
    
    std::cout << strReply << std::endl;


    strReply = request_reply(sockpi, "PASS asa@asas.com\r\n");
        
    //TODO implement PASV, LIST, RETR. 
    // Hint: implement a function that set the SP in passive mode and accept commands.

    std::cout << strReply << std::endl;

    strReply = request_reply(sockpi, "PASV\r\n");

    std::cout << strReply << std::endl;

    
    
    usleep(500);
    strReply = request_reply(sockpi, "QUIT\r\n");
    std::cout << strReply << std::endl;

    return 0;
}
