// Adapted from CSCI-4229/5229 course examples by professor Willem A. (Vlakkies) Schreuder

#include "CSCIx229.h"
#include "shaders.h"

static char* readText(const char* file)
{
   int   n;
   char* buffer;
   FILE* f = fopen(file,"rt");
   if (!f) Fatal("Cannot open text file %s\n",file);
   fseek(f,0,SEEK_END);
   n = ftell(f);
   rewind(f);
   buffer = (char*)malloc(n+1);
   if (!buffer) Fatal("Cannot allocate %d bytes for text file %s\n",n+1,file);
   if (fread(buffer,n,1,f)!=1) Fatal("Cannot read %d bytes for text file %s\n",n,file);
   buffer[n] = 0;
   fclose(f);
   return buffer;
}

static void printShaderLog(int obj,const char* file)
{
   int len=0;
   glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for shader log\n",len);
      glGetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      free(buffer);
   }
}

static void printProgramLog(int obj)
{
   int len=0;
   glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for program log\n",len);
      glGetProgramInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s\n",buffer);
      free(buffer);
   }
}

int loadShader(const char* vertexFile, const char* fragmentFile)
{
   int program = glCreateProgram();
   int vert = glCreateShader(GL_VERTEX_SHADER);
   char* vertexText = readText(vertexFile);
   glShaderSource(vert,1,(const char**)&vertexText,NULL);
   free(vertexText);
   glCompileShader(vert);
   printShaderLog(vert,vertexFile);
   glAttachShader(program,vert);
   if (fragmentFile) {
       int frag = glCreateShader(GL_FRAGMENT_SHADER);
       char* fragmentText = readText(fragmentFile);
       glShaderSource(frag,1,(const char**)&fragmentText,NULL);
       free(fragmentText);
       glCompileShader(frag);
       printShaderLog(frag,fragmentFile);
       glAttachShader(program,frag);
   }
   glLinkProgram(program);
   printProgramLog(program);
   return program;
}

void useShader(int shader)
{
   glUseProgram(shader);
}

void deleteShader(int shader)
{
   glDeleteProgram(shader);
}