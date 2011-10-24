#include <jni.h>
#include <android/log.h>

#include<GLES/gl.h>

//#include <GLES2/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
//#include <EGL/egl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "poolpaperc"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define GL_TEXTURE_WRAP_R 0x8072
bool setupGraphics(int w, int h);
void renderFrame();
void bitmap(int which, int id);



GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
void printGLString(const char *name, GLenum s);
void checkGlError(const char* op);

struct Vec2 {
	GLfloat x;
	GLfloat y;
};

struct Vec3 {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};


struct Matrix
{
	GLfloat e[4][4];
	Matrix();
	void transpose_out(GLfloat * to);
	void rot(GLfloat angle, int x, int y);
	void rot_z(GLfloat angle);
	void rot_x(GLfloat angle);
	void rot_y(GLfloat angle);
	void pers(float dist);
	void premul(Matrix & pre);
	void stretch(GLfloat x, GLfloat y, GLfloat z);
};
