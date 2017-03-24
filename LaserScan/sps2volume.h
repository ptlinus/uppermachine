#pragma once

#include <math.h>
#include <vector>
#include "triangle.h"

class sensorPoint
{
public:
	sensorPoint()	{ x = y = z = r = theta = 0; }
	~sensorPoint()	{}

	float x, y, z;	// x = r cos(theta), y = r sin(theta)
	float r, theta;

	void set(float _distance, float vertRadian, float horRadian)
	{
		float angleAB = -4.1f;// 2.5;// 3.5;
		float dt = vertRadian * 45 / atan(1.0f);
		float angle = (dt + angleAB)*atan(1.0f) / 45;
		theta = horRadian;
		r = _distance * cos(angle);	//sin(pi/2-vertRadian)
		z = _distance * sin(angle);	//cos(pi/2-vertRadian)
		x = r * cos(theta);
		y = r * sin(theta);
	}


	void update(float _z, float _r, float _theta)
	{
		z = _z;	r = _r; theta = _theta;
		x = r * cos(_theta);
		y = r * sin(_theta);
	}

	float xyDistance(sensorPoint &sp)
	{
		float dx = sp.x - x, dy = sp.y - y;
		return sqrt(dx * dx + dy * dy);
	}
};

class spMap
{
public:
	spMap()     {}
	~spMap()    {}

	std::vector<sensorPoint> vsp;
	std::vector<int> vborderIdx;

	void reset()
	{
		vsp.clear(); vborderIdx.clear();
	}

	void append(sensorPoint &sp, bool isborder = false)
	{
		vsp.push_back(sp);
		if (isborder) vborderIdx.push_back(vsp.size() - 1);
	}
};

class sp1dProject
{
	struct histCell{
		float maxZ;
		float minZ;
		float avgZ;
		float avgR;
		int npts;
		bool isMean;
		std::vector<int> idx;

		void reset()
		{
			maxZ = -FLT_MAX;
			minZ = FLT_MAX;
			avgZ = 0.0f;
			avgR = 0.0f;
			npts = 0;
			isMean = false;
		}
		void append(float z, float r)
		{
			if (z > maxZ)
				maxZ = z;
			if (z < minZ)
				minZ = z;
			avgZ += z;
			avgR += r;
			npts += 1;
		}
		bool output(float &z, float &r)
		{
			if (isMean)
			{
				z = avgZ;
				r = avgR;

				return true;
			}
			if (npts > 0)
			{
				avgZ /= npts;
				z = avgZ;
				avgR /= npts;
				r = avgR;

				isMean = true;
				return true;
			}

			return false;
		}
		bool isInvalid(float error)
		{
			float m = avgZ;
			if (!isMean)
				m /= npts;
			if (maxZ - m > error)
				return true;
			return false;
		}
		void update(float _z, float _r)
		{
			avgZ = _z;
			avgR = _r;
			maxZ = _z;
			minZ = _z;
			npts = 1;
			isMean = true;
		}
	};
	float error_;
public:
	sp1dProject()	{}
	~sp1dProject()	{}

	std::vector<histCell> hist;
	float reso;

	void create(float maxR, float device_error, float resolution_factor = 6.0f)
	{
		error_ = device_error;
		reso = device_error * resolution_factor;
		int hist_len = int(maxR / reso) + 1;
		hist.resize(hist_len);
	}
	void filter(spMap &out, sensorPoint *spLine, int num)
	{
		for (int i = 0; i < hist.size(); ++i)
		{
			hist[i].reset();
			hist[i].idx.clear();
		}

		int maxId = -INT_MAX, minId = INT_MAX;
		for (int i = 0; i < num; ++i)
		{
			if (spLine[i].z < 300)
				continue;
			int id = int(spLine[i].r / reso);
			if (id > maxId) maxId = id;
			if (id < minId) minId = id;
			hist[id].append(spLine[i].z, spLine[i].r);
			hist[id].idx.push_back(i);
		}
		for (int i = minId; i < maxId; ++i)
		{
			float z, r;
			hist[i].output(z, r);
			if (hist[i].isInvalid(6 * error_))
				hist[i].avgZ = hist[i].maxZ;
		}
		int nn = minId;
		for (; nn < maxId - 1; ++nn)
		{
			float z, r;
			hist[nn].output(z, r);
			for (int kk = 0; kk < hist[nn].idx.size(); ++kk)
			{
				if (spLine[hist[nn].idx[kk]].z >= 0.98f*z && spLine[hist[nn].idx[kk]].z <= hist[nn].maxZ)
				{
					out.append(spLine[hist[nn].idx[kk]]);
				}
			}
		}

		//for (int i = minId; i < maxId; ++i)
		//{
		//	if (hist[i].isInvalid(5 * error_))
		//		hist[i].idx.clear();
		//}
		//for (int nn = minId; nn < maxId; ++nn)
		//{
		//	for (int kk = 0; kk < hist[nn].idx.size(); ++kk)
		//		out.append(spLine[hist[nn].idx[kk]]);
		//}

		//std::cout<<hist[maxId].minZ<<"\t"<<hist[maxId].maxZ<<"\t"<<hist[maxId].npts<<std::endl;
		//std::cout<<hist[maxId-1].minZ<<"\t"<<hist[maxId-1].maxZ<<"\t"<<hist[maxId-1].npts<<std::endl;
		float last_z = 0.0f;
		int count_z = 0;
		for (; nn <= maxId; ++nn){
			float border_max_limit_z = hist[nn].maxZ;
			float border_min_limit_z = 0.96*hist[nn].maxZ > 0.9*last_z ? 0.96*hist[nn].maxZ : 0.9*last_z;
			hist[nn].reset();
			for (int kk = 0; kk < hist[nn].idx.size(); ++kk)
			{
				if (spLine[hist[nn].idx[kk]].z >= border_min_limit_z && spLine[hist[nn].idx[kk]].z <= border_max_limit_z)
				{
					//hist[nn].append(spLine[hist[nn].idx[kk]].z, spLine[hist[nn].idx[kk]].r);
					out.append(spLine[hist[nn].idx[kk]]);
					last_z += spLine[hist[nn].idx[kk]].z;
					count_z++;
				}
			}

			last_z /= count_z;
		}
		//std::cout<<hist[maxId].minZ<<"\t"<<hist[maxId].maxZ<<"\t"<<hist[maxId].npts<<std::endl;

		/* int forward = 0, tail = 0;

		int inValidPoints = 0;
		//最大最小的z差异点太多就把这条线剔除
		for(int i = minId; i < maxId; ++i)
		{
		if(hist[i].isInvalid(reso)) inValidPoints++;
		}
		if(maxId - minId - inValidPoints < 5) return;

		for(int i = minId; i <maxId; ++i)
		{
		sensorPoint sp;
		if(hist[i].output(sp.z, sp.r))
		{
		sp.update(hist[i].avgZ, hist[i].avgR, spLine[0].theta);
		out.append(sp);
		}
		else
		{
		forward = i ? 1 : 0;
		tail = 1;
		while(hist[i+tail].npts == 0 && (i+tail < maxId-1))
		{
		tail++;
		}
		if(i+tail >= (maxId-1))
		tail = 0;
		float ratio = float(forward)/(forward+tail);
		float fz, fr, tz, tr;
		hist[i-forward].output(fz, fr);
		hist[i+tail].output(tz, tr);
		float z =  ratio * fz + (1-ratio)*tz;
		float r = ratio * fr + (1-ratio)*tr;
		sp.update(z, r, spLine[0].theta);
		hist[i].update(z, r);
		out.append(sp);
		}
		}


		sensorPoint sp;
		hist[maxId].output(sp.z, sp.r);
		if(hist[maxId].maxZ - hist[maxId].minZ > reso)
		{
		float ratio = 0.8f;
		//std::cout<<hist[maxId-1].avgZ << "\t"<<hist[maxId].maxZ<<std::endl;
		float z = ratio * hist[maxId-1].avgZ + (1-ratio)*hist[maxId].maxZ;
		float r = hist[maxId].avgR;
		sp.update(z, r, spLine[0].theta);
		out.append(sp, true);
		}
		else
		{
		sp.update(hist[maxId].avgZ, hist[maxId].avgR, spLine[0].theta);
		out.append(sp, true);
		}*/
	}
};

class sps2volume
{
public:
	sps2volume()	{}
	~sps2volume()	{}

	double execute(sensorPoint *spList, int num)
	{
		if (!spList || num < 3)	return -1.0f;

		std::vector<float> vxy(num + num);
		triangulateio in, out;
		//inputs
		in.numberofpoints = num;
		in.pointlist = &vxy[0];
		sensorPoint *sp = spList; float *pi = in.pointlist;
		for (int k = 0; k < num; k++, sp++)
		{
			*pi++ = (float)sp->x;	*pi++ = (float)sp->y;
		}
		in.numberofpointattributes = 0;
		in.pointattributelist = NULL;
		in.pointmarkerlist = NULL;
		in.numberofsegments = 0;
		in.numberofholes = 0;
		in.numberofregions = 0;
		in.regionlist = NULL;
		//outputs
		out.numberoftriangles = 0;
		out.pointlist = NULL;
		out.pointattributelist = NULL;
		out.pointmarkerlist = NULL;
		out.trianglelist = NULL;
		out.triangleattributelist = NULL;
		out.neighborlist = NULL;
		out.segmentlist = NULL;
		out.segmentmarkerlist = NULL;
		out.edgelist = NULL;
		out.edgemarkerlist = NULL;
		triangulate("zQB", &in, &out, NULL);

		double vol = 0; int *vertex = out.trianglelist;
		for (int k = 0; k < out.numberoftriangles; k++, vertex += 3)
			vol += volTriPrism(spList + vertex[0], spList + vertex[1], spList + vertex[2]);

		if (out.pointlist)		free(out.pointlist);
		if (out.trianglelist)	free(out.trianglelist);
		return vol;
	}
private:
	float volTriPrism(sensorPoint *sp0, sensorPoint *sp1, sensorPoint *sp2)
	{
		float at = areaTriangle(sp0, sp1, sp2);
		float uz = (sp0->z + sp1->z + sp2->z) / 3;
		return at * uz;
	}
	float areaTriangle(sensorPoint *sp0, sensorPoint *sp1, sensorPoint *sp2)
	{
		float a = sp0->xyDistance(*sp1);
		float b = sp1->xyDistance(*sp2);
		float c = sp2->xyDistance(*sp0);
		float p = 0.5f*(a + b + c);
		return sqrt(p*(p - a)*(p - b)*(p - c));
	}
};
