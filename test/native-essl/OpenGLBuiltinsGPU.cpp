// Development oracle: exact original ESSL300 through a native GL driver; no production OpenGL dependency.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
constexpr GLenum GL_SHADING_LANGUAGE_VERSION=0x8B8C,GL_VERTEX_SHADER=0x8B31,GL_FRAGMENT_SHADER=0x8B30,GL_COMPILE_STATUS=0x8B81,GL_LINK_STATUS=0x8B82,GL_INFO_LOG_LENGTH=0x8B84;
constexpr GLenum GL_FRAMEBUFFER=0x8D40,GL_COLOR_ATTACHMENT0=0x8CE0,GL_FRAMEBUFFER_COMPLETE=0x8CD5,GL_RGBA32F=0x8814,GL_ELEMENT_ARRAY_BUFFER=0x8893,GL_STATIC_DRAW=0x88E4;
struct Case {
    std::string name,shader;std::uint32_t height=16,first=3,baseInstance=7,instances=1;bool indexed=false,frontTest=false,front=true;float z=0.2f,nearDepth=0,farDepth=1;bool visible=true,observed=false;float sourceZ=0;bool derivatives=false;
};

struct GLContext {
    HWND window{};HDC dc{};HGLRC context{};ATOM atom{};HINSTANCE module=GetModuleHandleW(nullptr);HMODULE library=LoadLibraryW(L"opengl32.dll");
    template<class F>F fn(const char* name){auto raw=wglGetProcAddress(name);auto v=reinterpret_cast<std::uintptr_t>(raw);if(v<=3||v==std::uintptr_t(-1))raw=GetProcAddress(library,name);if(!raw)throw std::runtime_error(std::string("GL function missing: ")+name);return reinterpret_cast<F>(raw);}
    static void check(){const auto e=glGetError();if(e!=GL_NO_ERROR)throw std::runtime_error("OpenGL error "+std::to_string(e));}
    GLContext(){try{
        WNDCLASSW cls{};cls.style=CS_OWNDC;cls.lpfnWndProc=DefWindowProcW;cls.hInstance=module;cls.lpszClassName=L"ThreeCppPrivateEsslBuiltinOracle";atom=RegisterClassW(&cls);if(!atom)throw std::runtime_error("Private hidden WGL class failed");
        window=CreateWindowExW(0,cls.lpszClassName,L"",WS_OVERLAPPED,0,0,1,1,nullptr,nullptr,module,nullptr);if(!window)throw std::runtime_error("Private hidden WGL window failed");dc=GetDC(window);
        PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof(pfd);pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;pfd.iLayerType=PFD_MAIN_PLANE;int format=ChoosePixelFormat(dc,&pfd);if(!format||!SetPixelFormat(dc,format,&pfd))throw std::runtime_error("WGL pixel format failed");
        context=wglCreateContext(dc);if(!context||!wglMakeCurrent(dc,context))throw std::runtime_error("WGL bootstrap failed");
        auto create=fn<HGLRC(WINAPI*)(HDC,HGLRC,const int*)>("wglCreateContextAttribsARB");const int attributes[]{0x2091,4,0x2092,5,0x9126,0x00000001,0};auto modern=create(dc,nullptr,attributes);if(!modern)throw std::runtime_error("WGL4.5 core context unavailable");
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);context=modern;if(!wglMakeCurrent(dc,context))throw std::runtime_error("WGL modern bind failed");check();
    }catch(...){reset();throw;}}
    void reset()noexcept{if(context){wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);}context=nullptr;if(dc&&window)ReleaseDC(window,dc);dc=nullptr;if(window)DestroyWindow(window);window=nullptr;if(atom)UnregisterClassW(L"ThreeCppPrivateEsslBuiltinOracle",module);atom=0;if(library)FreeLibrary(library);library=nullptr;}
    ~GLContext(){reset();}
    GLuint shader(GLenum type,const std::string& path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Original ESSL missing "+path);const std::string source{std::istreambuf_iterator<char>(in),{}};
        auto create=fn<GLuint(APIENTRY*)(GLenum)>("glCreateShader");auto destroy=fn<void(APIENTRY*)(GLuint)>("glDeleteShader");auto id=create(type);try{const char* text=source.data();const GLint length=GLint(source.size());fn<void(APIENTRY*)(GLuint,GLsizei,const char* const*,const GLint*)>("glShaderSource")(id,1,&text,&length);fn<void(APIENTRY*)(GLuint)>("glCompileShader")(id);GLint compiled=0;fn<void(APIENTRY*)(GLuint,GLenum,GLint*)>("glGetShaderiv")(id,GL_COMPILE_STATUS,&compiled);if(!compiled){GLint size=0;fn<void(APIENTRY*)(GLuint,GLenum,GLint*)>("glGetShaderiv")(id,GL_INFO_LOG_LENGTH,&size);std::string log(size,'\0');fn<void(APIENTRY*)(GLuint,GLsizei,GLsizei*,char*)>("glGetShaderInfoLog")(id,size,nullptr,log.data());throw std::runtime_error("Original ESSL driver compile rejected: "+path+" "+log);}return id;}catch(...){destroy(id);throw;}}
    std::vector<float> draw(const Case& c,const std::string& sources){GLuint vs=0,fs=0,program=0,texture=0,fbo=0,vao=0,index=0;
        const auto cleanup=[&]{fn<void(APIENTRY*)(GLuint)>("glUseProgram")(0);fn<void(APIENTRY*)(GLenum,GLuint)>("glBindFramebuffer")(GL_FRAMEBUFFER,0);fn<void(APIENTRY*)(GLuint)>("glBindVertexArray")(0);
            if(index)fn<void(APIENTRY*)(GLsizei,const GLuint*)>("glDeleteBuffers")(1,&index);if(vao)fn<void(APIENTRY*)(GLsizei,const GLuint*)>("glDeleteVertexArrays")(1,&vao);if(fbo)fn<void(APIENTRY*)(GLsizei,const GLuint*)>("glDeleteFramebuffers")(1,&fbo);if(texture)glDeleteTextures(1,&texture);if(program)fn<void(APIENTRY*)(GLuint)>("glDeleteProgram")(program);if(fs)fn<void(APIENTRY*)(GLuint)>("glDeleteShader")(fs);if(vs)fn<void(APIENTRY*)(GLuint)>("glDeleteShader")(vs);};
        try{
            vs=shader(GL_VERTEX_SHADER,sources+"/"+c.shader+".vert");fs=shader(GL_FRAGMENT_SHADER,sources+"/"+c.shader+".frag");program=fn<GLuint(APIENTRY*)()>("glCreateProgram")();auto attach=fn<void(APIENTRY*)(GLuint,GLuint)>("glAttachShader");attach(program,vs);attach(program,fs);fn<void(APIENTRY*)(GLuint)>("glLinkProgram")(program);GLint linked=0;fn<void(APIENTRY*)(GLuint,GLenum,GLint*)>("glGetProgramiv")(program,GL_LINK_STATUS,&linked);if(!linked){GLint size=0;fn<void(APIENTRY*)(GLuint,GLenum,GLint*)>("glGetProgramiv")(program,GL_INFO_LOG_LENGTH,&size);std::string log(size,'\0');fn<void(APIENTRY*)(GLuint,GLsizei,GLsizei*,char*)>("glGetProgramInfoLog")(program,size,nullptr,log.data());throw std::runtime_error("Original ESSL driver link rejected: "+log);}
            glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,20,c.height,0,GL_RGBA,GL_FLOAT,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            fn<void(APIENTRY*)(GLsizei,GLuint*)>("glGenFramebuffers")(1,&fbo);fn<void(APIENTRY*)(GLenum,GLuint)>("glBindFramebuffer")(GL_FRAMEBUFFER,fbo);fn<void(APIENTRY*)(GLenum,GLenum,GLenum,GLuint,GLint)>("glFramebufferTexture2D")(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
            if(fn<GLenum(APIENTRY*)(GLenum)>("glCheckFramebufferStatus")(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("RGBA32F GL framebuffer incomplete");
            glDisable(GL_SCISSOR_TEST);const GLfloat clear[]{-100,-100,-100,-100};fn<void(APIENTRY*)(GLenum,GLint,const GLfloat*)>("glClearBufferfv")(GL_COLOR,0,clear);
            glViewport(3,4,10,8);glScissor(3,4,10,8);glEnable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glFrontFace(GL_CCW);glDepthRange(c.nearDepth,c.farDepth);
            fn<void(APIENTRY*)(GLuint)>("glUseProgram")(program);fn<void(APIENTRY*)(GLsizei,GLuint*)>("glGenVertexArrays")(1,&vao);fn<void(APIENTRY*)(GLuint)>("glBindVertexArray")(vao);
            if(c.indexed){const std::uint32_t values[]{0,1,2};fn<void(APIENTRY*)(GLsizei,GLuint*)>("glGenBuffers")(1,&index);fn<void(APIENTRY*)(GLenum,GLuint)>("glBindBuffer")(GL_ELEMENT_ARRAY_BUFFER,index);fn<void(APIENTRY*)(GLenum,std::intptr_t,const void*,GLenum)>("glBufferData")(GL_ELEMENT_ARRAY_BUFFER,sizeof(values),values,GL_STATIC_DRAW);fn<void(APIENTRY*)(GLenum,GLsizei,GLenum,const void*,GLsizei,GLint,GLuint)>("glDrawElementsInstancedBaseVertexBaseInstance")(GL_TRIANGLES,3,GL_UNSIGNED_INT,nullptr,c.instances,c.first,c.baseInstance);}
            else fn<void(APIENTRY*)(GLenum,GLint,GLsizei,GLsizei,GLuint)>("glDrawArraysInstancedBaseInstance")(GL_TRIANGLES,c.first,3,c.instances,c.baseInstance);
            check();std::vector<float> bottom(20*c.height*4),top(bottom.size());glReadPixels(0,0,20,c.height,GL_RGBA,GL_FLOAT,bottom.data());check();for(std::uint32_t y=0;y<c.height;++y)std::memcpy(top.data()+y*80,bottom.data()+(c.height-1-y)*80,80*sizeof(float));cleanup();return top;
        }catch(...){cleanup();throw;}
    }
};
int main(int argc,char** argv){try{
    if(argc!=3)throw std::runtime_error("usage: OpenGLBuiltinsGPU <original ESSL sources> <pixel-output>");
    const std::vector<Case> cases{
        {"nonindexed-first3-instance7","positive"},
        {"indexed-base6-instance11","positive",16,6,11,1,true},
        {"two-instances-base5","positive",16,3,5,2},
        {"framebuffer24-offset-viewport","positive",24,3,7},
        {"negative-GL-clip-depth","negative",16,3,7,1,false,false,true,-0.5f},
        {"GL-depth-range","negative",16,3,7,1,false,false,true,-0.5f,0.2f,0.7f},
        {"GL-CCW-front","front",16,3,7,1,false,true,true},
        {"GL-CW-back","back",16,3,7,1,false,true,false},
        {"main-early-return","early",16,3,7,1,false,false,true,0.3f,0,1,true,true,0.3f},
        {"main-intermediate-read","intermediate",16,3,7,1,false,false,true,0.2f,0,1,true,true,-0.5f},
        {"positive-w2","positivew",16,3,7},
        {"negative-w-clips","negativew",16,3,7,1,false,false,true,0.2f,0,1,false},
        {"fragment-dFdy","derivatives",16,3,7,1,false,false,true,0.2f,0,1,true,false,0,true}
    };
    GLContext device;std::cout<<"device="<<glGetString(GL_RENDERER)<<" GL="<<glGetString(GL_VERSION)<<" GLSL="<<glGetString(GL_SHADING_LANGUAGE_VERSION)<<"\n";
    std::size_t checks=0,failed=0;
    for(const auto& c:cases){const auto pixels=device.draw(c,argv[1]);
        {std::filesystem::create_directories(argv[2]);std::ofstream out(std::filesystem::path(argv[2])/(c.name+".rgba32f"),std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);out.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*sizeof(float)));out.close();}
        std::size_t caseFailed=0;bool logged=false;
        for(std::uint32_t y=0;y<c.height;++y)for(std::uint32_t x=0;x<20;++x){const bool inside=c.visible && x>=3&&x<13&&y>=c.height-12&&y<c.height-4;
            float expected[4];std::fill(std::begin(expected),std::end(expected),-100.0f);
            if(inside){const float gx=x+0.5f,gy=c.height-(y+0.5f),u=(gx-3)/10,v=(gy-4)/8;const float depth=(c.z+1)*0.5f*(c.farDepth-c.nearDepth)+c.nearDepth;
                if(c.derivatives){expected[0]=1;expected[1]=1;expected[2]=1;expected[3]=0.125f;}
                else if(c.frontTest){expected[0]=c.front?1:0;expected[1]=c.front?0:1;expected[2]=depth;expected[3]=1;}
                else{expected[0]=gx;expected[1]=gy;expected[2]=depth;expected[3]=(c.instances-1)+(c.first+0.5f*u+v)/100+(c.observed?0.1f*c.sourceZ:0);}}
            for(unsigned k=0;k<4;++k){++checks;const float actual=pixels[(y*20+x)*4+k];if(!std::isfinite(actual)||std::abs(actual-expected[k])>0.0003f){++caseFailed;++failed;if(!logged){std::cout<<c.name<<" first mismatch x="<<x<<" topRow="<<y<<" channel="<<k<<" actual="<<actual<<" expected="<<expected[k]<<'\n';logged=true;}}}
        }
        std::cout<<c.name<<" failed="<<caseFailed<<"\n";
    }
    std::cout<<"cases="<<cases.size()<<" pixel-channel-checks="<<checks<<" failed="<<failed<<"\n";return failed?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
