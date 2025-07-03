//  Waves vertex shader
#version 120

void main()
{
   //  Texture coordinates
   gl_TexCoord[0] = gl_MultiTexCoord0;
   //  Set vertex position
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
