// hpangolinTemplate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<winsock.h>
#include <windows.h>
#include <conio.h>
#include <process.h> /* _beginthread, _endthread */
#include <iostream>
#include <string>
#include<queue>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include "hemisphereLaserSensor.h"
#include "hpangolinViewer.h"
#include <pangolin/pangolin.h>
#include "pointCloudFile.h"
#include "sps2volume.h"

#pragma comment(lib,"ws2_32.lib") 
//using namespace std;
//全局变量
//
HANDLE hComm; //句柄
OVERLAPPED m_ov; //用于异步输入输出的信息的结构体
COMSTAT comstat;  //结构体
DWORD m_dwCommEvents;

//struct pointStruct{
//	int theta_h;
//	int theta_v;
//	int len;
//};

std::queue<struct pointStruct>m_scanBuff;

//#define OFFLINE
#define HORI_PLUS 24000
#define VERT_PLUS 8000

struct wareHouseParam
{
	int heightCylinder;
	int diameterCylinder;

	int heightCone;
	int diameterOutlet;
	int unit;
};

struct commParam
{
	int port;
	int baud;
};

struct detectParam
{
	int HAngleResolution;
};

struct userParam
{
	bool hideExe;
	bool saveCloud2File;
};
struct wareHouseParam m_wareHouseParam;
struct commParam m_commParam;
struct detectParam m_detectParam;
struct userParam m_userParam;


std::ofstream out_;// ("out.txt");
char timePath[254];

bool m_ackOk_arduino_pc = 0;
bool m_ackOk_pc_pc = 0;
//int m_switch_on_arduino_pc = 0;

int m_pc2pcState = -1;  //0-首次启动   3-扫描完成   4-扫描出错
int scanProgress = 0;// 扫描进度
spMap g_map;



void initPhase();
bool loadConfig();





float plus2radialH = 8.0*atan(1.0f) / HORI_PLUS;
float plus2radialV = 8.0*atan(1.0f) / VERT_PLUS;

void change(float &h, float &v)
{
	h *= plus2radialH;
	v *= plus2radialV;
}




void initPhase()
{
	m_wareHouseParam.heightCylinder = 0;
	m_wareHouseParam.diameterCylinder = 0;
	m_wareHouseParam.heightCone = 0;
	m_wareHouseParam.diameterOutlet = 0;
	m_wareHouseParam.unit = 0;

	m_commParam.port = 8;
	m_commParam.baud = 115200;

	m_detectParam.HAngleResolution = 600;

	m_userParam.hideExe = 0;
	m_userParam.saveCloud2File = 0;

	loadConfig();


	m_HAngleResolution = m_detectParam.HAngleResolution;


	if (m_userParam.hideExe)
	{
		/*HWND hwnd;
		if (hwnd = ::FindWindow("ptLaserScan", NULL))
		::ShowWindow(hwnd, SW_HIDE);*/
	}




}
bool loadConfig()
{
	TCHAR charPath[MAX_PATH] = { 0 };
	::GetModuleFileName(NULL, charPath, MAX_PATH);
	// 设置ini路径到exe同一目录下
	//_tcsrchr() 反向搜索获得最后一个'\\'的位置，并返回该位置的指针  
	TCHAR *pFind = _tcsrchr(charPath, '\\');
	if (pFind != NULL)
	{
		*pFind = '\0';
	}

	std::string strPath = charPath;


	char ip[16];
	char add[20];
	char net[20];
	char set[20];


	strPath += "\\CONFIG.ini";
	char str[20];
	::GetPrivateProfileString("WAREHOUSEPARAM", "heightCylinder", "0", str, 20, strPath.c_str());
	m_wareHouseParam.heightCylinder = _ttoi(str);
	::GetPrivateProfileString("WAREHOUSEPARAM", "diameterCylinder", "0", str, 20, strPath.c_str());
	m_wareHouseParam.diameterCylinder = _ttoi(str);
	::GetPrivateProfileString("WAREHOUSEPARAM", "heightCone", "0", str, 20, strPath.c_str());
	m_wareHouseParam.heightCone = _ttoi(str);
	::GetPrivateProfileString("WAREHOUSEPARAM", "diameterOutlet", "0", str, 20, strPath.c_str());
	m_wareHouseParam.diameterOutlet = _ttoi(str);
	::GetPrivateProfileString("WAREHOUSEPARAM", "unit", "0", str, 20, strPath.c_str());
	m_wareHouseParam.unit = _ttoi(str);

	::GetPrivateProfileString("COMMPARAM", "port", "1", str, 20, strPath.c_str());
	m_commParam.port = _ttoi(str);
	::GetPrivateProfileString("COMMPARAM", "baud", "1", str, 20, strPath.c_str());
	m_commParam.baud = _ttoi(str);

	::GetPrivateProfileString("DETECTPARAM", "HAngleResolution", "2", str, 20, strPath.c_str());
	m_detectParam.HAngleResolution = _ttoi(str);

	::GetPrivateProfileString("USERPARAM", "hideExe", "3", str, 20, strPath.c_str());
	m_userParam.hideExe = _ttoi(str);
	::GetPrivateProfileString("USERPARAM", "saveCloud2File", "3", str, 20, strPath.c_str());
	m_userParam.saveCloud2File = _ttoi(str);
	return false;
}


void openFileSavePoint()
{
	if (m_userParam.saveCloud2File)
	{
		WaitForSingleObject(hMutex4, INFINITE);
		saveScanFile_flag = 1;
		ReleaseMutex(hMutex4);

		if (out_.is_open())
		{
			out_.close();
		}
		{
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			sprintf(timePath, "./data/%d-%02d-%02d(%02d-%02d-%02d).txt",
				st.wYear,
				st.wMonth,
				st.wDay,
				st.wHour,
				st.wMinute,
				st.wSecond);
			out_.open(timePath);
		}

	}
}

void closeFileSavePoint()
{
	if (out_.is_open())
	{
		out_.close();
	}
}
//打开串口
bool openport(char *portname)
{
	hComm = CreateFile(portname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (hComm == INVALID_HANDLE_VALUE)
		return FALSE;
	else
		return TRUE;

}

bool setupdcb(int rate_arg)
{

	DCB dcb;                             //在串口通信中都需要用到这个DCB，可以理解为开辟了一块内存块
	int rate = rate_arg;                   //设置波特率
	memset(&dcb, 0, sizeof(dcb));
	if (!GetCommState(hComm, &dcb))        //获取当前dcb配置
	{
		return FALSE;

	}
	dcb.DCBlength = sizeof(dcb);
	//串口配置
	dcb.BaudRate = rate;                  //波特率
	dcb.Parity = NOPARITY;                //无检验
	dcb.fParity = 0;
	dcb.StopBits = ONESTOPBIT;            //1位停止位
	dcb.ByteSize = 8;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fOutX = 0;
	dcb.fInX = 0;

	//misc参数（我不知道这个什么意思!!=_=）
	dcb.fErrorChar = 0;
	dcb.fBinary = 1;
	dcb.fNull = 0;
	dcb.fAbortOnError = 0;
	dcb.wReserved = 0;
	dcb.XonLim = 2;
	dcb.XoffLim = 4;
	dcb.XonChar = 0x13;
	dcb.XoffChar = 0x19;
	dcb.EvtChar = 0;

	//设置DCB
	if (!SetCommState(hComm, &dcb))
	{
		return false;
	}
	else
		return true;

}


//设置前后2个数据之间的发送时间间隔，还有最长间隔时间之类的。。。。。
bool setuptimeout(DWORD ReadInterval, DWORD ReadTotalMultiplier, DWORD ReadTotalConstant, DWORD WriteTotalMultiplier, DWORD WriteTotalConstant)
{

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = ReadInterval;
	timeouts.ReadTotalTimeoutConstant = ReadTotalConstant;
	timeouts.ReadTotalTimeoutMultiplier = ReadTotalMultiplier;
	timeouts.WriteTotalTimeoutConstant = WriteTotalConstant;
	timeouts.WriteTotalTimeoutMultiplier = WriteTotalMultiplier;
	if (!SetCommTimeouts(hComm, &timeouts))
		return false;
	else
		return true;
}


//发送数据函数
void WriteChar(const char* m_szWriteBuffer, DWORD m_nToSend)              //参考ReceiveChar()函数
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;
	DWORD BytesSent = 0;
	HANDLE m_hWriteEvent = NULL;
	ResetEvent(m_hWriteEvent);
	if (bWrite)
	{
		m_ov.Offset = 0;
		m_ov.OffsetHigh = 0;
		//清空缓存区
		bResult = WriteFile(hComm, m_szWriteBuffer, m_nToSend, &BytesSent, &m_ov);
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
			{
				//继续
				BytesSent = 0;
				bWrite = FALSE;
				break;
			}
			default:
			{
				break;
			}
			}
		}

	}
	if (!bWrite)
	{
		bWrite = TRUE;
		bResult = GetOverlappedResult(hComm, &m_ov, &BytesSent, TRUE);
		if (!bResult)
		{
			printf("GetOverlappedResults() in WriteFile()");
		}
	}
	if (BytesSent != m_nToSend)
	{
		printf("WARNING:WriteFile() error..Bytes Sent:%d;Message Length:%d\n", BytesSent, strlen((char*)m_szWriteBuffer));
	}
	//return true;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawCoordinatesystem(GLfloat tx, GLfloat ty, GLfloat rx, GLfloat ry)
{
	//glPointSize(8);
	glLineWidth(5);


	glPushMatrix();
	//glTranslatef(tx, ty, 0);
	//glRotatef(rx, 1, 0, 0);
	//glRotatef(ry, 0, 1, 0);
	glBegin(GL_LINES);
	//x
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(2, 0, 0);
	//y
	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 2, 0);
	//z
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 2);
	glEnd();
	glPopMatrix();

	glFlush();

}


void drawCoordinatesystem1(GLfloat x, GLfloat y, GLfloat z, GLfloat len)
{
	glLineWidth(3);
	glColor4f(1, 0, 0, 1);
	pangolin::glDrawLine(x, y, z, x + len, y, z);

	glColor4f(0, 1, 0, 1);
	pangolin::glDrawLine(x, y, z, x, y + len, z);

	glColor4f(0, 0, 1, 1);
	pangolin::glDrawLine(x, y, z, x, y, z + len);

}


#define  MAX_BUFF 10024

pointCloudFile pcf;
pangolinViewer pv;
BYTE m_InPutBuff[MAX_BUFF];
#define STARTSIZE 3
unsigned char serialBuf_start[STARTSIZE] = { 11, 22, 55 };  //起始标志
unsigned char serialBuf_data[MAX_BUFF];
int serial_data_indx = 0;
HANDLE hMutex1;
HANDLE hMutex2_arduinoPC;
HANDLE hMutex2_pc2PC;
HANDLE hMutex3_arduinoPC;
//int m_switch_on_ack = 0;

HANDLE hMutex_pc2PCState;


int ackSignal_ok[] = { 11, 22, 55, 18, 1, 66, 66, 0xb2 };
int ackSignal_error[] = { 11, 22, 55, 18, 1, 44, 44, 0xb2 };


int startSignal[] = { 11, 22, 55, 15, 1, 55, 55, 0xb2 }; //首次启动
int rateOfProgressSignal[] = { 11, 22, 55, 13, 1, 0, 0, 0xb2 };//扫描进度
int bulkSignal[] = { 11, 22, 55, 14, 1, 0, 0, 0xb2 };//扫描体积
int finshSignal[] = { 11, 22, 55, 19, 1, 55, 55, 0xb2 };//扫描完成
int errorSignal[] = { 11, 22, 55, 20, 1, 44, 44, 0xb2 };//扫描出错


#define DATAMARKBITNUM 7
char g_receValidDataBuf[DATAMARKBITNUM][50];
#define STARTSIZE 3
WSADATA wsaData;                                   //指向WinSocket信息结构的指针
SOCKET sockListener;
SOCKADDR_IN saUdpServ;
int nSize = sizeof(SOCKADDR_IN);


void ThreadFuncSerialSendAckInterface(PVOID param)
{
	sps2volume vol;
	int volume = 0;

	int pc2pcSwitch = -1;    //PC和pc
	int switch_on_ack = -1;  //arduino和PC
	unsigned char pariXOR = 0;
	char sendBuf[50];
	int HAngleResolution = 500;
	int len = 0;
	while (true)
	{
		WaitForSingleObject(hMutex3, INFINITE);
		int flagSave = saveImage_flag;
		ReleaseMutex(hMutex3);


		if (-1 != flagSave)
		{
			WaitForSingleObject(hMutex3, INFINITE);
			saveImage_flag = -1;
			ReleaseMutex(hMutex3);
			switch_on_ack = flagSave;
			openFileSavePoint();
		}
		switch (switch_on_ack)
		{
		case 0:		//发送停止信号
			sprintf(sendBuf, "55 13 0 2 2 77 ");
			len = (strlen(sendBuf));
			WriteChar(sendBuf, len);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
				if (m_ackOk_arduino_pc)
				{
					m_ackOk_arduino_pc = 0;
					switch_on_ack = -1;
					ReleaseMutex(hMutex2_arduinoPC);
					std::cout << "pc->arduino: 发送停止信号" << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_arduinoPC);
			}
			break;
		case 1:    //发送启动信号
			sprintf(sendBuf, "55 12 0 1 1 77 ");
			len = (strlen(sendBuf));
			WriteChar(sendBuf, len);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
				if (m_ackOk_arduino_pc)
				{
					m_ackOk_arduino_pc = 0;
					switch_on_ack = 2;
					ReleaseMutex(hMutex2_arduinoPC);
					std::cout << "pc->arduino: 发送启动信号" << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_arduinoPC);
			}
			break;
		case 2:    //发送横向角分辨率
			pariXOR ^= m_detectParam.HAngleResolution / 256;
			pariXOR ^= m_detectParam.HAngleResolution % 256;

			sprintf(sendBuf, "55 16 %d %d %d 77 ", m_detectParam.HAngleResolution / 256, m_detectParam.HAngleResolution % 256, pariXOR);
			len = (strlen(sendBuf));
			WriteChar(sendBuf, len);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
				if (m_ackOk_arduino_pc)
				{
					m_ackOk_arduino_pc = 0;
					switch_on_ack = -1;
					ReleaseMutex(hMutex2_arduinoPC);
					std::cout << "pc->arduino: 发送横向角分辨率" << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_arduinoPC);
			}
			break;
		default://判断应答信号
			break;
		}
		/////////////////////////////////////////////////////////////


		WaitForSingleObject(hMutex_pc2PCState, INFINITE);
		int pc2pcState = m_pc2pcState;
		ReleaseMutex(hMutex_pc2PCState);
		if (pc2pcState != -1)
		{
			m_pc2pcState = -1;
			pc2pcSwitch = pc2pcState;
		}
		switch (pc2pcSwitch)
		{
		case 0:  //首次启动15
			len = sizeof(startSignal);// / sizeof(int);
			sendto(sockListener, (char *)&startSignal, len, 0, (SOCKADDR *)&saUdpServ, nSize);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_pc2PC, INFINITE);
				if (m_ackOk_pc_pc)
				{
					m_ackOk_pc_pc = 0;
					pc2pcSwitch = -1;
					ReleaseMutex(hMutex2_pc2PC);
					std::cout << "PC <- PC: 首次启动" << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_pc2PC);
			}
			break;
		case 1:  //扫描进度13
			len = sizeof(rateOfProgressSignal);// / sizeof(int);
			WaitForSingleObject(hMutex_pc2PCState, INFINITE);
			rateOfProgressSignal[5] = (float)scanProgress / (float)HORI_PLUS * 100;
			ReleaseMutex(hMutex_pc2PCState);
			rateOfProgressSignal[6] = rateOfProgressSignal[5];
			sendto(sockListener, (char *)&rateOfProgressSignal, len, 0, (SOCKADDR *)&saUdpServ, nSize);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_pc2PC, INFINITE);
				if (m_ackOk_pc_pc)
				{
					m_ackOk_pc_pc = 0;
					pc2pcSwitch = -1;
					ReleaseMutex(hMutex2_pc2PC);
					std::cout << "PC <- PC: 扫描进度_" << rateOfProgressSignal[5] << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_pc2PC);
			}
			break;
		case 2:  //扫描体积14
			volume = vol.execute(&g_map.vsp[0], g_map.vsp.size()) / 1000000;//立方米
			bulkSignal[5] = volume;
			bulkSignal[6] = bulkSignal[5];

			std::cout << volume << std::endl;

			len = sizeof(bulkSignal);// / sizeof(int);
			sendto(sockListener, (char *)&bulkSignal, len, 0, (SOCKADDR *)&saUdpServ, nSize);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_pc2PC, INFINITE);
				if (m_ackOk_pc_pc)
				{
					m_ackOk_pc_pc = 0;
					pc2pcSwitch = -1;
					ReleaseMutex(hMutex2_pc2PC);
					std::cout << "PC <- PC: 扫描体积_" << bulkSignal[5] << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_pc2PC);
			}
			break;
		case 3:  //扫描完成19
			len = sizeof(finshSignal);// / sizeof(int);
			sendto(sockListener, (char *)&finshSignal, len, 0, (SOCKADDR *)&saUdpServ, nSize);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_pc2PC, INFINITE);
				if (m_ackOk_pc_pc)
				{
					m_ackOk_pc_pc = 0;
					pc2pcSwitch = 2;  //发送扫描体积
					ReleaseMutex(hMutex2_pc2PC);
					std::cout << "PC <- PC: 扫描完成" << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_pc2PC);
			}
			break;
		case 4:  //扫描出错20
			len = sizeof(errorSignal);// / sizeof(int);
			sendto(sockListener, (char *)&errorSignal, len, 0, (SOCKADDR *)&saUdpServ, nSize);
			for (int i = 0; i < 20; i++)
			{
				Sleep(100);
				WaitForSingleObject(hMutex2_pc2PC, INFINITE);
				if (m_ackOk_pc_pc)
				{
					m_ackOk_pc_pc = 0;
					pc2pcSwitch = -1;
					ReleaseMutex(hMutex2_pc2PC);
					std::cout << "PC <- PC: 扫描出错" << std::endl;
					break;
				}
				ReleaseMutex(hMutex2_pc2PC);
			}
			break;
		default:
			break;
		}
	}
}



void ThreadFuncSerial(PVOID param)
{
#ifdef OFFLINE
	return;
#endif
	BOOL bRead = TRUE;
	BOOL bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	char RXBuff;                                                       //接收到的数据
	DWORD dwLength;

	char strBuf[20];
	sprintf(strBuf, "COM%d", m_commParam.port);
	if (openport(strBuf))                                                          //打开串口
	{
		printf("open comport success\n");
	}
	else
		return;
	if (setupdcb(m_commParam.baud))                                                            //设置波特率为9600，其他采用默认值
		printf("setup DCB success\n");
	else
		return;
	if (setuptimeout(0, 0, 0, 0, 0))                                                   //设置延时等待时间
		printf("setup timeouts success\n");
	else
		return;
	PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);     //清空缓冲区


	unsigned char dataMarkBitIdx = 0;

	WaitForSingleObject(hMutex3, INFINITE);
	saveImage_flag = 0;
	ReleaseMutex(hMutex3);

	while (1)
	{
		bResult = ClearCommError(hComm, &dwError, &comstat);

		dwLength = comstat.cbInQue;
		if (dwLength > MAX_BUFF)
			dwLength = MAX_BUFF;

		if (dwLength > 0)
			ReadFile(hComm, m_InPutBuff, dwLength, &BytesRead, &m_ov);
		static int lenIndex = 0;
		bool flagEnd = 0;
		static bool flagS = 1;
		static int switch_on = 0;
		static int trueNum = 0;
		static int errorNum = 0;
		static unsigned char len = 0;
		int indX = 0;
		static unsigned char verifyBit = 0;
		for (int i = 0; i < dwLength; i++)
		{
			switch (switch_on)
			{
			case 0:  //起始标志位
			{
				if (flagS)
				{
					if (m_InPutBuff[i] == serialBuf_start[0])
					{
						flagS = 0;
						lenIndex++;
						continue;   //起始标志位的第一个字节----找到
					}
					else
					{
					//	printf("\n起始标志位未找到---%d(%d)\n", trueNum, errorNum++);  //起始标志位的第一个字节------未找到
						flagEnd = 1;
						continue;
					}
				}
				else
				{

					if (m_InPutBuff[i] != serialBuf_start[lenIndex])
					{
						//printf("\n数据包出错---重新再来---%d(%d)\n", trueNum, errorNum++);
						flagS = 1;
						flagEnd = 1;
						lenIndex = 0;
						i--;

						continue;  //数据包出错---重新再来
					}
					lenIndex++;
					if (STARTSIZE == lenIndex)
					{
						lenIndex = 0;
						flagS = 1;
						switch_on = 1;  //状态转移

						//indX = i+1;

						break;
					}
				}
			}
			break;
			case 1:  //数据标识位
				//12->应答信号    arduino--->PC
				//13->点云数据    arduino--->PC
				//14->仓库扫描完毕  arduino--->PC
				//15->仓库扫描出错  arduino--->PC


				if (m_InPutBuff[i] / 10 == 1)
				{
					unsigned char LowBit = m_InPutBuff[i] % 10;
					if (1 < LowBit && LowBit < 9)
					{
						dataMarkBitIdx = LowBit;
						switch_on = 2;
					}
					else
					{
						switch_on = 0; //状态转移
					}
				}
				else
				{
					switch_on = 0; //状态转移
				}
				break;
			case 2:  //数据长度len
				len = m_InPutBuff[i];
				//	indX = i + 1;
				serial_data_indx = 0;
				verifyBit = 0;

				switch_on = 3;  //状态转移
				break;
			case 3:  //读取len个数据
				serialBuf_data[serial_data_indx++] = m_InPutBuff[i];
				verifyBit ^= m_InPutBuff[i];
				if (serial_data_indx == len)
				{
					switch_on = 4; //状态转移
				}

				break;
			case 4: //读取校验码
				if (verifyBit != m_InPutBuff[i])
				{
					//校验出错----丢弃数据包
					//printf("\n校验出错----丢弃数据包%d(%d)\n", trueNum, errorNum++);
					switch_on = 0; //状态转移
				}
				else
				{
					switch_on = 5; //状态转移
				}
				break;
			case 5: //结束标志位
				if (0xB2 != m_InPutBuff[i])
				{
					//出错----丢弃数据包
					if (dataMarkBitIdx == 2)
					{
						WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
						m_ackOk_arduino_pc = 0;
						ReleaseMutex(hMutex2_arduinoPC);
					}
				}
				else
				{
					if (dataMarkBitIdx == 3)
					{
						for (int i = 0; i < len; i += 6)
						{
							struct pointStruct pt;
							pt.theta_h = serialBuf_data[i] * 256 + serialBuf_data[i + 1];
							pt.theta_v = serialBuf_data[i + 2] * 256 + serialBuf_data[i + 3];
							pt.len = serialBuf_data[i + 4] * 256 + serialBuf_data[i + 5];
							WaitForSingleObject(hMutex1, INFINITE);
							m_scanBuff.push(pt);
							ReleaseMutex(hMutex1);
						}
					}
					//printf("\n成功接收数据包——>%d(%d)\n", (trueNum++)*8, errorNum);
					else if (dataMarkBitIdx == 2)
					{
						WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
						m_ackOk_arduino_pc = 1;
						ReleaseMutex(hMutex2_arduinoPC);
						std::cout << "Arduino->PC: 应答信号" << std::endl;
					}
					else if (dataMarkBitIdx == 4)
					{
						char sendBuf[50];
						sprintf(sendBuf, "55 14 0 1 1 77 ");//发送应答信息  当arduino发送给PC数据后
						len = (strlen(sendBuf));
						WriteChar(sendBuf, len);
						std::cout << "Arduino->PC: 扫描完毕！" << std::endl;

						WaitForSingleObject(hMutex_pc2PCState, INFINITE);
						m_pc2pcState = 3;
						ReleaseMutex(hMutex_pc2PCState);
					}
					else if (dataMarkBitIdx == 5)
					{
						char sendBuf[50];
						sprintf(sendBuf, "55 14 0 1 1 77 ");//发送应答信息  当arduino发送给PC数据后
						len = (strlen(sendBuf));
						WriteChar(sendBuf, len);
						std::cout << "Arduino->PC: 扫描出错！" << std::endl;
						WaitForSingleObject(hMutex_pc2PCState, INFINITE);
						m_pc2pcState = 4;
						ReleaseMutex(hMutex_pc2PCState);
					}

				}
				switch_on = 0; //状态转移
			default:
				switch_on = 0; //状态转移
				break;
			}
		}
		memset(m_InPutBuff, 0, MAX_BUFF);
		::Sleep(2);
	}
}

void StringSplit(std::string s, char splitchar, std::vector<std::string>& vec)
{
	if (vec.size()>0)//保证vec是空的  
		vec.clear();
	int length = s.length();
	int start = 0;
	for (int i = 0; i<length; i++)
	{
		if ((s[i] > 47 && s[i] < 58) || s[i] == 32)
		{
			if (s[i] == splitchar && i == 0)//第一个就遇到分割符  
			{
				start += 1;
			}
			else if (s[i] == splitchar)
			{
				vec.push_back(s.substr(start, i - start));
				start = i + 1;
			}
			else if (i == length - 1)//到达尾部  
			{
				vec.push_back(s.substr(start, i + 1 - start));
			}
		}
		else
		{
			return;
		}

	}
}


void parseData_3po(std::string inputFile)
{
	std::cout << std::endl << "Parsing dataset " << inputFile << " ... " << std::endl;
	//
	// open the file
	std::ifstream inFile(inputFile.c_str());
	if (!inFile) {
		std::cerr << "Error opening dataset file " << inputFile << std::endl;
		return;
	}
	int ss = 0;
	//	 // go through the dataset file
	int error_t = 0;
	int sumCnt = 0;


	while (!inFile.eof()) {
		/*	if (++sumCnt == 1600)
		{
		break;
		std::cout << std::endl;
		}*/
		std::string line;
		getline(inFile, line);
		std::vector< std::string> datBuf;// = split(line, " ");
		StringSplit(line, ' ', datBuf);
		//std::vector<std::wstring> vecSegTag;
		// boost::is_any_of这里相当于分割规则了     
		//boost::split(datBuf, line, boost::is_any_of((" ")));
		if (datBuf.size() == 3)
		{
			int Hdat = std::stoi(datBuf[0]);
			int Vdat = std::stoi(datBuf[1]);
			int Rdat = std::stoi(datBuf[2]);;
			{
				struct pointStruct pt;
				pt.theta_h = Hdat;
				pt.theta_v = Vdat;
				pt.len = Rdat;
				m_scanBuff.push(pt);

			}


		}
		else
		{
			error_t++;
		//	std::cout << "error。。。" << error_t << std::endl;
		}
	}
	//std::cout << pointBuff.size() << error_t << std::endl;

}



void ThreadFuncShowCloud(PVOID param)
{
	sp1dProject pro;
	pro.create(4000, 20);
	int oneLinePointNum = 0;
	float last_h;
	std::vector<sensorPoint> linePoints;


#ifdef OFFLINE
	int  cx = GetSystemMetrics(SM_CXSCREEN);
	int  cy = GetSystemMetrics(SM_CYSCREEN);
	pv.create("立体式粮仓储量检测系统-大野感知", cx, cy, 160);
	parseData_3po("C:\\2017-01-12(16-08-09).txt");
#endif
	while (1)
	{
		WaitForSingleObject(hMutex1, INFINITE);
		bool flag = m_scanBuff.empty();
		ReleaseMutex(hMutex1);
		while (!flag)
		{
			WaitForSingleObject(hMutex1, INFINITE);
			struct pointStruct pt = m_scanBuff.front();
			m_scanBuff.pop();
			ReleaseMutex(hMutex1);

			WaitForSingleObject(hMutex4, INFINITE);
			bool saveFile = saveScanFile_flag;
			ReleaseMutex(hMutex4);
			if (saveFile)
			{
				if (out_.is_open())
				{
					out_ << pt.theta_h << " " << pt.theta_v << " " << pt.len << std::endl;
				}
			}
			sensorPoint sp;
			static bool startData = 1;
			if (startData)
			{
				startData = 0;
				last_h = pt.theta_h;
			}
			else
			{

				if (abs(last_h - pt.theta_h) > 5)
				{
					WaitForSingleObject(hMutex_pc2PCState, INFINITE);
					m_pc2pcState = 1;  //扫描进度
					scanProgress = pt.theta_h;
					ReleaseMutex(hMutex_pc2PCState);

					if (linePoints.size() > 0)
					{
						pro.filter(g_map, &linePoints[0], oneLinePointNum);
					}
					linePoints.clear();
					oneLinePointNum = 0;
				}

				last_h = pt.theta_h;
				float h = pt.theta_h;
				float v = pt.theta_v;
				change(h, v);
				sp.set(pt.len, v, h);
				linePoints.push_back(sp);
				oneLinePointNum++;
			}
#ifdef OFFLINE
			float ypr[3];
			ypr[0] = sp.x;
			ypr[1] = sp.y;
			ypr[2] = sp.z;
			pv.updateCloudPoint(ypr);
#endif
			WaitForSingleObject(hMutex1, INFINITE);
			flag = m_scanBuff.empty();
			ReleaseMutex(hMutex1);
#ifdef OFFLINE
			if (flag)
			{
				static bool END = 1;
				if (END)
				{
					END = 0;
					g_color = 0;
					float ypr[3];
					for (int i = 0; i < g_map.vsp.size(); i++)
					{
						ypr[0] = g_map.vsp[i].x;
						ypr[1] = g_map.vsp[i].y;
						ypr[2] = g_map.vsp[i].z;

					//	pv.updateCloudPoint2(ypr);
					}

					/*for (int i = 0; i < g_map.vborderIdx.size(); i++)
					{
						ypr[0] = g_map.vsp[g_map.vborderIdx[i]].x;
						ypr[1] = g_map.vsp[g_map.vborderIdx[i]].y;
						ypr[2] = g_map.vsp[g_map.vborderIdx[i]].z;
						pv.updateCloudPoint2(ypr);

					}*/
				}

				sps2volume vol;
				double volume = vol.execute(&g_map.vsp[0], g_map.vsp.size()) / 1000000;//立方米
				std::cout << volume << std::endl;
			}
#endif
		}


		::Sleep(5);

	}
}



void ThreadFuncUserInterface(PVOID param)
{
	//saClient用来从广播地址接收消息
	char cRecvBuff[800];                               //定义接收缓冲区
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)           //进行WinSocket的初始化
	{
		printf("Can't initiates windows socket!Program stop.\n");//初始化失败返回-1
	}
	sockListener = socket(PF_INET, SOCK_DGRAM, 0);
	BOOL fBroadcast = TRUE;
	setsockopt(sockListener, SOL_SOCKET, SO_BROADCAST, (CHAR *)&fBroadcast, sizeof(BOOL));


	saUdpServ.sin_family = AF_INET;
	saUdpServ.sin_port = htons(11011);             //发送端使用的发送端口，可以根据需要更改
	saUdpServ.sin_addr.s_addr = htonl(INADDR_BROADCAST);

#define MAXLEN 200 //数组长度
	int recvpacket[MAXLEN];
	static int lenIndex = 0;
	bool flagEnd = 0;
	static bool flagS = 1;

	unsigned char dataMarkBitIdx = 0;
	static int trueNum = 0;
	static int errorNum = 0;
	int switch_on = 0;
	unsigned char validDataBuf[50];
	int validIndx = 0;


	WaitForSingleObject(hMutex_pc2PCState, INFINITE);
	m_pc2pcState = 0;
	ReleaseMutex(hMutex_pc2PCState);

	//起始字节55+参数标识位+数据长度LEN+数据DATA+校验码X0R+结束字节77

	while (1)
	{
		int recvLen = recvLen = recvfrom(sockListener, (char *)&recvpacket, MAXLEN * 4, 0,
			(SOCKADDR *)&saUdpServ, &nSize);
		if (recvLen <= 0)
		{
			continue;
		}
		static unsigned char len = 0;
		int indX = 0;
		static int verifyBit = 0;
		for (int i = 0; i < recvLen / 4; i++)
		{
			switch (switch_on)
			{
			case 0:  //起始标志位
			{
				if (flagS)
				{
					if (recvpacket[i] == serialBuf_start[0])
					{
						flagS = 0;
						lenIndex++;
						continue;   //起始标志位的第一个字节----找到
					}
					else
					{
						//printf("\n起始标志位未找到---%d(%d)\n", trueNum, errorNum++);  //起始标志位的第一个字节------未找到
						flagEnd = 1;
						continue;
					}
				}
				else
				{

					if (recvpacket[i] != serialBuf_start[lenIndex])
					{
						//printf("\n数据包出错---重新再来---%d(%d)\n", trueNum, errorNum++);
						flagS = 1;
						flagEnd = 1;
						lenIndex = 0;
						i--;

						continue;  //数据包出错---重新再来
					}
					lenIndex++;
					if (STARTSIZE == lenIndex)
					{
						lenIndex = 0;
						flagS = 1;
						switch_on = 1;  //状态转移

						//indX = i+1;

						break;
					}
				}
			}
			break;
			case 1:  //数据标识位
				//12->建模参数          A->B
				//13->扫描进度(百分比)   B->A
				//14->扫描体积(长度可变)  B->A
				//15->首次启动(握手信号)  B->A
				//16->启动设备            A->B
				//17->关闭设备            A->B
				//18->数据应答             A--B
				//19->扫描完成             B->A
				//20->扫描出错             B->A
				if (recvpacket[i] / 10 == 1)
				{
					unsigned char LowBit = recvpacket[i] % 10;
					if (1 < LowBit && LowBit < 9)
					{
						dataMarkBitIdx = LowBit;
						switch_on = 2;
					}
					else
					{
						switch_on = 7; //状态转移
					}
				}
				else
				{
					switch_on = 7; //状态转移
				}

				break;
			case 2:  //数据长度len
			{
				len = recvpacket[i];// / 4;
				//	indX = i + 1;
				validIndx = 0;
				verifyBit = 0;

				switch_on = 3;  //状态转移
			}
			break;
			case 3:  //读取len个数据
				validDataBuf[validIndx++] = recvpacket[i];
				verifyBit ^= recvpacket[i];
				if (validIndx == len)
				{
					switch_on = 4; //状态转移
				}
				break;
			case 4: //读取校验码
				if (verifyBit != recvpacket[i])
				{
					//校验出错----丢弃数据包
					//printf("\n校验出错----丢弃数据包%d(%d)\n", trueNum, errorNum++);
					switch_on = 7; //状态转移
				}
				else
				{
					switch_on = 5; //状态转移
				}
				break;
			case 5: //结束标志位
				if (0xB2 != recvpacket[i])
				{
					//出错----丢弃数据包
					switch_on = 7; //状态转移
				}
				else{
					if (6 == dataMarkBitIdx) //启动设备
					{
						WaitForSingleObject(hMutex3, INFINITE);
						saveImage_flag = 1;
						ReleaseMutex(hMutex3);
						switch_on = 6; //数据包接收完成----应答正确
						std::cout << "PC->PC: 启动设备！" << std::endl;
					}
					else if (7 == dataMarkBitIdx) //停止设备
					{
						WaitForSingleObject(hMutex3, INFINITE);
						saveImage_flag = 0;
						ReleaseMutex(hMutex3);
						switch_on = 6; //数据包接收完成----应答正确
						std::cout << "PC->PC: 关闭设备！" << std::endl;
					}
					else if (8 == dataMarkBitIdx)
					{
						WaitForSingleObject(hMutex2_pc2PC, INFINITE);
						m_ackOk_pc_pc = 1;
						ReleaseMutex(hMutex2_pc2PC);
						switch_on = 0; //状态转移
						std::cout << "PC->PC: 应答成功！" << std::endl;
						break;
					}
					else if (2 == dataMarkBitIdx)
					{
						std::cout << "PC->PC: 建模参数！" << std::endl;
					}

					/*for (int i = 0; i < len; i += 6)
					{
					g_receValidDataBuf[dataMarkBitIdx][i] = validDataBuf[i];
					}*/
					//printf("\n成功接收数据包——>%d(%d)\n", (trueNum++) * 8, errorNum);

				}
			case 6: //应答信号---正确
				sendto(sockListener, (char *)&ackSignal_ok, 32, 0, (SOCKADDR *)&saUdpServ, nSize);
				std::cout << "PC <- PC: 应答(succed)" << std::endl;
				switch_on = 0; //状态转移
				break;
			case 7: //应答信号---错误
				sendto(sockListener, (char *)&ackSignal_error, 32, 0, (SOCKADDR *)&saUdpServ, nSize);
				std::cout << "PC <- PC: 应答(error)" << std::endl;
				switch_on = 0; //状态转移
				break;
			default:
				switch_on = 0; //状态转移
				break;
			}

		}
	}
	closesocket(sockListener);
	WSACleanup();
}


int main(int argc, char* argv[])
{
	initPhase();
	hMutex1 = CreateMutex(NULL, FALSE, NULL);
	hMutex2_arduinoPC = CreateMutex(NULL, FALSE, NULL);
	hMutex3_arduinoPC = CreateMutex(NULL, FALSE, NULL);

	hMutex2_pc2PC = CreateMutex(NULL, FALSE, NULL);
	hMutex_pc2PCState = CreateMutex(NULL, FALSE, NULL);

	_beginthread(ThreadFuncSerialSendAckInterface, 0, NULL);
	_beginthread(ThreadFuncSerial, 0, NULL);
	_beginthread(ThreadFuncShowCloud, 0, NULL);
	_beginthread(ThreadFuncUserInterface, 0, NULL);


	while (true)
	{
		::Sleep(100);
	}

	return 0;
}

