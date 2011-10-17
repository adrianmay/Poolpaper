
/*
 * TODO:
 * Walls
 * Fog
 * Memory mapping
 * Fresnel (ambient above)
 * Caustics
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
int bitmap_id;
GLfloat width, height;
void bitmap(int id) {bitmap_id=id;}

GLuint gProgramMain, gProgramCaustics, gvPositionMain, gvNormalMain, gvPositionCaustics, gvNormalCaustics, gvSamplerHandle, gvTrans,
	gvEyepos, gvSunpos, gvSunsize, gvDepth, gvCausticsTexture, gvCausture, gvFrameBuffer;

extern char gVertexMain[];
extern char gFragmentMain[];
extern char gVertexCaustics[];
extern char gFragmentCaustics[];

struct Vertex {
	Vec3 pos;
	Vec2 norm;
};

#define PLOP_RATE 1200
#define PLOP_SIZE 0.0012
#define PLOP_WIDTH 1.0

const int VERTEX_GAPS=150;
const GLfloat VERTEX_PLANE_WIDTH = 1.0f;

GLfloat eye_long = 145*3.14159/180.0;
GLfloat eye_lat = 3.14159/14.0;
GLfloat eye_dist = 1.0;
//GLfloat eye_long = 0.0;
//GLfloat eye_lat = 3.14159/2.0;
//GLfloat eye_dist = 2.0;
Vec3 eye;
Vec3 sun;

#define VERTEX_COUNT (VERTEX_GAPS+1)*(VERTEX_GAPS+1)
#define INDEX_COUNT 2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1)

Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1];
GLushort indices[INDEX_COUNT];
GLfloat velocities[VERTEX_GAPS+1][VERTEX_GAPS+1];

GLfloat matrix[16];

void update_eye_cartesian()
{
	eye.z = -eye_dist * sin(eye_lat);
	GLfloat temp = eye_dist * cos(eye_lat);
	eye.x = temp * sin(eye_long);
	eye.y = -temp * cos(eye_long);
	Matrix m_tot, m_rot_x, m_rot_z, m_pers, m_scale;
	m_scale.stretch(4.0f*height/width, 4.0f, 1.0);
	m_rot_z.rot_z(-eye_long);
	m_rot_x.rot_x(3.14159/2.0-eye_lat);
	m_pers.pers(eye_dist);
	m_tot.premul(m_rot_z);
	m_tot.premul(m_rot_x);
	m_tot.premul(m_pers);
	m_tot.premul(m_scale);
	m_tot.transpose_out(matrix);
	//velocities[(int)(VERTEX_GAPS/2 + eye.x/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)][(int)(VERTEX_GAPS/2 + eye.y/3.0*VERTEX_GAPS/VERTEX_PLANE_WIDTH)] +=0.1;
}

void move_eye()
{
	return;
	eye_long-=0.007*3.0;
//	eye_lat+=0.002f;
	update_eye_cartesian();
}

float dir=1.0;

void plop()
{
   int x,y;
	if (rand()%PLOP_RATE)
	{
		dir *=-1.0;
		GLfloat plop_width = PLOP_WIDTH;//(rand()%1+1);
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

void init_vertices()
{

   int x,y,i;
   for (x=0;x<VERTEX_GAPS+1;x++)
       for (y=0;y<VERTEX_GAPS+1;y++)
       {
           vertices[x][y].pos.x=((GLfloat)(x-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].pos.y=((GLfloat)(y-VERTEX_GAPS/2))*VERTEX_PLANE_WIDTH/VERTEX_GAPS;
           vertices[x][y].pos.z=0.0f;
           vertices[x][y].norm.x=0;
           vertices[x][y].norm.y=0;
           velocities[x][y]=0.0;
       }
   for (i=0;i<50;i++)
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

void adjust_vertices()
{
   int x,y,i;
   //plop();
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
		   velocities[x][y] += acc*2;
		   //velocities[x][y] *= 0.999;
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

GLuint vboIds[2];

void getLocationsMain()
{
    gvPositionMain = glGetAttribLocation(gProgramMain, "a_position"); checkGlError("glGetAttribLocation");
    gvNormalMain = glGetAttribLocation(gProgramMain, "a_normal"); checkGlError("glGetAttribLocation");
    gvPositionCaustics = glGetAttribLocation(gProgramCaustics, "a_position"); checkGlError("glGetAttribLocation");
    gvNormalCaustics = glGetAttribLocation(gProgramCaustics, "a_normal"); checkGlError("glGetAttribLocation");
    gvSamplerHandle = glGetUniformLocation(gProgramMain, "u_sampler"); checkGlError("glGetAttribLocation");
    gvCausture = glGetUniformLocation(gProgramMain, "u_causture"); checkGlError("glGetAttribLocation");
    gvTrans = glGetUniformLocation(gProgramMain, "u_trans"); checkGlError("glGetAttribLocation");
    gvEyepos = glGetUniformLocation(gProgramMain, "u_eyepos"); checkGlError("glGetAttribLocation");
    gvSunpos = glGetUniformLocation(gProgramMain, "u_sunpos"); checkGlError("glGetAttribLocation");
    gvSunsize = glGetUniformLocation(gProgramMain, "u_sunsize"); checkGlError("glGetAttribLocation");
    gvDepth = glGetUniformLocation(gProgramMain, "u_depth"); checkGlError("glGetAttribLocation");
}

bool setupGraphics(int w, int h) {
	width=w; height=h;
    glViewport(0, 0, 128, 128);
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);

    init_vertices();

	update_eye_cartesian();
    sun.x = -eye.x+0.15;
    sun.y = -eye.y;
    sun.z = eye.z;
    float sunmag = sqrt ( sun.x*sun.x + sun.y*sun.y + sun.z*sun.z );
    sun.x/=sunmag;
    sun.y/=sunmag;
    sun.z/=sunmag;

    gProgramMain = createProgram(gVertexMain, gFragmentMain);
    if (!gProgramMain) {
        LOGE("Could not create program.");
        return false;
    }
    gProgramCaustics = createProgram(gVertexCaustics, gFragmentCaustics);
    if (!gProgramCaustics) {
        LOGE("Could not create program.");
        return false;
    }

    glGenBuffers(2, vboIds);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * INDEX_COUNT, indices, GL_DYNAMIC_DRAW);

    getLocationsMain();

    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//    eglCreatePbufferSurface();
    glDisable(GL_DEPTH_TEST);
//    glDepthRangef(-20.0, 20.0);

    checkGlError("glViewport");
    return true;
}


void renderFrame() {
	adjust_vertices();
	move_eye();

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glBufferData(GL_ARRAY_BUFFER, 5*sizeof(GLfloat)*VERTEX_COUNT, vertices, GL_DYNAMIC_DRAW); checkGlError("glBufferData GL_ARRAY_BUFFER");



    glUseProgram(gProgramCaustics); checkGlError("glUseProgram Caustics");
    glEnableVertexAttribArray(gvPositionCaustics); checkGlError("glEnableVertexAttribArray gvPositionCaustics");
    glVertexAttribPointer(gvPositionCaustics, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (const void*)0); checkGlError("glVertexAttribPointer");
//    glEnableVertexAttribArray(gvNormalCaustics); checkGlError("glEnableVertexAttribArray gvNormalCaustics");
//    glVertexAttribPointer(gvNormalCaustics, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");

    //caustics pass

    glGenTextures(1, &gvCausticsTexture);
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    /*
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    */
    glGenFramebuffers(1, &gvFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gvCausticsTexture, 0);
    glViewport(0, 0, 128, 128);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
    	LOGI("Framebuffer incomplete\n");


    glClearColor(0.0, 0.0, 0.0, 0.0); checkGlError("glClearColor");
    glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear");

    glBindTexture ( GL_TEXTURE_2D, 0 ); checkGlError("glBindTexture 1");
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);



    //main pass
    glUseProgram(gProgramMain); checkGlError("glUseProgram");
    glEnableVertexAttribArray(gvPositionMain); checkGlError("glEnableVertexAttribArray gvPositionMain");
    glVertexAttribPointer(gvPositionMain, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (const void*)0); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvNormalMain); checkGlError("glEnableVertexAttribArray gvNormalMain");
    glVertexAttribPointer(gvNormalMain, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");

    glUniform3f ( gvEyepos, eye.x, eye.y, eye.z ); checkGlError("set Eyepos");
    glUniformMatrix4fv(	gvTrans, 1, false, matrix); checkGlError("set matrix");
    glUniform1f ( gvSunsize, cos(2.0*2*3.14159/360.0) ); checkGlError("set Eyesize");
    glUniform1f ( gvDepth, 0.5 ); checkGlError("set depth");
    glUniform3f ( gvSunpos, sun.x, sun.y, sun.z ); checkGlError("set Eyepos");
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture 0");
    glBindTexture ( GL_TEXTURE_CUBE_MAP, bitmap_id ); checkGlError("glBindTexture 0");
    glActiveTexture(GL_TEXTURE1); checkGlError("glActiveTexture 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    glUniform1i ( gvSamplerHandle, 0 ); checkGlError("gvSamplerHandle 0");
    glUniform1i ( gvCausture, 1 ); checkGlError("gvSamplerHandle 1");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    glClearColor(0.5, 0.5, 0.5, 1.0f); checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); checkGlError("glClear");
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements");

    glDeleteTextures(1, &gvCausticsTexture);
    glDeleteFramebuffers(1, &gvFrameBuffer);
}


#define DECLS_MAIN \
	    "const float c_one = 1.0;" \
	    "const float c_two = 2.0;" \
		"const vec4 c_darkblue = vec4(0.2,0.2,0.5,1.0);" \
		"const vec4 c_lightblue = vec4(0.5,0.5,0.8,1.0);" \
		"const vec4 c_white = vec4(1.0,1.0,1.0,1.0);" \
		"const vec4 c_ambient = vec4(1.5,1.0,0.5,1.0);" \
		"const vec4 c_transparent = vec4(0.0,0.0,0.0,0.0);" \
		"const vec4 c_fog = vec4(0.25, 0.5, 0.9, 1.0);" \
	    "uniform bool u_phase;" \
	    "uniform samplerCube u_texture;" \
	    "uniform sampler2D u_causture;" \
	    "uniform mat4 u_trans;" \
	    "uniform vec3 u_eyepos;" \
	    "uniform float u_depth;" \
	    "uniform vec3 u_sunpos;" \
	    "uniform float u_sunsize;" \
	    "invariant varying vec3 v_normal;" \
		"invariant varying vec3 v_reflect;" \
		"invariant varying vec3 v_refract;" \
		"invariant varying vec3 v_divi;" \
	    "invariant varying vec3 v_splat;" \
	    "invariant varying vec4 v_fog;" \
	    "invariant varying vec4 v_shine;"

char gVertexMain[] =
	"attribute vec3 a_position;"
	"attribute vec2 a_normal;"
	DECLS_MAIN

	"void findsplat(in vec3 from, in vec3 to, out vec3 splat) "
	"{"
	"  float signx = (to.x>0.0) ? 1.0 : -1.0;"
	"  float signy = (to.y>0.0) ? 1.0 : -1.0;"
	"  vec3 tocorner = vec3( signx*0.5 , signy*0.5 , -u_depth ) - from;"
	"  vec3 divi = tocorner/to;"
	"  if (divi.x > divi.y)"
	"  {" //front or back
	"     if (divi.z > divi.y)"
	"     {"
	"       vec2 temp = ( from.xz + (divi.y)*to.xz );"
	"       splat = vec3(temp.x, signy*0.5, temp.y);"
	"     }"
	"     else"
	"     {"
	"       splat = vec3(from.xy + (divi.z)*to.xy, -u_depth);"
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
	"       splat = vec3(from.xy + (divi.z)*to.xy, -u_depth);"
	"     }"
	"  }"
	"}"

	"void fresnel(in vec3 incom, in vec3 normal, in float index_external, in float index_internal, out float reflectance, out float transmittance) "
	"{"
	"  float eta = index_external/index_internal;"
	"  float cos_theta1 = dot(incom, normal);"
	"  float cos_theta2 = sqrt(1.0 - ((eta * eta) * ( 1.0 - (cos_theta1 * cos_theta1))));"
	"  float fresnel_rs = (index_external * cos_theta1 - index_internal * cos_theta2 ) / (index_external * cos_theta1 + index_internal * cos_theta2);"
	"  float fresnel_rp = (index_internal * cos_theta1 - index_external * cos_theta2 ) / (index_internal * cos_theta1 + index_external * cos_theta2);"
	"  reflectance = (fresnel_rs * fresnel_rs + fresnel_rp * fresnel_rp) / 2.0;"
	"  transmittance =((1.0-fresnel_rs) * (1.0-fresnel_rs) + (1.0-fresnel_rp) * (1.0-fresnel_rp)) / 2.0;"
	"}"

    "void main() {"
    "  v_normal.x = a_normal.x;"
    "  v_normal.y = a_normal.y;"
    "  v_normal.z = c_one;"
    "  v_normal = normalize(v_normal);"
    "  v_reflect = normalize(a_position - u_eyepos);"
	"  v_refract = refract(v_reflect, v_normal, 0.75);"
	"  v_reflect = reflect(v_reflect, -v_normal);"
	"  float water;"
	"  water = length(v_splat - a_position);"
	"  v_fog = pow(c_fog, vec4(water, water, water, 1.0));"
	"  float r, t;"
	"  fresnel(-v_refract, v_normal, 1.33, 1.0, r, t);"
	"  v_fog = v_fog * t;"
    "  gl_Position = u_trans * vec4(a_position, 1.0);"
	"  fresnel(-v_reflect, v_normal, 1.0, 1.33, r, t);"
	"  v_shine = (dot(u_sunpos,v_reflect) >= u_sunsize) ? c_white : c_ambient*r;"
	"  findsplat(a_position, v_refract, v_splat);"
	"  water = v_splat.y;"
	"  v_splat.y = -v_splat.z;"
	"  v_splat.z = water;"
//    "  v_position = a_position;"
    "}"
	;
//	"  vec2 texcoord = mod(floor(v_splat * 10.0), 2.0);\n"
//	"  float delta = abs(texcoord.x - texcoord.y);\n"
//	"  v_colour = v_colour + mix(c_darkblue, c_lightblue, delta);\n"
//	"  texture2D(u_texture, (v_splat+0.25)*5.0); "

//    "			     0.2*((v_normal.x+v_normal.y)/2.0)*c_darkblue+0.2*(1.0-(v_normal.x+v_normal.y)/2.0)*c_lightblue"
//	"               + (( dot(normalize(gl_Position-u_sunpos),v_reflect) >= u_sunsize) ? c_white : c_darkblue)"


char gFragmentMain[] =
    "precision mediump float;"
	DECLS_MAIN
    "void main() {"
	"    gl_FragColor = v_shine + textureCube(u_texture, v_splat)/*texture2D(u_causture, vec2(v_splat.x, -v_splat.z))*/ *v_fog;"
//	"    gl_FragColor = v_shine + textureCube(u_texture, v_splat)*testcol()*v_fog;"
    "}"

	;
//	"  vec2 texcoord = mod(floor(v_splat * 10.0), 2.0);\n"
//	"  float delta = abs(texcoord.x - texcoord.y);\n"
//	"    mix(c_darkblue, c_lightblue, delta);\n"
//    "  gl_FragColor = v_colour;"
//	"                ( dot(u_sunpos,v_reflect) >= u_sunsize) ? c_white : "
//	"                 v_colour + ("
//	"				    ( mod( (floor(v_splat.x)+floor(v_splat.y)) ,2.0)==0.0 ) ? c_darkblue : c_lightblue);"
//	"                 vec4(v_splat.x, v_splat.y, 0.0, 1.0);"
//    "  gl_FragColor.w = 1.0;\n"


#define DECLS_CAUSTICS \
"invariant varying vec3 v_position;"


char gVertexCaustics[] =
	"attribute vec3 a_position;"
	"attribute vec2 a_normal;"
	DECLS_CAUSTICS

    "void main() {"
    "  v_position = 10.0*a_position;"
    "  gl_Position = vec4(v_position, 1.0);"
    "}"
	;


char gFragmentCaustics[] =
    "precision mediump float;"
	DECLS_CAUSTICS
    "vec4 testcol()"
    "{"
    "  vec3 temp;"
	"  temp = v_position*18.0;"
//	"  return vec4(1.0, 1.0, 1.0, 1.0) * (0.5 + 0.25*(cos(temp.x+temp.y) + cos(temp.x-temp.y)) );"
	"  return ( (cos(temp.x+temp.y) + cos(temp.x-temp.y))>0.0 ) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);"
    "}"
    "void main() {"
	"    gl_FragColor = testcol();"
    "}"
	;
//	"  vec2 texcoord = mod(floor(v_splat * 10.0), 2.0);\n"
//	"  float delta = abs(texcoord.x - texcoord.y);\n"
//	"    mix(c_darkblue, c_lightblue, delta);\n"
//    "  gl_FragColor = v_colour;"
//	"                ( dot(u_sunpos,v_reflect) >= u_sunsize) ? c_white : "
//	"                 v_colour + ("
//	"				    ( mod( (floor(v_splat.x)+floor(v_splat.y)) ,2.0)==0.0 ) ? c_darkblue : c_lightblue);"
//	"                 vec4(v_splat.x, v_splat.y, 0.0, 1.0);"
//    "  gl_FragColor.w = 1.0;\n"


