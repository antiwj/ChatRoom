#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <mutex>
#include <thread>
#include <ncurses.h>
#define DEFAULT_PORT    17777                   // 服务器端口
#define DEFAULT_ADDR    "127.0.0.1"     		// 服务器地址
#define BUFFER_SIZE     1024
#define INVALID_SOCKET  -1
#define CLEAR() printf("\033[2J")               // 清屏函数

static int line = 2;

class ClientMsg {
public:
	char m_UserName[20], m_Message[BUFFER_SIZE-30];
	ClientMsg() {
		m_UserName[0] = '\0';
		m_Message[0] = '\0';
	}
};

// 对收到的信息进行解码
void decode(char* buffer, ClientMsg &obj) {
	memcpy(&obj, buffer, sizeof(ClientMsg));
}

	
void chatMsg(WINDOW* chatWin, int client_socket) {
	char inBuffer[BUFFER_SIZE] = { 0 };
	ClientMsg sMsg;
	int y = 0, x = 0;
	int hei = 0, wid = 0;
	getmaxyx(chatWin, hei, wid);
	int i = 0;
	while (true) {
		// ClientMsg sMsg;
		// char inBuffer[BUFFER_SIZE] = { 0 };
		int ret = recv(client_socket, inBuffer, sizeof(inBuffer), 0);
		if (ret <= 0) {
			wprintw(chatWin, "receive message failed!");
			return;
		}
		// inBuffer[strlen(inBuffer)] = '\0';
		decode(inBuffer, sMsg);
		getyx(chatWin, y, x);
		if (y >= hei-1) {
			wscrl(chatWin, 1);
			wmove(chatWin, y-1, 2);
		}
		else
			wmove(chatWin, y, 2);
		inBuffer[0] = '\0';
		wprintw(chatWin, sMsg.m_UserName);
		wprintw(chatWin, ": ");
		wprintw(chatWin, sMsg.m_Message);
		wprintw(chatWin, "\n");
		wrefresh(chatWin);
	}
}

void sendMsg(WINDOW* sendWin, int client_socket) {
	char inBuffer[BUFFER_SIZE-30]{ 0 };
	while (true) {
		int offset = 1;
		inBuffer[0] = 0x02;
		box(sendWin, '|', '-');
		mvwprintw(sendWin, 1, 1, "Please enter: ");
		wgetstr(sendWin, inBuffer+offset);
		if (strlen(inBuffer) > 1) {
			int ret = send(client_socket, inBuffer, strlen(inBuffer), 0);
			inBuffer[0] = '\0';
			if (ret <= 0) {
				wprintw(sendWin, "发送消息失败, 网络断开.");
				break;
			}
		}
		wrefresh(sendWin);
		werase(sendWin);
	}
}

void uiInit() {
	CLEAR();
	std::cout << "欢迎进入聊天室" << std::endl;
	std::cout << "输入昵称：";
}

std::string login(int client_socket) {
	uiInit();
	char nickName[20] = { 0 };
	nickName[0] = 0x01;
	std::cin >> (nickName+1);
	int ret = send(client_socket, nickName, strlen(nickName), 0);
	return std::string(nickName+1);
}

int main() {
	// 1.创建socket
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);            // 流式协议 SOCK_STREAM 帧式协议 SOCK_DGRAM
	if (INVALID_SOCKET == client_socket) {
		std::cout << "create client socket failed!" << std::endl;
		return -1;
	}

	// 2.将socket绑定到端口号
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(DEFAULT_PORT);
	sock_addr.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);

	// 3.连接服务器
	int ret = connect(client_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	if (INVALID_SOCKET == ret) {
		std::cout << "connect server failed!" << std::endl;
		return -1;
	}

	std::string username = login(client_socket);
	username = "Hi! " + username;

	// 初始化聊天窗口
	int winHLineNumber, winWLineNumber;
	WINDOW* titleWin;
	WINDOW* sendWin;
	WINDOW* chatWin;

	initscr();
	getmaxyx(stdscr, winHLineNumber, winWLineNumber);

	char label[winWLineNumber];
	for (int i = 0; i < winWLineNumber-1; i++) {
		label[i] = '-';
	}

	titleWin = subwin(stdscr, 3, winWLineNumber - 3, 0, 0);
	chatWin = subwin(stdscr, (winHLineNumber / 3 * 2) - 2, winWLineNumber - 3, 3, 0);
	sendWin = subwin(stdscr, (winHLineNumber / 3) - 1, winWLineNumber - 3, (winHLineNumber / 3 * 2) + 1, 0);
	mvwprintw(titleWin, 2, 0, label);
	mvwprintw(titleWin, 1,1, username.c_str());
	refresh();
	scrollok(chatWin, TRUE);
	setscrreg(2,18);
	curs_set(0);


	std::thread recvMsg(chatMsg, chatWin, client_socket);
	std::thread enterMsg(sendMsg, sendWin, client_socket);

	recvMsg.join();
	enterMsg.join();

	close(client_socket);
}
