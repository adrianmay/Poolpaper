#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "poolpaperc"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

bool setupGraphics(int w, int h);
void renderFrame();
void bitmap(int id);



GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
void printGLString(const char *name, GLenum s);
void checkGlError(const char* op);


