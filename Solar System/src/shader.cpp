/*
Wesley Kepke's Shader Helper Class - Implementation File

This file contains the implementation for a class that will assist in the 
reading, compiling, and linking of the vertex shader and the fragment shader 
files.

Class: CS 480 (Fall 2014)
Last Modified: September 8, 2014 
*/

// Header Files
#include "shader.h"

// Function Implementations
/*
This is the constructor for an object of the shader class. 

For every shader object that is created, the pointers that correspond to the 
.glsl shader code and the name of the shader file will be initialized to NULL.   

Finally, we'll assign the "shaderType" variable to a default value of 0. 
*/
shader::shader()
   {
   shaderFileName = NULL; 
   shaderFileContent = NULL;
   shaderType = 0;   
   } 

/*
This is the destructor for the shader class.

For every shader object that is created, we will need to deallocate the memory
set aside for both the .glsl shader code and the name of the shader file. 
Both pointers will additionally be assigned to a safe value (NULL).

Finally, we'll reassign "shaderType" to our default value of 0.  
*/
shader::~shader()
   {
   delete []shaderFileName; 
   delete []shaderFileContent;
   shaderFileName = NULL; 
   shaderFileContent = NULL;  
   shaderType = 0; 
   }

/*
This function will allocate the necesary memory and will assign the name of the
shader file to a shader object. 
*/
void shader::setShaderFile( const char *glslFile )
   {
   int glslFileNameLength = strlen( glslFile );
   shaderFileName = new char[glslFileNameLength + 1];   
   strcpy( shaderFileName, glslFile );
   shaderFileName[glslFileNameLength] = '\0';  
   }

/*
This function will assign the type of the shader to a shader object. 
*/
void shader::setShaderType( GLenum typeOfShader )
   {
   shaderType = typeOfShader; 
   } 

/*
This function will read the contents of the .glsl shader code into a
null-terminated string.

If the file is invalid, the function will display an error message and the
program will abort.  
*/
void shader::readShaderFile( void )
   {
   // Initialize function/variables 
   long shaderFileLength = 0; 
   ifstream fileObj( shaderFileName );

   // Determine if the file was valid
   if( fileObj )
      {
      // Determine the length of the file 
      fileObj.seekg( 0, fileObj.end ); 
      shaderFileLength = fileObj.tellg();
      fileObj.seekg( 0, fileObj.beg ); 

      // Allocate memory and read contents 
      shaderFileContent = new char[shaderFileLength + 1];
      fileObj.read( shaderFileContent, shaderFileLength );
      shaderFileContent[shaderFileLength] = '\0';
      }

   // If file is invalid, display error message and end program 
   else
      {
      cout << "ERROR: " << shaderFileName << " is invalid." << endl; 
      exit( EXIT_FAILURE );
      } 

   fileObj.close();    
   }

/*
This function will attempt to compile and link the vertex and fragment shaders
into a program object. It does this by iterating through an array of shader 
objects and compiling and linking both shaders one at a time. Various checks 
exist in the function to ensure every phase of the compilation and linking 
process is successful. 

This function will return an active program object (type: GLuint). 
*/ 
GLuint shader::compileAndLinkShaders( shader shaderArray[] )
   {
   // Initialize function/variables
   int shaderIndex;
   GLuint program = glCreateProgram();
   GLuint shaderObj;
   GLint shaderStatus;  

   // Iterate through shader array  
   for( shaderIndex = 0; 
        shaderIndex < NUM_SHADERS; 
        shaderIndex++ )
      {
      // Create an empty shader object 
      shaderObj = glCreateShader( shaderArray[shaderIndex].shaderType );

      // Identify where the source code for the shader is coming from 
      glShaderSource( shaderObj, 1, 
                      (const GLchar**) &shaderArray[shaderIndex].shaderFileContent,
                      NULL );

      // Compile the shader object 
      glCompileShader( shaderObj );

      // Check compilation status
      glGetShaderiv( shaderObj, GL_COMPILE_STATUS, &shaderStatus );
      if( !shaderStatus )
         {
         // Display error and abort program if shader did not compile correctly
         cout << "ERROR: " << shaderArray[shaderIndex].shaderFileName;
         cout << " did not compile correctly." << endl << endl; 
         GLint logSize;
         glGetShaderiv( shaderObj, GL_INFO_LOG_LENGTH, &logSize );
         char *logMsg = new char[ logSize ];
         glGetShaderInfoLog( shaderObj, logSize, NULL, logMsg );
         std::cerr << logMsg << std::endl;
         delete []logMsg;
         exit( EXIT_FAILURE );   
         }

      // If shader compiled, attatch it to the program object
      glAttachShader( program, shaderObj );

      // If both shaders are attached, link application program with shaders
      if( shaderIndex == (NUM_SHADERS - 1) )
         {
         // Perform and verify linkage
         glLinkProgram( program ); 
         glGetProgramiv( program, GL_LINK_STATUS, &shaderStatus ); 
         if( !shaderStatus )
            {
            // Display error and abort program if linking process was
            // unsuccessful
            cout << "ERROR: The shaders could not link to the program.";
            cout << endl;
            exit( EXIT_FAILURE );  
            }
         } 
      } 

   // Determine our active program object
   glUseProgram( program );
   return program; 
   } 
