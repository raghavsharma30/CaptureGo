#include "network.h"
#include<iostream>
#include<cstring>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
using namespace std;
Network::Network() {
	initialize();
}

Network::~Network(){
	cleanup();	
}

void Network::initialize(){
	
}
void Network::cleanup(){
	
}

int Network::createServerSocket(int port){
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSocket < 0){
		cerr << "Socket creation failed\n";
		return -1;
	}
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);
	if(bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0){
		cerr<<"bind failed\n";
		close(serverSocket);
		return -1;
	}
	if(listen(serverSocket,2)<0){
		cerr << "listen failed\n";
		return -1;
		
	}
	return serverSocket;
}
int Network::acceptClent(int serverSocket){
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int clientSocket = accept(serverSocket, (structsockaddr*)&clientAddr, &addrlen);
	if(clientSocket<0){
		cerr<<"accept failed\n";
		return -1;
	}
	return clientSocket;
}
int Network::createClientSocket(const string& serverIp, int port){
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket<0){
		cerr<<"socket creation failed\n";
		return -1;
	}
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);
	if(connect(cleintSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0){
		cerr<<"connection fsiled\n";
		close(clientSocket);
		return -1;
	}
	return clientSocket;
}
bool Network::sendData(int socket, const String& data){
	int sent = send (socket, data.c_str(), data.size(), 0);
	return sent >= 0;
}
string Network::recieveData(int socket){
	char buffer[1024] = {0};
	int received = recv(socket, buffer, sizeof(buffer), 0)	
	if(received <= 0) return "";
	return string(buffer, received);
}
void Network::closeSocket(int socket){
	close(socket);
}
