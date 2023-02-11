/*
Auther: John Blue
Time: 2023/2
Platform: VS2017
Object: A demonstration of Client and Server socket.
        In main function, Client and Server socket will be called and created.
        In Client Class, SendMsgToServer and ReceiveMsgFromServer will be run as threads, and they will keep sending or receiving maessage from Server.
        In Server Class, ServerCommunity will become a thread, waiting to connect with Client.
        In each socket created inside ServerCommunity, SendMsgToClient and ReceiveClient will keep sending or receiving maessage from Client.
Server: new socket >> set socket >> bind() >> listen() >> clientsocket = accept()
Server: new socket >> set socket >> connect()
Disclaimer:
	Unfortunately, it has a big problem that ::accept() function cannot find the with client to communicate due to unknow issue.
	Overall, this program is not wholely testified.
*/

// basic
#include <iostream>

// thread libarary
#include <chrono>
#include <thread>

// winsocket
#include <WS2tcpip.h>
#pragma comment (lib, "ws2_32.lib")
//
//#include <WinSock2.h>
//#pragma comment (lib, "Ws2_32.lib")
//
//#include <winsock.h>
//#pragma comment (lib, "wsock32.lib")

using namespace std;

class Server
{
public:
    // Start the Server
	void Commence(int myPort, int allowNum)
	{
	    // setting of Server
		ServerEnd(myPort, allowNum);
        
        // staring the server thread
		thread th_s(&Server::ServerCommunity, this);
		th_s.detach();// must .detach() or .join()

        // pthread version ... complicated
	// pthread also have confliction problem with wincocket.h (https://www.bilibili.com/read/cv8833392?from=search)
		//pthread th_s;
		//pthread_create(&th_s, NULL, Server::ServerCommunity, NULL);
		//pthread_join(th_s, NULL);
	}
	
	~Server() {
	    // Close listening socket
		closesocket(listening);
	}

private:
    // socket
	SOCKET listening;

    // setting
	void ServerEnd(int myPort, int allowNum)
	{
		// Initialize winsocket
		WSADATA wsDaata;
		WORD ver = MAKEWORD(2, 2);
		int wsOk = WSAStartup(ver, &wsDaata);
		if (wsOk != 0) {
			cerr << "Cannot Initialize winsocket!" << endl;
			return;
		}
		
		// Create a socket
		listening = socket(AF_INET, SOCK_STREAM, 0);// Stream 因為我們要使用TCP協議，需使用流式的Socket
		if (listening == INVALID_SOCKET) {
			cerr << "Cannott create a socket!" << endl;
			return;
		}
		
		// Bind the socket to an ip address and port
		// sockaddr_in 網路參數struct
		//
		// 通訊家族
		//AF_INET：使用 IPv4
		//AF_INET6：使用 IPv6

		//SOCK_STREAM：使用 TCP 協議
		//SOCK_DGRAM：使用 UDP 協議
		//
		//IPPROTO_TCP：使用 TCP
		//IPPROTO_UDP：使用 UDP
		//
		// 利用IPAddress.Any方法得到本機的ＩＰ，同時聽網內/網外IP
		//
		// 完整設定介紹
		//https://xyz.cinc.biz/2014/02/c-socket-server-client.html
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(myPort);
		hint.sin_addr.S_un.S_addr = INADDR_ANY;//https://www.cnblogs.com/tuyile006/p/10737066.html
		//cout << "Server port: " << ntohs(hint.sin_port) << endl;

		// why ::bind ?
		//https://blog.csdn.net/qq_32563917/article/details/85156757?spm=1035.2023.3001.6557&utm_medium=distribute.pc_relevant_bbs_down_v2.none-task-blog-2~default~ESQUERY~Rate-4-85156757-bbs-370051845.pc_relevant_bbs_down_cate&depth_1-utm_source=distribute.pc_relevant_bbs_down_v2.none-task-blog-2~default~ESQUERY~Rate-4-85156757-bbs-370051845.pc_relevant_bbs_down_cate
		::bind(listening, (sockaddr*)&hint, sizeof(hint));
		if (listening == SOCKET_ERROR) {
			cerr << "Cannot bind socket! Error" << endl;
			closesocket(listening);
			return;
		}

		// Tell winsocket the socket is for listening
		//listen(listening, allowNum);
		listen(listening, SOMAXCONN);// SOMAXCONN 表示為系統最大可監聽數量
		if (listening == SOCKET_ERROR) {
			cerr << "Can't listen on socket! Error" << endl;
			closesocket(listening);
			return;
		}

		cout << "Listening...\n";
	}

	void ServerCommunity()
	{
		while (true)
		{
			// Wait for connection
			sockaddr_in client;
			int clientSize = sizeof(client);
			SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);// accept(): blocking function
			if (clientSocket == INVALID_SOCKET) {
				cout << "Cannot accept a client!" << endl;
				closesocket(clientSocket);
			}

            // accepted Client info
			char host[NI_MAXHOST];       // Client's remote name
			char service[NI_MAXHOST];    // Service (i.e. port) the client is connect on

			ZeroMemory(host, NI_MAXHOST);// Window version, same as memset(host, 0, NI_MAXHOST);
			ZeroMemory(service, NI_MAXHOST);
			
			if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXHOST, 0) == 0) {
				cout << host << " (up) connected on port " << service << endl;
			}
			else {
				inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
				cout << host << " (down) connected on port " << ntohs(client.sin_port) << endl;
			}

            // Threading of recieving and sending
			thread rec(&Server::ReceiveClient, this, clientSocket);
			rec.detach();

			thread sed(&Server::SendMsgToClient, this, clientSocket);
			sed.detach();
		}
	}

	void SendMsgToClient(SOCKET clientSocket)
	{
	    // the message waiting to send to client
		char buf[] = "Echo from Server";

        // keep sending
		while (true) {
		    // check socket null?
			if (clientSocket == 0) {
				cout << "Client socket null" << endl;
				break;
			}

			// Echo message back to client
			send(clientSocket, buf, (int)strlen(buf), 0);

			// Echo message back to client
			//cin.get(buf, ...);
			//send(clientSocket, buf, (int)strlen(buf), 0);
		}

		// Close the socket
		closesocket(clientSocket);
	}

	void ReceiveClient(SOCKET clientSocket)
	{
	    // buffer
		char buf[4096];
		
		// keep recieving
		while (true) {
			ZeroMemory(buf, 4096);

			// Wait for client to snd data ... a blocking function
			int check = recv(clientSocket, buf, 4096, 0);

            // error checking
			if (check == SOCKET_ERROR) {
				cerr << "Error in recv(). Quitting" << endl;
				break;
			}
            // null checking
			if (check == 0) {
				cout << "Client disconnected" << endl;
				return;
			}

			// show buf (message)
			cout << "Client: " << buf << endl;
		}

		// Close the socket
		closesocket(clientSocket);
	}

};



class Client {
public:
    // Start the Client
	void Commence(string ip, int port)
	{
	    // Setting and starting the client thread
		ClientSocket(ip, port);
	}
	
	~Client() {
        // Close the socket
    	closesocket(sock);
	}

private:
	SOCKET sock;
	
	void ClientSocket(string ip, int port)
	{
		// Initialize winsocket
		WSADATA wsDaata;
		WORD ver = MAKEWORD(2, 2);
		int wsOk = WSAStartup(ver, &wsDaata);
		if (wsOk != 0) {
			cerr << "Cannot Initialize winsocket! (in Client)" << endl;
			return;
		}

		// Create a socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) {
			cerr << "Cannot create a socket! (in Client)" << endl;
			return;
		}

		// Bind the socket to an ip address and port
		sockaddr_in hint;
		//hint.sin_family = AF_INET;
		hint.sin_port = htons(port);
		inet_pton(AF_INET, ip.c_str(), &hint.sin_addr);
		//hint.sin_addr.S_un.S_addr = inet_addr(ip.c_str());// too old ... compile error

		// Connect
		connect(sock, (SOCKADDR*)&hint, sizeof(hint));
		if (sock == 0) {
		    cout << "Connection Fail! (in Client)" << endl;
		}

        // Threading of recieving and sending
		thread rec(&Client::ReceiveServer, this);
		rec.detach();

		thread sed(&Client::SendMsgToServer, this);
		sed.detach();
	}
	
	void SendMsgToServer()
	{
	    // the message waiting to send to Server
		char buf[] = "Hello from Client";

        // keep sending
		while (true) {
		    // null check
			if (sock == 0) {
				cout << "Sock is null (in Client)" << endl;
				return;
			}

			// Send message
			send(sock, buf, (int)strlen(buf), 0);
		}
	}

	void ReceiveServer()
	{
	    // buffer
		char buf[4096];

        // keep recieving
		while (true)
		{
		    // null check
			if (sock == 0) {
				cout << "Sock is null (in Client)" << endl;
				return;
			}
			
			// Wait for client to snd data
			ZeroMemory(buf, 4096);
            int check = recv(sock, buf, 4096, 0);
            
            // error check
			if (check == SOCKET_ERROR) {
				cerr << "Error in recv(). Quitting (in Client)" << endl;
				break;
			}
			// null check
			if (check == 0) {
				cout << "Server disconnected (in Client)" << endl;
				return;
			}

			// show buf (message)
			cout << "Server: " << buf << endl;
		}
	}
};



int main() {
    //
	// ip 192.168.50.126 or 127.0.0.1
	// port 60000
	//
	
	// one Server
	Server sv;
	sv.Commence(60000, 10);
	
	// one Client
	Client ct[10];
	for (unsigned int i = 0; i < 10; i++) {
		ct[i].Commence("192.168.50.126", 60000);
	}
	
	// let main thread run for a period of time
	std::this_thread::sleep_for(std::chrono::seconds(12));

    // !!! clean up
	WSACleanup();
}

