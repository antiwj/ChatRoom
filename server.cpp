#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <mutex>
#include <vector>
#include <thread>
#include <algorithm>

#define DEFAULT_PORT 17777 // 服务器端口
#define DEFAULT_ADDR "0.0.0.0" // 服务器地址
#define BUFFER_SIZE 1024
#define INVALID_SOCKET -1

std::mutex mtx;
std::vector<int> clients;

class ClientInfo {
public:
	char m_UserName[20];
	in_addr m_IpAddr;
	// unsigned int m_Port;
	ClientInfo() {
		m_UserName[0] = '\0';
	}
};

class ClientMsg {
public:
	char m_UserName[20], m_Message[BUFFER_SIZE-30];
	ClientMsg() {
        m_UserName[0] = '\0';
		m_Message[0] = '\0';
    }
};

void encode(char* buffer, ClientMsg &obj) {
	memcpy(buffer, &obj, sizeof(ClientMsg));
}
void decode(char* buffer, ClientMsg &obj) {
	memcpy(&obj, buffer, sizeof(ClientMsg));
}

void broadcastMsg(ClientMsg &message) {
	char buffer[BUFFER_SIZE]{ 0 };
	std::lock_guard<std::mutex> lock(mtx);
	encode(buffer, message);
	ClientMsg tmp;
	for (int client_socket : clients) {
		int ret = send(client_socket, buffer, sizeof(buffer), 0);
		if (ret <= 0){	
			std::cout << client_socket << "broad发送失败." << std::endl;
		}
	}
}

void ServerRecv(int client_socket, in_addr ipAddr) {
	ClientInfo cInfo;
	cInfo.m_IpAddr = ipAddr;
	int offset = 1;
	char messageType;
	std::string username;
	std::string broadMsg;
	while (true) {
		ClientMsg cMsg;
		std::string broadMsg;
		char buffer[BUFFER_SIZE] = { 0 };
		int ret = recv(client_socket, buffer, sizeof(buffer), 0);
		if (ret <= 0) {
			std::cout << "接收消息失败, 断开链接" << std::endl;
			break;
		}
		messageType = buffer[0];

		if (messageType == 0x01) {
			username = std::string(buffer+offset);
			std::memcpy(cInfo.m_UserName, username.c_str(), username.length());
			cInfo.m_UserName[username.length()] = '\0';
			broadMsg = "User[";
			broadMsg.append(username);
			broadMsg.append("] Enter this Chat Room.");
			std::strcpy(cMsg.m_UserName, "Server");
			std::memcpy(cMsg.m_Message, broadMsg.c_str(), broadMsg.length());
			cMsg.m_Message[broadMsg.length()] = '\0';
			broadcastMsg(cMsg);
		} else if (messageType == 0x02) {
			std::string message(buffer+offset);
			std::memcpy(cMsg.m_UserName, cInfo.m_UserName, strlen(cInfo.m_UserName));
			cMsg.m_UserName[strlen(cInfo.m_UserName)] = '\0';
			std::memcpy(cMsg.m_Message, message.c_str(), message.length());
			cMsg.m_Message[message.length()] = '\0';
			broadcastMsg(cMsg);
			std::cout << "接收消息成功: " << cInfo.m_UserName << ": " << message << std::endl;
		} else {
			std::cout << "消息类型错误" << std::endl;
		}

		messageType = '\0';
	}

	std::lock_guard<std::mutex> lock(mtx);
	clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
	close(client_socket);
}

int main() {
	// 1.创建socket
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);		// 流式协议 SOCK_STREAM 帧式协议 SOCK_DGRAM
	if (INVALID_SOCKET == listen_socket) {
		std::cout << "create listen socket failed!" << std::endl;
		return -1;
	}

	// 2.将socket绑定到端口号
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(DEFAULT_PORT);
	sock_addr.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);

	int ret = bind(listen_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	if (INVALID_SOCKET == ret) {
		std::cout << "bind socket failed!" << std::endl;
		return -1;
	}

	// 3.监听此socket
	ret = listen(listen_socket, 10);
	if (INVALID_SOCKET == ret) {
		std::cout << "start listen failed!" << std::endl;
		return -1;
	}

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_size = sizeof(client_addr);
		int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_addr_size);
		if (client_socket == INVALID_SOCKET) {
			std::cout << "连接客户端失败" << std::endl;
			close(client_socket);
			continue;
		}

		clients.push_back(client_socket);
		in_addr ipAddr = client_addr.sin_addr;
		// unsigned int m_Port = ntohs(client_addr.sin_port);
		std::cout << "客户端连接: " << inet_ntoa(client_addr.sin_addr) << ": ";	
		std::cout << clients[0] << std::endl;

		std::thread server_recv(ServerRecv, client_socket, ipAddr);
		server_recv.detach();
	}

	close(listen_socket);
}
