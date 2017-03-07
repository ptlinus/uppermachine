#pragma once

#include<vector>
#include<iostream>
#include<fstream>
#include<string>
#include <math.h>
struct pointStruct{
	int theta_h;
	int theta_v;
	int len;
};
class hPointCloud
{
public:
	hPointCloud()	{ pts = NULL; num = 0; }
	~hPointCloud()	{}

	float	*pts;
	int		num;

};

class pointCloudFile : public hPointCloud
{


	std::vector<float> mvptsBuf;
public:
	pointCloudFile()	{ }
	~pointCloudFile()	{}

	bool open(char *filename)
	{
		std::vector<struct pointStruct> vrpts;
		loadData(vrpts, filename);
		convert(vrpts);
		return pts && num > 0;
	}
	

	void addPoint(struct pointStruct &vpts)
	{
		float plus2radialH = 8.0f*atan(1.0f) / 24000.0f;
		float plus2radialV = 8.0f*atan(1.0f) / 8000.0f;



		float theta0 = -3.6f * atan(1.0f) / 45.0f;
		float *pp = pts; pointStruct *pt = &vpts;
		{
			float theta = plus2radialV * (pt->theta_v * 2) + theta0;
			float phi = plus2radialH * (pt->theta_h * 2);
			float rs = pt->len * cos(theta);
			mvptsBuf.push_back( rs * cos(phi));// +b*sin(phi) + c*cos(phi);
			mvptsBuf.push_back(rs * sin(phi));// +b*cos(phi) + c*sin(phi);
			mvptsBuf.push_back(pt->len * sin(theta));// +a;
		}
		num = mvptsBuf.size();
		pts = &mvptsBuf[0];

	}
private:
	void convert(std::vector<struct pointStruct> &vpts)
	{
		if (vpts.empty())	return;

		float plus2radialH = 8.0f*atan(1.0f) / 24000.0f;
		float plus2radialV = 8.0f*atan(1.0f) / 8000.0f;


		num = (int)vpts.size();
		mvptsBuf.resize(num * 3);
		pts = &mvptsBuf[0];

		float theta0 = -3.6f * atan(1.0f) / 45.0f;
		float *pp = pts; pointStruct *pt = &vpts[0];
		for (int i = 0; i < num; i++, pt++, pp+=3)
		{
			float theta = plus2radialV * (pt->theta_v * 2) + theta0;
			float phi = plus2radialH * (pt->theta_h * 2);
			float rs = pt->len * cos(theta);
			pp[0] = rs * cos(phi);// +b*sin(phi) + c*cos(phi);
			pp[1] = rs * sin(phi);// +b*cos(phi) + c*sin(phi);
			pp[2] = pt->len * sin(theta);// +a;
		}
	}
	void loadData(std::vector<struct pointStruct> &vpts, char *filename)
	{
		vpts.clear();
		// open the file
		std::ifstream inFile(filename);
		if (!inFile) return;
		int ss = 0;
		//	 // go through the dataset file
		int error_t = 0;
		int sumCnt = 0;
		while (!inFile.eof())
		{
			/*	if (++sumCnt == 1600)
			{
			break;
			std::cout << std::endl;
			}*/
			std::string line;
			getline(inFile, line);
			std::vector<std::string> datBuf;// = split(line, " ");
			StringSplit(line, ' ', datBuf);
			//std::vector<std::wstring> vecSegTag;
			// boost::is_any_of这里相当于分割规则了     
			//boost::split(datBuf, line, boost::is_any_of((" ")));
			if (datBuf.size() == 6)
			{
				int Sdat = std::stoi(datBuf[0]);

				int Hdat = std::stoi(datBuf[1]);
				//	if (Hdat > 1600)
				{
					//		break;
				}
				int Vdat = std::stoi(datBuf[2]);
				int Rdat = std::stoi(datBuf[3]);;
				int XORdat = std::stoi(datBuf[4]);;
				int Edat = std::stoi(datBuf[5]);;
				int xor = Hdat^Vdat;
				if (xor == XORdat && Sdat == 11 && Edat == 22)
				{
					struct pointStruct dat;

					dat.theta_h = Hdat;
					dat.theta_v = Vdat;
					dat.len = Rdat;
					vpts.push_back(dat);
				}
				else
				{
					error_t++;
				}
			}
			else
			{
				error_t++;
			}
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
};