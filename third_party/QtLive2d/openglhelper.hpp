#ifndef OPENGLHELPER_H
#define OPENGLHELPER_H
#include <QOpenGLFunctions>

class OpenGLHelper{
public:
    inline static QOpenGLFunctions* get(){
        return instance;
    }
    inline static void release(){
        if(instance!=nullptr)
            delete instance;
        instance=nullptr;
    }
    inline static void init(QOpenGLContext * context)
    {
        if(instance==nullptr)
        {
            instance=new QOpenGLFunctions(context);
            instance->initializeOpenGLFunctions();
        }
    }
private:
    inline OpenGLHelper(){}
    inline ~OpenGLHelper(){}
    inline static QOpenGLFunctions* instance=nullptr;
};

#ifdef _WIN32
// On Windows, GL extension functions (GL 1.2+) are not in opengl32.dll.
// Redirect raw GL calls to Qt's QOpenGLFunctions wrapper.
#define glActiveTexture OpenGLHelper::get()->glActiveTexture
#define glAttachShader OpenGLHelper::get()->glAttachShader
#define glBindBuffer OpenGLHelper::get()->glBindBuffer
#define glBindFramebuffer OpenGLHelper::get()->glBindFramebuffer
#define glBlendFuncSeparate OpenGLHelper::get()->glBlendFuncSeparate
#define glCompileShader OpenGLHelper::get()->glCompileShader
#define glCreateProgram OpenGLHelper::get()->glCreateProgram
#define glCreateShader OpenGLHelper::get()->glCreateShader
#define glDeleteFramebuffers OpenGLHelper::get()->glDeleteFramebuffers
#define glDeleteProgram OpenGLHelper::get()->glDeleteProgram
#define glDeleteShader OpenGLHelper::get()->glDeleteShader
#define glDetachShader OpenGLHelper::get()->glDetachShader
#define glDisableVertexAttribArray OpenGLHelper::get()->glDisableVertexAttribArray
#define glEnableVertexAttribArray OpenGLHelper::get()->glEnableVertexAttribArray
#define glGenFramebuffers OpenGLHelper::get()->glGenFramebuffers
#define glGenRenderbuffers OpenGLHelper::get()->glGenRenderbuffers
#define glBindRenderbuffer OpenGLHelper::get()->glBindRenderbuffer
#define glDeleteRenderbuffers OpenGLHelper::get()->glDeleteRenderbuffers
#define glRenderbufferStorage OpenGLHelper::get()->glRenderbufferStorage
#define glFramebufferRenderbuffer OpenGLHelper::get()->glFramebufferRenderbuffer
#define glFramebufferTexture2D OpenGLHelper::get()->glFramebufferTexture2D
#define glCheckFramebufferStatus OpenGLHelper::get()->glCheckFramebufferStatus
#define glGetAttribLocation OpenGLHelper::get()->glGetAttribLocation
#define glGetProgramInfoLog OpenGLHelper::get()->glGetProgramInfoLog
#define glGetProgramiv OpenGLHelper::get()->glGetProgramiv
#define glGetShaderInfoLog OpenGLHelper::get()->glGetShaderInfoLog
#define glGetShaderiv OpenGLHelper::get()->glGetShaderiv
#define glGetUniformLocation OpenGLHelper::get()->glGetUniformLocation
#define glGetVertexAttribiv OpenGLHelper::get()->glGetVertexAttribiv
#define glLinkProgram OpenGLHelper::get()->glLinkProgram
#define glShaderSource OpenGLHelper::get()->glShaderSource
#define glUniform1f OpenGLHelper::get()->glUniform1f
#define glUniform1i OpenGLHelper::get()->glUniform1i
#define glUniform4f OpenGLHelper::get()->glUniform4f
#define glUniformMatrix4fv OpenGLHelper::get()->glUniformMatrix4fv
#define glUseProgram OpenGLHelper::get()->glUseProgram
#define glValidateProgram OpenGLHelper::get()->glValidateProgram
#define glVertexAttribPointer OpenGLHelper::get()->glVertexAttribPointer
#endif

#endif // OPENGLHELPER_H
