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
#include "hemisphereLaserSensor.h"
#include "hpangolinViewer.h"
#include <pangolin/pangolin.h>
#include "pointCloudFile.h"

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
struct wareHouseParam m_wareHouseParam;
struct commParam m_commParam;
struct detectParam m_detectParam;

bool m_ackOk_arduino_pc = 0;
//int m_switch_on_arduino_pc = 0;
void initPhase();
bool loadConfig();


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
	loadConfig();


	m_HAngleResolution = m_detectParam.HAngleResolution;
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
	return false;
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
HANDLE hMutex3_arduinoPC;
//int m_switch_on_ack = 0;




void ThreadFuncSerialSendAckInterface(PVOID param)
{
	int switch_on_ack = 0;
	unsigned char pariXOR = 0;
	char sendBuf[50];
	int HAngleResolution = 500;
	int len = 0;
	while (true)
	{
		WaitForSingleObject(hMutex3, INFINITE);
		int flagSave = saveImage_flag;
		ReleaseMutex(hMutex3);

		if (1 == flagSave)
		{
			WaitForSingleObject(hMutex3, INFINITE);
			saveImage_flag = 0;
			ReleaseMutex(hMutex3);
			//WaitForSingleObject(hMutex3_arduinoPC, INFINITE);
			switch_on_ack = 1;
			//ReleaseMutex(hMutex3_arduinoPC);

			printf("start\n");


		}
		if (2 == flagSave)
		{
			WaitForSingleObject(hMutex3, INFINITE);
			saveImage_flag = 0;
			ReleaseMutex(hMutex3);
			//WaitForSingleObject(hMutex3_arduinoPC, INFINITE);
			switch_on_ack = 0;
			//ReleaseMutex(hMutex3_arduinoPC);

			printf("stop\n");

		}
		//WaitForSingleObject(hMutex5, INFINITE);
		//bool sendPwmFlag = set_pwm;
		//ReleaseMutex(hMutex5);
		//if (sendPwmFlag)
		//{
		//	WaitForSingleObject(hMutex5, INFINITE);
		//	set_pwm = 0;
		//	HAngleResolution = m_HAngleResolution;
		//	ReleaseMutex(hMutex5);

		//	//WaitForSingleObject(hMutex3_arduinoPC, INFINITE);
		//	switch_on_ack = 2;
		//	//ReleaseMutex(hMutex3_arduinoPC);
		//}
	/*	WaitForSingleObject(hMutex3_arduinoPC, INFINITE);
		switch_on_ack = m_switch_on_ack;
		ReleaseMutex(hMutex3_arduinoPC);*/
		switch (switch_on_ack)
		{
		case 0:		//发送停止信号
			sprintf(sendBuf, "55 13 0 2 2 77 ");
			len = (strlen(sendBuf));
			WriteChar(sendBuf, len);
			Sleep(2000);
			WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
			if (m_ackOk_arduino_pc)
			{
				m_ackOk_arduino_pc = 0;
				switch_on_ack = 4;
			}
			ReleaseMutex(hMutex2_arduinoPC);

			break;
		case 1:    //发送启动信号
			sprintf(sendBuf, "55 12 0 1 1 77 ");
			 len = (strlen(sendBuf));
			WriteChar(sendBuf, len);
			Sleep(2000);
			WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
			if (m_ackOk_arduino_pc)
			{
				m_ackOk_arduino_pc = 0;
				switch_on_ack = 2;
			}
			ReleaseMutex(hMutex2_arduinoPC);
			break;
		case 2:    //发送横向角分辨率
			pariXOR ^= m_detectParam.HAngleResolution / 256;
			pariXOR ^= m_detectParam.HAngleResolution % 256;

			sprintf(sendBuf, "55 16 %d %d %d 77 ", m_detectParam.HAngleResolution / 256, m_detectParam.HAngleResolution % 256, pariXOR);
			len = (strlen(sendBuf));
			WriteChar(sendBuf, len);
			Sleep(2000);
			WaitForSingleObject(hMutex2_arduinoPC, INFINITE);
			if (m_ackOk_arduino_pc)
			{
				m_ackOk_arduino_pc = 0;
				switch_on_ack = 4;
			}
			ReleaseMutex(hMutex2_arduinoPC);
			break;
		case 4:   //判断应答信号
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
						printf("\n起始标志位未找到---%d(%d)\n", trueNum, errorNum++);  //起始标志位的第一个字节------未找到
						flagEnd = 1;
						continue;
					}
				}
				else
				{

					if (m_InPutBuff[i] != serialBuf_start[lenIndex])
					{
						printf("\n数据包出错---重新再来---%d(%d)\n", trueNum, errorNum++);
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
					printf("\n校验出错----丢弃数据包%d(%d)\n", trueNum, errorNum++);
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
					}
				
				}
				switch_on = 0; //状态转移
			default:
				switch_on = 0; //状态转移
				break;
			}
			printf("switch_on: %d\n", switch_on);
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
			std::cout << "error。。。" << error_t << std::endl;
		}
	}
	//std::cout << pointBuff.size() << error_t << std::endl;

}



void ThreadFuncShowCloud(PVOID param)
{
//	pcf.init();
	//pcf.open("5.txt");
	statisticMapper csm;
	float fp[3] = { 0, 0, 5000.0f };
	csm.createColumn(5, 5, 1000, 5000);
	csm.setSensor(0, fp);
	//csm.appendSensor(0, fp);
	//csm.create(5, 5, 2000, 2000, 1000);
	int xxx = 0;
	float plus2radialH = 8.0*atan(1.0f) / 24000.0f;
	float plus2radialV = 8.0*atan(1.0f) / 8000.0f;

	int  cx = GetSystemMetrics(SM_CXSCREEN);
	int  cy = GetSystemMetrics(SM_CYSCREEN);
	pv.create("立体式粮仓储量检测系统-大野感知", cx, cy, 160);

#ifdef OFFLINE
	parseData_3po("C:\\2017-02-23(17-45-01).txt");
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

			float ypr[3];
			float len_center = 0.0;
			float angleAB = -4.1f;// 2.5;// 3.5;
			float a = 0.0f, b = 0, c = 0;

			float angle = plus2radialV * (pt.theta_v);
			float dt = angle * 45 / atan(1.0f);
			angle = (dt + angleAB)*atan(1.0f) / 45;
			//	printf("%d %f\n", i, dt);
			float theta = (angle);// +2.0f*atan(1.0f);
			float phi = plus2radialH * (pt.theta_h);

			float rs = pt.len * cos(theta);
			ypr[0] = rs * cos(phi);// +b*sin(phi) + c*cos(phi);
			ypr[1] = rs * sin(phi);// +b*cos(phi) + c*sin(phi);
			ypr[2] = pt.len * sin(theta);// +a;
			//if (sqrt(pow(ypr[0], 2)+pow(ypr[1], 2)) > 1000)
			//{
			//	continue;
			//}
			//if (ypr[2] > 600)
			//{
				WaitForSingleObject(hMutex2, INFINITE);
				csm.update(0, ypr[0], ypr[1], ypr[2], 1.0f);
				ReleaseMutex(hMutex2);

			//}
			
				
			//csm.postprocess(900.0f, 15);
			//csm.xyzList()
			pv.updateCloudPoint(ypr);
			
			WaitForSingleObject(hMutex1, INFINITE);
			flag = m_scanBuff.empty();
			ReleaseMutex(hMutex1);
		}
	
		::Sleep(5);

	}
}


int startSignal[] = { 11, 22, 55, 15, 1, 0x55, 0x55, 0xb2 };
int ackSignal_ok[] = { 11, 22, 55, 18, 1, 0x66, 0x66, 0xb2 };
int ackSignal_error[] = { 11, 22, 55, 18, 1, 0x44, 0x44, 0xb2 };

#define DATAMARKBITNUM 7
char g_receValidDataBuf[DATAMARKBITNUM][50];
#define STARTSIZE 3
void ThreadFuncUserInterface(PVOID param)
{
	WSADATA wsaData;                                   //指向WinSocket信息结构的指针
	SOCKET sockListener;
	SOCKADDR_IN saUdpServ;                          
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

	int nSize = sizeof(SOCKADDR_IN);

	sendto(sockListener, (char *)&startSignal, 32, 0, (SOCKADDR *)&saUdpServ, nSize);

	//起始字节55+参数标识位+数据长度LEN+数据DATA+校验码X0R+结束字节77

	while (1)
	{
		int recvLen = recvLen = recvfrom(sockListener, (char *)&recvpacket, MAXLEN*4, 0,
			(SOCKADDR *)&saUdpServ, &nSize);
		if (recvLen <= 0)
		{
			continue;
		}
		static unsigned char len = 0;
		int indX = 0;
		static unsigned char verifyBit = 0;
		for (int i = 0; i < recvLen/4; i++)
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
						printf("\n起始标志位未找到---%d(%d)\n", trueNum, errorNum++);  //起始标志位的第一个字节------未找到
						flagEnd = 1;
						continue;
					}
				}
				else
				{

					if (recvpacket[i] != serialBuf_start[lenIndex])
					{
						printf("\n数据包出错---重新再来---%d(%d)\n", trueNum, errorNum++);
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
				len = recvpacket[i]/4;
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
					printf("\n校验出错----丢弃数据包%d(%d)\n", trueNum, errorNum++);
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
					for (int i = 0; i < len; i += 6)
					{
						g_receValidDataBuf[dataMarkBitIdx][i] = validDataBuf[i];
					}
					printf("\n成功接收数据包——>%d(%d)\n", (trueNum++)*8, errorNum);

					switch_on = 6; //数据包接收完成----应答正确
				}
			case 6: //应答信号---正确
				sendto(sockListener, (char *)&ackSignal_ok, 32, 0, (SOCKADDR *)&saUdpServ, nSize);

				switch_on = 0; //状态转移
				break;
			case 7: //应答信号---错误
				sendto(sockListener, (char *)&ackSignal_error, 32, 0, (SOCKADDR *)&saUdpServ, nSize);

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
	
	_beginthread(ThreadFuncSerialSendAckInterface, 0, NULL);
	_beginthread(ThreadFuncSerial, 0, NULL);
	_beginthread(ThreadFuncShowCloud, 0, NULL);
	//_beginthread(ThreadFuncUserInterface, 0, NULL);

	
	while (true)
	{
		::Sleep(100);
	}
	
	return 0;
}
