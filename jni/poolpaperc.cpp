
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
GLuint gvTexHandle;
GLuint gvSamplerHandle;
GLuint gvTrans;
GLuint gvTime;

static const char gVertexShader[] = 
    "uniform mat4 u_trans;\n"
    "uniform float u_time;\n"
    "attribute vec4 a_position;\n"
    "attribute vec2 a_texCoord;\n"
    "varying vec2 v_texCoord;\n"
    "void main() {\n"
    "  v_texCoord = a_texCoord;\n"
    "  gl_Position = a_position;\n"
    "  gl_Position.z = 0.1*(gl_Position.x+0.5)*sin(10.0*gl_Position.x + u_time);\n"
    "  gl_Position = u_trans * gl_Position;\n"
    "}\n";

static const char gFragmentShader[] = 
    "precision mediump float;\n"
    "varying vec2 v_texCoord;\n"
    "uniform sampler2D u_texture;\n"
    "void main() {\n"
//    "  gl_FragColor = vec4 ( v_texCoord.x, v_texCoord.y, 0.0, 1.0 ) ;\n"
    "  gl_FragColor = texture2D(u_texture, v_texCoord);\n"
    "}\n";

struct Vertex {
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat tx;
	GLfloat ty;
};
const int VERTEX_GAPS=40.0f;
const GLfloat VERTEX_PLANE_WIDTH = 1.0f;
Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1]; 
#define VERTEX_COUNT (VERTEX_GAPS+1)*(VERTEX_GAPS+1)
#define INDEX_COUNT 2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1)
GLushort indices[INDEX_COUNT];

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
	void stretch(GLfloat x, GLfloat y)
	{
		for (int col=0;col<4;col++)
		{
			e[0][col]*=x;
			e[1][col]*=y;
		}
	}
};

void init_vertices()
{

   int x,y,i;
   for (x=0;x<VERTEX_GAPS+1;x++) 
       for (y=0;y<VERTEX_GAPS+1;y++) 
       {
           vertices[x][y].x=((GLfloat)(x-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].y=((GLfloat)(y-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].z=0.0f;
           vertices[x][y].tx=((GLfloat)x)/(VERTEX_GAPS);
           vertices[x][y].ty=((GLfloat)y)/(VERTEX_GAPS);
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
    		indices[i++]=(x+1)*(VERTEX_GAPS+1);
    }
}

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
    gvTexHandle = glGetAttribLocation(gProgram, "a_texCoord"); checkGlError("glGetAttribLocation");
    gvSamplerHandle = glGetUniformLocation(gProgram, "u_sampler"); checkGlError("glGetAttribLocation");
    gvTrans = glGetUniformLocation(gProgram, "u_trans"); checkGlError("glGetAttribLocation");
    gvTime = glGetUniformLocation(gProgram, "u_time"); checkGlError("glGetAttribLocation");

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    glEnable(GL_DEPTH_TEST);
    return true;
}

GLfloat angle1=0;
GLfloat angle2=-0.4;

void renderFrame() {
	angle1+=0.02f;
	//angle2+=0.0084f;

	Matrix m_tot, m_rot_x, m_rot_y, m_pers, m_scale;
	m_scale.stretch(1.0f, 1.0f*width/height);
	m_rot_y.rot_y(angle1);
	m_rot_x.rot_x(angle2);
	m_pers.pers(0.55);
	m_tot.premul(m_rot_y);
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

    glVertexAttribPointer(gvTexHandle, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), ((char*)vertices)+3*sizeof(GLfloat)); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvTexHandle); checkGlError("glEnableVertexAttribArray");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture ( GL_TEXTURE_2D, bitmap_id );
    glUniform1f ( gvTime, time );
    glUniform1i ( gvSamplerHandle, 0 );
    glUniformMatrix4fv(	gvTrans, 1, false, matrix);

    //glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, indices); checkGlError("glDrawElements");
}

void bitmap(int id) {bitmap_id=id;}








