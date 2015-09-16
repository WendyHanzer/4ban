/*
Wesley Kepke's Shader Helper Class - Header File

This file contains the declaration for a class that will assist in the reading,
compiling, and linking of the vertex shader and the fragment shader files.

Class: CS 480 (Fall 2014)
Last Modified: September 8, 2014 
*/

// Header Guards
#ifndef SHADER_H
#define SHADER_H

// Header Files
#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <string.h>
using namespace std; 

// Constant Values
#define NUM_SHADERS 2

// Class Declaration
class shader
   {
   public:
      shader(); 
      ~shader();
  
      void setShaderFile( const char *glslFile ); 
      void setShaderType( GLenum typeOfShader ); 

      void readShaderFile( void );   
      GLuint compileAndLinkShaders( shader shaderArray[] );               

   private:
      char *shaderFileName; // Name of the file that contains the shader
      char *shaderFileContent; // String of the .glsl shader code
      GLenum shaderType; // Either a vertex shader or a fragment shader  
   };

#endif // #ifndef SHADER_H
