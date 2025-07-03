// Adapted from CSCI-4229/5229 course examples by professor Willem A. (Vlakkies) Schreuder

#include "CSCIx229.h"

void Project(double fov,double asp,double dim)
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   if (fov)
      gluPerspective(fov,asp,dim/16,16*dim);
   else
      glOrtho(-asp*dim,asp*dim,-dim,+dim,-dim,+dim);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

