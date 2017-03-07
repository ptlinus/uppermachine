#pragma once
#include <math.h>
#include <stdio.h>
#include <vector>
#pragma warning(disable: 4996)

template<typename bType>
class xbufManager
{
	char	*mpBuf;
	int		mSize, mAlign;
public:
	xbufManager()	{ mpBuf = NULL; mSize = 0; mAlign = 16; }
	~xbufManager()	{ destroy(); }

	void destroy()		{ if (mpBuf)	{ free(mpBuf); mpBuf = NULL; } }
	inline bool valid()	{ return mpBuf != NULL; }

	bool create(int size, bool keepPoint = false)
	{
		if (size <= 0)	return false;
		int nbytes = size*sizeof(bType) + mAlign;
		if (!mpBuf)
		{
			if ((mpBuf = (char *)malloc(nbytes)) == NULL)
				return false;
			mSize = nbytes;
		}
		if (nbytes > mSize)
		{
			if (keepPoint)
			{
				if ((mpBuf = (char *)realloc(mpBuf, nbytes)) == NULL)
					return false;
			}
			else
			{
				free(mpBuf); mpBuf = NULL;
				if ((mpBuf = (char *)malloc(nbytes)) == NULL)
					return false;
			}
			mSize = nbytes;
		}
		return true;
	}
	inline bType *ptr()		{ return (bType *)(((size_t)mpBuf + mAlign - 1) & -mAlign); }
};
//-------------------------------//
class sphericalPoint
{
public:
	sphericalPoint()	{ r = phi = theta = 0; intensity = 1.0f; }
	~sphericalPoint()	{}
	float r;
	float phi;		//[0,2pi)
	float theta;	//[0,pi)
	float intensity;

	void cartesian(float &x, float &y, float &z)
	{
		float rs = r * cos(theta);	//sin(pi/2-theta)
		x = rs * cos(phi);
		y = rs * sin(phi);
		z = r * sin(theta);	//cos(pi/2-theta)
	}
};
class sphericalPack
{
	xbufManager<sphericalPoint> mbuf;
public:
	sphericalPack()		{ clear(); }
	~sphericalPack()	{}

	int idSensor;			//传感器标识
	int num;				//包含点数
	sphericalPoint *sps;	//球面点系列

	void clear()	{ idSensor = -1; num = 0; sps = NULL; }
	bool valid()	{ return num > 0 && sps != NULL;  }

	void simulate(int maxSensor, float maxR)
	{
		int nn = 360;
		num = 0;
		mbuf.create(nn);
		sps = mbuf.ptr();

		idSensor = rand() % maxSensor;

		float dr = maxR / 255.0f, pi = atan(1.0f) * 4, dt = pi / nn;
		float phi = (rand() % 360) * (pi + pi) / 360, ta = 0;
		sphericalPoint *sp = sps;
		for (int k = 0; k < nn; k++, sp++, ta += dt)
		{
			sp->r =  (rand() % 256) * dr;
			sp->phi = phi;
			sp->theta = ta;
			sp->intensity = (float)(rand() % 100) + 1.0f;
			num++;
		}
	}
};
//-------------------------------//
class sensorProfile
{
public:
	sensorProfile()		{ id = -1; }
	~sensorProfile()	{}

	int id;
	float fixedPosition[3];

	void set(int idCode, float positon[3])
	{
		id = idCode;
		fixedPosition[0] = positon[0];
		fixedPosition[1] = positon[1];
		fixedPosition[2] = positon[2];
	}

	bool read(FILE *fp, float xlimit, float ylimit, float zlimit)
	{
		if (!fp)	return false;
		char title[80];
		if (5 != fscanf(fp, "%s %d %f %f %f\n", title, &id, &fixedPosition[0], &fixedPosition[1], &fixedPosition[2])
			|| stricmp(title, "sensor") != 0
			|| fixedPosition[0] < 0 || fixedPosition[0] > xlimit
			|| fixedPosition[1] < 0 || fixedPosition[1] > ylimit
			|| fixedPosition[2] < 0 || fixedPosition[2] > zlimit)
			return false;
		return true;
	}
	bool write(FILE *fp)
	{
		if (!fp)	return false;
		return 5 == fprintf(fp, "sensor %d %f %f %f\n", id, fixedPosition[0], fixedPosition[1], fixedPosition[2]);
	}
};
class sensorLayout
{
public:
	sensorLayout()	{ clear(); }
	~sensorLayout()	{}

	int num;
	sensorProfile sensors[128];

	void clear()	{ num = 0; }
	sensorProfile* get(int id)
	{
		for (int k = 0; k < num; k++)
		{
			if (sensors[k].id == id)	return &sensors[k];
		}
		return NULL;
	}

	void appendSensor(int idSensor, float fixedPosition[3])
	{
		if (num >= 128)	return;

		int idx = -1;
		for (int k = 0; k < num; k++)
		{
			if (sensors[k].id == idSensor)	{ idx = k; break; }
		}
		if (idx >= 0)
		{
			sensors[idx].set(idSensor, fixedPosition);
		}
		else
		{
			sensors[num].set(idSensor, fixedPosition);
			num++;
		}
	}

	bool read(FILE *fp, float xlimit, float ylimit, float zlimit)
	{
		num = 0;
		if (!fp)	return false;
		while (num < 128)
		{
			if (sensors[num].read(fp, xlimit, ylimit, zlimit))
				num++;
			else
				break;
		}
		return num > 0;
	}
	bool write(FILE *fp)
	{
		if (!fp)	return false;

		bool bRet = true;
		for (int k = 0; bRet && k < num; k++)
			bRet &= sensors[k].write(fp);

		return bRet;
	}
};
//-------------------------------//
class smGridCell
{
public:
	smGridCell()	{ clear(); }
	~smGridCell()	{}

	float cx, cy, area;	//网格中心与面积
	int num;			//投中点数
	float sh, sw;		//投中累加权值与权重
	float shh, h0, h1;	

	void clear()									{ cx = cy = area = 0; clearGrid();	}
	void clearGrid()								{ sh = sw = 0; num = 0; shh = 0;	}
	void disable()									{ area = 0; num = -1; }
	void set(float xc, float yc, float areaCell)	{ cx = xc; cy = yc; area = areaCell; clearGrid(); }
	void update(float v, float weight = 1.0f)		
	{ 
		if (v <= 0)		return;
		
		float vw = v*weight;
		sh += vw; sw += weight; shh += v*v*weight;
		num++;
	}
	float variance()	
	{
		return (num <= 0 || sw <= 0) ? -1.0f : (shh - sh*sh / sw) / sw;
	}
	bool isEnable()	{ return area > 0; }
	bool valid(float acceptedVar)	
	{
		float vh = variance();
		return vh > 0 && vh < acceptedVar;
	}
	float geth()		{ return sw <= 0 ? 0 : sh / sw; }
	float volume()		{ return sw <= 0 ? 0 : area * sh / sw;	}
	
	void filter(float normdd, smGridCell *gcs, int offset[], int nn, int filterMask)
	{
		if (!gcs || nn <= 0)	return;

		for (int k = 0; k < nn; k++)
		{
			smGridCell *gc = gcs + offset[k];
			float wd = 1.0f - dist2(gc)*normdd;
			update(gc->geth(), wd);
		}
		num = filterMask;
	}
private:
	float dist2(smGridCell *gc)
	{
		float dx = gc->cx - cx, dy = gc->cy - cy;
		return dx*dx + dy*dy;
	}
};

smGridCell *gcs_ = NULL;
int total_;	//网格数
class statisticMapper
{
	struct{
		float normdd;
		int nr, nn, dx[100], dy[100], offset[100];
		void init(int r, int stride, float wcell, float hcell)
		{
			if (r < 1)	r = 1;
			if (r > 5)	r = 5;
			if (r == nr) return;
			nr = r; nn = 0;
			normdd = 1.0f / ((r*wcell + 0.5f)*(r*hcell + 0.5f));
			int yp = -r * stride, rr = r*r;
			for (int j = -r; j <= r; j++, yp += stride)
			{
				int jj = j*j;
				for (int i = -r; i <= r; i++)
				{
					if (i*i + jj > rr)	continue;
					dx[nn] = i; dy[nn] = j; offset[nn] = yp + i; nn++;
				}
			}
		}
	}m_nn;

	xbufManager<smGridCell> mbuf;
	float mwCell, mhCell;
	float mXlen, mYlen, mZlen, mMargin[4], mRadius, mRR;
	int mType;
public:
	statisticMapper()	{ mType = smUNKONW;  m_nn.nr = -1; xNum = yNum = total = 0; gcs = NULL; }
	~statisticMapper()	{}

	enum { smUNKONW = -1, smCUBIC = 0, smCOLUMN };

	sensorLayout slayout;
	int xNum, yNum, total;	//网格数
	smGridCell *gcs;		//格子系列
	//----------------------------------------//
	bool createCubic(float wGrid, float hGrid, float xLength, float yLength, float zLength)
	{
		gcs = NULL; mType = smUNKONW;
		if (wGrid <= 0 || hGrid <= 0)	return false;

		mXlen = xLength; mYlen = yLength; mZlen = zLength;
		xNum = (int)(xLength / wGrid + 0.5f);
		yNum = (int)(yLength / hGrid + 0.5f);
		if (xNum <= 0 || yNum <= 0)		return false;
		total = xNum * yNum;
		total_ = total;
		if (!mbuf.create(total))		return false;
		gcs = mbuf.ptr();

		mType = smCUBIC;
		mMargin[0] = -mXlen*0.5f;	mMargin[1] = -mYlen*0.5f;
		mMargin[2] = -mMargin[0];	mMargin[3] = -mMargin[1];

		mwCell = mXlen / xNum;	mhCell = mYlen / yNum;
		smGridCell *gc = gcs;
		gcs_ = gcs;
		float cellArea = mwCell * mhCell, yc = mMargin[1] + mhCell*0.5f, xc0 = mMargin[0] + mwCell*0.5f;
		for (int j = 0; j < yNum; j++, yc += mhCell)
		{
			float xc = xc0;
			for (int i = 0; i < xNum; i++, xc += mwCell, gc++)
				gc->set(xc, yc, cellArea);
		}
		return true;
	}
	bool createColumn(float wGrid, float hGrid, float radius, float zLength)
	{
		gcs = NULL; mType = smUNKONW;
		if (wGrid <= 0 || hGrid <= 0)	return false;

		mRadius = radius;  mXlen = radius + radius + 1.0f;	mYlen = mXlen; mZlen = zLength; mRR = radius*radius;

		xNum = (int)(mXlen / wGrid + 0.5f);
		yNum = (int)(mYlen / hGrid + 0.5f);
		if (xNum <= 0 || yNum <= 0)		return false;
		total = xNum * yNum;
		total_ = total;

		if (!mbuf.create(total))		return false;
		gcs = mbuf.ptr();

		mType = smCOLUMN;
		mMargin[0] = -mXlen*0.5f;	mMargin[1] = -mYlen*0.5f;
		mMargin[2] = -mMargin[0];	mMargin[3] = -mMargin[1];

		mwCell = mXlen / xNum;	mhCell = mYlen / yNum;
		smGridCell *gc = gcs;
		gcs_ = gcs;

		float cellArea = mwCell * mhCell, yc = mMargin[1] + mhCell*0.5f, xc0 = mMargin[0] + mwCell*0.5f;
		for (int j = 0; j < yNum; j++, yc += mhCell)
		{
			float xc = xc0, dyy = yc*yc;
			for (int i = 0; i < xNum; i++, xc += mwCell, gc++)
			{
				gc->set(xc, yc, cellArea);
				if (xc*xc + dyy > mRR)	gc->disable();
			}		
		}
		return true;
	}
	//----------------------------------------//
	void setSensor(int idSensor, float fixedPosition[3])
	{
		slayout.appendSensor(idSensor, fixedPosition);
	}
	//----------------------------------------//
	void update(int idSensor, float r, float phi, float theta)
	{
		if (r <= 0)	return;

		float wgt = 500.0f / (r + 0.1f);
		if (wgt > 0.95f)		wgt = 0.95f;
		else if (wgt < 0.05f)	wgt = 0.05f;

		float rs = r * cos(theta);
		update(idSensor, rs * cos(phi), rs * sin(phi), r * sin(theta), wgt);
	}
	void update(int idSensor, float x, float y, float z, float weight = 1.0f)
	{
		sensorProfile *sp = slayout.get(idSensor);
		if (!sp)	return;

		int ic = mapCellIndex(x + sp->fixedPosition[0], y + sp->fixedPosition[1]);
		if (ic < 0 || !gcs[ic].isEnable())	return;
		gcs[ic].update(sp->fixedPosition[2] - z, weight);
	}
	//----------------------------------------//
	float volume()
	{
		float sh = 0;
		for (int k = 0; k < total; k++)
			sh += gcs[k].geth();
		return sh * mwCell * mhCell;
	}
	//----------------------------------------//
	void postprocess(float acceptedVar, int maxIteration)
	{
		if (acceptedVar > 0)
		{
			for (int k = 0; k < total; k++)
			{
				if (!gcs[k].valid(acceptedVar))
					gcs[k].clearGrid();
				else
				{
					gcs[k].num += 50;
					if (gcs[k].num > 255)	gcs[k].num = 255;
				}
			}
		}

		if (maxIteration <= 0)	return;

		int nHoles = 0;
		for (int k = 0; k < total; k++)
		{
			if (gcs[k].num <= 0)	nHoles++;
		}
		if (nHoles <= 0)	return;

		xbufManager<int> buf;
		if (!buf.create(nHoles * 3))	return;

		int *pos = buf.ptr(), *pp = pos, idx = 0;
		for (int j = 0; j < yNum; j++)
		{
			for (int i = 0; i < xNum; i++, idx++)
			{
				if (gcs[idx].num > 0)	continue;
				pp[0] = i; pp[1] = j; pp[2] = idx; pp += 3;
			}
		}

		int rfilter = 2;	m_nn.init(rfilter, xNum, mwCell, mhCell);
		int offset[100], iter = 0, fltMask = -112358;
		while (nHoles > 0 && iter < maxIteration)
		{
			int leave = 0, *npp = pos;	pp = pos;
			for (int k = 0; k < nHoles; k++, pp += 3)
			{
				int nn = findValidNeighbor(offset, gcs, pp[2], pp[0], pp[1]);
				if (nn <= 0)
				{
					*npp++ = pp[0];	*npp++ = pp[1];	*npp++ = pp[2]; leave++;
				}
				else
				{
					gcs[pp[2]].filter(m_nn.normdd, gcs, offset, nn, 1);
				}
			}
			//	printf("%d / %d  [%d][%d]\n", leave, nHoles, nHoles - leave, nHoles / 100 + 1);
			if (nHoles - leave <= 50)
			{
				rfilter++;	m_nn.init(rfilter, xNum, mwCell, mhCell);
			}
			/*pp = pos;
			for (int k = 0; k < nHoles; k++, pp += 3)
			{
			if (gcs[pp[2]].num == fltMask)	gcs[pp[2]].num = 1;
			}*/
			nHoles = leave;	iter++;
		}
	}
	//----------------------------------------//
	int xyzList(std::vector<float> &vxyz, float acceptedVar)
	{
		vxyz.clear();
		if (total <= 0)	return 0;
		int C = 4;
		vxyz.resize(total * C);
		int nn = 0;
		float *po = &vxyz[0]; smGridCell *gc = gcs;
		for (int k = 0; k < total; k++, gc++)
		{
			if (acceptedVar > 0 && !gc->valid(acceptedVar))
				continue;

			po[0] = gc->cx;
			po[1] = gc->cy;
			po[2] = gc->sh / gc->sw;
			po[3] = (float)gc->num;
			po += C; nn++;
		}
		if (nn != total)	vxyz.resize(nn * C);
		return C;
	}
private:
	int mapCellIndex(float x, float y)
	{
		switch (mType)
		{
		case smCUBIC:		
			if (x < mMargin[0] || x > mMargin[2] || y < mMargin[1] || y > mMargin[3])	return -1;
			break;
		case smCOLUMN:		
			if (x*x + y*y > mRR)														return -1;
			break;
		}
		return ((int)((y - mMargin[1]) / mhCell))*xNum + ((int)((x - mMargin[0]) / mwCell));
	}
	int findValidNeighbor(int offset[], smGridCell *ptr, int cp, int cx, int cy)
	{
		int nn = 0;
		for (int k = 0; k < m_nn.nn; k++)
		{
			int x = cx + m_nn.dx[k];
			if (x < 0 || x >= xNum)	continue;
			int y = cy + m_nn.dy[k];
			if (y < 0 || y >= yNum)	continue;
			int np = cp + m_nn.offset[k];
			if (ptr[np].num <= 0)	continue;

			offset[nn++] = np;
		}
		return nn;
	}
};

//-------------------------------//
/*
void ttcsm()
{
	cubicStatisticMapper csm;

	float fp[3] = { 0, 0, 0 };
	csm.appendSensor(0, fp);
	csm.create(10, 10, 3000, 3000, 1000);

	//for (int k = 0; k < npts; k++)
	//	csm.update(0, r, phi, theta);

	std::vector<float> vxyz;
	csm.xyzList(vxyz, 900.0f);

}
*/