//#include <stdio.h>  
//#include<WS2tcpip.h>
//#include <Winsock2.h>
//#include <stdlib.h>
//#include <iostream>
//
//#pragma comment(lib,"WS2_32.lib")  
//
//#define MAXLEN 3 //数组长度
//int recvpacket[MAXLEN];
//int countdata();
//void main()
//{
//	//socket版本
//	WORD wVersion;
//	WSADATA wsaData;
//	int err;
//	wVersion = MAKEWORD(1, 1);
//	err = WSAStartup(wVersion, &wsaData);
//	if (err != 0)
//	{
//		return;
//	}
//	if ((LOBYTE(wsaData.wVersion) != 1) || (HIBYTE(wsaData.wVersion) != 1))
//	{
//		WSACleanup();
//		return;
//	}
//	//建立socket
//	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
//	SOCKADDR_IN addSrv;
//	addSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//	addSrv.sin_family = AF_INET;
//	addSrv.sin_port = htons(7777);
//	bind(sockSrv, (SOCKADDR*)&addSrv, sizeof(SOCKADDR));//绑定
//	listen(sockSrv, 5);//监听
//	int len = sizeof(SOCKADDR);
//	SOCKADDR_IN addClient;	
//	SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addClient, &len);
//	char sendBuf[100];
//	sprintf(sendBuf, "succed pt...");
//	send(sockConn, sendBuf, strlen(sendBuf) + 1, 0);//发送欢迎消息
//	while (1)
//	{
//		recv(sockConn, (char *)&recvpacket, sizeof(recvpacket), 0);//接收数组
//		int myresult = countdata();//计算数组     
//		send(sockConn, (char *)&myresult, sizeof(myresult), 0);//发送结果
//	}
//	closesocket(sockConn);
//
//}
////计算
//int countdata()
//{
//	int s = 0;
//	for (int i = 0; i<MAXLEN; i++)
//	{
//		std::cout << recvpacket[i] << ", ";
//		s = s + recvpacket[i];
//	}
//	std::cout << std::endl;
//	return s;
//}
#include <stdio.h>
#include <string>
#include <iostream>
#include <winsock.h>
using namespace std;
//创建新的套接字之前需要调用一个引入Ws2_32.dll库的函数,否则服务器和客户端连接不上
#pragma comment(lib,"ws2_32.lib")
using namespace std;
void main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)     //进行WinSocket的初始化
	{
		printf("Can't initiates windows socket!Program stop.\n");//初始化失败返回-1
	}
	/*err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		return;
	}*/

	if (LOBYTE(wsaData.wVersion) != 1 ||
		HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return;
	}
	SOCKADDR_IN sin, saClient;                          //设置两个地址，sin用来绑定

	SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0);
	//BOOL fBroadcast = TRUE;
	//setsockopt(sockClient, SOL_SOCKET, SO_BROADCAST, (CHAR *)&fBroadcast, sizeof(BOOL));
	//inet_pton(AF_INET, "127.0.0.1", (void*)&addrSrv.sin_addr.S_un.S_addr);
	sin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);// htonl(INADDR_BROADCAST);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(7001);
	if (bind(sockClient, (SOCKADDR FAR *)&sin, sizeof(sin)) != 0)
	{
		printf("Can't bind socket to local port!Program stop.\n");//初始化失败返回-1
	}
#define MAXLEN 8
	int sendBuff[MAXLEN] = { 11, 22, 55, 12, 1, 34, 35, 0xb2 };
	int ncount = 0;
	while (true)
	{
	//	sprintf(sendBuff, "Message %d is: ok", ncount++);    //将ncount的值放入字符串senBuff中
		//**********************  第三步使用sendto函数进行通信    *************************// 
	//	sendto(sockClient, (char *)&sendBuff, MAXLEN*4/*lstrlen((char *)&sendBuff)*//*sizeof(udpPack)*/, 0, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR_IN));

		int nSize = sizeof(SOCKADDR_IN);
		int recvLen = recvfrom(sockClient, (char *)&sendBuff, MAXLEN*4, 0,
			(SOCKADDR FAR *)&saClient, &nSize);
		for (size_t i = 0; i < recvLen/4; i++)
		{
			printf("%d ", sendBuff[i]);
		}
		printf("\n");

	}

	closesocket(sockClient);
	WSACleanup();
}
