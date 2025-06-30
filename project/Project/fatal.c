// Adapted from CSCI-4229/5229 course examples by professor Willem A. (Vlakkies) Schreuder

#include "CSCIx229.h"

void Fatal(const char* format , ...)
{
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}
