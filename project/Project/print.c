// Adapted from CSCI-4229/5229 course examples by professor Willem A. (Vlakkies) Schreuder

#include "CSCIx229.h"

#define LEN 8192

void Print(const char* format , ...)
{
   char    buf[LEN];
   char*   ch=buf;
   va_list args;
   
   va_start(args,format);
   vsnprintf(buf,LEN,format,args);
   va_end(args);
   
   while (*ch)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
}