
/*
 * TODO:
 * ~ Simple square
 * Image on square
 * Perspective
 * Waves
 *
 *
 */
#include "header.h"
int bitmap_id;
GLfloat width, height;

GLuint gProgram;
GLuint gvPositionHandle;
GLuint gvNormal;
//GLuint gvSamplerHandle;
GLuint gvTrans;
GLuint gvEyepos;
//GLuint gvSunpos;
//GLuint gvSunsize;

static const char gVertexShader[] = 
    "uniform mat4 u_trans;\n"
    "uniform vec4 u_eyepos;\n"
    "attribute vec4 a_position;\n"
    "attribute vec2 a_normal;\n"
    "varying vec4 v_normal;\n"
//	"varying vec4 v_reflect;\n"
    "void main() {\n"
    "  v_normal.x = a_normal.x;\n"
    "  v_normal.y = a_normal.y;\n"
    "  v_normal.z = 1.0;\n"
    "  v_normal.w = 1.0;\n"
//    "  v_normal = normalize(v_normal);\n"
//    "  v_reflect = normalize(a_position - u_eyepos);\n"
//	"  v_reflect -= 2.0 * dot(v_normal, v_reflect) * v_normal;\n"
    "  gl_Position = a_position;\n"
    "  gl_Position = u_trans * gl_Position;\n"
    "}\n";

static const char gFragmentShader[] = 
    "precision mediump float;\n"
//    "uniform vec4 u_sunpos;\n"
//    "uniform float u_sunsize;\n"
    "varying vec4 v_normal;\n"
//	"varying vec4 v_reflect;\n"
//    "uniform sampler2D u_texture;\n"
    "void main() {\n"
    "  gl_FragColor = ((v_normal.x+v_normal.y)/2.0)*vec4(0.0,0.0,0.1,1.0)+(1.0-(v_normal.x+v_normal.y)/2.0)*vec4(0.4,0.4,0.6,1.0) ;\n"
//    "  gl_FragColor = texture2D(u_texture, v_normal);\n"
    "  gl_FragColor.w = 1.0;\n"
    "}\n";

struct Vec2 {
	GLfloat x;
	GLfloat y;
};

struct Vec3 {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

struct Vertex {
	Vec3 pos;
	Vec2 norm;
};

const int VERTEX_GAPS=100.0f;
const GLfloat VERTEX_PLANE_WIDTH = 1.0f;

#define VERTEX_COUNT (VERTEX_GAPS+1)*(VERTEX_GAPS+1)
#define INDEX_COUNT 2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1)

Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1];
GLushort indices[INDEX_COUNT];
GLfloat velocities[VERTEX_GAPS+1][VERTEX_GAPS+1];

GLfloat matrix[16];

struct Matrix
{
	GLfloat e[4][4];
	Matrix()
	{
		for (int row=0;row<4;row++)
			for (int col=0;col<4;col++)
				e[row][col] = (row==col) ? 1.0f : 0.0f ;
	}
	void transpose_out(GLfloat * to)
	{
		for (int col=0;col<4;col++)
			for (int row=0;row<4;row++)
				*to++ = e[row][col];
	}
	void rot(GLfloat angle, int x, int y)
	{
		GLfloat co = cos(angle);
		GLfloat si = sin(angle);
		GLfloat a,b,c,d;
		a = co*e[x][x] - si*e[y][x];
		b = co*e[x][y] - si*e[y][y];
		c = si*e[x][x] + co*e[y][x];
		d = si*e[x][y] + co*e[y][y];
		e[x][x] = a;
		e[x][y] = b;
		e[y][x] = c;
		e[y][y] = d;
	}
	void rot_z(GLfloat angle) { rot(angle, 0,1); }
	void rot_x(GLfloat angle) { rot(angle, 1,2); }
	void rot_y(GLfloat angle) { rot(angle, 2,0); }
	void pers(float sev)
	{
		e[3][2]=sev;
	}
	void premul(Matrix & pre)
	{
		Matrix old(*this);
		for (int col=0;col<4;col++)
			for (int row=0;row<4;row++)
			{
				GLfloat res=0.0;
				for (int i=0;i<4;i++)
					res += pre.e[row][i] * old.e[i][col];
				e[row][col]=res;
			}
	}
	void stretch(GLfloat x, GLfloat y, GLfloat z)
	{
		for (int col=0;col<4;col++)
		{
			e[0][col]*=x;
			e[1][col]*=y;
			e[2][col]*=z;
		}
	}
};


#define PLOP_RATE 1200
#define PLOP_SIZE 0.001
#define PLOP_WIDTH 1.0

float dir=1.0;

void plop()
{
   int x,y;
	if (rand()%PLOP_RATE)
	{
		dir *=-1.0;
		GLfloat plop_width = PLOP_WIDTH;//*(rand()%2+1);
		x = rand()%(VERTEX_GAPS+1);
		y = rand()%(VERTEX_GAPS+1);
		int n = (int)(PLOP_WIDTH*5.0);
		for (int i=-n;i<=n;i++)
			for (int j=-n;j<=n;j++)
				if (x+i>=0 && x+i<=VERTEX_GAPS && y+j>=0 && y+j<=VERTEX_GAPS)
				{
					GLfloat dist = sqrt((float)i*i+j*j)/plop_width;
					GLfloat plop = dir*PLOP_SIZE*sin(dist)/(0.3+dist);
					velocities[x+i][y+j]+=plop;
				}
	}

}

void adjust_vertices()
{
   int x,y,i;
   plop();
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
		   vertices[x][y].pos.z += velocities[x][y];
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   GLfloat acc = (
				   vertices[x][y+1].pos.z +
				   vertices[x][y-1].pos.z +
				   vertices[x+1][y].pos.z +
				   vertices[x-1][y].pos.z
			   ) / 4.0 - vertices[x][y].pos.z;
		   velocities[x][y] += acc/6.0;
		   //velocities[x][y] *= 0.999;
	   }
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
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   vertices[x][y].norm.x = ( vertices[x-1][y].pos.z - vertices[x+1][y].pos.z ) / (2*VERTEX_PLANE_WIDTH/VERTEX_GAPS);
		   vertices[x][y].norm.y = ( vertices[x][y-1].pos.z - vertices[x][y+1].pos.z ) / (2*VERTEX_PLANE_WIDTH/VERTEX_GAPS);
	   }
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
}

void init_vertices()
{

   int x,y,i;
   for (x=0;x<VERTEX_GAPS+1;x++)
       for (y=0;y<VERTEX_GAPS+1;y++)
       {
           vertices[x][y].pos.x=((GLfloat)(x-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].pos.y=((GLfloat)(y-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].pos.z=0.0f;
           vertices[x][y].norm.x=((GLfloat)x)/(VERTEX_GAPS);
           vertices[x][y].norm.y=((GLfloat)y)/(VERTEX_GAPS);
           velocities[x][y]=0.0;
       }
   for (i=0;i<200;i++)
	   plop();
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
}

GLfloat eye_long = 0;
GLfloat eye_lat = 2.0;
GLfloat eye_dist = 2.0;
Vec3 eye;

bool setupGraphics(int w, int h) {
	width=w; height=h;
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);

    init_vertices();

    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "a_position"); checkGlError("glGetAttribLocation");
    gvNormal = glGetAttribLocation(gProgram, "a_normal"); checkGlError("glGetAttribLocation");
//    gvSamplerHandle = glGetUniformLocation(gProgram, "u_sampler"); checkGlError("glGetAttribLocation");
    gvTrans = glGetUniformLocation(gProgram, "u_trans"); checkGlError("glGetAttribLocation");
    gvEyepos = glGetUniformLocation(gProgram, "u_eyepos"); checkGlError("glGetAttribLocation");
//    gvSunpos = glGetUniformLocation(gProgram, "u_sunpos"); checkGlError("glGetAttribLocation");
//    gvSunsize = glGetUniformLocation(gProgram, "u_sunsize"); checkGlError("glGetAttribLocation");

//    glUniform1f ( gvSunsize, cos(5.0*2*3.14159/360.0) ); checkGlError("set Eyesize");
//    glUniform4f ( gvSunpos, 0.0, eye_dist*cos(eye_lat), eye_dist*sin(eye_lat), 1.0 ); checkGlError("set Eyepos");

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

void update_eye_cartesian()
{
	eye.z = eye_dist * sin(eye_lat);
	GLfloat temp = eye_dist * cos(eye_lat);
	eye.x = temp * sin(eye_long);
	eye.y = -temp * cos(eye_long);
//	vertices[(int)(VERTEX_GAPS/2 + eye.x/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)][(int)(VERTEX_GAPS/2 + eye.y/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)].pos.z+=0.03;
}

void renderFrame() {
	eye_long+=0.002f;
	update_eye_cartesian();
	adjust_vertices();
	Matrix m_tot, m_rot_x, m_rot_z, m_pers, m_scale;
	m_scale.stretch(4.0f, 4.0f*width/height, 1.0);
	m_rot_z.rot_z(-eye_long);
	m_rot_x.rot_x(-eye_lat);
	m_pers.pers(0.55);
	m_tot.premul(m_rot_z);
	m_tot.premul(m_rot_x);
	m_tot.premul(m_pers);
	m_tot.premul(m_scale);
	m_tot.transpose_out(matrix);

	static float time=0.0;
    time -= 0.3;
    float grey = 0.5;
    glClearColor(grey, grey, grey, 1.0f); checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); checkGlError("glClear");

    glUseProgram(gProgram); checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), vertices); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle); checkGlError("glEnableVertexAttribArray");

    glVertexAttribPointer(gvNormal, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), ((char*)vertices)+3*sizeof(GLfloat)); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvNormal); checkGlError("glEnableVertexAttribArray");
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture ( GL_TEXTURE_2D, bitmap_id );
//    glUniform1i ( gvSamplerHandle, 0 );
    glUniform4f ( gvEyepos, eye.x, eye.y, eye.z, 1.0 ); checkGlError("set Eyepos");
    glUniformMatrix4fv(	gvTrans, 1, false, matrix); checkGlError("set matrix");

    //glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, indices); checkGlError("glDrawElements");
}

void bitmap(int id) {bitmap_id=id;}








