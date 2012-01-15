//#define JUSTCAUSTICS

/*
 *
 * X points right
 * Y points to top of screen
 * Z points into screen away from eye
 * dont care what it says on the web
 *
 * TODO:
 * Walls
 * Memory mapping
 * Settings
 * Fetch ad
 * Shallow end
 * Surroundings, more interesting reflections
 * Fish?
 * Manual plop, speedboat
 * Manual fly/zoom
 *
 */
#include "header.h"
int bitmap_ids[5];
GLfloat width, height;
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

#define PLOP_HEIGHT 0.0025
#define PLOP_WIDTH 2.5

const int VERTEX_GAPS=128.0;
const GLfloat VERTEX_PLANE_WIDTH = 1.0;
const int CAUSTURE_RES=512.0;

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
#define VERTEX_COUNT_EXTRA 4
#define INDEX_COUNT_SURF (2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1))
#define INDEX_COUNT_EXTRA 14
#define VERTEX_COUNT_ALL (VERTEX_COUNT_SURF+VERTEX_COUNT_EXTRA)
#define INDEX_COUNT_ALL (INDEX_COUNT_SURF+INDEX_COUNT_EXTRA)
#define GAP_WIDTH (VERTEX_PLANE_WIDTH/VERTEX_GAPS)

//Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1];
struct _ver {
	Vertex ices[VERTEX_GAPS+1][VERTEX_GAPS+1];
	Vertex ners[VERTEX_COUNT_EXTRA];
} ver;

#define vertices ver.ices
#define verners ver.ners

GLushort indices[INDEX_COUNT_ALL];
GLfloat velocities[VERTEX_GAPS+1][VERTEX_GAPS+1];

GLfloat matrix[16];

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


	float av = (height+width)/2.0;

	m_squelch.squelch(zoom*height/av, zoom*width/av,0.01);
	m_tot.premul(m_squelch);

	m_trans.trans(0.0,0.55,0.0);
	m_tot.premul(m_trans);

//	m_scale.stretch(zoom*height/width, zoom, 1.0);
//	m_tot.premul(m_scale);

	m_tot.transpose_out(matrix);
	//velocities[(int)(VERTEX_GAPS/2 + eye.x/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)][(int)(VERTEX_GAPS/2 + eye.y/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)] +=0.1;
}

void move_eye()
{
//	return;
	eye_long-=0.0007;
	zoom = 3.0 + cos(4.0*eye_long);
	//eye_lat+=0.001;
	update_eye_cartesian();
}

float dir=1.0;

void plop(float width)
{
   int x,y;
	dir *=-1.0;
	x = rand()%(VERTEX_GAPS+1);
	y = rand()%(VERTEX_GAPS+1);
	int n = (int)(6.0*width);
	for (int i=-n;i<=n;i++)
		for (int j=-n;j<=n;j++)
			if (x+i>=0 && x+i<=VERTEX_GAPS && y+j>=0 && y+j<=VERTEX_GAPS)
			{
				GLfloat dist = sqrt((float)i*i+j*j)/width;
				GLfloat plop = dir*PLOP_HEIGHT* (dist<0.1 ? 1.0 : (sin(dist)/dist) );
				velocities[x+i][y+j]+=plop;
			}
}

void init_vertices()
{
	eye_long =300.0*3.14159/180.0;
	eye_lat = 14.5*3.14159/180.0; //8.5

   int x,y,i;
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

   for (i=0;i<10;i++)
   {
	   plop(3.14159/4.0);
	   plop(3.14159/2.0);
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

    for (x=0;x<VERTEX_COUNT_EXTRA;x++)
    {
        verners[x].pos.x = -0.5 + (float)(x/2);
        verners[x].pos.y = -0.5 + (float)(x%2);
        verners[x].pos.z = 0.5;
        verners[x].norm.x=0.0;
        verners[x].norm.y=0.0;
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
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = (VERTEX_GAPS+1)*VERTEX_GAPS;
    indices[i++] = 2 + VERTEX_COUNT_SURF;
    indices[i++] = 0;
    indices[i++] = 0 + VERTEX_COUNT_SURF;
    indices[i++] = VERTEX_GAPS;
    indices[i++] = 1 + VERTEX_COUNT_SURF;
    indices[i++] = indices[INDEX_COUNT_SURF-1];
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = 3 + VERTEX_COUNT_SURF;
    indices[i++] = 1 + VERTEX_COUNT_SURF;
    indices[i++] = 2 + VERTEX_COUNT_SURF;
    indices[i++] = 0 + VERTEX_COUNT_SURF;

}


void adjust_vertices(long when)
{

   int x,y,i;
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
		   vertices[x][y].pos.z += velocities[x][y];

   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   vertices[x][y].norm.x = ( vertices[x-1][y].pos.z - vertices[x+1][y].pos.z ) / (2.0*GAP_WIDTH);
		   vertices[x][y].norm.y = ( vertices[x][y-1].pos.z - vertices[x][y+1].pos.z ) / (2.0*GAP_WIDTH);
		   //these point down towards +ve z
	   }
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   float g = 12.0*GAP_WIDTH;
		   float delnormx = ( vertices[x+1][y].norm.x - vertices[x-1][y].norm.x ) / g;
		   float delnormy = ( vertices[x][y+1].norm.y - vertices[x][y-1].norm.y ) / g;
//		   vertices[x][y].concentration =1.0/(1.0+abs((1.0+delnormx/6.0)*(1.0+delnormy/6.0)));//depth
		   vertices[x][y].concentration = fabs((1.0+delnormx)*(1.0+delnormy));//depth
	   }
   /*
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
		   vertices[x][y].concentration = 0.33*(vertices[x][y].concentration + 2.0*vertices[x+1][y].concentration);
   for (y=1;y<VERTEX_GAPS;y++)
	   for (x=1;x<VERTEX_GAPS;x++)
		   vertices[x][y].concentration = 0.33*(vertices[x][y].concentration + 2.0*vertices[x][y+1].concentration);

   for (x=VERTEX_GAPS-1;x>0;x--)
	   for (y=1;y<VERTEX_GAPS;y++)
		   vertices[x][y].concentration = 0.33*(vertices[x][y].concentration + 2.0*vertices[x-1][y].concentration);
   for (y=VERTEX_GAPS-1;y>0;y--)
	   for (x=1;x<VERTEX_GAPS;x++)
		   vertices[x][y].concentration = 0.33*(vertices[x][y].concentration + 2.0*vertices[x][y-1].concentration);
*/
   for (i=0;i<VERTEX_GAPS+1;i++)
   {
	   vertices[0][i].norm.x = vertices[1][i].norm.x;
	   vertices[0][i].norm.y = vertices[1][i].norm.y;
	   vertices[VERTEX_GAPS][i].norm.x = vertices[VERTEX_GAPS-1][i].norm.x;
	   vertices[VERTEX_GAPS][i].norm.y = vertices[VERTEX_GAPS-1][i].norm.y;
	   vertices[i][0].norm.x = vertices[i][1].norm.x;
	   vertices[i][0].norm.y = vertices[i][1].norm.y;
	   vertices[i][VERTEX_GAPS].norm.x = vertices[i][VERTEX_GAPS-1].norm.x;
	   vertices[i][VERTEX_GAPS].norm.y = vertices[i][VERTEX_GAPS-1].norm.y;

   }

   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   GLfloat neighbours = (
				   vertices[x][y+1].pos.z +
				   vertices[x][y-1].pos.z +
				   vertices[x+1][y].pos.z +
				   vertices[x-1][y].pos.z
			   )
			   + (
				   vertices[x+1][y+1].pos.z +
				   vertices[x+1][y-1].pos.z +
				   vertices[x-1][y+1].pos.z +
				   vertices[x-1][y-1].pos.z
			   )*0.707;
		   GLfloat force = neighbours - 4.0*1.707*vertices[x][y].pos.z;
		   //GLfloat blur =  neighbours + 4.0*(1.707)*vertices[x][y].pos.z;
		   velocities[x][y] += force*0.2;// - 0.001*(vertices[x][y].concentration-1.0);//copysign(0.00003, (vertices[x][y].concentration-1.0)) ;
	   }
/*
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

 //   glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);checkGlError("glTexParameterf");

//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#ifndef JUSTCAUSTICS
    glGenTextures(1, &gvCausticsTexture); checkGlError("glGenTextures");
    glGenFramebuffers(1, &gvFrameBuffer); checkGlError("glGenFramebuffers A 1");
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture A 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAUSTURE_RES, CAUSTURE_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); checkGlError("glTexImage2D 1");
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer); checkGlError("glBindFramebuffer 3");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gvCausticsTexture, 0); checkGlError("glFramebufferTexture2D 3");
#endif

//    glDepthRangef(-20.0, 20.0);

    checkGlError("glViewport");
    return true;
}


void renderFrame(long when) {
	adjust_vertices(when);
	move_eye();

//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*VERTEX_COUNT_ALL, vertices, GL_DYNAMIC_DRAW); checkGlError("glBufferData GL_ARRAY_BUFFER");

    //caustics pass

    glUseProgram(gProgramCaustics); checkGlError("glUseProgram Caustics");

//    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glEnableVertexAttribArray(gvPositionCaustics); checkGlError("glEnableVertexAttribArray gvPositionCaustics");
    glVertexAttribPointer(gvPositionCaustics, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvNormalCaustics); checkGlError("glEnableVertexAttribArray gvNormalCaustics");
    glVertexAttribPointer(gvNormalCaustics, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvConcentrationCaustics); checkGlError("glEnableVertexAttribArray gvConcentrationCaustics");
    glVertexAttribPointer(gvConcentrationCaustics, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(5*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");

    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture B 2");
    glBindTexture ( GL_TEXTURE_2D, bitmap_ids[1] ); checkGlError("glBindTexture 1");
    glUniform1i ( gvBwTexCaustics, 0 ); checkGlError("gvSamplerHandle 0");

#ifndef JUSTCAUSTICS
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer);  glViewport(0, 0, CAUSTURE_RES, CAUSTURE_RES);
#else
    glViewport(0, 0, width, height);
#endif
    glClearColor(0.0, 0.0, 0.0, 0.0); checkGlError("glClearColor");
    glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear");
    //    glDisable(GL_BLEND);
    glEnable(GL_BLEND);checkGlError("glEnable Blend");
    glBlendFunc(GL_ONE, GL_ONE);checkGlError("glBlendFunc");
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glDisable(GL_DEPTH_TEST);
    //glEnable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT_SURF, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements Caustics");

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
    //glGenerateMipmap(GL_TEXTURE_2D);
    glUniform1i ( gvSamplerHandle, 0 ); checkGlError("gvSamplerHandle 0");
    glUniform1i ( gvCausture, 1 ); checkGlError("gvSamplerHandle 1");


//    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(0, 0, width, height);checkGlError("glViewport");
    glClearColor(0.0,0.0,0.0,0.0); checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); checkGlError("glClear");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDisable(GL_BLEND);checkGlError("glEnable Blend");
    glEnable(GL_DEPTH_TEST);
    //glDepthRangef(-1.0, 1.0);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT_ALL, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements");
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}


#define DECLS_CAUSTICS \
		"precision highp float;" \
	    "uniform sampler2D u_BwTexture;" \
	    "invariant varying float v_concentration;" \
		"invariant varying vec3 v_position;"

char gVertexCaustics[] =
	"attribute vec3 a_position;"
	"attribute vec2 a_normal;"
	"attribute float a_concentration;"
	DECLS_CAUSTICS

    "void main() {"
    "  v_position = a_position + 0.1667* vec3(a_normal, 0.0);"
    "  gl_Position = vec4(v_position.x*2.0, v_position.y*2.0, 0.0, 1.0);"
//    "  gl_Position = vec4(a_position.x*10.0*480.0/800.0, a_position.y*10.0, 0.0, 1.0);"
    "  v_concentration  = (0.2+1.3/(1.05+a_concentration)/2.0);"
//    "  v_concentration  = a_position.z*100.0+0.5;"
    "}"
	;

char gFragmentCaustics[] =
	DECLS_CAUSTICS
    "void main() {"
	"    gl_FragColor = vec4(v_concentration,0.0,0.0,1.0);"
//	"    gl_FragColor = texture2D(u_BwTexture, vec2(v_concentration/2.0+0.25, 0.5));"
    "}"
	;



#define DECLS_MAIN \
	    "const float c_one = 1.0;" \
	    "const float c_two = 2.0;" \
		"const vec4 c_darkblue = vec4(0.2,0.2,0.5,1.0);" \
		"const vec4 c_lightblue = vec4(0.5,0.5,0.8,1.0);" \
		"const vec4 c_white = vec4(1.0,1.0,1.0,1.0);" \
		"const vec4 c_black = vec4(0.0,0.0,0.0,0.0);" \
		"const vec4 c_red = vec4(1.0,0.0,0.0,1.0);" \
		"const vec4 c_green = vec4(0.0,1.0,0.0,1.0);" \
		"const vec4 c_blue = vec4(0.0,0.0,1.0,1.0);" \
		"const vec4 c_ambient = vec4(1.0,0.75,0.5,1.0);" \
		"const vec4 c_transparent = vec4(0.0,0.0,0.0,0.0);" \
		"const vec4 c_fog = vec4(0.3, 0.06, 0.01, 0.0);" \
	    "uniform bool u_phase;" \
	    "uniform samplerCube u_texture;" \
	    "uniform sampler2D u_causture;" \
	    "uniform mat4 u_trans;" \
	    "uniform vec3 u_eyepos;" \
	    "uniform float u_depth;" \
	    "uniform vec3 u_sunpos;" \
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

	"void findsplat(in vec3 from, in vec3 to, out vec3 splat) "
	"{"
	"  float signx = (to.x>0.0) ? 1.0 : -1.0;"
	"  float signy = (to.y>0.0) ? 1.0 : -1.0;"
	"  vec3 tocorner = vec3( signx*0.5 , signy*0.5 , u_depth ) - from;"
	"  vec3 divi = tocorner/to;"
	"  if (divi.x > divi.y)"
	"  {" //front or back
	"     if (divi.z > divi.y)"
	"     {"
	"       highp vec2 temp = ( from.xz + (divi.y)*to.xz );"
	"       splat = vec3(temp.x, signy*0.5, temp.y);"
	"     }"
	"     else"
	"     {"
	"       splat = vec3(from.xy + (divi.z)*to.xy, u_depth);"
	"     }"
	"  }"
	"  else"
	"  {" //sides
	"     if (divi.z > divi.x)"
	"     {"
	"       splat = vec3(0.5*signx, (from.yz + (divi.x)*to.yz));"
	"     }"
	"     else"
	"     {"
	"       splat = vec3(from.xy + (divi.z)*to.xy, u_depth);"
	"     }"
	"  }"
	"}"

	"void fresnel(in vec3 incom, in vec3 normal, in float eta, out vec3 reflection, out vec3 refraction, out float reflectance, out float transmittance) "
	"{"
	"  float cos_theta1 = dot(incom, -normal);"
	"  float cos_theta2 = sqrt(1.0 - ((eta * eta) * ( 1.0 - (cos_theta1 * cos_theta1))));"
	"  reflection = incom + 2.0 * cos_theta1 * normal;"
	"  refraction = (eta * incom) + (eta * cos_theta1 - cos_theta2) * normal;"
	"  float fresnel_rs = (eta * cos_theta1 - cos_theta2 ) / (eta * cos_theta1 + cos_theta2);"
	"  float fresnel_rp = (eta * cos_theta2 - cos_theta1 ) / (eta * cos_theta2 + cos_theta1);"
	"  reflectance = (fresnel_rs*fresnel_rs + fresnel_rp*fresnel_rp)/2.0;"
	"  transmittance = 1.0 - reflectance;"
	"}"

    "void main() {"
	"  float water, t;"
	"  vec3 dummy;"
    "  gl_Position = u_trans * vec4(a_position, 1.0);"
    "  v_position = normalize(a_position - u_eyepos);"
//	"  normalize(a_position - u_eyepos);"
    "  v_normal.x = -a_normal.x;"
    "  v_normal.y = -a_normal.y;"
    "  v_normal.z = -c_one;"
    "  v_normal = normalize(v_normal);"

//	"  fresnel(normalize(a_position - u_eyepos), v_normal, 0.75, v_reflect, v_refract, v_r, t);"
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

//	"  findsplat(a_position, v_refract, v_splat);"
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


	"  v_shine = (dot(u_sunpos,vec3(abs(v_reflect.x),abs(v_reflect.y),v_reflect.z)) >= u_sunsize) ? c_white : c_ambient*v_r ;"
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
    "void main() {"
//    "    vec4 cubecol = textureCube(u_texture, vec3(v_position.x,0.5,-v_position.y));"
    "    vec4 cubecol = (textureCube(u_texture, v_splat)+0.25)*0.8;"
	"    vec4 causcol = texture2D(u_causture, v_causlookup);"
	"    gl_FragColor =   v_shine + cubecol * v_fog * causcol.r * 2.0 ;"
//	"    gl_FragColor =   vec4(v_reflect.x*10.0,v_reflect.y*10.0,0.2,1.0) ;"
    "}"
	;

