#include "header.h"


Matrix::Matrix()
{
	for (int row=0;row<4;row++)
		for (int col=0;col<4;col++)
			e[row][col] = (row==col) ? 1.0f : 0.0f ;
}
void Matrix::transpose_out(GLfloat * to)
{
	for (int col=0;col<4;col++)
		for (int row=0;row<4;row++)
			*to++ = e[row][col];
}
void Matrix::rot(GLfloat angle, int x, int y)
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
void Matrix::rot_z(GLfloat angle) { rot(angle, 0,1); }
void Matrix::rot_x(GLfloat angle) { rot(angle, 1,2); }
void Matrix::rot_y(GLfloat angle) { rot(angle, 2,0); }
void Matrix::pers(float dist)
{
	e[3][2]=1.0/dist;
}
void Matrix::premul(Matrix & pre)
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
void Matrix::stretch(GLfloat x, GLfloat y, GLfloat z)
{
	for (int col=0;col<4;col++)
	{
		e[0][col]*=x;
		e[1][col]*=y;
		e[2][col]*=z;
	}
}


void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
        hell: goto hell;
    }
}

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader vertex");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader fragment");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}


extern "C" {
    JNIEXPORT void JNICALL Java_com_didlio_android_poolpaper_C_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_didlio_android_poolpaper_C_step(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_didlio_android_poolpaper_C_bitmap(JNIEnv * env, jobject obj, jint which, jint id);
};



JNIEXPORT void JNICALL Java_com_didlio_android_poolpaper_C_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_didlio_android_poolpaper_C_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}

JNIEXPORT void JNICALL Java_com_didlio_android_poolpaper_C_bitmap(JNIEnv * env, jobject obj, jint which, jint id)
{
    bitmap(which, id);
}







