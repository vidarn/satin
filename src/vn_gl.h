#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include "os/os.h"

#define SATIN_SHADER_SUFFIX ""
#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #include <OpenGLES/ES3/gl.h>
        #include <OpenGLES/ES3/glext.h>
static const char *glsl_version_string = "#version 300 es\n";
    #else
        //#define GLFW_INCLUDE_GLCOREARB
        //NOTE(Vidar):This worked before
        //#include <GLFW/glfw3.h>
        //#include "opengl.h"

//static const char *glsl_version_string = "#version 330\n";
    #endif
#endif
#ifdef _WIN32
    //#include "GL/gl3w.h"
    //#define GLFW_INCLUDE_GLCOREARB
    //#define GL_GLEXT_PROTOTYPES
    //#include <GL/glew.h>
	#include "opengl/opengl.h"
    //#include <GLFW/glfw3.h>
    static const char *glsl_version_string = "#version 330\n";
    //#include <GL/glext.h>
#endif
#if defined(__GNUC__)
	#include "opengl/opengl.h"
#ifdef __APPLE__
    static const char *glsl_version_string = "#version 330\n";
#else
    static const char *glsl_version_string = "#version 120\n";
#undef SATIN_SHADER_SUFFIX
#define SATIN_SHADER_SUFFIX "_120"
#endif
#endif
static GLint check_compilation(GLuint handle,GLenum flag,
	char *error_buffer,size_t error_buffer_len,const char *name)
{
	GLint ret = GL_FALSE;
	if(flag==GL_COMPILE_STATUS){
		glGetShaderiv(handle,flag,&ret);
	}
	if(flag==GL_LINK_STATUS){
		glGetProgramiv(handle,flag,&ret);
	}
	if(ret!=GL_TRUE){
		sprintf(error_buffer,"%s:\n",name);
		size_t buff_len=strlen(error_buffer);
		GLsizei len = 0;
		if(flag==GL_COMPILE_STATUS){
			glGetShaderInfoLog(handle,(GLsizei)(error_buffer_len-buff_len),&len,
               error_buffer+buff_len);
		}
		if(flag==GL_LINK_STATUS){
			glGetProgramInfoLog(handle,(GLsizei)(error_buffer_len-buff_len),
                &len,error_buffer+buff_len);
		}
		error_buffer[len+buff_len]=0;
	}
	return ret;
}

static int compile_shader(const char* vert_source,const char * frag_source,
	char *error_buffer, int error_buffer_len, const char *version_string)
{
	int shader_handle=glCreateProgram();
	int vert_handle=glCreateShader(GL_VERTEX_SHADER);
	int frag_handle=glCreateShader(GL_FRAGMENT_SHADER);
    const char *vert_char[2] = {version_string,vert_source};
    const char *frag_char[2] = {version_string,frag_source};
	glShaderSource(vert_handle,2,vert_char,0);
	glShaderSource(frag_handle,2,frag_char,0);
	glCompileShader(vert_handle);
	if(check_compilation(vert_handle,GL_COMPILE_STATUS,
		error_buffer,error_buffer_len,"vertex shader")!=GL_TRUE)
	{
		return -1;
	}
	glCompileShader(frag_handle);
	if(check_compilation(frag_handle,GL_COMPILE_STATUS,
		error_buffer,error_buffer_len,"fragment shader")!=GL_TRUE)
	{
		return -1;
	}
	glAttachShader(shader_handle,vert_handle);
	glAttachShader(shader_handle,frag_handle);
	glLinkProgram(shader_handle);
	if(check_compilation(shader_handle,GL_LINK_STATUS,
		error_buffer,error_buffer_len,"shader program")!=GL_TRUE)
	{
		return -1;
	}
	return shader_handle;
}

static char *read_entire_file(const char* filename, const char *extension,
                              int *len, struct GameData *data)
{
    FILE *f=open_file(filename, extension, "rt", data);
	if(f==NULL){
		return 0;
	}
	fseek(f,0,SEEK_END);
	long fsize=ftell(f);
	fseek(f,0,SEEK_SET);
	char *s=(char*)calloc(fsize+1,sizeof(char));
	fread(s,fsize,1,f);
	fclose(f);
	s[fsize]=0;
	*len=(int)fsize;
	return s;
}

struct Shader{
	int shader;
	int num_params;
	int *params;
};

static struct Shader compile_shader_filename_vararg(const char* vert_source,const char * frag_source,
	char *error_buffer,int error_buffer_len, const char *version_string, struct GameData *data, va_list args)
{
	struct Shader ret={0};
	ret.shader=-1;
	int len;
	char *vert_s=read_entire_file(vert_source,".vert",&len,data);
	if(vert_s==0){
		sprintf(error_buffer,"\"%s.vert\" does not exist\n",vert_source);
		return ret;
	}

	char *frag_s=read_entire_file(frag_source,".frag",&len,data);
	if(frag_s==0){
		sprintf(error_buffer,"\"%s.frag\" does not exist\n",frag_source);
		return ret;
	}

    /* BOOKMARK(Vidar): OpenGL call
	ret.shader = compile_shader(vert_s,frag_s,error_buffer,error_buffer_len,
        glsl_version_string);
     */
    /*
	if(ret.shader!=-1){
		char *s=va_arg(args,char*);
		while(s!=0){
			int param_loc=glGetAttribLocation(ret.shader,s);
			if(param_loc==-1){
				param_loc=glGetUniformLocation(ret.shader,s);
                if(param_loc != -1){
                    printf("%s is a uniform\n",s);
                }
			}else{
                printf("%s is an attribute\n",s);
            }
			if(param_loc==-1){
                printf("Param \"%s\" not found!!\n", s);
            }else{
                ret.num_params++;
                int *tmp=ret.params;
                ret.params=(int*)calloc(ret.num_params,sizeof(int));
                if(ret.params!=0){
                    memcpy(ret.params,tmp,(ret.num_params-1)*sizeof(int));
                    free(tmp);
                }
                ret.params[ret.num_params-1]=param_loc;
            }
			s=va_arg(args,char*);
		}
	}
    */

	free(vert_s);
	free(frag_s);
	return ret;
}


//NOTE(Vidar): You must pass (char*)0 as the last argument!!
static struct Shader compile_shader_filename(const char* vert_source,const char * frag_source,
	char *error_buffer,int error_buffer_len, const char *version_string, struct GameData *data, ...)
{
    va_list args;
    va_start(args,data);
    struct Shader s = compile_shader_filename_vararg(vert_source, frag_source, error_buffer,
            error_buffer_len, version_string, data, args);
    va_end(args);
    return s;
}


