//glfw version
#ifdef SIMPLEDRAW

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  include <windows.h>
#endif

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <float.h>
#include <string>
#include <array>
#include <unistd.h>

#include <pthread.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GL/glfw.h>
#else
#include <GLFW/glfw3.h>
#endif
//#include <rendercheck_gl.h>

#include <sstream>
#include <string>
#include <vector>
//#include <iostream>

namespace simpledraw_impl
{
//using namespace std;
static char* title;

//2d colormaps:
static GLuint* texture;
static int*			texMAGinfo;
static int*    texUPD;
static int*    texSZ;
static float*    texMAG;
static float*    texPOS;
static float** texturesDataArrs;
static void** DataArrs;
static int* DataType;

//particles and 1d plots structures:
typedef float color[3]; 
typedef float bounds[4]; //x0,y0,x1,y1

static int*    particleCloudNum; // number of particle clouds for all tiles
static bounds*    particleBounds; // bounds for all clouds for each tile
static int** particlesSZ; // number of particles for each cloud
static float*** particles; // particles pointers
static color** particlesColor; // particles color

//1d plots:
static int*    plotNum; // number of plots clouds for all tiles
static bounds*    plotBounds; // bounds for all plots for each tile
static int**    plotsSZ; // size of plots
static float***    plots; // plots data pointers
static color** plotsColor; // plot color

static int* close_win;
static pthread_mutex_t				close_win_mutex;
static pthread_mutex_t				data_update_mutex;

static int XSize = 512;
static int YSize = 512;
static int XnewSize = XSize;
static int YnewSize = YSize;
static int Nx;
static int Ny;
static int count;
static int side_max;
static double xmouse, ymouse;
static float pdx_drag, pdy_drag;
static volatile int initedfirst=0;
static	int *inited;
static int *tileUsage; // array of tile size with usage flag: -1 - not used, 0 - 2d data 1 - points in X-Y, 2 - 1d plots

void	showMessage(GLfloat, GLfloat, const char*);

static float red(float val, float max)
{
	if(val>0)	return sqrt(val/max);
	else return (val/max)*(val/max)*0.35;
}

static float green(float val, float max)
{
	if(val<0) return (val/max)*(val/max)*0.75+sqrt(-val/max)*0.25;
	else	return (val/max)*(val/max);
}
static float blue(float val, float max)
{
	if(val<0)	return sqrt(-val/max);
	else return 0.0;
}

static bool draG=false;
static int wheel_state=0;
static int xi=0;
static int yi=0;
static int sq=0;
static float xxx=0;
static float yyy=0;
static	int vh=0;

#define IMPOSSIBLE_FLOAT 13131313.0f // this is the end marker for charmap contour

struct point
{
	float x;
	float y;
	bool end() const
	{
		return x == IMPOSSIBLE_FLOAT && y == IMPOSSIBLE_FLOAT;
	}
};

static point*** charmap;
}//namespace

#ifdef __MINGW32__
#define FONT_START binary_font_dat_start
#define FONT_END binary_font_dat_end
#else
#define FONT_START _binary_font_dat_start
#define FONT_END _binary_font_dat_end
#endif

extern char FONT_START;
extern char FONT_END;

__attribute__((constructor)) void fadey_onload()
{
	simpledraw_impl::charmap = new simpledraw_impl::point**[256];
	for(size_t i = 0; i != 256; i++)
		simpledraw_impl::charmap[i] = nullptr;
	
  //printf("loading simpledrawlibrary, loading embedded font of size %ld\n",(size_t) (&FONT_END) - (size_t) (&FONT_START));

	std::stringstream ss;
	ss.write(&FONT_START, (size_t) (&FONT_END) - (size_t) (&FONT_START));
	char curchar = 0;
	std::vector<std::vector<simpledraw_impl::point>> curdrawing;
	int i = 0;
	simpledraw_impl::point pt;
	
	while(!ss.eof())
	{
		std::string s;
		ss >> s;
		if(s.c_str()[0] == '\'')
		{
			if(curchar != 0)
			{
				simpledraw_impl::charmap[curchar] = new simpledraw_impl::point*[curdrawing.size() + 1];
				size_t i(0);
				for(; i != curdrawing.size(); i++)
				{
					size_t j(0);
					simpledraw_impl::charmap[curchar][i] = new simpledraw_impl::point[curdrawing[i].size() + 1];
					for(; j != curdrawing[i].size(); j++)
						simpledraw_impl::charmap[curchar][i][j] = curdrawing[i][j];
					simpledraw_impl::charmap[curchar][i][j].x = IMPOSSIBLE_FLOAT;
					simpledraw_impl::charmap[curchar][i][j].y = IMPOSSIBLE_FLOAT;
				}
				simpledraw_impl::charmap[curchar][i] = nullptr;
				curdrawing.clear();
			}
			curchar = s.c_str()[1];
		}
		else if(s == "c")
			curdrawing.push_back(std::vector<simpledraw_impl::point>());
		else
		{
			if(i==0)
			{
				std::stringstream _ss(s);
				_ss >> pt.x;
			}
			else
			{
				std::stringstream _ss(s);
				_ss >> pt.y;
				if(curdrawing.size())
					curdrawing.back().push_back(pt);
				else
					printf("check font.datpossibly you forgot to add 'c' for path after character header\n", pt.x, pt.y);
			}
			i = i == 0  ? 1 : 0;
		}
	}

/*
	for(size_t i = 0; i != 256; i++)
	{
		if(simpledraw_impl::charmap[i] == nullptr)
			continue;
		printf("%c: ",i);
		printf("{");
		size_t j(0);
		while(simpledraw_impl::charmap[i][j])
		{
			size_t k(0);
			printf("{");
			while(!simpledraw_impl::charmap[i][j][k].end())
			{
				printf("%g:%g ", simpledraw_impl::charmap[i][j][k].x, simpledraw_impl::charmap[i][j][k].y);
				k++;
			}
			printf("}");
			j++;
		}
		printf("}\n");
	}
*/
}


namespace simpledraw_impl
{
static void mouse_wheel(GLFWwindow* win, double new_wheel_pos,  double new_wheel_pos_tr)
{
		double wheel = new_wheel_pos_tr;//-wheel_state;
		double x,y;
		glfwGetCursorPos(win, &x, &y);
		float xa = float(x) / float(XnewSize);
		float ya = float(y) / float(YnewSize);
		int ii = floor(xa * float(side_max));
		int jj = floor(ya * float(side_max));
		int number = ii +jj * side_max;
		float pmag = texMAG[number];
		texMAG[number] += (float) wheel / 13.0f;
		float lxa = xa * float(side_max) - float(ii);
		float lya = ya * float(side_max) - float(jj);
		texPOS[number * 2] += lxa * (pmag - texMAG[number]);
		texPOS[number * 2 + 1] += lya * (pmag - texMAG[number]);
		wheel_state = new_wheel_pos;
		texMAGinfo[number] = 113;
		//printf("texposx=%f,\ttexposy=%f,\tmag=%f\n",texPOS[number*2],texPOS[number*2+1],texMAG[number]);
}

static void mouse_drag(GLFWwindow* win, int button, int PressRelease, int mod)
{
	if(button==GLFW_MOUSE_BUTTON_LEFT)
	{
		glfwGetCursorPos(win, &xmouse, &ymouse);
		pdx_drag = 0.0f;
		pdy_drag = 0.0f;
		if (PressRelease==GLFW_PRESS) draG=true;
		else if (PressRelease==GLFW_RELEASE) draG=false;
	}
	if((button==GLFW_MOUSE_BUTTON_RIGHT)&(PressRelease==GLFW_PRESS)) vh=(vh<1)?vh+1:-1;
}

static void mouse_move(GLFWwindow* win, double x, double y)
{
	if(draG==true)
	{
		float xa=float(xmouse)/float(XnewSize);
		float ya=float(ymouse)/float(YnewSize);
		int ii=floor(xa*float(side_max));
		int jj=floor(ya*float(side_max));
		int number=ii+jj*side_max;
		if(number>=count)
			return;
		float dx = (float) x - (float) xmouse;
		float dy = (float) y - (float) ymouse;
		//printf("GL: drag x=%d y=%d\n",dx,dy);
		float dxa = dx/float(XnewSize)*float(side_max)*texMAG[number];
		float dya = dy/float(YnewSize)*float(side_max)*texMAG[number];
		texPOS[number*2]-=dxa-pdx_drag;
		texPOS[number*2+1]-=dya-pdy_drag;	
		pdx_drag = dxa;
		pdy_drag = dya;
		//printf("texposx=%f,\ttexposy=%f\n",texPOS[number*2],texPOS[number*2+1]);
		texMAGinfo[number] = 113;
	}
//	else
	{
		float xa = (float) x / (float) XnewSize;
		float ya = (float) y / (float) YnewSize;
		int ii=floor(xa*float(side_max));
		int jj=floor(ya*float(side_max));
		sq=ii+jj*side_max;
		if(sq>=count)
		{
			sq=-1;
			return;
		}
		xxx=xa*float(side_max)-floor(xa*float(side_max));
		yyy=ya*float(side_max)-floor(ya*float(side_max));
		xi=floor((xxx*texMAG[sq]+texPOS[sq*2]-floor(xxx*texMAG[sq]+texPOS[sq*2]))*texSZ[sq*2]);
		yi=floor((yyy*texMAG[sq]+texPOS[sq*2+1]-floor(yyy*texMAG[sq]+texPOS[sq*2+1]))*texSZ[sq*2+1]);

	}

}

static void win_sz(GLFWwindow* win, int x, int y)
{
		glViewport(0,0,x,y);
		XnewSize=x;
		YnewSize=y;
//		printf("GL (Resize): XSize=%d\n",XSize);
}

static void 
onwinclose(GLFWwindow* win)
{
	inited[0]=0;
}


bool ifInBounds(float x, float y, bounds bb)
{
	if((x>=bb[0])&(x<=bb[2])&(y>=bb[1])&(y<=bb[3]))
		return true;
	else
		return false;
}

void transformTex(float* x, float* y, float texposx, float texposy, float texmag, float dx, float dy)
{
	*x+=-texposx*dx;	
	*y+=-texposy*dy;
	*x/=texmag;
	*y/=texmag;
}

bool crossLineWithBB(float* x0, float* y0, float* x1, float* y1, bounds bb)
{
	int i=0;
	float* p[4];
	float po[4];
	p[0]=x0;
	p[1]=y0;
	p[2]=x1;
	p[3]=y1;
	//find good point
	if(ifInBounds(*x0,*y0,bb))
	{
		po[i*2]=*x0;
		po[i*2+1]=*y0;
		i++;
	}
	if(ifInBounds(*x1,*y1,bb))
	{
		po[i*2]=*x1;
		po[i*2+1]=*y1;
		i++;
	}
	int j=0;
	while((i<2)&(j<4))
	{
		//j=0,2 : /(x0-x1) j=0 xmin=bb[0], j=2 xmax=bb[2]
		//j=1,3 : /(y0-y1) j=0 ymin=bb[1], j=3 ymax=bb[3]
		float c=((bb[j]-*p[2+j%2])**p[1-j%2]+*p[2+1-j%2]*(*p[j%2]-bb[j]))/(*p[j%2]-*p[2+j%2]);
		//printf("j=%d cross is %f line(%g,%g)--(%g,%g) crossing %g\n",j,c,*p[j%2],*p[1-j%2],*p[2+j%2],*p[2+1-j%2],bb[j]);
		if(((c-bb[2+1-j%2])*(c-bb[1-j%2])<=0)&((c-*p[1-j%2])*(c-*p[2+1-j%2])<=0))
		{
			po[i*2+j%2]=bb[j];
			po[i*2+1-j%2]=c;
			//printf("real cross found\n");
			i++;
		}
		j++;
	}
	if(i==1);
		//printf("simpledraw2D: error occured during crossing line (%g,%g)--(%g,%g) with bb (%g,%g)-(%g,%g) found only one cross\n",*x0,*y0,*x1,*y1,bb[0],bb[1],bb[2],bb[3]);
	else if(i==2)
	{
		*x0=po[0];
		*y0=po[1];
		*x1=po[2];
		*y1=po[3];
		return true;
	}
	return false;
		
}

static void 
display(GLFWwindow* win)
{
	//printf("display %d\n",rand());
	glMatrixMode (GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	glClearColor(.3, .3, .3, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	int ii,jj;
	int x=0;
	int y=0;
	int dx = (int) floor((float) XSize / (float) side_max);
	int dy = (int) floor((float) YSize / (float) side_max);

	bounds tbb; // setting tile bounds
	tbb[0]=0.0f;
	tbb[1]=0.0f;
	tbb[2]=dx;
	tbb[3]=dy;

	for(jj=0;jj!=side_max;jj++)
	{
		x=0;
		for(ii=0;ii!=side_max;ii++)
		{
			int number=ii+jj*side_max;
			if(number>=count)
				continue;
			float xc=x+xxx*dx;
			float yc=y+yyy*dy;
			if(tileUsage[number]==0)
			{
				if((vh!=0)&(number==sq)) glColor3f(0.5f,0.5f,0.5f);
				else glColor3f(1.0f,1.0f,1.0f);
				float mag=texMAG[number];
				float xtex=texPOS[number*2];
				float ytex=texPOS[number*2+1];
				glBindTexture(GL_TEXTURE_2D, texture[number]);
				glBegin(GL_QUADS);
				glTexCoord2f(xtex, ytex);
				glVertex2f(x+1, y+1);
				glTexCoord2f(xtex+1.0*mag, ytex);
				glVertex2f(x+dx-1, y+1);
				glTexCoord2f(xtex+1.0*mag, ytex+1.0*mag);
				glVertex2f(x+dx-1, y+dy-1);
				glTexCoord2f(xtex, ytex+1.0*mag);
				glVertex2f(x+1, y+dy-1);
				glEnd();

				glBindTexture(GL_TEXTURE_2D, 0);
				if(number==sq && DataArrs[sq])
				{
					glColor3f(1,0,0);
					glBegin(GL_QUADS);
					glVertex2f(xc-3+1, yc-3+1);
					glVertex2f(xc+3-1, yc-3+1);
					glVertex2f(xc+3-1, yc+3-1);
					glVertex2f(xc-3+1, yc+3-1);
					glEnd();

					glColor3f(0.5,0.5,0.5);
					if(vh==1)
					{
						glBegin(GL_LINES);
						glVertex2f(1+x, yc+1);
						glVertex2f(1+x+dx, yc+1);
						glEnd();

						int i;
						float max=-1.0e11;
						float min=1.0e11;
						for(i=0;i<texSZ[sq*2];i++)
						{
							int ii=(float(i)/float(texSZ[sq*2])*texMAG[sq]+texPOS[sq*2]-floor(float(i)/float(texSZ[sq*2])*texMAG[sq]+texPOS[sq*2]))*float(texSZ	[sq*2]);
							float value = 0.0f;
							if(DataType[sq] == 1)
								value = ((float*) DataArrs[sq])[yi*texSZ[sq*2]+ii];
							else if(DataType[sq] == 2)
								value = (float) ((double*) DataArrs[sq])[yi*texSZ[sq*2]+ii];
							max=std::max(max, value);
							min=std::min(min, value);
						}
						glColor3f(0.5,0.5,0.5);
						glBegin(GL_LINES);
						glVertex2f(1+x, y+1+max/(max-min)*dy);
						glVertex2f(1+x+dx, y+1+max/(max-min)*dy);
						glEnd();
						for(i=0;i<11;i++)
						{
							sprintf(title,"%.2e\n",min+(max-min)*float(i)/10.0);
							showMessage(1+x,y+dy-3-i*(dy-6)/10,title);
						}
						glBegin(GL_LINES);
						glColor3f(1,1,1);
						for(i=0;i<texSZ[sq*2]-1;i++)
						{
							int ii = round( (float(i)/float(texSZ[sq*2])*texMAG[sq]+texPOS[sq*2]-floor(float(i)/float(texSZ[sq*2])*texMAG[sq]+texPOS[sq*2]))*float(texSZ[sq*2]) );
							int ii_1 = round( (float(i+1)/float(texSZ[sq*2])*texMAG[sq]+texPOS[sq*2]-floor(float(i+1)/float(texSZ[sq*2])*texMAG[sq]+texPOS[sq*2]))*float(texSZ[sq*2]) );
							if((ii_1>=0)&(ii_1<texSZ[sq*2])&(ii>=0)&(ii<texSZ[sq*2]))
							{
								float v1 = 0.0f;
								float v2 = 0.0f;
								if(DataType[sq] == 1)
								{
									v1 = ((float*) DataArrs[sq])[yi*texSZ[sq*2]+ii];
									v2 = ((float*) DataArrs[sq])[yi*texSZ[sq*2]+ii_1];
								}
								else if(DataType[sq] == 2)
								{
									v1 = (float) ((double*) DataArrs[sq])[yi*texSZ[sq*2]+ii];
									v2 = (float) ((double*) DataArrs[sq])[yi*texSZ[sq*2]+ii_1];
								}
								glVertex2f(1.0f + (float) x + float(dx*i)/float(texSZ[sq*2]), (float) y + 1.0f + (max - v1) / (max-min) * (float) dy);
								glVertex2f(1.0f + (float) x + float(dx*(i+1))/float(texSZ[sq*2]), (float) y + 1.0f + (max - v2) / (max-min) * (float) dy);
							}
						}
						glEnd();

					}

					if(vh==-1)
					{
						glBegin(GL_LINES);
						glVertex2f(1+xc, 1+y);
						glVertex2f(1+xc, dy+y+1);
						glEnd();

						int i;
						float max=-1.0e11;
						float min=1.0e11;
						for(i=0;i<texSZ[sq*2+1];i++)
						{
							int ii=(float(i)/float(texSZ[sq*2+1])*texMAG[sq]+texPOS[sq*2+1]-floor(float(i)/float(texSZ[sq*2+1])*texMAG[sq]+texPOS[sq*2+1]))*float(texSZ[sq*2+1]);
							float value = 0.0f;
							if(DataType[sq] == 1)
								value = ((float*) DataArrs[sq])[ii*texSZ[sq*2]+xi];
							else if(DataType[sq] == 2)
								value = (float) ((double*) DataArrs[sq])[ii*texSZ[sq*2]+xi];
							max=std::max(max, value);
							min=std::min(min, value);
						}
						glColor3f(0.5,0.5,0.5);
						glBegin(GL_LINES);
						glVertex2f(1+x, y+1+max/(max-min)*dy);
						glVertex2f(1+x+dx, y+1+max/(max-min)*dy);
						glEnd();
						for(i=0;i<11;i++)
						{
							sprintf(title,"%.2e\n",min+(max-min)*float(i)/10.0);
							showMessage(1+x,y+dy-3-i*(dy-6)/10,title);
						}
						glBegin(GL_LINES);
						glColor3f(1,1,1);
						for(i=0;i<texSZ[sq*2+1]-1;i++)
						{
							int ii=round( (float(i)/float(texSZ[sq*2+1])*texMAG[sq]+texPOS[sq*2+1]-floor(float(i)/float(texSZ[sq*2+1])*texMAG[sq]+texPOS[sq*2+1]))*float(texSZ[sq*2+1]) );
							int ii_1=round( (float(i+1)/float(texSZ[sq*2+1])*texMAG[sq]+texPOS[sq*2+1]-floor(float(i+1)/float(texSZ[sq*2+1])*texMAG[sq]+texPOS[sq*2+1]))*float(texSZ[sq*2+1]) );
							if((ii_1>=0)&(ii_1<texSZ[sq*2+1])&(ii>=0)&(ii<texSZ[sq*2+1]))
							{
								float v1 = 0.0f;
								float v2 = 0.0f;
								if(DataType[sq] == 1)
								{
									v1 = ((float*) DataArrs[sq])[ii*texSZ[sq*2]+xi];
									v2 = ((float*) DataArrs[sq])[ii_1*texSZ[sq*2]+xi];
								}
								else if(DataType[sq] == 2)
								{
									v1 = (float) ((double*) DataArrs[sq])[ii*texSZ[sq*2]+xi];
									v2 = (float) ((double*) DataArrs[sq])[ii_1*texSZ[sq*2]+xi];
								}
								glVertex2f(1.0f + (float) x + float(dx*i) / float(texSZ[sq*2+1]), (float) y + 1.0f + (max-v1) / (max-min) * (float) dy);
								glVertex2f(1.0f + (float) x + float(dx*(i+1)) / float(texSZ[sq*2+1]), (float) y + 1.0f + (max-v2) / (max-min) * (float) dy);
							}
						}
						glEnd();
					}
				}
			}
			// particles section and 1d graphs
			if(tileUsage[number]==1) // particles
			{
				glBegin(GL_POINTS);
				//printf("tile #%d plotting point clouds N=%d\n",number,particleCloudNum[number]);
				for(int i=0;i<particleCloudNum[number];i++) // loop over clouds
				{
					bounds bb;
					for(int j=0;j<4;j++) bb[j]=particleBounds[number][j];
					color* C=&particlesColor[number][i];
					glColor3f((*C)[0],(*C)[1],(*C)[2]);
					//printf("\tcloud[%d] has %d particles with color (%g, %g, %g)\n",i,particlesSZ[number][i],*C[0],*C[1],*C[2]);
					for(int j=0;j<particlesSZ[number][i];j++) // loop over points
					{
						//draw
						float px=particles[number][i][j*2];
						float py=particles[number][i][j*2+1];
						//if(j<13)
						//	printf("x=%g,y=%g\n",particles[number][i][j*2],particles[number][i][j*2+1]);
						//glVertex2f(xc-3+y, yc-3+x);
						float spx=(px-bb[0])/(bb[2]-bb[0]+DBL_MIN)*dx;
						float spy=(py-bb[1])/(bb[3]-bb[1]+DBL_MIN)*dy;
						transformTex(&spx,&spy,texPOS[number*2],texPOS[number*2+1],texMAG[number],dx,dy);
						if(ifInBounds(spx,spy,tbb))
							glVertex2f(x+spx, y+spy);
					}
				}
				glEnd();
			}
			if(tileUsage[number]==2) // 1d plots
			{
				glBegin(GL_LINES);
				//printf("tile #%d plotting point clouds N=%d\n",number,particleCloudNum[number]);
				for(int i=0;i<plotNum[number];i++) // loop over clouds
				{
					bounds bb;
					for(int j=0;j<4;j++) bb[j]=plotBounds[number][j];
					color* C=&plotsColor[number][i];
					glColor3f((*C)[0],(*C)[1],(*C)[2]);
					//printf("\tcloud[%d] has %d particles with color (%g, %g, %g)\n",i,particlesSZ[number][i],*C[0],*C[1],*C[2]);
					for(int j=0;j<plotsSZ[number][i]-1;j++) // loop over points
					{
						//draw
						float px0=plots[number][i][j*2];
						float py0=plots[number][i][j*2+1];
						float px1=plots[number][i][(j+1)*2];
						float py1=plots[number][i][(j+1)*2+1];
						//if(j<13)
						//	printf("x=%g,y=%g\n",particles[number][i][j*2],particles[number][i][j*2+1]);
						//glVertex2f(xc-3+y, yc-3+x);
						float spx0=(px0-bb[0])/(bb[2]-bb[0]+DBL_MIN)*dx;
						float spy0=(py0-bb[1])/(bb[3]-bb[1]+DBL_MIN)*dy;
						transformTex(&spx0,&spy0,texPOS[number*2],texPOS[number*2+1],texMAG[number],dx,dy);
						float spx1=(px1-bb[0])/(bb[2]-bb[0]+DBL_MIN)*dx;
						float spy1=(py1-bb[1])/(bb[3]-bb[1]+DBL_MIN)*dy;
						transformTex(&spx1,&spy1,texPOS[number*2],texPOS[number*2+1],texMAG[number],dx,dy);
						if(ifInBounds(spx0,spy0,tbb)&ifInBounds(spx1,spy1,tbb)) //both points inside
						{
							glVertex2f(x+spx0, y+spy0);
							glVertex2f(x+spx1, y+spy1);
						}
						else// if(ifInBounds(spx0,spy0,tbb)^ifInBounds(spx1,spy1,tbb)) // only one point is inside
						{
							if(crossLineWithBB(&spx0,&spy0,&spx1,&spy1,tbb))
							{
								glVertex2f(x+spx0, y+spy0);
								glVertex2f(x+spx1, y+spy1);
							}
						}
					}
				}
				glEnd();
			}
			if(texMAGinfo[number])
			{
				showMessage(x + dx - 50, y + 10, std::to_string(texMAG[number]).c_str());
				showMessage(x + dx - 50, y + 20, std::to_string(texPOS[number*2]).c_str());
				showMessage(x + dx - 50, y + 30, std::to_string(texPOS[number*2+1]).c_str());
				texMAGinfo[number]--;
			}
			x+=dx;
		}
		y+=dy;
	}

	if(sq>=0 && (xi>=0) &&(xi<texSZ[sq*2]) && (yi>=0) && (yi<texSZ[sq*2+1]))
	{
		float value = 0.0f;
		if(DataType[sq] == 1)
			value = ((float*) DataArrs[sq])[xi+yi*texSZ[sq*2]];
		else if(DataType[sq] == 2)
			value = (float) ((double*) DataArrs[sq])[xi+yi*texSZ[sq*2]];
		sprintf(title,"f[%d](%d,%d)=%f",sq,xi,yi,value);
	}
	glfwSetWindowTitle(win, title);
	//glMatrixMode (GL_MODELVIEW);

}

static bool idle()
{
 	int i;
 	bool refresh(false);
    for(i=0;i<count;i++)
    {
        if(texUPD[i]==0)
        {
			refresh = true;
		    glBindTexture(GL_TEXTURE_2D, texture[i]);
#ifdef GL_VERSION_1_1
		    glTexImage2D(GL_TEXTURE_2D, 0, 4,texSZ[i*2],texSZ[i*2+1], 0, GL_RGBA, GL_FLOAT, texturesDataArrs[i]);
#else
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB,texSZ[i*2],texSZ[i*2+1], 0, GL_RGBA, GL_FLOAT, texturesDataArrs[i]);
#endif
		    texUPD[i]=1;
		}
    }
    return refresh;
}

static pthread_t	GLloop_th_id;

void* GLloop(void*)
{

//	printf("GLloop: activated\n");
	int i,j,k;
	texturesDataArrs=new float*[count];
	DataArrs=new void*[count];
	DataType=new int[count]; // 0 - undef, 1 - float, 2 - double
	texUPD=new int[count];
	texMAGinfo=new int[count];
	texSZ=new int[count*2];
	texMAG=new float[count];
	texPOS=new float[count*2];
	for(k=0;k!=count;k++)
	{
		texSZ[k*2]=Nx;
		texSZ[k*2+1]=Ny;
		texMAG[k]=1.0;
		texPOS[k*2]=0.0;
		texPOS[k*2+1]=0.0;
		texMAGinfo[k] = 0;
		texUPD[k]=0;
	}
	for(i=0;i<count;i++) texturesDataArrs[i]=new float[texSZ[i*2]*texSZ[i*2+1]*4];
	for(i=0;i<count;i++) DataArrs[i] = nullptr;
	for(i=0;i<count;i++) DataType[i] = 0;
	texture=new GLuint[count];

	for(k=0;k!=count;k++)
	{
		for(j=0;j<texSZ[k*2+1];j++)
		{
		  for(i=0;i<texSZ[k*2];i++)
		  {
			 	float x=float(i-Nx/2)/float(Nx);
				float y=float(j-Ny/2)/float(Ny);
				texturesDataArrs[k][(j*texSZ[k*2]+i)*4]=exp(-(x*x+y*y)*13.0*(1+k));
				texturesDataArrs[k][(j*texSZ[k*2]+i)*4+1]=0.0;
				texturesDataArrs[k][(j*texSZ[k*2]+i)*4+2]=0.0;
				texturesDataArrs[k][(j*texSZ[k*2]+i)*4+3]=0.0;
		  }
		}
	}

	glfwInit();
	GLFWwindow* win = glfwCreateWindow( XSize, YSize, "fadey draw", nullptr, nullptr);
	glfwMakeContextCurrent(win); // what does it mean... no idea. but is wont work wihtout it 
	glfwSetWindowSizeCallback(win, win_sz);
	glfwSetMouseButtonCallback(win, mouse_drag);
	glfwSetCursorPosCallback(win, mouse_move);
	glfwSetScrollCallback(win, mouse_wheel);
	glfwSetWindowCloseCallback(win, onwinclose);

	glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho (0, XSize, YSize, 0, -1, 1);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_LIGHTING);

	int ii, jj;
	side_max=-floor(-sqrt(float(count)));
  //glActiveTexture(GL_TEXTURE0);
  glGenTextures(count, texture);
	for(jj=0;jj<side_max;jj++)
	{
		for(ii=0;ii<side_max;ii++)
		{
			int number=ii+jj*side_max;
			if(number<count)
			{
				//printf("making texture number %d\n",number);
				glBindTexture(GL_TEXTURE_2D, texture[number]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifdef GL_VERSION_1_1
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, texSZ[number*2], texSZ[number*2+1], 0, GL_RGBA, GL_FLOAT, texturesDataArrs[number]);
#else
				glTexImage2D(GL_TEXTURE_2D, 0, 4, texSZ[number*2], texSZ[number*2+1], 0, GL_RGBA, GL_FLOAT, texturesDataArrs[number]);
#endif
				texUPD[number]=1;				
			}
		}
	}
	//printf("side_max=%d\n",side_max);
	pthread_mutex_unlock(&close_win_mutex);
	while(inited[0]==1)
	{
		glfwPollEvents();
		if(!idle())
			glfwWaitEvents();
		display(win);
		glfwSwapBuffers(win);
		#ifdef __MINGW32__
    Sleep(1);
    #else
    usleep(1000);
    #endif
	}
	printf("fadey_draw: quitting... preparing to destroy data\n");
	pthread_mutex_lock(&data_update_mutex);
	printf("fadey_draw: quitting... destroying window\n");
	glfwDestroyWindow(win);
	delete[] texture;
	delete[] texUPD;
	delete[] texSZ;
	delete[] texMAG;
	delete[] texMAGinfo;
	delete[] texPOS;
	for(i=0;i<count;i++) delete[] texturesDataArrs[i];
	delete[] DataArrs;
	delete[] DataType;
	delete[] texturesDataArrs;
	delete[] close_win;
	delete[] title;
	pthread_mutex_unlock(&data_update_mutex);
	printf("fadey_draw: clean exit\n");
	pthread_mutex_unlock(&close_win_mutex);
	return 0;
}


void fetch_data(int* plotNum, bounds* bb, int** SZ, float*** localdata, color** Color, int index, float* userData, int N, int tile, double r=-1,  double g=-1, double b=-1)
{
	if(index>=plotNum[tile])
	{
		//make new cloud: create SZ, localdata and particleColors then copy previous data there
		int* pSZ=new int[index+1];
		color *pC=new color[index+1];
		float** pp=new float*[index+1];
		printf("preparing structures for new particle cloud at index %d\n",index);
		for(int i=0;i<plotNum[tile];i++)
		{
			pSZ[i]=SZ[tile][i];
			for(int j=0;j<3;j++)
				pC[i][j]=Color[tile][i][j];
			pp[i]=localdata[tile][i];
		}
		printf("\told data stored\n");
		for(int i=plotNum[tile];i<index+1;i++)
		{
			pSZ[i]=-1;
			for(int j=0;j<3;j++)
				pC[i][j]=1.0; // add default white color for newly created pointCloud
		}
		printf("\tnew data added\n");
		if(plotNum[tile]!=0)
		{
			delete[] SZ[tile]; 
			delete[] Color[tile]; 
			delete[] localdata[tile];
		}
		else
			printf("\tdoing this first time thus do not releasing memory\n");
		SZ[tile]=pSZ;
		Color[tile]=pC;
		localdata[tile]=pp;
		plotNum[tile]=index+1;
	}
	if(N!=SZ[tile][index])
	{
		//printf("allocating space for new particle cloud coordinates of size %d\n", N);
		//if N > localdataNum create new localdata[tile][index] no copy needed color is preserved
		if(SZ[tile][index]!=-1)
			delete[] localdata[tile][index];
		else
			printf("allocating space for new particle cloud first time thus do not releasing memory\n");
		localdata[tile][index]=new float[N*2];
		SZ[tile][index]=N; // update number
	}
	// now we are ready to fill data (we are sure that we reserved memory for all data fields):
	for(int i=0;i<N;i++)
	{
		localdata[tile][index][i*2]=userData[i*2]; // copy x
		localdata[tile][index][i*2+1]=userData[i*2+1]; // copy y
		for(int j=0;j<4;j++)
			bb[tile][j]=(((j<2)?-1.0:1.0)*(bb[tile][j]-userData[i*2+j%2])<0)?userData[i*2+j%2]:bb[tile][j];
	}
	if(r+g+b>0)
	{// simply add color if it is set
		 Color[tile][index][0]=r; 
		 Color[tile][index][1]=g; 
		 Color[tile][index][2]=b; 
	}
}

void	showMessage(GLfloat x, GLfloat y, const char *message)
{
  glPushMatrix();
  glTranslatef(x, y, 0);
	float a=4.0f;
	float b=4.0f;
	float xg=0.0f;
	int i=0;
	char c;
	while(*message)
	{
		xg=1.3*a*i;
		c=*message;
		glColor3f(0.5f,0.7f,0.5f);
		glBegin(GL_LINES);
		const size_t id = (size_t) c; //index in charmap
		if(charmap[id])
		{
			//printf("found character %c\n", c);
			size_t cont_id(0);
			while(charmap[id][cont_id])
			{
				size_t pt_id(0);
				while(true)
				{
					const point p1 = charmap[id][cont_id][pt_id];
					if(p1.end())
						break;
					const point p2 = charmap[id][cont_id][pt_id + 1]; 
					if(p2.end())
						break;
					glVertex2f(xg + p1.x * a, p1.y * b);
					glVertex2f(xg + p2.x * a, p2.y * b);
					pt_id++;
				}
				cont_id++;
			}
		}
		glEnd();
		message++;
		i++;
  }
  glPopMatrix();
}
}//namespace


void fadey_init(int Nx_, int Ny_, int count_)
{
	if(simpledraw_impl::initedfirst==0)
	{
		simpledraw_impl::inited=new int[1];
		simpledraw_impl::inited[0]=0;
		simpledraw_impl::initedfirst=1;
	}
	if(simpledraw_impl::inited[0]==0)
	{
		simpledraw_impl::tileUsage=new int[count_];
		for(int i=0;i<count_;i++) simpledraw_impl::tileUsage[i]=-1; // do not use tiles on init.
		simpledraw_impl::title=new char[256];
		simpledraw_impl::inited[0]=1;
		simpledraw_impl::Nx=Nx_;
		simpledraw_impl::Ny=Ny_;
		simpledraw_impl::count=count_;
		pthread_create(&simpledraw_impl::GLloop_th_id, NULL, &simpledraw_impl::GLloop, NULL);
		simpledraw_impl::close_win=new int[1];
		simpledraw_impl::close_win[0]=0;

		simpledraw_impl::particleCloudNum=new int[count_];
		simpledraw_impl::particleBounds=new simpledraw_impl::bounds[count_];
		simpledraw_impl::particles=new float**[count_];
		simpledraw_impl::particlesColor=new simpledraw_impl::color*[count_];
		simpledraw_impl::particlesSZ=new int*[count_];
		for(int i=0;i<count_;i++)
		{
			simpledraw_impl::particleCloudNum[i]=0;
			for(int j=0;j<4;j++)
				simpledraw_impl::particleBounds[i][j]=(j<2)?DBL_MAX:-DBL_MAX;
		}

		simpledraw_impl::plotNum=new int[count_];
		simpledraw_impl::plotBounds=new simpledraw_impl::bounds[count_];
		simpledraw_impl::plots=new float**[count_];
		simpledraw_impl::plotsColor=new simpledraw_impl::color*[count_];
		simpledraw_impl::plotsSZ=new int*[count_];
		for(int i=0;i<count_;i++)
		{
			simpledraw_impl::plotNum[i]=0;
			for(int j=0;j<4;j++)
				simpledraw_impl::plotBounds[i][j]=(j<2)?DBL_MAX:-DBL_MAX;
		}

		pthread_mutex_init(&simpledraw_impl::close_win_mutex,NULL);
		pthread_mutex_init(&simpledraw_impl::data_update_mutex,NULL);
		pthread_mutex_lock(&simpledraw_impl::close_win_mutex);
		pthread_mutex_lock(&simpledraw_impl::close_win_mutex);
		//printf("init: bye\n");
	}
}

void fadey_draw(float* DataArr, int Nx_, int Ny_, int count_)
{
	pthread_mutex_lock(&simpledraw_impl::data_update_mutex);
	if(simpledraw_impl::inited[0]==1)
	{  
		int i,j;
		int k=count_;
		simpledraw_impl::tileUsage[k]=0;
		simpledraw_impl::DataArrs[k] = (void*) DataArr;
		simpledraw_impl::DataType[k] = 1;
		if ((Nx_!=simpledraw_impl::texSZ[k*2])||(Ny_!=simpledraw_impl::texSZ[k*2+1]))
		{
			delete[] simpledraw_impl::texturesDataArrs[k];
			simpledraw_impl::texSZ[k*2]=Nx_;
			simpledraw_impl::texSZ[k*2+1]=Ny_;
			simpledraw_impl::texturesDataArrs[k]=new float[simpledraw_impl::texSZ[k*2] * simpledraw_impl::texSZ[k*2+1]*4];
	//	printf("texture %d new size is (%d x %d)\n",k,texSZ[k*2],texSZ[k*2+1]);
		}
		float max=0.0;
		float min=0.0;
		for(j = 0;j != simpledraw_impl::texSZ[k*2+1]; j++)
		{
			for(i = 0; i != simpledraw_impl::texSZ[k*2]; i++)
			{
				max = std::max(max, DataArr[j*simpledraw_impl::texSZ[k*2]+i]);
				min = std::min(min, DataArr[j*simpledraw_impl::texSZ[k*2]+i]);
			}
		}
		max = std::max(max,std::abs(min));
		for(j = 0; j != simpledraw_impl::texSZ[k*2+1]; j++)
		{
			for(i = 0;i != simpledraw_impl::texSZ[k*2]; i++)
			{
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+0] = simpledraw_impl::red(DataArr[j*simpledraw_impl::texSZ[k*2]+i],max);
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+1] = simpledraw_impl::green(DataArr[j*simpledraw_impl::texSZ[k*2]+i],max);
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+2] = simpledraw_impl::blue(DataArr[j*simpledraw_impl::texSZ[k*2]+i],max);
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+3] = 0.0f;
			}
		}
		simpledraw_impl::texUPD[k]=0;
  }
	pthread_mutex_unlock(&simpledraw_impl::data_update_mutex);
	glfwPostEmptyEvent();
}

void fadey_draw(double* DataArr, int Nx_, int Ny_, int count_)
{
	printf("fadey_draw_double\n");
	pthread_mutex_lock(&simpledraw_impl::data_update_mutex);
	if(simpledraw_impl::inited[0]==1)
	{  
		int i,j;
		int k=count_;
		simpledraw_impl::tileUsage[k]=0;
		simpledraw_impl::DataArrs[k] = (void*) DataArr;
		simpledraw_impl::DataType[k] = 2;
		if ((Nx_!=simpledraw_impl::texSZ[k*2])||(Ny_!=simpledraw_impl::texSZ[k*2+1]))
		{
			delete[] simpledraw_impl::texturesDataArrs[k];
			simpledraw_impl::texSZ[k*2]=Nx_;
			simpledraw_impl::texSZ[k*2+1]=Ny_;
			simpledraw_impl::texturesDataArrs[k] = new float[simpledraw_impl::texSZ[k*2] * simpledraw_impl::texSZ[k*2+1]*4];
//		printf("texture %d new size is (%d x %d)\n",k,texSZ[k*2],texSZ[k*2+1]);
		}
		double max=0.0;
		double min=0.0;
		for(j = 0; j != simpledraw_impl::texSZ[k*2+1]; j++)
		{
			for(i = 0; i != simpledraw_impl::texSZ[k*2]; i++)
			{
				max = std::max(max, DataArr[j*simpledraw_impl::texSZ[k*2]+i]);
				min = std::min(min, DataArr[j*simpledraw_impl::texSZ[k*2]+i]);
			}
		}
		max = std::max(max,std::abs(min));
		for(j = 0; j != simpledraw_impl::texSZ[k*2+1]; j++)
		{
			for(i = 0; i != simpledraw_impl::texSZ[k*2]; i++)
			{
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+0] = simpledraw_impl::red((float) DataArr[j*simpledraw_impl::texSZ[k*2]+i], (float) max);
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+1] = simpledraw_impl::green((float) DataArr[j*simpledraw_impl::texSZ[k*2]+i], (float) max);
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+2] = simpledraw_impl::blue((float) DataArr[j*simpledraw_impl::texSZ[k*2]+i], (float) max);
				simpledraw_impl::texturesDataArrs[k][(j*simpledraw_impl::texSZ[k*2]+i)*4+3] = 0.0f;
			}
		}
		simpledraw_impl::texUPD[k]=0;
  }
	pthread_mutex_unlock(&simpledraw_impl::data_update_mutex);
	glfwPostEmptyEvent();
	printf("fadey_draw_double DONE\n");

}

void fadey_draw_particles_reset_bounds(int tile)
{
	for(int j=0;j<4;j++)
		simpledraw_impl::particleBounds[tile][j]=(j<2)?DBL_MAX:-DBL_MAX;
}

void fadey_draw_particles(int index, float* particleArr, int N, int tile, double r=-1,  double g=-1, double b=-1)
{
	simpledraw_impl::tileUsage[tile]=1;
	simpledraw_impl::fetch_data(simpledraw_impl::particleCloudNum, simpledraw_impl::particleBounds, simpledraw_impl::particlesSZ, simpledraw_impl::particles, simpledraw_impl::particlesColor, index, particleArr, N,tile, r, g, b);
}

void fadey_draw_1D(int index, float* graphArr, int N, int tile, double r=-1,  double g=-1, double b=-1)
{
	simpledraw_impl::tileUsage[tile]=2;
	simpledraw_impl::fetch_data(simpledraw_impl::plotNum, simpledraw_impl::plotBounds, simpledraw_impl::plotsSZ, simpledraw_impl::plots, simpledraw_impl::plotsColor, index, graphArr, N,tile, r, g, b);
}

void fadey_close()
{
	if(simpledraw_impl::inited[0]==1)
	{
		simpledraw_impl::close_win[0]=1;
		simpledraw_impl::inited[0]=0;
		glfwPostEmptyEvent();
		pthread_mutex_lock(&simpledraw_impl::close_win_mutex);
		pthread_mutex_destroy(&simpledraw_impl::close_win_mutex);
		pthread_mutex_destroy(&simpledraw_impl::data_update_mutex);
		printf("fadey draw: bye!\n");
	}
	else
		printf("fadey draw::close not inited\n");
	
}
#endif
