
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
int width, height;

static const char gVertexShader[] = 
    "attribute vec4 a_position;\n"
    "attribute vec2 a_texCoord;\n"
    "varying vec2 v_texCoord;\n"
    "void main() {\n"
    "  gl_Position = a_position;\n"
    "  v_texCoord = a_texCoord;\n"
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
const int VERTEX_GAPS=10.0f;
const GLfloat VERTEX_PLANE_WIDTH = 0.5f;
Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1]; 
#define VERTEX_COUNT (VERTEX_GAPS+1)*(VERTEX_GAPS+1)
#define INDEX_COUNT 2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1)
GLushort indices[INDEX_COUNT];

void init_vertices()
{
   int x,y,i;
   for (x=0;x<VERTEX_GAPS+1;x++) 
       for (y=0;y<VERTEX_GAPS+1;y++) 
       {
           vertices[x][y].x=1500.0/width*((GLfloat)(x-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].y=1500.0/height*((GLfloat)(y-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
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
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n", gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}


void renderFrame() {
    static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f); checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); checkGlError("glClear");

    glUseProgram(gProgram); checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), vertices); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle); checkGlError("glEnableVertexAttribArray");

    glVertexAttribPointer(gvTexHandle, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), ((char*)vertices)+3*sizeof(GLfloat)); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvTexHandle); checkGlError("glEnableVertexAttribArray");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture ( GL_TEXTURE_2D, bitmap_id );
    glUniform1i ( gvSamplerHandle, 0 );
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, indices); checkGlError("glDrawElements");
}

void bitmap(int id) {bitmap_id=id;}








