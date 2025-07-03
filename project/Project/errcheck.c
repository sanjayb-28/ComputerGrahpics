// Adapted from CSCI-4229/5229 course examples by professor Willem A. (Vlakkies) Schreuder

#include "CSCIx229.h"

void ErrCheck(const char* where)
{
   int err = glGetError();
   if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}
