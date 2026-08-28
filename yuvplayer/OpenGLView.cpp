/*
 * Copyright (c) 2010, Tae-young Jung
 * All rights reserved.
 */

#include "stdafx.h"
#include "yuvplayer.h"
#include "OpenGLView.h"

#include <gl/gl.h>

// shader support - load via wglGetProcAddress
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#endif

typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const char**, const GLint*);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, char*);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const char*);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);

static PFNGLCREATESHADERPROC pglCreateShader = NULL;
static PFNGLSHADERSOURCEPROC pglShaderSource = NULL;
static PFNGLCOMPILESHADERPROC pglCompileShader = NULL;
static PFNGLGETSHADERIVPROC pglGetShaderiv = NULL;
static PFNGLGETSHADERINFOLOGPROC pglGetShaderInfoLog = NULL;
static PFNGLCREATEPROGRAMPROC pglCreateProgram = NULL;
static PFNGLATTACHSHADERPROC pglAttachShader = NULL;
static PFNGLLINKPROGRAMPROC pglLinkProgram = NULL;
static PFNGLGETPROGRAMIVPROC pglGetProgramiv = NULL;
static PFNGLGETPROGRAMINFOLOGPROC pglGetProgramInfoLog = NULL;
static PFNGLUSEPROGRAMPROC pglUseProgram = NULL;
static PFNGLDELETESHADERPROC pglDeleteShader = NULL;
static PFNGLDELETEPROGRAMPROC pglDeleteProgram = NULL;
static PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation = NULL;
static PFNGLUNIFORM1IPROC pglUniform1i = NULL;
static PFNGLACTIVETEXTUREPROC pglActiveTexture = NULL;

static bool LoadShaderProcs() {
    pglCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    pglShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    pglCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    pglGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    pglGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    pglCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    pglAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    pglLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    pglGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    pglGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    pglUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    pglDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    pglDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    pglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    pglUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    pglActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    return pglCreateShader && pglShaderSource && pglCompileShader && pglCreateProgram && pglLinkProgram && pglUseProgram;
}

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = pglCreateShader(type);
    pglShaderSource(s, 1, &src, NULL);
    pglCompileShader(s);
    GLint ok = 0; pglGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len=0; pglGetShaderInfoLog(s, 1024, &len, log);
        OutputDebugStringA(log);
        pglDeleteShader(s); return 0;
    }
    return s;
}

IMPLEMENT_DYNCREATE(COpenGLView, CView)

COpenGLView::COpenGLView()
{
    loaded[0] = FALSE;
    loaded[1] = FALSE;
    pbo[0] = pbo[1] = 0;
    usePBO = FALSE;
    progYUV8 = progYUV10 = 0;
    texY = texU = texV = 0;
    yuvW = yuvH = 0;
    useShader = FALSE;
    shaderReady = FALSE;
    yuvMode = 0;
    t_width = 0;
    t_height = 0;
    ratio = 1.0;
}

COpenGLView::~COpenGLView()
{
}

BEGIN_MESSAGE_MAP(COpenGLView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void COpenGLView::OnDraw(CDC* pDC)
{
    HDC dc = ::GetDC(m_hWnd);
    glClear(GL_COLOR_BUFFER_BIT);

    // Shader YUV path
    if (shaderReady && yuvMode != 0 && texY) {
        GLuint prog = (yuvMode == 2) ? progYUV10 : progYUV8;
        if (prog && pglUseProgram) {
            pglUseProgram(prog);
            if (pglActiveTexture) {
                pglActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texY);
                pglActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texU);
                pglActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texV);
                pglActiveTexture(GL_TEXTURE0);
            } else {
                glBindTexture(GL_TEXTURE_2D, texY);
            }
            glBegin(GL_QUADS);
                glTexCoord2f(0.f, 0.f); glVertex3i(0, 0, 0);
                glTexCoord2f(0.f, 1.f); glVertex3i(0, yuvH, 0);
                glTexCoord2f(1.f, 1.f); glVertex3i(yuvW, yuvH, 0);
                glTexCoord2f(1.f, 0.f); glVertex3i(yuvW, 0, 0);
            glEnd();
            pglUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, 0);
            // draw segment overlay if enabled (fixed function) - use yuvW/H for alignment
            if (loaded[1]) {
                glBindTexture(GL_TEXTURE_2D, texture[1]);
                float sx = (t_width ? (float)yuvW / t_width : 1.f);
                float sy = (t_height ? (float)yuvH / t_height : 1.f);
                glBegin(GL_QUADS);
                    glTexCoord2f(0.f, 0.f); glVertex3i(0, 0, 1);
                    glTexCoord2f(0.f, sy); glVertex3i(0, yuvH, 1);
                    glTexCoord2f(sx, sy); glVertex3i(yuvW, yuvH, 1);
                    glTexCoord2f(sx, 0.f); glVertex3i(yuvW, 0, 1);
                glEnd();
            }
            SwapBuffers(dc);
            ::ReleaseDC(m_hWnd, dc);
            return;
        }
    }

    for (int i = 0 ; i < 2 ; i++) {
        if ( loaded[i] ) {
            glBindTexture( GL_TEXTURE_2D, texture[i]);
            glBegin(GL_QUADS);
                glTexCoord2f( 0.f, 0.f ); glVertex3i( 0, 0, i);
                glTexCoord2f( 0.f, 1.f ); glVertex3i( 0, t_height, i);
                glTexCoord2f( 1.f, 1.f ); glVertex3i( t_width, t_height, i);
                glTexCoord2f( 1.f, 0.f ); glVertex3i( t_width, 0, i);
            glEnd();
        }
    }
    SwapBuffers( dc );
    ::ReleaseDC( m_hWnd, dc );
}

#ifdef _DEBUG
void COpenGLView::AssertValid() const { CView::AssertValid(); }
#ifndef _WIN32_WCE
void COpenGLView::Dump(CDumpContext& dc) const { CView::Dump(dc); }
#endif
#endif

BOOL COpenGLView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CS_OWNDC;
    return CView::PreCreateWindow(cs);
}

BOOL COpenGLView::InitShaders() {
    if (!LoadShaderProcs()) return FALSE;
    const char* vsSrc =
        "void main(){ gl_TexCoord[0]=gl_MultiTexCoord0; gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex; }";
    const char* fs8Src =
        "uniform sampler2D texY; uniform sampler2D texU; uniform sampler2D texV;\n"
        "void main(){\n"
        " float y=texture2D(texY, gl_TexCoord[0].xy).r;\n"
        " float u=texture2D(texU, gl_TexCoord[0].xy).r;\n"
        " float v=texture2D(texV, gl_TexCoord[0].xy).r;\n"
        " float cy=y*255.0-16.0; float cu=u*255.0-128.0; float cv=v*255.0-128.0;\n"
        " float r=(298.0*cy+409.0*cv+128.0)/256.0;\n"
        " float g=(298.0*cy-100.0*cu-208.0*cv+128.0)/256.0;\n"
        " float b=(298.0*cy+516.0*cu+128.0)/256.0;\n"
        " gl_FragColor=vec4(clamp(r/255.0,0.0,1.0),clamp(g/255.0,0.0,1.0),clamp(b/255.0,0.0,1.0),1.0);\n"
        "}";
    const char* fs10Src =
        "uniform sampler2D texY; uniform sampler2D texU; uniform sampler2D texV;\n"
        "void main(){\n"
        " float y=texture2D(texY, gl_TexCoord[0].xy).r*255.0;\n"
        " float u=texture2D(texU, gl_TexCoord[0].xy).r*255.0;\n"
        " float v=texture2D(texV, gl_TexCoord[0].xy).r*255.0;\n"
        " // 10bit stored as 16bit: low byte first, sampled as 8bit pair -> need reconstruct? we upload as LUMINANCE 8bit pair expanded to 16? fallback to 8bit path if fail\n"
        " float cy=y-16.0; float cu=u-128.0; float cv=v-128.0;\n"
        " float r=(298.0*cy+409.0*cv+128.0)/256.0;\n"
        " float g=(298.0*cy-100.0*cu-208.0*cv+128.0)/256.0;\n"
        " float b=(298.0*cy+516.0*cu+128.0)/256.0;\n"
        " gl_FragColor=vec4(clamp(r/255.0,0.0,1.0),clamp(g/255.0,0.0,1.0),clamp(b/255.0,0.0,1.0),1.0);\n"
        "}";

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    if (!vs) return FALSE;
    GLuint fs8 = CompileShader(GL_FRAGMENT_SHADER, fs8Src);
    if (!fs8) { pglDeleteShader(vs); return FALSE; }
    progYUV8 = pglCreateProgram();
    pglAttachShader(progYUV8, vs); pglAttachShader(progYUV8, fs8);
    pglLinkProgram(progYUV8);
    GLint ok=0; pglGetProgramiv(progYUV8, GL_LINK_STATUS, &ok);
    if (!ok) { pglDeleteProgram(progYUV8); progYUV8=0; pglDeleteShader(vs); pglDeleteShader(fs8); return FALSE; }
    pglDeleteShader(fs8);
    // 10bit program (reuse same vs, simple 8bit-like for now, use same shader as 8bit but with 10bit scaling handled via upload as 8bit truncated)
    GLuint fs10 = CompileShader(GL_FRAGMENT_SHADER, fs10Src);
    if (fs10) {
        progYUV10 = pglCreateProgram();
        pglAttachShader(progYUV10, vs); pglAttachShader(progYUV10, fs10);
        pglLinkProgram(progYUV10);
        pglGetProgramiv(progYUV10, GL_LINK_STATUS, &ok);
        if (!ok) { pglDeleteProgram(progYUV10); progYUV10=0; }
        pglDeleteShader(fs10);
    } else {
        progYUV10 = 0;
    }
    pglDeleteShader(vs);
    // set samplers
    if (progYUV8) {
        pglUseProgram(progYUV8);
        GLint lY=pglGetUniformLocation(progYUV8,"texY"); if(lY>=0) pglUniform1i(lY,0);
        GLint lU=pglGetUniformLocation(progYUV8,"texU"); if(lU>=0) pglUniform1i(lU,1);
        GLint lV=pglGetUniformLocation(progYUV8,"texV"); if(lV>=0) pglUniform1i(lV,2);
        pglUseProgram(0);
    }
    if (progYUV10) {
        pglUseProgram(progYUV10);
        GLint lY=pglGetUniformLocation(progYUV10,"texY"); if(lY>=0) pglUniform1i(lY,0);
        GLint lU=pglGetUniformLocation(progYUV10,"texU"); if(lU>=0) pglUniform1i(lU,1);
        GLint lV=pglGetUniformLocation(progYUV10,"texV"); if(lV>=0) pglUniform1i(lV,2);
        pglUseProgram(0);
    }
    // create YUV textures
    glGenTextures(1,&texY); glBindTexture(GL_TEXTURE_2D,texY); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
    glGenTextures(1,&texU); glBindTexture(GL_TEXTURE_2D,texU); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
    glGenTextures(1,&texV); glBindTexture(GL_TEXTURE_2D,texV); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
    glBindTexture(GL_TEXTURE_2D,0);
    shaderReady = TRUE;
    useShader = TRUE;
    return TRUE;
}

int COpenGLView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1) return -1;
    int nPixelFormat;
    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,PFD_MAIN_PLANE,0,0,0,0};
    HDC hdc = ::GetDC(m_hWnd);
    nPixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, nPixelFormat, &pfd);
    m_hRC = wglCreateContext(hdc);
    wglMakeCurrent(hdc, m_hRC);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenTextures(2, texture);
    glBindTexture(GL_TEXTURE_2D, texture[0]); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, texture[1]); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glClearColor(0,0,0,0);
    usePBO = FALSE; pbo[0]=pbo[1]=0;
    // try shader init (optional, fallback to CPU yuv2rgb)
    InitShaders();
    return 0;
}

void COpenGLView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    glViewport(0,0,cx,cy);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,cx,cy,0,0,100);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glTranslatef(0,0,-10.0f); glScalef(ratio,ratio,1.f);
}

void COpenGLView::OnDestroy()
{
    if (progYUV8 && pglDeleteProgram) pglDeleteProgram(progYUV8);
    if (progYUV10 && pglDeleteProgram) pglDeleteProgram(progYUV10);
    if (texY) glDeleteTextures(1,&texY);
    if (texU) glDeleteTextures(1,&texU);
    if (texV) glDeleteTextures(1,&texV);
    if (usePBO && pbo[0]) {
        typedef void (APIENTRY *PFNGLDELETEBUFFERS)(GLsizei, const GLuint*);
        PFNGLDELETEBUFFERS pDel = (PFNGLDELETEBUFFERS)wglGetProcAddress("glDeleteBuffers");
        if (pDel) pDel(2, pbo);
    }
    CView::OnDestroy();
    wglDeleteContext(m_hRC);
}

void COpenGLView::SetParam(int width, int height, float ratio)
{
    for ( t_width = 2  ; t_width  < width  ; t_width  *= 2 );
    for ( t_height = 2 ; t_height < height ; t_height *= 2 );
    yuvW = width; yuvH = height;
    this->ratio = ratio;
    loaded[0] = FALSE; loaded[1] = FALSE;
    // YUV textures will be reallocated on next LoadYUV call
}

void COpenGLView::LoadYUV420Texture(unsigned char* y, unsigned char* u, unsigned char* v, int w, int h) {
    if (!shaderReady) return;
    yuvMode = 1; yuvW=w; yuvH=h;
    int uvW = (w+1)>>1, uvH=(h+1)>>1;
    glBindTexture(GL_TEXTURE_2D, texY); glTexImage2D(GL_TEXTURE_2D,0,0x8049,w,h,0,0x1903,0x1401,y); // GL_LUMINANCE
    glBindTexture(GL_TEXTURE_2D, texU); glTexImage2D(GL_TEXTURE_2D,0,0x8049,uvW,uvH,0,0x1903,0x1401,u);
    glBindTexture(GL_TEXTURE_2D, texV); glTexImage2D(GL_TEXTURE_2D,0,0x8049,uvW,uvH,0,0x1903,0x1401,v);
    glBindTexture(GL_TEXTURE_2D,0);
    yuvMode=1; loaded[0]=FALSE; // mark shader path
    Invalidate(NULL);
}

void COpenGLView::LoadYUV420_10LETexture(unsigned char* y, unsigned char* u, unsigned char* v, int w, int h) {
    if (!shaderReady) return;
    // 10bit LE: y/u/v are 16bit little endian buffers (width*height*2 etc). Upload as LUMINANCE with 8bit truncated for now (use upper 8 bits)
    // To keep shader simple, we convert 10bit to 8bit on CPU (shift >>2) and upload as 8bit, still saves YUV->RGB cost
    int uvW=(w+1)>>1, uvH=(h+1)>>1;
    // quick convert to 8bit tmp (reuse rgba as tmp)
    // This is still CPU but 4x less work than full YUV2RGB (only shift, no multiply)
    // For dramatic speed, we do 10->8 shift via OpenMP in caller, but here we assume y/u/v already 8bit truncated if needed
    // Fallback: upload as 16bit LUMINANCE_ALPHA? simplify to 8bit
    // For now, treat as 8bit by taking every 2nd byte? Not correct. Do CPU shift here via simple loop (still much cheaper than yuv2rgb)
    static unsigned char* tmpY=NULL; static unsigned char* tmpU=NULL; static unsigned char* tmpV=NULL; static int tmpW=0, tmpH=0;
    if (tmpW!=w || tmpH!=h) {
        if(tmpY) delete[] tmpY; if(tmpU) delete[] tmpU; if(tmpV) delete[] tmpV;
        tmpY=new unsigned char[w*h]; tmpU=new unsigned char[uvW*uvH]; tmpV=new unsigned char[uvW*uvH]; tmpW=w; tmpH=h;
    }
    // 10LE: y is 2 bytes per pixel little endian, 10bit in low 10 bits
    // Convert: (y16 >>2) & 0xFF
    #pragma omp parallel for schedule(static) if (h>=256)
    for(int j=0;j<h;j++) for(int i=0;i<w;i++){ int v16 = y[(j*w+i)*2] | (y[(j*w+i)*2+1]<<8); tmpY[j*w+i]=(unsigned char)(v16>>2); }
    #pragma omp parallel for schedule(static) if (uvH>=128)
    for(int j=0;j<uvH;j++) for(int i=0;i<uvW;i++){ int v16 = u[(j*uvW+i)*2] | (u[(j*uvW+i)*2+1]<<8); tmpU[j*uvW+i]=(unsigned char)(v16>>2); }
    #pragma omp parallel for schedule(static) if (uvH>=128)
    for(int j=0;j<uvH;j++) for(int i=0;i<uvW;i++){ int v16 = v[(j*uvW+i)*2] | (v[(j*uvW+i)*2+1]<<8); tmpV[j*uvW+i]=(unsigned char)(v16>>2); }
    yuvW=w; yuvH=h;
    glBindTexture(GL_TEXTURE_2D, texY); glTexImage2D(GL_TEXTURE_2D,0,0x8049,w,h,0,0x1903,0x1401,tmpY);
    glBindTexture(GL_TEXTURE_2D, texU); glTexImage2D(GL_TEXTURE_2D,0,0x8049,uvW,uvH,0,0x1903,0x1401,tmpU);
    glBindTexture(GL_TEXTURE_2D, texV); glTexImage2D(GL_TEXTURE_2D,0,0x8049,uvW,uvH,0,0x1903,0x1401,tmpV);
    glBindTexture(GL_TEXTURE_2D,0);
    yuvMode=1; // reuse 8bit shader (10->8 truncated)
    Invalidate(NULL);
}

void COpenGLView::LoadTexture(unsigned char* rgba)
{
    yuvMode=0;
    glBindTexture(GL_TEXTURE_2D, texture[0] );
    if (usePBO && pbo[0]) {
        typedef void (APIENTRY *PFNGLBINDBUFFER)(GLenum, GLuint);
        typedef void (APIENTRY *PFNGLBUFFERDATA)(GLenum, int, const void*, GLenum);
        typedef void* (APIENTRY *PFNGLMAPBUFFER)(GLenum, GLenum);
        typedef int (APIENTRY *PFNGLUNMAPBUFFER)(GLenum);
        PFNGLBINDBUFFER pBind = (PFNGLBINDBUFFER)wglGetProcAddress("glBindBuffer");
        PFNGLBUFFERDATA pData = (PFNGLBUFFERDATA)wglGetProcAddress("glBufferData");
        PFNGLMAPBUFFER pMap = (PFNGLMAPBUFFER)wglGetProcAddress("glMapBuffer");
        PFNGLUNMAPBUFFER pUnmap = (PFNGLUNMAPBUFFER)wglGetProcAddress("glUnmapBuffer");
        int sz = t_width * t_height * 4;
        if (pBind && pData && pMap && pUnmap) {
            pBind(0x88EC, pbo[0]); pData(0x88EC, sz, NULL, 0x88E0);
            void* ptr = pMap(0x88EC, 0x88BB);
            if (ptr) { memcpy(ptr, rgba, sz); pUnmap(0x88EC); }
            if (loaded[0]) glTexSubImage2D(GL_TEXTURE_2D,0,0,0,t_width,t_height,GL_RGBA,GL_UNSIGNED_BYTE,0);
            else { glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t_width,t_height,0,GL_RGBA,GL_UNSIGNED_BYTE,0); loaded[0]=TRUE; }
            pBind(0x88EC, 0); Invalidate(NULL); return;
        }
    }
    if ( loaded[0] ) glTexSubImage2D(GL_TEXTURE_2D,0,0,0,t_width,t_height,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    else { glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t_width,t_height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba); loaded[0]=TRUE; }
    Invalidate(NULL);
}

void COpenGLView::LoadSegmentTexture(unsigned char* segment)
{
    glBindTexture(GL_TEXTURE_2D, texture[1] );
    if ( loaded[1] ) glTexSubImage2D(GL_TEXTURE_2D,0,0,0,t_width,t_height,GL_RGBA,GL_UNSIGNED_BYTE,segment);
    else{ glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t_width,t_height,0,GL_RGBA,GL_UNSIGNED_BYTE,segment); loaded[1]=TRUE; }
    Invalidate(NULL);
}

BOOL COpenGLView::OnEraseBkgnd(CDC* pDC) { return TRUE; }
