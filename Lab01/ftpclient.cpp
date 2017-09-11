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

#include <vector>
#include <fstream>

#define BUFFER_LENGTH 2048
#define WAITING_TIME 100000

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
        //std::cout << "by ip" << std::endl;
        //Stores an integer value suitable for use as an Internet address
        saddr.sin_addr.s_addr =  inet_addr(host.c_str());
    }
    else {
    	//If the host string was not split and stored successfully, then it is not an ip addr
        //std::cout << "by name";
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
    //Continously receive data until no more data is available
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
/**
*   Gets an ip from the server reply
*
*   @param sting The server reply after entering passive mode
*/
std::string getIp(std::string reply)
{
	int choice = std::stoi(reply.substr(0, 3));
	std::vector<std::string> items;
	std::string ip;
	switch(choice) {
		case 227:
			ip = reply.substr(26);
			//std::cout << ip << std::endl;
			ip = ip.substr(1, (ip.length() - 4));
			break;
		default:
			ip = "x";
			break;
	}
	return ip;
}
/**
*   Gets the port from an ip address
*
*   @param string The ip address
*   @return The port in integer form
*/
int getPort(std::string ip) {
	int a1,a2,a3,a4,p1,p2,newPort;
    //Stores the ip into each variable, returns the number of arguments successfully filled
    if (sscanf(ip.c_str(), "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2 ) == 6)
    {
    	return (p1 << 8) | p2;
    }
	return -1;
}
/**
*   Creates a new connection and enters passive mode
*
*   @param sockpi The socket descriptor for the main server
*   @return A socket descriptor of the file transfer server
*/
int enterPassiveMode(int sockpi) {
	std::string reply, newIp;
	int passiveSocket, newPort;
	//Enter passive mode
	reply = request_reply(sockpi, "PASV\r\n");
	//Get the ip from reply
	newIp = getIp(reply);
	//Get the port from the ip
	newPort = getPort(newIp);
	std::cout << newPort << std::endl;
	//If we couldn't get the port, return -1
	if(newPort == -1) return -1;
	//std::cout << "[227] Connection created successfully " << std::endl;
	//Create a new socket descriptor
	passiveSocket = create_connection("130.179.16.134", newPort);
	return passiveSocket;
}
/**
*   Command used to show the user the commands they can use
*
*   @return String of all commands and descriptions
*/
std::string helpCommand() {
	std::string strReply = "LIST - Lists the current directory.\n";
	strReply += "GET filename - Gets the filename from the current directory and saves to your computer.\n";
	strReply += "SHOW filename - Shows the filename contents in terminal.\n";
	strReply += "INTO directory - Changes directory to specified directory.\n";
	strReply += "QUIT - Quits the program.\n";
	return strReply;
}
/**
*   Command used to list everything in the current directory
*
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return String depending on whether the directory was listed or not
*/
std::string listCommand(int sockpi, int sockpasv) {
	std::string strReply = request_reply(sockpi, "LIST\r\n");
	if(strReply.substr(0, 3) == "150") {
		strReply = reply(sockpasv);
		return strReply;
	}
	else {
		return "Unable to list files";
	}
}
/**
*   Writes to a file
*
*   @param name The name of the file to write to
*   @param content The content to write to the file
*/
void writeToFile(std::string name, std::string content) {
	std::ofstream file;
	file.open(name);
	file << content << "\n";
	file.close();
}
/**
*   Command used to download the file chosen by the user
*
*   @param name The name of the file
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return String depending on whether the file was written successfuly
*/
std::string getCommand(std::string name, int sockpi, int sockpasv) {
	std::string strReply = request_reply(sockpi, "RETR " + name + "\r\n");
	if(strReply.substr(0, 3) == "150") {
		strReply = reply(sockpasv);
		writeToFile(name, strReply);
		return "Written to file!";
	}
	else {
		return "Unable to get file";
	}
}
/**
*   Command used to show the user the chosen file in the terminal
*
*   @param name The name of the file
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return String depending on whether the file was retrieved successfully
*/
std::string showCommand(std::string name, int sockpi, int sockpasv) {
	std::string strReply = request_reply(sockpi, "RETR " + name + "\r\n");
	if(strReply.substr(0, 3) == "150") {
		strReply = reply(sockpasv);
		return strReply;
	}
	else {
		return "Unable to get file";
	}
}
/**
*   Command used to change directory
*
*   @param name The name of the directory
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return String depending on whether the directory change was successful or not
*/
std::string intoCommand(std::string name, int sockpi, int sockpasv) {
	std::string strReply = request_reply(sockpi, "CWD " + name + "\r\n");
	if(strReply.substr(0, 3) == "250") {
		return strReply.substr(5);
	}
	else {
		return "Could not change into new directory";
	}
}
/**
*   Takes a one word user command and handles the phrase
*
*   @param command The word typed by the user
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return True or false depending on the user command chosen
*/
bool parseSingleCommand(std::string command, int sockpi, int sockpasv) {
	std::string strReply = "";
	//Used for getting reply from main server
	bool helper = true;
	if(command == "QUIT" || command == "6") {
		std::cout << "Exiting..." << std::endl;
		close(sockpasv);
		return false;
	} else if(command == "LIST" || command == "2") {
		strReply = listCommand(sockpi, sockpasv);
	} else if(command == "HELP" || command == "1") {
		strReply = helpCommand();
		helper = false;
	} else {
		strReply = "Command " + command + " not recognized, please try again.";
	}
	std::cout << strReply << std::endl;
	close(sockpasv);
	if(helper) strReply = reply(sockpi);
	return true;
}
/**
*   Takes a two word user command and handles the phrase
*
*   @param command1 The first word in the user command
*   @param command2 The second word in the user command
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return True when the command has been properly handled
*/
bool parseDoubleCommand(std::string command1, std::string command2, int sockpi, int sockpasv) {
	std::string strReply = "";
	if(command1 == "GET" || command1 == "3") {
		strReply = getCommand(command2, sockpi, sockpasv);
	} else if(command1 == "SHOW" || command1 == "4") {
		strReply = showCommand(command2, sockpi, sockpasv);
	} else if(command1 == "INTO" || command1 == "5") {
		strReply = intoCommand(command2, sockpi, sockpasv);
		close(sockpasv);
		return true;
	} else {
		strReply = "Command " + command1 + " " + command2 + " not recognized, please try again.";
	}
	std::cout << "\n" << strReply << std::endl;
	close(sockpasv);
	strReply = reply(sockpi);
	return true;
}
/**
*   Gets and handles user input
*   
*   @param sockpi The socket descriptor for the main server
*   @param sockpasv The socket descriptor for the file transfer server in passive mode
*   @return True or false depending on whether the user wants to inputt more commands
*/
bool handleUserInput(int sockpi, int sockpasv) {
	std::string userInput,
				comm[3];
	char* pch;
	const char* delim = " \r\n";
	int numOfCommands = 0;
	std::getline(std::cin, userInput);
	pch = strtok((char*) userInput.c_str(), delim);
	while(pch != NULL) {
		comm[numOfCommands] = std::string(pch);
		pch = strtok(NULL, delim);
		numOfCommands++;
	}
	if(numOfCommands == 1) {
		return parseSingleCommand(comm[0], sockpi, sockpasv);
	} else if(numOfCommands == 2) {
		return parseDoubleCommand(comm[0], comm[1], sockpi, sockpasv);
	} else {
		std::cout << "This is not a valid command, please try again" << std::endl;
		return true;
	}
}
/**
*   Displays a menu to the user
*/
void showMenu() {
	std::cout << "|----------------------|" << std::endl;
	std::cout << "|-------Commands-------|" << std::endl;
	std::cout << "|----------------------|" << std::endl << std::endl;
	std::cout << "1. HELP\t\t4. SHOW" << std::endl;
	std::cout << "2. LIST\t\t5. INTO" << std::endl;
	std::cout << "3. GET\t\t6. QUIT" << std::endl;
}
/**
*   Closes the socket currently connected to
*
*   @param sockpi The socket descriptor
*/
void quitCommand(int sockpi) {
	std::string strReply = request_reply(sockpi, "QUIT\r\n");
    if(strReply.substr(0, 3) == "221") {
		std::cout << "Goodbye." << std::endl;
	} else {
		std::cout << strReply.c_str() << std::endl;
	}
    strReply.clear();
}

int main(int argc , char *argv[])
{
    int sockpi, sockpasv;
    std::string strReply;
    
    //TODO  arg[1] can be a dns or an IP address.
    if (argc > 2)
        sockpi = create_connection(argv[1], atoi(argv[2]));
    if (argc == 2)
        sockpi = create_connection(argv[1], 21);
    else
        sockpi = create_connection("130.179.16.134", 21);
    strReply = reply(sockpi);
    
    strReply.clear();
    strReply = request_reply(sockpi, "USER anonymous\r\n");
    //TODO parse srtReply to obtain the status. 
	// Let the system act according to the status and display
    // friendly message to the user 
	// You can see the ouput using std::cout << strReply  << std::endl;
	if(strReply.substr(0, 3) == "331") {
		std::cout << "USERNAME INCORRECT, EXITING..." << std::endl;
	}

    strReply = request_reply(sockpi, "PASS asa@asas.com\r\n");
    //TODO implement PASV, LIST, RETR. 
    // Hint: implement a function that set the SP in passive mode and accept commands.
    if(strReply.substr(0, 3) != "230") {
		std::cout << "PASSWORD INCORRECT, EXITING..." << std::endl;
	}

	//PASSIVE MODE
	bool isRunning = true;
	while(isRunning) {
		showMenu();
		sockpasv = enterPassiveMode(sockpi);
		if(sockpasv == -1) break;
		isRunning = handleUserInput(sockpi, sockpasv);
	}
	quitCommand(sockpi);    
    return 0;
}