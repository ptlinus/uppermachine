#pragma once
#pragma warning(disable:4996)

#include <thread>
#include <mutex>
#include <vector>
#include <pangolin/pangolin.h>
#include<time.h>
#include <fstream>

HANDLE hMutex2;
HANDLE hMutex3;
HANDLE hMutex4;
HANDLE hMutex5;



float g_color = 0.9;

int saveImage_flag = -1;  //1-启动设备  2-停止设备
bool saveScanFile_flag = 0;
bool set_pwm = 0;
int m_HAngleResolution = 500;
//int vpwm_value = 60;
class pangolinViewer
{
	bool			m_bRunning;
	std::thread		*m_thViewer;
	int				mviewW, mviewH, mpanelW;
	std::mutex		m_mutexShow;
	std::string		m_winTitle;

	std::vector<float> vpts;
	std::vector<float> vpts1;
	std::vector<float> vpts2;

	
public:
	pangolinViewer()	{ m_bRunning = false; m_thViewer = NULL; pts = NULL; npts = 0; }
	~pangolinViewer()	{ 
		stop(); 
	}

	float	*pts;
	int		npts;

	void create(char *windowsname, int winW, int winH, int panelW = 0)
	{
		//desktop = FindWindow("ProgMan", NULL);
		//task = FindWindow("Shell_TrayWnd", NULL);
		//ShowWindow(desktop, SW_HIDE);
		//ShowWindow(task, SW_HIDE);//隐藏任务栏 


		hMutex2 = CreateMutex(NULL, FALSE, NULL);
		hMutex3 = CreateMutex(NULL, FALSE, NULL);
		hMutex4 = CreateMutex(NULL, FALSE, NULL);
		hMutex5 = CreateMutex(NULL, FALSE, NULL);


		if (winW <= 0 || winH <= 0)
			return;

		m_bRunning = true;
		m_winTitle = windowsname == NULL ? "_view_" : windowsname;
		mviewW = winW;	mviewH = winH; mpanelW = panelW < 0 ? 0 : panelW;
		if (!m_thViewer)
		{
			m_thViewer = new std::thread(&pangolinViewer::run, this);
		}
	}

	void join()
	{
		if (m_thViewer)	m_thViewer->join();
	}

	void run()
	{
		//printf("START PANGOLIN!\n");

		pangolin::CreateWindowAndBind(m_winTitle, mviewW, mviewH);
		glEnable(GL_DEPTH_TEST);

		//float hspace[3] = { 320.0f, 240.0f, 800.0f };
		// 3D visualization
		pangolin::OpenGlRenderState Visualization3D_sensor(
			//pangolin::ProjectionMatrixOrthographic(-hspace[0], hspace[0], -hspace[1], hspace[1], -hspace[2], hspace[2]),
			pangolin::ProjectionMatrix(mviewW, mviewH, 1000, 1000, mviewW / 2, mviewH / 2, 0.1, 10000),
			pangolin::ModelViewLookAt(0, 0, 1000.0f, 0, 0, 0, pangolin::AxisY));


		pangolin::View& Visualization3D_display = pangolin::CreateDisplay()
			.SetBounds(0.0, 1.0, pangolin::Attach::Pix(mpanelW), 1.0, -(float)mviewW / mviewH)
			.SetHandler(new pangolin::Handler3D(Visualization3D_sensor));

		// parameter reconfigure gui
		if (mpanelW > 0)
		{
			pangolin::CreatePanel("ui").SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(mpanelW));
		}
		pangolin::Var<bool> start_button("ui.Start", false, false);
		pangolin::Var<bool> stop_button("ui.Stop", false, false);
		pangolin::Var<bool> clear_button("ui.Clear", false, false);

		pangolin::Var<bool> settings_showSensor("ui.Sensor_xx", true, true);
		pangolin::Var<bool> Save_file("ui.Save_file", false, true);


		//pangolin::Var<int> W_int_log("ui.W_step", 5, 1, 1E4, true);
		//pangolin::Var<int> H_int_log("ui.H step", 5, 1, 1E4, true);
		pangolin::Var<int> HAngleResolution("ui.HAngle", m_HAngleResolution, 300, 5000, true);
	//	pangolin::Var<int> int_vpwm("ui.Int_VPwm", 60, 40, 240, true);

		//pangolin::Var<bool> send_pwm("ui.Send_Param", false, false);
		//pangolin::Var<bool> send_Vpwm("ui.Send_VPwm", false, false);
		pangolin::Var<bool> save_scan("ui.Save_Scan", false, false);

		//	pangolin::Var<bool> settings_showFullTrajectory("ui.FullTrajectory", false, true);
		//	pangolin::Var<bool> settings_showActiveConstraints("ui.ActiveConst", true, true);

		while (!pangolin::ShouldQuit() && m_bRunning)
		{
		//	printf("%d\n", (int)int_pwm);

			// Clear entire screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.125f, 0.13f, 0.12f, 1.0f);

			Visualization3D_display.Activate(Visualization3D_sensor);
			//pangolin::BindToContext("pt");

			//if (pangolin::Pushed(send_pwm))
			//{
			////	printf("%d\n", (int)int_pwm);
			//	WaitForSingleObject(hMutex5, INFINITE);
			//	set_pwm = 1;
			//	m_HAngleResolution = (int)HAngleResolution;
			//	ReleaseMutex(hMutex5);

			//}
			if (pangolin::Pushed(start_button))
			{
				WaitForSingleObject(hMutex3, INFINITE);
				saveImage_flag = 1;
				ReleaseMutex(hMutex3);
			}
			if (pangolin::Pushed(stop_button))
			{
				WaitForSingleObject(hMutex3, INFINITE);
				saveImage_flag = 2;
				ReleaseMutex(hMutex3);

			}
			if (pangolin::Pushed(clear_button))
			{
				vpts.clear();
			}
			//static bool saveOnce = 1;
			//if (Save_file.Get())
			//{
			//	if (saveOnce)
			//	{
			//		saveOnce = 0;
			//		SYSTEMTIME st = { 0 };
			//		GetLocalTime(&st);
			//		sprintf(timePath, "./data/%d-%02d-%02d(%02d-%02d-%02d).txt",
			//			st.wYear,
			//			st.wMonth,
			//			st.wDay,
			//			st.wHour,
			//			st.wMinute,
			//			st.wSecond);
			//		out_.open(timePath);
			//	}
			//	WaitForSingleObject(hMutex4, INFINITE);
			//	saveScanFile_flag = 1;
			//	ReleaseMutex(hMutex4);
			//}
			//else
			//{
			//	if (out_.is_open())
			//	{
			//		out_.close();
			//	}
			//	saveOnce = 1;
			//}
			if (pangolin::Pushed(save_scan)) {
				static int numPath = 0;
				char pathBuf[254];
				SYSTEMTIME st = { 0 };
				GetLocalTime(&st);
				sprintf(pathBuf, "./data/scan_%d-%02d-%02d(%02d-%02d-%02d)",
					st.wYear,
					st.wMonth,
					st.wDay,
					st.wHour,
					st.wMinute,
					st.wSecond);
				Visualization3D_display.SaveOnRender(pathBuf);
			}
			if (settings_showSensor.Get())
			{
				glPointSize(2.0f);
				glBegin(GL_POINTS);
				glColor3f(0.9, 0.9, 0.9);
				WaitForSingleObject(hMutex2, INFINITE);
		/*		for (int k = 0; k < vpts.size(); k+=3)
				{
					glVertex3f(vpts[k], vpts[k + 1], vpts[k + 2]);
				}*/
				glPointSize(5.0f);
				glColor3f(0.9, 0, 0);
				for (int k = 0; k < vpts1.size(); k += 3)
				{
					glVertex3f(vpts1[k], vpts1[k + 1], vpts1[k + 2]);
				}
			/*	glPointSize(10.0f);
				glColor3f(0.9, 0.9, 0);
				for (int k = 0; k < vpts2.size(); k += 3)
				{
					glVertex3f(vpts2[k], vpts2[k + 1], vpts2[k + 2]);
				}
*/
				ReleaseMutex(hMutex2);

				glEnd();
			}
			else
			{
				glPointSize(2.0f);
				glBegin(GL_POINTS);
				glColor3f(0.97, 0.85, 0.029);

				WaitForSingleObject(hMutex2, INFINITE);
				smGridCell *gc = gcs_;
				for (int k = 0; k < total_; k++, gc++)
				{
					//if (acceptedVar > 0 && !gc->valid(acceptedVar))
					//	continue;
					if (gc->sw <= 0)	continue;
				
					{
						glVertex3f(gc->cx, gc->cy, gc->sh / gc->sw);
					}
				}
				ReleaseMutex(hMutex2);
				glEnd();

			}
			drawCoordinatesystem(0, 0, 0, 500);
			pangolin::FinishFrame();
		}

		//printf("QUIT Pangolin thread!\n");
		exit(1);
	}

	void updateCloudPoint(float *points, int num)
	{
		//WaitForSingleObject(hMutex2, INFINITE);
		pts = points; npts = num;
		//ReleaseMutex(hMutex2);

	}
	void updateCloudPoint(float *points)
	{
		WaitForSingleObject(hMutex2, INFINITE);
		vpts.push_back(points[0]);
		vpts.push_back(points[1]);
		vpts.push_back(points[2]);
		ReleaseMutex(hMutex2);

	}
	void updateCloudPoint1(float *points)
	{
		WaitForSingleObject(hMutex2, INFINITE);
		vpts1.push_back(points[0]);
		vpts1.push_back(points[1]);
		vpts1.push_back(points[2]);
		ReleaseMutex(hMutex2);

	}
	void updateCloudPoint2(float *points)
	{
		WaitForSingleObject(hMutex2, INFINITE);
		vpts2.push_back(points[0]);
		vpts2.push_back(points[1]);
		vpts2.push_back(points[2]);
		ReleaseMutex(hMutex2);

	}
	void stop()
	{
	/*	if (out_.is_open())
		{
			out_.close();
		}*/
		WaitForSingleObject(hMutex3, INFINITE);
		saveImage_flag = 2;
		ReleaseMutex(hMutex3);
		m_bRunning = false;
		m_thViewer->detach();
		if (m_thViewer) delete m_thViewer; m_thViewer = NULL;

		//ShowWindow(task, SW_SHOW);//显示  
		//ShowWindow(desktop, SW_SHOW);

		::Sleep(1000);

	}

	void drawCoordinatesystem(GLfloat x, GLfloat y, GLfloat z, GLfloat len)
	{
		

		glLineWidth(1.5f);
		glColor4f(1, 0, 0, 1);
		pangolin::glDrawLine(x, y, z, x + len, y, z);

		glColor4f(0, 1, 0, 1);
		pangolin::glDrawLine(x, y, z, x, y + len, z);

		glColor4f(0, 0, 1, 1);
		pangolin::glDrawLine(x, y, z, x, y, z + len);

	}
};