
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
int bitmap_ids[5];
GLfloat width, height;
void bitmap(int which, int id)
{
	LOGI("bitmap\n");
	bitmap_ids[which]=id;
}

GLuint gProgramMain, gvPositionMain, gvNormalMain, gvSamplerHandle, gvTrans, gvEyepos, gvSunpos, gvSunsize, gvDepth, gvCausticsTexture, gvCausture;
GLuint gProgramCaustics, gvPositionCaustics, gvNormalCaustics, gvFrameBuffer, gvCaustexMain, gvConcentrationCaustics;

extern char gVertexMain[];
extern char gFragmentMain[];
extern char gVertexCaustics[];
extern char gFragmentCaustics[];

struct Vertex {
	Vec3 pos;
	Vec2 norm;
	GLfloat concentration;
};

#define PLOP_RATE 1200
#define PLOP_SIZE 0.002
#define PLOP_WIDTH 1.0

const int VERTEX_GAPS=128;
const GLfloat VERTEX_PLANE_WIDTH = 1.0f;
const int CAUSTURE_RES=256;

GLfloat eye_long = 145*3.14159/180.0;
GLfloat eye_lat = 15.0*3.14159/180.0;
GLfloat eye_dist = 0.5;
//GLfloat eye_long = 0.0;
//GLfloat eye_lat = 3.14159/2.0;
//GLfloat eye_dist = 1.0;
Vec3 eye;
Vec3 sun;

#define VERTEX_COUNT (VERTEX_GAPS+1)*(VERTEX_GAPS+1)
#define INDEX_COUNT 2*(VERTEX_GAPS*(VERTEX_GAPS+2)-1)
#define GAP_WIDTH (VERTEX_PLANE_WIDTH/VERTEX_GAPS)

Vertex vertices[VERTEX_GAPS+1][VERTEX_GAPS+1];
GLushort indices[INDEX_COUNT];
GLfloat velocities[VERTEX_GAPS+1][VERTEX_GAPS+1];
Vec2 caustex[VERTEX_GAPS+1][VERTEX_GAPS+1];

GLfloat matrix[16];

void update_eye_cartesian()
{
	eye.z = eye_dist * sin(eye_lat);
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
	eye_long-=0.03;
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
           vertices[x][y].pos.x=((GLfloat)(x-VERTEX_GAPS/2))*GAP_WIDTH;
           vertices[x][y].pos.y=((GLfloat)(y-VERTEX_GAPS/2))*GAP_WIDTH;
           vertices[x][y].pos.z=0.0f;
           vertices[x][y].norm.x=0;
           vertices[x][y].norm.y=0;
           velocities[x][y]=0.0;
           caustex[x][y].x = x%2 ? 0.1 : 0.9;
           caustex[x][y].y = y%2 ? 0.1 : 0.9;
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
		   GLfloat force = (
				   vertices[x][y+1].pos.z +
				   vertices[x][y-1].pos.z +
				   vertices[x+1][y].pos.z +
				   vertices[x-1][y].pos.z
			   ) - 4.0*vertices[x][y].pos.z;
		   velocities[x][y] += force/2.0;
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
		   vertices[x][y].norm.x = ( vertices[x-1][y].pos.z - vertices[x+1][y].pos.z ) / (2*GAP_WIDTH);
		   vertices[x][y].norm.y = ( vertices[x][y-1].pos.z - vertices[x][y+1].pos.z ) / (2*GAP_WIDTH);
	   }
   for (x=1;x<VERTEX_GAPS;x++)
	   for (y=1;y<VERTEX_GAPS;y++)
	   {
		   float delnormx = ( vertices[x+1][y].norm.x - vertices[x-1][y].norm.x ) / (2*GAP_WIDTH);
		   float delnormy = ( vertices[x][y+1].norm.y - vertices[x][y-1].norm.y ) / (2*GAP_WIDTH);
		   vertices[x][y].concentration = (1.0-delnormx/6.0)*(1.0-delnormy/6.0);//depth
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

GLuint vboIds[3];

void getLocations()
{
    gvPositionMain = glGetAttribLocation(gProgramMain, "a_position"); checkGlError("glGetAttribLocation");
    gvNormalMain = glGetAttribLocation(gProgramMain, "a_normal"); checkGlError("glGetAttribLocation");
    gvSamplerHandle = glGetUniformLocation(gProgramMain, "u_sampler"); checkGlError("glGetAttribLocation");
    gvCausture = glGetUniformLocation(gProgramMain, "u_causture"); checkGlError("glGetAttribLocation");
    gvTrans = glGetUniformLocation(gProgramMain, "u_trans"); checkGlError("glGetAttribLocation");
    gvEyepos = glGetUniformLocation(gProgramMain, "u_eyepos"); checkGlError("glGetAttribLocation");
    gvSunpos = glGetUniformLocation(gProgramMain, "u_sunpos"); checkGlError("glGetAttribLocation");
    gvSunsize = glGetUniformLocation(gProgramMain, "u_sunsize"); checkGlError("glGetAttribLocation");
    gvDepth = glGetUniformLocation(gProgramMain, "u_depth"); checkGlError("glGetAttribLocation");

    gvCaustexMain = glGetAttribLocation(gProgramCaustics, "a_caustex"); checkGlError("glGetAttribLocation");
    gvPositionCaustics = glGetAttribLocation(gProgramCaustics, "a_position"); checkGlError("glGetAttribLocation");
    gvNormalCaustics = glGetAttribLocation(gProgramCaustics, "a_normal"); checkGlError("glGetAttribLocation");
    gvConcentrationCaustics = glGetAttribLocation(gProgramCaustics, "a_concentration"); checkGlError("glGetAttribLocation");
}

bool setupGraphics(int w, int h) {
	width=w; height=h;
    glViewport(0, 0, CAUSTURE_RES, CAUSTURE_RES);
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);

    init_vertices();

	update_eye_cartesian();
    sun.x = -eye.x+0.10;
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

    glGenBuffers(3, vboIds);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * INDEX_COUNT, indices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2)*VERTEX_COUNT, caustex, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]);

    getLocations();

    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &gvCausticsTexture);
    glGenFramebuffers(1, &gvFrameBuffer);
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAUSTURE_RES, CAUSTURE_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gvCausticsTexture, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");

    glDisable(GL_DEPTH_TEST);
//    glDepthRangef(-20.0, 20.0);

    checkGlError("glViewport");
    return true;
}


void renderFrame() {
	adjust_vertices();
	move_eye();

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*VERTEX_COUNT, vertices, GL_DYNAMIC_DRAW); checkGlError("glBufferData GL_ARRAY_BUFFER");

    //caustics pass

    glUseProgram(gProgramCaustics); checkGlError("glUseProgram Caustics");
    glBindBuffer(GL_ARRAY_BUFFER, vboIds[2]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glEnableVertexAttribArray(gvCaustexMain); checkGlError("glEnableVertexAttribArray gvCaustexMain");
    glVertexAttribPointer(gvCaustexMain, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), (const void*)0); checkGlError("glVertexAttribPointer");

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]); checkGlError("glBindBuffer GL_ARRAY_BUFFER");
    glEnableVertexAttribArray(gvPositionCaustics); checkGlError("glEnableVertexAttribArray gvPositionCaustics");
    glVertexAttribPointer(gvPositionCaustics, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvNormalCaustics); checkGlError("glEnableVertexAttribArray gvNormalCaustics");
    glVertexAttribPointer(gvNormalCaustics, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvConcentrationCaustics); checkGlError("glEnableVertexAttribArray gvConcentrationCaustics");
    glVertexAttribPointer(gvConcentrationCaustics, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(5*sizeof(GLfloat))); checkGlError("glVertexAttribPointer");
    glBindFramebuffer(GL_FRAMEBUFFER, gvFrameBuffer);
    glViewport(0, 0, CAUSTURE_RES, CAUSTURE_RES);
    glClearColor(0.0, 0.0, 0.0, 0.0); checkGlError("glClearColor");
    glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear");
    glEnable(GL_BLEND);checkGlError("glEnable Blend");
    glBlendFunc(GL_ONE, GL_ONE);checkGlError("glBlendFunc");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements Caustics");
    glDisable(GL_BLEND);

    //main pass

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
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture 0");
    glBindTexture ( GL_TEXTURE_CUBE_MAP, bitmap_ids[0] ); checkGlError("glBindTexture 0");
    glActiveTexture(GL_TEXTURE1); checkGlError("glActiveTexture 1");
    glBindTexture ( GL_TEXTURE_2D, gvCausticsTexture ); checkGlError("glBindTexture 1");
    //glGenerateMipmap(GL_TEXTURE_2D);
    glUniform1i ( gvSamplerHandle, 0 ); checkGlError("gvSamplerHandle 0");
    glUniform1i ( gvCausture, 1 ); checkGlError("gvSamplerHandle 1");


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClearColor(0.5,0.45,0.15,1.0); checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); checkGlError("glClear");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLE_STRIP, INDEX_COUNT, GL_UNSIGNED_SHORT, 0); checkGlError("glDrawElements");

}
//"const vec4 c_ambient = vec4(1.5,1.0,0.5,1.0);" \


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
		"const vec4 c_ambient = vec4(0.5,0.45,0.15,1.0);" \
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
	    "invariant varying vec3 v_normal;" \
		"invariant varying vec3 v_reflect;" \
		"invariant varying vec3 v_refract;" \
		"invariant varying vec3 v_divi;" \
	    "invariant varying vec3 v_splat;" \
	    "invariant varying vec4 v_fog;" \
	    "invariant varying vec4 v_causcol;" \
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
	"  float water, r, t;"
	"  vec3 dummy;"
    "  gl_Position = u_trans * vec4(a_position, 1.0);"
    "  v_normal.x = a_normal.x;"
    "  v_normal.y = a_normal.y;"
    "  v_normal.z = c_one;"
    "  v_normal = normalize(v_normal);"
	"  fresnel(normalize(a_position - u_eyepos), v_normal, 0.75, v_reflect, v_refract, r, t);"
	"  v_shine = (dot(u_sunpos,v_reflect) >= u_sunsize) ? c_white : c_ambient*r ;"
//	"  v_shine = c_black;"
	"  findsplat(a_position, v_refract, v_splat);"
	"  water = length(v_splat - a_position);"
	"  v_fog = exp(-c_fog*water*3.0);"
	"  v_fog.w = 1.0; "
	"  v_fog = v_fog * t;"
//	"  v_causcol = texture2D(u_causture, vec2(v_splat.x+0.5, v_splat.y+0.5));"
	"  water = v_splat.y;"
	"  v_splat.y = -v_splat.z;"
	"  v_splat.z = water;"
    "}"
	;


char gFragmentMain[] =
    "precision mediump float;"
	DECLS_MAIN
    "void main() {"
    "    float pz = -v_splat.y + 1.5;"
    "    vec4 causcol = vec4(0.0);"
	"    causcol += texture2D(u_causture, vec2((v_splat.x-v_splat.y*0.1)*0.85+0.5, (v_splat.z-v_splat.y*0.1)*0.85+0.5));"
    "    vec4 cubecol = (textureCube(u_texture, v_splat)+0.25)*0.8;"
	"    gl_FragColor =  v_shine + cubecol*v_fog*causcol;"
    "}"

	;


#define DECLS_CAUSTICS \
		"uniform sampler2D u_whitetrick;" \
	    "invariant varying vec2 v_caustex;" \
	    "invariant varying float v_concentration;" \
		"invariant varying vec3 v_position;"

char gVertexCaustics[] =
	"attribute vec3 a_position;"
	"attribute vec2 a_normal;"
	"attribute vec2 a_caustex;"
	"attribute float a_concentration;"
	DECLS_CAUSTICS

    "void main() {"
    "  v_position = a_position - 0.1667* vec3(a_normal, 0.0);"
    "  gl_Position = vec4(2.0*v_position.x, 2.0*v_position.y, 0.0, 1.0);"
    "  v_caustex = a_caustex;"
    "  v_concentration  = 0.4+0.85/(1.0+a_concentration);"
    "}"
	;

char gFragmentCaustics[] =
    "precision mediump float;"
	DECLS_CAUSTICS
    "void main() {"
//    "    float d = gl_PointCoord.x + gl_PointCoord.y - 2.0;"
//    "    float c = pow(0.15,1.0+d*d);"
//	"    gl_FragColor = vec4(c,c,c,1.0);"
	"    float brightness = v_concentration;"
	"    gl_FragColor = vec4(brightness, brightness, brightness, 1.0);"//vec4(0.6,0.6,0.6,1.0);"
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


