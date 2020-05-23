//#define SIMPLETEST
#define LOADINITSTATE

#ifndef SIMPLETEST
#define WANTCAUSTICS
//#define JUSTCAUSTICS

/*
 *
 * X points right
 * Y points to top of screen
 * Z points into screen away from eye
 * dont care what it says on the web
 *
 * TODO:
 * Settings
 * Fetch ad
 * Manual fly/zoom
 *
 */
#include "header.h"
int bitmap_ids[5];
GLfloat width, height, weight;
//#define float double
void bitmap(int which, int id)
{
	LOGI("bitmap\n");
	bitmap_ids[which]=id;
}

GLuint gProgramMain, gvPositionMain, gvNormalMain, gvSamplerHandle, gvTrans, gvEyepos, gvSunpos, gvSunsize, gvDepth, gvCausticsTexture, gvCausture;
GLuint gProgramCaustics, gvPositionCaustics, gvNormalCaustics, gvFrameBuffer, gvCaustexMain, gvConcentrationCaustics, gvBwTexCaustics;

extern char gVertexMain[];
extern char gFragmentMain[];
extern char gVertexCaustics[];
extern char gFragmentCaustics[];

struct Vertex {
	Vec3 pos;
	Vec2 norm;
	GLfloat concentration;
};

#define RAIN_CHANCE 15
#define POKE_SPLASH 7.0
#define POKE_WIDTH 0.75
#define PLOP_HEIGHT 0.0015
#define PLOP_WIDTH 0.008
//#define TURN_SPEED 0.0
#define TURN_SPEED 0.1

const int VERTEX_GAPS=64.0;
const GLfloat VERTEX_PLANE_WIDTH = 1.0;
const int CAUSTURE_RES=512;
#define PLOP_CELLS (PLOP_WIDTH*VERTEX_GAPS)

GLfloat eye_long;
GLfloat eye_lat;

GLfloat eye_dist = 0.6;
GLfloat zoom = 4.0;
//GLfloat eye_long = 0.0;
//GLfloat eye_lat = 3.14159/2.0;
//GLfloat eye_dist = 1.0;
Vec3 eye;
Vec3 sun=Vec3(10.0,10.0,-7.0);


#define VERTEX_COUNT_SURF ((VERTEX_GAPS+1)*(VERTEX_GAPS+1))
#define VERTEX_COUNT_EXTRA 9
#define INDEX_COUNT_SURF (2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1))
#define INDEX_COUNT_EXTRA 15
#define VERTEX_COUNT_ALL (VERTEX_COUNT_SURF+VERTEX_COUNT_EXTRA)
#define INDEX_COUNT_ALL (INDEX_COUNT_SURF+INDEX_COUNT_EXTRA)
#define GAP_WIDTH (VERTEX_PLANE_WIDTH/VERTEX_GAPS)

//Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1];
struct _ver {
	Vertex ices[VERTEX_GAPS+1][VERTEX_GAPS+1];
	Vertex ners[VERTEX_COUNT_EXTRA];
	GLfloat ocities[VERTEX_GAPS+1][VERTEX_GAPS+1];
};

unsigned char verdump[sizeof(struct _ver)]
#ifdef LOADINITSTATE
={
#include "poolstate.h"
}
#endif
;
struct _ver * ver = (struct _ver *) verdump;

#define vertices ver->ices
#define verners ver->ners
#define velocities ver->ocities

GLushort indices[INDEX_COUNT_ALL];

GLfloat matrix[16];
Matrix inverse;

void dumpstate()
{
	FILE * fp = fopen("/sdcard/poolstate.h", "w");
	barfIfNull("FILE * fp", (unsigned int) fp);
	for (int a=0;a<sizeof(struct _ver);a++)
		fprintf(fp, "%s%u%s", a==0?"":", ", verdump[a], a%32==0?"\n":"");
	fclose(fp);
}

void update_eye_cartesian()
{
	eye.z = -eye_dist * sin(eye_lat);
	GLfloat temp = eye_dist * cos(eye_lat);
	eye.x = temp * sin(eye_long);
	eye.y = -temp * cos(eye_long);
	Matrix m_tot, m_rot_x, m_rot_z, m_rot_y, m_pers, m_scale, m_trans, m_squelch;

	m_rot_z.rot_z(-eye_long);
	m_rot_x.rot_x(3.14159/2.0-eye_lat);
	m_tot.premul(m_rot_z);
	m_tot.premul(m_rot_x);

	m_pers.pers(eye_dist);
	m_tot.premul(m_pers);

	m_squelch.squelch(zoom*height/weight, zoom*width/weight,0.01);
	m_tot.premul(m_squelch);

	m_trans.trans(0.0,0.55,0.0);
	m_tot.premul(m_trans);

//	m_scale.stretch(zoom*height/width, zoom, 1.0);
//	m_tot.premul(m_scale);

	m_tot.transpose_out(matrix);
	m_tot.inverse(inverse);
	//velocities[(int)(VERTEX_GAPS/2 + eye.x/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)][(int)(VERTEX_GAPS/2 + eye.y/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)] +=0.1;
}

void move_eye(long when)
{
//	return;
	eye_long-=TURN_SPEED*when/1000.0;
	zoom = 3.0 + cos(4.0*eye_long);
	//eye_lat+=0.001;
	update_eye_cartesian();
}

float dir=1.0;

void plop_at(float height, float width, int x, int y)
{
	int n = (int)(6.0*width);
	for (int i=-n;i<=n;i++)
		for (int j=-n;j<=n;j++)
			if (x+i>=0 && x+i<=VERTEX_GAPS && y+j>=0 && y+j<=VERTEX_GAPS)
			{
				GLfloat dist = sqrt((float)i*i+j*j)/width;
				GLfloat plop = dir*height*PLOP_HEIGHT* (dist<0.1 ? 1.0 : (sin(dist)/dist) );
				velocities[x+i][y+j]+=plop;
			}
}

void plop(float width)
{
	dir *=-1.0;
	plop_at(1.0/width, width, rand()%(VERTEX_GAPS+1), rand()%(VERTEX_GAPS+1));
}

void rain()
{
//	return;
	if (rand()%100<RAIN_CHANCE)
	{
		plop(PLOP_CELLS*(rand()%6+1)); //
		plop(PLOP_CELLS*(rand()%3+1));
	}
}
void poked(int type, int xs, int ys)
{
//	dumpstate();
//	return;
	static GLfloat prex, prey;
	static bool prev;

	GLfloat src[4], dst[4];
	if (type != 0)
	{
		prev=false;
		//return;
	}
	dir = -1.0;
	src[0] = xs*2.0/width-1.0;
	src[1] = (height-ys)*2.0/height-1.0;
	src[2] = 0.0; //don't care
	src[3] = 1.0;
	inverse.act(src, dst);
	LOGI("POKED at %f by %f by %f!!!\n", dst[0], dst[1], dst[2]);
	GLfloat g = eye.z / (eye.z - dst[2]);
	Vec2 drop;
	drop.x = g*dst[0] + (1.0-g)*eye.x ;
	drop.y = g*dst[1] + (1.0-g)*eye.y ;
	int ix = ( drop.x + 0.5 )*VERTEX_GAPS + 0.5;
	int iy = ( drop.y + 0.5 )*VERTEX_GAPS + 0.5;
	float splash, s2;
	if (prev)
	{
		splash = drop.x - prex;
		splash *= splash;
		s2 = drop.y - prey;
		s2 *= s2;
		splash += s2;
		splash = sqrt(sqrt(splash));
		splash *= 2.0*POKE_SPLASH;
	}
	else
		splash = POKE_SPLASH;
	if (ix >=0 && iy >= 0 && ix <= VERTEX_GAPS && iy <= VERTEX_GAPS)
	{
		plop_at(splash, POKE_WIDTH, ix, iy);
		//plop_at(splash/3.0, type==0?0.33:1.0, ix, iy);
	}
	if (type == 2)
		prev = false;
	else
		prex = drop.x; prey = drop.y; prev=true;

}

void init_vertices()
{
	eye_long =160.0*3.14159/180.0;
	eye_lat = 14.5*3.14159/180.0; //8.5

   int x,y,i;

#ifndef LOADINITSTATE

   for (x=0;x<VERTEX_GAPS+1;x++)
       for (y=0;y<VERTEX_GAPS+1;y++)
       {
           vertices[x][y].pos.x=((GLfloat)(x-VERTEX_GAPS/2.0))*GAP_WIDTH;
           vertices[x][y].pos.y=((GLfloat)(y-VERTEX_GAPS/2.0))*GAP_WIDTH;
           vertices[x][y].pos.z=0.0;
           vertices[x][y].norm.x=0.0;
           vertices[x][y].norm.y=0.0;
           velocities[x][y]=0.0;
       }

   for (i=0;i<100;i++)
      rain();

#endif
   for (i=0;i<VERTEX_COUNT_EXTRA;i++)
   {
       verners[i].pos.x = -0.5 + (float)((i%4)/2);
       verners[i].pos.y = -0.5 + (float)(i%2);
       verners[i].pos.z = i<4 ? 0.5 : -1.0/128.0;
       verners[i].norm.x=100.0*verners[i].pos.x;
       verners[i].norm.y=100.0*verners[i].pos.y;
   }


   i=0;

    for (x=0;x<VERTEX_GAPS;x++)
    {
    	if (x!=0)
    		indices[i++]=x*(VERTEX_GAPS+1);
    	for (y=0;y<VERTEX_GAPS+1;y++)
    	{
    		indices[i++]=x*(VERTEX_GAPS+1)+y;
    		indices[i++]=(x+1)*(VERTEX_GAPS+1)+y;
    	}
    	if (x!=VERTEX_GAPS-1)
    		indices[i++]=(x+1)*(VERTEX_GAPS+1)+y-1;
    }

    /*
    verners[0].pos.x = -0.1;
    verners[0].pos.y = 0.0;
    verners[0].pos.z = 0.0;
    verners[1].pos.x = 0.1;
    verners[1].pos.y = 0.0;
    verners[1].pos.z = 0.0;
    verners[2].pos.x = 0.0;
    verners[2].pos.y = 0.5;
    verners[2].pos.z = 0.0;
    verners[3].pos.x = 0.0;
    verners[3].pos.y = 0.0;
    verners[3].pos.z = 0.1;


    indices[i++] = VERTEX_COUNT_SURF;
//    indices[i++] = -1 + VERTEX_COUNT_SURF;
    indices[i++] = 1 + VERTEX_COUNT_SURF;
    indices[i++] = 2 + VERTEX_COUNT_SURF;
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = 0 + VERTEX_COUNT_SURF;
    indices[i++] = 1 + VERTEX_COUNT_SURF;
    */
    indices[i++] = indices[INDEX_COUNT_SURF-1];
    indices[i++] = 7 + VERTEX_COUNT_SURF;
    indices[i++] = 7 + VERTEX_COUNT_SURF;
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = 6 + VERTEX_COUNT_SURF;
    indices[i++] = 2 + VERTEX_COUNT_SURF;
    indices[i++] = 4 + VERTEX_COUNT_SURF;
    indices[i++] = 0 + VERTEX_COUNT_SURF;
    indices[i++] = 5 + VERTEX_COUNT_SURF;
    indices[i++] = 1 + VERTEX_COUNT_SURF;
    indices[i++] = 7 + VERTEX_COUNT_SURF;
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = 1 + VERTEX_COUNT_SURF;
    indices[i++] = 2 + VERTEX_COUNT_SURF;
    indices[i++] = 0 + VERTEX_COUNT_SURF;

}

float prev_avgz=0;

void adjust_vertices(long when)
{

   int x,y,i;
   float this_avgz=0, t = ((float)when)/((float)TYPICAL_INTERVAL);
   //add old velocities to positions
   for (x=0;x<=VERTEX_GAPS;x++)
	   for (y=0;y<=VERTEX_GAPS;y++)
	   {
		   this_avgz += vertices[x][y].pos.z;
		   vertices[x][y].pos.z += velocities[x][y]*t - prev_avgz;
	   }
   prev_avgz = this_avgz/(VERTEX_GAPS+1)/(VERTEX_GAPS+1);
   //get new velocities
   for (x=0;x<=VERTEX_GAPS;x++)
	   for (y=0;y<=VERTEX_GAPS;y++)
	   {
		   GLfloat force = (
				   (y<VERTEX_GAPS ? (vertices[x][y+1].pos.z - vertices[x][y].pos.z) : 0 ) +
				   (y>0           ? (vertices[x][y-1].pos.z - vertices[x][y].pos.z) : 0 ) +
				   (x<VERTEX_GAPS ? (vertices[x+1][y].pos.z - vertices[x][y].pos.z) : 0 ) +
				   (x>0           ? (vertices[x-1][y].pos.z - vertices[x][y].pos.z) : 0 )
			   ) ;
//		   GLfloat force = neighbours - 4.0*vertices[x][y].pos.z;
		   velocities[x][y] = velocities[x][y]*0.997 + force*0.125*t;// - 0.001*(vertices[x][y].concentration-1.0);//copysign(0.00003, (vertices[x][y].concentration-1.0)) ;
	   }

/*
      //treat edge velocities and positions
      for (i=0;i<VERTEX_GAPS+1;i++)
      {
   	   vertices[0][i].pos.z = vertices[1][i].pos.z;
   	   velocities[0][i] = velocities[1][i];
   	   vertices[VERTEX_GAPS][i].pos.z = vertices[VERTEX_GAPS-1][i].pos.z;
   	   velocities[VERTEX_GAPS][i] = velocities[VERTEX_GAPS-1][i];

   	   vertices[i][0].pos.z = vertices[i][1].pos.z;
   	   velocities[i][0] = velocities[i][1];
   	   vertices[i][VERTEX_GAPS].pos.z = vertices[i][VERTEX_GAPS-1].pos.z;
   	   velocities[i][VERTEX_GAPS] = velocities[i][VERTEX_GAPS-1];
      }
*/

}

void adjust_vertices_test(long when)
{

   int x,y,i;
   float this_avgz=0, t = ((float)when)/((float)TYPICAL_INTERVAL);
   //add old velocities to positions
   for (x=0;x<=VERTEX_GAPS;x++)
	   for (y=0;y<=VERTEX_GAPS;y++)
	   {
		   vertices[x][y].pos.z = -0.01;
		   velocities[x][y]=0;
	   }
   }



void get_norms_and_curves()
{
   int x,y,i; float t;
   //get norms
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   t = ( vertices[x-1][y].pos.z - vertices[x+1][y].pos.z ) / (4.0*GAP_WIDTH);
		   vertices[x][y].norm.x = t;
		   t = ( vertices[x][y-1].pos.z - vertices[x][y+1].pos.z ) / (4.0*GAP_WIDTH);
		   vertices[x][y].norm.y = t;

		   //these point down towards +ve z
	   }

   //get curvature
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   float g = 12.0*GAP_WIDTH;
		   float delnormx = ( vertices[x+1][y].norm.x - vertices[x-1][y].norm.x ) / g;
		   float delnormy = ( vertices[x][y+1].norm.y - vertices[x][y-1].norm.y ) / g;
		   vertices[x][y].concentration = fabs((1.0+delnormx)*(1.0+delnormy));//depth
	   }
/*
   //treat norms at edges
   for (i=0;i<VERTEX_GAPS+1;i++)
   {
	   vertices[0][i].norm.x = vertices[1][i].norm.x;
	   vertices[0][i].norm.y = vertices[1][i].norm.y;
	   vertices[0][i].concentration = 1.0;  //do this once only!
	   vertices[VERTEX_GAPS][i].norm.x = vertices[VERTEX_GAPS-1][i].norm.x;
	   vertices[VERTEX_GAPS][i].norm.y = vertices[VERTEX_GAPS-1][i].norm.y;
	   vertices[VERTEX_GAPS][i].concentration = 1.0;
	   vertices[i][0].norm.x = vertices[i][1].norm.x;
	   vertices[i][0].norm.y = vertices[i][1].norm.y;
	   vertices[i][0].concentration = 1.0;
	   vertices[i][VERTEX_GAPS].norm.x = vertices[i][VERTEX_GAPS-1].norm.x;
	   vertices[i][VERTEX_GAPS].norm.y = vertices[i][VERTEX_GAPS-1].norm.y;
	   vertices[i][VERTEX_GAPS].concentration = 1.0;

   }
*/
}

GLuint vboIds[3];

void getLocations()
{
#ifndef JUSTCAUSTICS
    gvPositionMain = glGetAttribLocation(gProgramMain, "a_position"); checkGlError("glGetAttribLocation");    barfIfNull("gvPositionMain", gvPositionMain);
    gvNormalMain = glGetAttribLocation(gProgramMain, "a_normal"); checkGlError("glGetAttribLocation");    barfIfNull("gvNormalMain", gvNormalMain);
    gvSamplerHandle = glGetUniformLocation(gProgramMain, "u_sampler"); checkGlError("glGetAttribLocation");    barfIfNull("gvSamplerHandle", gvSamplerHandle);
    gvCausture = glGetUniformLocation(gProgramMain, "u_causture"); checkGlError("glGetAttribLocation");    barfIfNull("gvCausture", gvCausture);
    gvTrans = glGetUniformLocation(gProgramMain, "u_trans"); checkGlError("glGetAttribLocation");    barfIfNull("gvTrans", gvTrans);
    gvEyepos = glGetUniformLocation(gProgramMain, "u_eyepos"); checkGlError("glGetAttribLocation");    barfIfNull("gvEyepos", gvEyepos);
    gvSunpos = glGetUniformLocation(gProgramMain, "u_sunpos"); checkGlError("glGetAttribLocation");    barfIfNull("gvSunpos", gvSunpos);
    gvSunsize = glGetUniformLocation(gProgramMain, "u_sunsize"); checkGlError("glGetAttribLocation");    barfIfNull("gvSunsize", gvSunsize);
    gvDepth = glGetUniformLocation(gProgramMain, "u_depth"); checkGlError("glGetAttribLocation");    barfIfNull("gvDepth", gvDepth);
#endif
    gvPositionCaustics = glGetAttribLocation(gProgramCaustics, "a_position"); checkGlError("glGetAttribLocation");    barfIfNull("gvPositionCaustics", gvPositionCaustics);
    gvNormalCaustics = glGetAttribLocation(gProgramCaustics, "a_normal"); checkGlError("glGetAttribLocation");    barfIfNull("gvNormalCaustics", gvNormalCaustics);
    gvConcentrationCaustics = glGetAttribLocation(gProgramCaustics, "a_concentration"); checkGlError("glGetAttribLocation");    barfIfNull("gvConcentrationCaustics", gvConcentrationCaustics);
    gvBwTexCaustics = glGetUniformLocation(gProgramCaustics, "u_BwTexture"); checkGlError("glGetAttribLocation");    barfIfNull("gvBwTexCaustics", gvBwTexCaustics);

}

bool setupGraphics(int w, int h) {
	width=w; height=h;
    glViewport(0, 0, width, height);
    weight = (width+height)/2.0;
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);

    init_vertices();

	update_eye_cartesian();
    float sunmag = sqrt ( sun.x*sun.x + sun.y*sun.y + sun.z*sun.z );
    sun.x/=sunmag;
    sun.y/=sunmag;
    sun.z/=sunmag;

#ifndef JUSTCAUSTICS
    gProgramMain = createProgram(gVertexMain, gFragmentMain);
    if (!gProgramMain) {
        LOGE("Could not create program.");
        return false;
    }
#endif

    gProgramCaustics = createProgram(gVertexCaustics, gFragmentCaustics);
    if (!gProgramCaustics) {
        LOGE("Could not create program.");
        return false;
    }

    glGenBuffers(2, vboIds); checkGlError("glGenBuffers");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[1]); checkGlError("glBindBuffer A");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * INDEX_COUNT_ALL, indices, GL_DYNAMIC_DRAW);

    getLocations();

#ifndef JUSTCAUSTICS
    glGenTextures(1, &gvCausticsTexture); checkGlError("glGenTextures");
    glGenFramebuffers(1, &gvFrameBuffer); checkGlError("glGenFramebuffers A 1");
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture A 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAUSTURE_RES, CAUSTURE_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE/*GL_UNSIGNED_SHORT_5_5_5_1*/, NULL); checkGlError("glTexImage2D 1");
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer); checkGlError("glBindFramebuffer 3");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gvCausticsTexture, 0); checkGlError("glFramebufferTexture2D 3");
#endif


    checkGlError("glViewport");
    return true;
}


void renderFrame(long when) {

    rain();
	for (int w=when; w>0; )
	{
		int t = (w>MAX_INTERVAL) ? MAX_INTERVAL : w;
		adjust_vertices(t);
		w -= t;
		break; //turns out to be a bad idea
	}

//	adjust_vertices(when);
	get_norms_and_curves();
	move_eye(when);


    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*VERTEX_COUNT_ALL, vertices, GL_DYNAMIC_DRAW); checkGlError("glBufferData GL_ARRAY_BUFFER");



    //caustics pass
#ifdef WANTCAUSTICS
    glUseProgram(gProgramCaustics); checkGlError("glUseProgram Caustics");
    glEnableVertexAttribArray(gvPositionCaustics); checkGlError("glEnableVertexAttribArray gvPositionCaustics");
    glVertexAttribPointer(gvPositionCaustics, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvNormalCaustics); checkGlError("glEnableVertexAttribArray gvNormalCaustics");
    glVertexAttribPointer(gvNormalCaustics, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvConcentrationCaustics); checkGlError("glEnableVertexAttribArray gvConcentrationCaustics");
    glVertexAttribPointer(gvConcentrationCaustics, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(5*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");

    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture B 2");
    glBindTexture ( GL_TEXTURE_2D, bitmap_ids[1] ); checkGlError("glBindTexture 1");
    glUniform1i ( gvBwTexCaustics, 0 ); checkGlError("gvSamplerHandle 0");

#ifdef JUSTCAUSTICS
//    glViewport(0, 0, width, height);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer);
    glViewport(0, 0, CAUSTURE_RES, CAUSTURE_RES);
#endif
    glClearColor(0.0, 0.0, 0.0, 0.0); checkGlError("glClearColor");
    glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear 1");
    glEnable(GL_BLEND);checkGlError("glEnable Blend");
    glBlendFunc(GL_ONE, GL_ONE);checkGlError("glBlendFunc");
    glDisable(GL_DEPTH_TEST);
//   	glDepthMask(GL_FALSE);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT_SURF, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements Caustics");
#endif // WANTCAUSTICS



    //main pass
#ifndef JUSTCAUSTICS
    glUseProgram(gProgramMain); checkGlError("glUseProgram");
    glEnableVertexAttribArray(gvPositionMain); checkGlError("glEnableVertexAttribArray gvPositionMain");
    glVertexAttribPointer(gvPositionMain, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvNormalMain); checkGlError("glEnableVertexAttribArray gvNormalMain");
    glVertexAttribPointer(gvNormalMain, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");
    glUniform3f ( gvEyepos, eye.x, eye.y, eye.z ); checkGlError("set Eyepos");
    glUniformMatrix4fv(	gvTrans, 1, false, matrix); checkGlError("set matrix");
    glUniform1f ( gvDepth, 0.5 ); checkGlError("set depth");
    glUniform3f ( gvSunpos, sun.x, sun.y, sun.z ); checkGlError("set Eyepos");
    glUniform1f ( gvSunsize, cos(3.0*2*3.14159/360.0) ); checkGlError("set Eyesize");
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture C 0");
    glBindTexture ( GL_TEXTURE_CUBE_MAP, bitmap_ids[0] ); checkGlError("glBindTexture 0");
    glActiveTexture(GL_TEXTURE1); checkGlError("glActiveTexture D 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    glUniform1i ( gvSamplerHandle, 0 ); checkGlError("gvSamplerHandle 0");
    glUniform1i ( gvCausture, 1 ); checkGlError("gvSamplerHandle 1");
    glBindFramebuffer(GL_FRAMEBUFFER, 0); checkGlError("glBindFramebuffer");
    glViewport(0, 0, width, height);checkGlError("glViewport");
    glClearColor(0.0,0.0,0.0,0.0); checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); checkGlError("glClear 2");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDisable(GL_BLEND);checkGlError("glEnable Blend");
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT_ALL, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements");
#endif
}


#define DECLS_CAUSTICS \
		"precision highp float;" \
	    "uniform sampler2D u_BwTexture;" \
	    "invariant varying highp float v_concentration;" \
		"invariant varying highp vec3 v_position;"

char gVertexCaustics[] =
	"attribute vec3 a_position;"
	"attribute vec2 a_normal;"
	"attribute float a_concentration;"
	DECLS_CAUSTICS

    "void main() {"
	"  float len = length(a_normal);"
	"  float reduce = len/(0.06*(1.0-exp(-len/0.06)));"
    "  v_position = a_position + 0.3* vec3(a_normal/reduce, 0.0);"
    "  gl_Position = vec4(v_position.x*2.0, v_position.y*2.0, 0.0, 1.0);"
    "  v_concentration  = a_concentration;"
    "}"
	;

char gFragmentCaustics[] =
	DECLS_CAUSTICS
    "void main() {"
	"    gl_FragColor = vec4("
	"      (gl_FrontFacing?1.0:0.0) * ((-log(v_concentration)+1.5)/7.0) "
	"     , (gl_FrontFacing?0.0:1.0) * ((-log(v_concentration)+1.5)/7.0) "
	"    ,0.1"
	"    ,1.0);"
    "}"
	;



#define DECLS_MAIN \
		"precision highp float;" \
        "const highp float c_one = 1.0;" \
		"const vec4 c_white = vec4(1.0,1.0,1.0,1.0);" \
		"const vec4 c_ambient = vec4(1.0,0.5,0.05,1.0);" \
		"const vec4 c_transparent = vec4(0.0,0.0,0.0,0.0);" \
		"const vec4 c_fog = vec4(0.3, 0.06, 0.01, 0.0);" \
	    "uniform highp samplerCube u_texture;" \
	    "uniform highp sampler2D u_causture;" \
	    "uniform highp mat4 u_trans;" \
	    "uniform highp vec3 u_eyepos;" \
	    "uniform float u_depth;" \
	    "uniform highp vec3 u_sunpos;" \
	    "uniform float u_sunsize;" \
	    "invariant varying highp vec3 v_position;" \
	    "invariant varying highp vec3 v_normal;" \
		"invariant varying highp vec3 v_reflect;" \
		"invariant varying highp vec3 v_refract;" \
		"invariant varying highp vec3 v_divi;" \
	    "invariant varying highp vec3 v_splat;" \
	    "invariant varying highp vec4 v_fog;" \
	    "invariant varying highp vec2 v_causlookup;" \
	    "invariant varying highp float v_r;" \
	    "invariant varying highp vec4 v_shine;"

char gVertexMain[] =
	"attribute vec3 a_position;"
	"attribute vec2 a_normal;"
	DECLS_MAIN
    "void main() {"
	"  float water, t;"
	"  highp vec3 dummy;"
    "  gl_Position = u_trans * vec4(a_position, 1.0);"
    "  v_position = normalize(a_position - u_eyepos);"
    "  v_normal.x = -a_normal.x;"
    "  v_normal.y = -a_normal.y;"
    "  v_normal.z = -c_one;"
    "  v_normal = normalize(v_normal);"

	"  float eta = 0.75;"
	"  vec3 incom = normalize(a_position - u_eyepos);"
	"  float cos_theta1 = dot(incom, -v_normal);"
	"  float cos_theta2 = sqrt(1.0 - ((eta * eta) * ( 1.0 - (cos_theta1 * cos_theta1))));"
	"  v_reflect = incom + 2.0 * cos_theta1 * v_normal;"
	"  v_refract = (eta * incom) + (eta * cos_theta1 - cos_theta2) * v_normal;"
	"  float fresnel_rs = (eta * cos_theta1 - cos_theta2 ) / (eta * cos_theta1 + cos_theta2);"
	"  float fresnel_rp = (eta * cos_theta2 - cos_theta1 ) / (eta * cos_theta2 + cos_theta1);"
	"  v_r = (fresnel_rs*fresnel_rs + fresnel_rp*fresnel_rp)/2.0;"
	"  t = 1.0 - v_r;"

	"  vec3 from = a_position;"
	"  vec3 to = v_refract;"
	"  float signx = (to.x>0.0) ? 1.0 : -1.0;"
	"  float signy = (to.y>0.0) ? 1.0 : -1.0;"
	"  vec3 tocorner = vec3( signx*0.5 , signy*0.5 , u_depth ) - from;"
	"  vec3 divi = tocorner/to;"
	"  if (divi.x > divi.y)"
	"  {" //front or back
	"     if (divi.z > divi.y)"
	"     {"
	"       highp vec2 temp = ( from.xz + (divi.y)*to.xz );"
	"       v_splat = vec3(temp.x, signy*0.5, temp.y);"
	"     }"
	"     else"
	"     {"
	"       v_splat = vec3(from.xy + (divi.z)*to.xy, u_depth);"
	"     }"
	"  }"
	"  else"
	"  {" //sides
	"     if (divi.z > divi.x)"
	"     {"
	"       v_splat = vec3(0.5*signx, (from.yz + (divi.x)*to.yz));"
	"     }"
	"     else"
	"     {"
	"       v_splat = vec3(from.xy + (divi.z)*to.xy, u_depth);"
	"     }"
	"  }"


	"  v_shine = (dot(u_sunpos,vec3(abs(v_reflect.x),abs(v_reflect.y),v_reflect.z)) >= u_sunsize) ? c_white : 2.0*c_ambient*v_r ;"
	"  water = length(v_splat - a_position);"
	"  v_fog = exp(-c_fog*water*3.0);"
	"  v_fog = v_fog * t;"
	"  v_fog.w = 1.0; "
	"  water = v_splat.y;"
	"  v_splat.y = -v_splat.z;"
	"  v_splat.z = water;"
	"  v_causlookup = vec2((v_splat.x-v_splat.y*0.1)*0.85+0.5, (v_splat.z-v_splat.y*0.1)*0.85+0.5);"
	"  v_position.x = dot(v_position, -v_normal); "
    "}"
	;


char gFragmentMain[] =
    "precision highp float;"
	DECLS_MAIN
	"vec4 lookup(float cc) {"
	"   return vec4("
	"      mod(floor(cc*2.0), 2.0)>0.5 ?1.0:0.0,"
	"      mod(floor(cc*4.0), 2.0)>0.5 ?1.0:0.0,"
	"      mod(floor(cc*8.0), 2.0)>0.5 ?1.0:0.0,"
	"      1.0);"
	"  "
	"}"
	""
    "void main() {"
    "    vec4 cubecol = (textureCube(u_texture, v_splat)+0.25)*0.8;"
	"    vec4 causcol = texture2D(u_causture, v_causlookup);"
	"    float cc = (causcol.r-causcol.g/9.0);"
	"    gl_FragColor =   v_shine + cubecol * v_fog * mix(0.3, 0.3+cc, -v_splat.y*2.0) * 2.0 ;"
    "}"
	;

//################################################################

#else

#include "header.h"

#define SIZE 0.3

struct Vertex {
	Vec3 pos;
	GLfloat concentration;
	Vertex(float x, float y):pos(x*SIZE, -y*SIZE, 0.0), concentration(1.0) {}
};


Vertex vertices[8]=	{
		Vertex(-2,-2),
		Vertex(-2, 1),
		Vertex( 1,-2),
		Vertex( 1, 1),
		Vertex(-1,-1),
		Vertex(-1, 2),
		Vertex( 0, -3),
		Vertex( 2,-1)
};

/*
		{{-2*SIZE, -2*SIZE, 0.0}, 1.0},
		{{-2*SIZE,  1*SIZE, 0.0}, 1.0},
		{{ 1*SIZE, -2*SIZE, 0.0}, 1.0},
		{{ 1*SIZE,  1*SIZE, 0.0}, 1.0},
		{{-1*SIZE, -1*SIZE, 0.0}, 1.0},
		{{-1*SIZE,  2*SIZE, 0.0}, 1.0},
		{{ 2*SIZE, -1*SIZE, 0.0}, 1.0},
		{{ 2*SIZE,  2*SIZE, 0.0}, 1.0},

*/
GLushort indices[8]={0,1,2,3,4,5,6,7};

char gVertex[] =
	"attribute vec3 a_position;"
	"attribute float a_concentration;"

    "void main() {"
    "  gl_Position = vec4(a_position,a_concentration);"
	"}"
	;

char gFragment[] =
    "void main() {"
	"    gl_FragColor = vec4("
	"	 	(gl_FrontFacing ? 0.3 : 0.0 ) "// - (gl_FrontFacing?0.0:1.0) / v_concentration * 0.5"
	"	 ,	(gl_FrontFacing ? 0.0 : 0.3 ) "// - (gl_FrontFacing?0.0:1.0) / v_concentration * 0.5"
//	"	 ,	(gl_FrontFacing ? 0.0 : v_concentration<=1.0 ? 0.1 : 0.0 ) / v_concentration "// - (gl_FrontFacing?0.0:1.0) / v_concentration * 0.5"
//	"    ,  (gl_FrontFacing?0.0:1.0) / v_concentration * 0.1"
	"    ,0.0"
	"    ,1.0);"
    "}"
	;

GLuint gProgram, vboIds[2], gvPosition, gvConcentration;
GLfloat width, height;


bool setupGraphics(int w, int h) {
	width=w; height=h;
    gProgram = createProgram(gVertex, gFragment);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    glGenBuffers(2, vboIds); checkGlError("glGenBuffers");

    gvPosition = glGetAttribLocation(gProgram, "a_position"); checkGlError("glGetAttribLocation");    barfIfNull("gvPosition", gvPosition);
    gvConcentration = glGetAttribLocation(gProgram, "a_concentration"); checkGlError("glGetAttribLocation");    barfIfNull("gvConcentration", gvConcentration);

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*8, vertices, GL_DYNAMIC_DRAW); checkGlError("glBufferData GL_ARRAY_BUFFER");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[1]); checkGlError("glBindBuffer A");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 8, indices, GL_DYNAMIC_DRAW);


    //main pass
	return true;
}

void renderFrame(long when) {

    glUseProgram(gProgram); checkGlError("glUseProgram Caustics");

//    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glEnableVertexAttribArray(gvPosition); checkGlError("glEnableVertexAttribArray gvPosition");
    glVertexAttribPointer(gvPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvConcentration); checkGlError("glEnableVertexAttribArray gvConcentration");
    glVertexAttribPointer(gvConcentration, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");

    glViewport(0, 0, width, height);

    glClearColor(0.0, 0.0, 0.0, 0.0); checkGlError("glClearColor");
    //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear 1");
    //    glDisable(GL_BLEND);
    glEnable(GL_BLEND);checkGlError("glEnable Blend");
    glBlendFunc(GL_ONE, GL_ONE);checkGlError("glBlendFunc");
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    //glEnable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements Caustics");

}


void poked(int type, int xs, int ys){}
void bitmap(int which, int id){}

#endif
