/*
This program uses the class defined in "shader.h" and implemented in 
"shader.cpp" - this small library provides a collection of useful tools that
will assist in the reading, compiling, and linking of the vertex shader and the
fragment shader. 

This program also loads models (with textures!) using Assimp (Open Asset Import 
Library), which is a library that is used to load various well-known 3D model 
formats.

NOTE: I used Magick++ to load my image. 

The section of code that pertains to Assimp in this program was adapted from
what Matt and Daniel showed us in class and from the tutorial provided in their
powerpoint: http://ogldev.atspace.co.uk/www/tutorial22/tutorial22.html 

This program also features some interaction (it's the same as our second 
assignment):

Left-click: Object changes direction.

Right-click: Menu is displayed with options for starting the rotation (if the
object is stopped), for stopping the rotation (if the object is rotating),
changing the direction of the object, and for quitting the program.

Lowercase/Uppercase a/A: Either key will change the direction of rotation
for the object. 

ESC key: The program will terminate.  

Last Modified: 10/08/14 - Wesley Kepke 
*/

// Header Files
// The aforementioned "shader.h" header file
// Note: "shader.h" contains <GL/glew.h>, <GL/glut.h>, <iostream>, <fstream>,
// and <string.h>  
#include "shader.h"
#include <vector>
#include <chrono>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/color4.h>
//#define GLM_FORCE_RADIANS // To ignore compiler warnings.. 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier
#include <Magick++.h> // Image library used for Assimp textures
 
//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat uv[2];
};

/*
Struct: celestialObj
objectIdentifier - Either P (planet) or M (moon) 
objectName - Name of the object
objectID - Index of the planet in the solar system 
objectParentPlanet - Identifier of the planet that the object orbits around
objectImageFile - Image file used for texture of object
objectRadius - Radius of object
objectRevolutionRadius - Radius of rotation (distance) around sun
objectRevolutionSpeed - Speed at which object rotates 
objectModel - Model matrix for the object 
*/
struct celestialObj
   {
   char objectIdentifier;
   char objectName[20];
   int objectID;
   int objectParentPlanet;
   char objectImageFile[20];
   float objectRadius;
   float objectRevolutionRadius;
   float objectRevolutionSpeed;
   float objectSelfRevolution; 
   glm::mat4 objectModel;
   glm::vec3 position;
   float xPos;
   float yPos;
   float zPos;
   };

// Vector of celestial objects
std::vector<celestialObj> spaceObjects;
std::vector<celestialObj> files[2]; 
bool scaled = true; 

// The following "readFile" function is used to read in the data file that
// contains the solar system data
void readFile( int fileVersion,
               const char *fileName );

//--Evil Global variables
//Just for this example!
int w = 1080, h = 1080;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry[3];// VBO handle for our geometry
GLuint *text = NULL; 

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_uv; 

// Spinning of the cube variables - Added by Wesley - 9/05/14
bool reverseSpin = false; 
bool stopSpin = false; 

//transform matrices
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void mouse( int button, int state, int x, int y ); // Added by Wesley - 9/05/14
void menu( int selection ); // Added by Wesley - 9/05/14
void specialKeys( int button, int x, int y );

float zoomVal = 10.0; 
float shiftVal = 0.0;
float height = 0.0;
glm::vec3 focus = glm::vec3( 0.0f );
int focusPlanet = 0;
 
//--Resource management
bool initialize();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

// Assimp object loader - Added by Wesley - 9/28/14
int numberOfVertices = 0;
int numberOfRingVertices = 0; 
Vertex *objLoader( const char *imageFile,
                   int &numVertices );  
GLfloat colorConvert( GLfloat value ); 
Magick::Blob *m_blob;
void calculateOrbitPath( Vertex circlePoints[360] );

//--Main
int main(int argc, char **argv)
{
    // Initialize Glut/ImageMagick
    Magick::InitializeMagick( *argv ); 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);

    // Name and create the Window
    glutCreateWindow("Assignment 07 - Solar System");
    readFile( 0, "niceData" );
    readFile( 1, "scaleData" );  
    spaceObjects = files[0]; 

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input
    glutMouseFunc( mouse ); // Called if there is mouse input
    glutSpecialFunc( specialKeys );
	
    // Menu
    glutCreateMenu( menu );
    glutAddMenuEntry( "Start Rotation", 1 );
    glutAddMenuEntry( "Stop Rotation", 2 ); 
    glutAddMenuEntry( "Change Scaling", 3 );
    glutAddMenuEntry( "Quit", 4 );  
    glutAttachMenu( GLUT_RIGHT_BUTTON );   

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

//--Implementations
void readFile( int fileVersion, 
               const char *fileName )
   {
   // Initialize function/variables 
   std::ifstream f( fileName );
   celestialObj temp;

   // Check for valid file
   if( f == NULL )
      {
      std::cout << "ERROR: Invalid data file." << std::endl;
      exit( EXIT_FAILURE );
      }

   // Read contents of data file
   while( f >> temp.objectIdentifier >> temp.objectName >> temp.objectID >>
          temp.objectParentPlanet >> temp.objectImageFile >>
          temp.objectRadius >> temp.objectRevolutionRadius >>
          temp.objectRevolutionSpeed >> temp.objectSelfRevolution )
      {
      temp.objectModel = glm::mat4(1.0f);
      temp.xPos = temp.yPos = temp.zPos = 0.0;
      temp.position = glm::vec3(0.0f);
      files[fileVersion].push_back( temp );
      //spaceObjects.push_back( temp ); 
      }

   // Function conclusion
   f.close();
   }

void render()
{
    // Initialize function/variables
    int geometryVertices = 0;  

    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the active texture 
    glActiveTexture( GL_TEXTURE0 );

    // Prepare interactive zooming
    if( focusPlanet != 0 )
       {

       view = glm::lookAt( glm::vec3(zoomVal*spaceObjects[focusPlanet].xPos, 
                           height, 
                           zoomVal*spaceObjects[focusPlanet].zPos), //Eye Position
                           spaceObjects[focusPlanet].position, //Focus point
                           glm::vec3(0.0, 1.0, 0.0));

/*
       view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal+spaceObjects[focusPlanet].xPos, 
                           height, 
                           sin(shiftVal)*zoomVal+spaceObjects[focusPlanet].zPos), //Eye Position
                           spaceObjects[focusPlanet].position, //Focus point
                           glm::vec3(0.0, 1.0, 0.0));
*/
	}

     else
        {
        view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal, 
                            height, 
                            sin(shiftVal)*zoomVal), //Eye Position
                            spaceObjects[focusPlanet].position, //Focus point
                            glm::vec3(0.0, 1.0, 0.0));
	}

    // Iterate through all objects and render them 
    for( unsigned int i = 0; i < spaceObjects.size(); i++ )
       {
       // Premultiply mvp matrix 
       mvp = projection * view * spaceObjects[i].objectModel;

       // Determine if we are rendering a planet/moon
       if( spaceObjects[i].objectIdentifier != 'R' )
          {
          // Bind buffer associated with planet object
          glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry[0]);
          geometryVertices = numberOfVertices;          
          }

       // Otherwise, we are rendering a planet's rings
       else
          {
          // Bind buffer associated with planet-rings object
          glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry[1]);
          geometryVertices = numberOfRingVertices;          
          }

       //enable the shader program
       glUseProgram(program);

       //upload the matrix to the shader
       glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));

       //set up the Vertex Buffer Object so it can be drawn
       glEnableVertexAttribArray(loc_position);
       glEnableVertexAttribArray(loc_uv);

       //set up the textures
       glBindTexture(GL_TEXTURE_2D, text[i]);    

       //set pointers into the vbo for each of the attributes(position and color)
       glVertexAttribPointer( loc_position,//location of attribute
                              3,//number of elements
                              GL_FLOAT,//type
                              GL_FALSE,//normalized?
                              sizeof(Vertex),//stride
                              0);//offset

       glVertexAttribPointer( loc_uv,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (void*)offsetof(Vertex,uv));

       glDrawArrays(GL_TRIANGLES, 0, geometryVertices);//mode, starting index, count
 
       // Draw orbit rings
       if( spaceObjects[i].objectIdentifier == 'P' )
          {
          // Bind buffer for orbit path 
          glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry[2]);

          // Calculate scaling factor for orbit path 
          glm::mat4 orbitScale = glm::scale( glm::mat4(1.0f), 
                                             glm::vec3( spaceObjects[i].objectRevolutionRadius,
                                                        0.0, 
                                                        spaceObjects[i].objectRevolutionRadius) ); 
          // Calculate mvp for orbit path 
          mvp = projection * view * orbitScale;  

          // Enable the shader program
          glUseProgram(program);

          // Upload the matrix to the shader
          glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));

          // Set up the Vertex Buffer Object so it can be drawn
          glEnableVertexAttribArray(loc_position);
          glEnableVertexAttribArray(loc_uv);
          
          // Set up the textures
          glBindTexture(GL_TEXTURE_2D, text[0]);   
 
          // Set pointers into the vbo for each of the attributes(position and color)
          glVertexAttribPointer( loc_position,//location of attribute
                                 3,//number of elements
                                 GL_FLOAT,//type
                                 GL_FALSE,//normalized?
                                 sizeof(Vertex),//stride
                                 0);//offset

          glVertexAttribPointer( loc_uv,
                                 2,
                                 GL_FLOAT,
                                 GL_FALSE,
                                 sizeof(Vertex),
                                 (void*)offsetof(Vertex,uv)); 

          // Draw orbit path 
          glDrawArrays(GL_LINE_LOOP, 0, 360);//mode, starting index, count */
         }
     }

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);

    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    // Initialize function/variables 
    glm::mat4 planetTranslation(1.0f);
    glm::mat4 planetRotation(1.0f);  
    glm::mat4 planetScaling(1.0f); 
    static float angle = 0.0;
    float dt = getDT(); // if you have anything moving, use dt. 

    // Check if we need to stop the cube's rotation 
    if( stopSpin == true )
       {
       // If so, simulate as if no time has gone by (cube will stay wherever
       // it already is)
       dt = 0; 
       }

    // Check if we need to reverse the spinning of the cube
    if( reverseSpin == true )
       {
       angle -= dt * M_PI/2; //move through 90 degrees a second 
       }

    else 
       {
       angle += dt * M_PI/2; //move through 90 degrees a second
       }

    // Calculate movement of celestial objects 
    for( unsigned int i = 0; i < spaceObjects.size(); i++ )
       {
       // Find the parent planet
       int parentPlanet = spaceObjects[i].objectParentPlanet;
       glm::mat4 parentMatrix(1.0f);  
       spaceObjects[i].objectModel = glm::mat4(1.0f); 
       if( parentPlanet != 0 )
          {
          parentMatrix = spaceObjects[parentPlanet].objectModel; 
          }

       // Update position
       spaceObjects[i].position = glm::vec3(spaceObjects[i].objectRevolutionRadius 
                           * sin( angle * spaceObjects[i].objectRevolutionSpeed ), 
                           0.0, 
                           spaceObjects[i].objectRevolutionRadius 
                           * cos( angle * spaceObjects[i].objectRevolutionSpeed ));
       spaceObjects[i].xPos = spaceObjects[i].objectRevolutionRadius 
                           * sin( angle * spaceObjects[i].objectRevolutionSpeed );

       spaceObjects[i].yPos = 0.0;
       spaceObjects[i].zPos = spaceObjects[i].objectRevolutionRadius 
                           * cos( angle * spaceObjects[i].objectRevolutionSpeed );

       // Calculate translation matrix for object
       planetTranslation = glm::translate( parentMatrix,
                           glm::vec3(spaceObjects[i].objectRevolutionRadius 
                           * sin( angle * spaceObjects[i].objectRevolutionSpeed ), 
                           0.0, 
                           spaceObjects[i].objectRevolutionRadius 
                           * cos( angle * spaceObjects[i].objectRevolutionSpeed )));

       // Calculation scaling matrix for each object 
       planetScaling = glm::scale( glm::mat4(1.0f), 
                                   glm::vec3(spaceObjects[i].objectRadius) ); 
/*
       // Calculate the rotation matrix for object
       if( spaceObjects[i].objectIdentifier != 'R' )
          {
          planetRotation = glm::rotate( glm::mat4(1.0f), 
                                        angle * spaceObjects[i].objectSelfRevolution, 
                                        glm::vec3( 0.0, 1.0, 0.0 ) );  
          }

       else
          {
          planetRotation = glm::rotate( glm::mat4(1.0f), 
                                        0.0f, 
                                        glm::vec3( 0.0, 1.0, 0.0 ) );  
          }
*/
       // Calculate the rotation matrix for object

          planetRotation = glm::rotate( glm::mat4(1.0f), 
                                        angle * spaceObjects[i].objectSelfRevolution, 
                                        glm::vec3( 0.0, 1.0, 0.0 ) );  

       // Calculate the model matrix - remember, order matters - planet
       spaceObjects[i].objectModel *= planetTranslation * planetRotation * planetScaling;
       }    

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}

void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;

    //Change the viewport to be correct
    glViewport( 0, 0, w, h);

    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);

}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    // Handle keyboard input
    switch( key )
      {
      // ESC
      case 27: 
         exit(0);
         break; 

      // Lowercase/Uppercase a/A will reverse spin direction
      case 'a':
      case 'A':
         reverseSpin = !reverseSpin; 
         break; 
      case ' ':
         focusPlanet = 0;
         zoomVal = 10;
		 height = 0;
         break;
      case 'z':
         focusPlanet = 1;
         zoomVal = 4;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.1;
		 }
         break;
      case 'x':
         focusPlanet = 2;
         zoomVal = 4;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.1;
		 }
         break;
      case 'c':
         focusPlanet = 3;
         zoomVal = 4;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.1;
		 }
         break;
      case 'v':
         focusPlanet = 5;
         zoomVal = 3;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.1;
		 }
         break;
      case 'b':
         focusPlanet = 6;
         zoomVal = 2;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.4;
		 }
         break;
      case 'n':
         focusPlanet = 7;
         zoomVal = 2;
		 if( !scaled )
		 {
			zoomVal = 1.3;
		 }
         break;
      case 'm':
         focusPlanet = 9;
         zoomVal = 2;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.1;
		 }
         break;
      case ',':
         focusPlanet = 10;
         zoomVal =1.5;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.05;
		 }
         break;
      case '.':
         focusPlanet = 21;
         zoomVal = 1.5;
		 height = 0;
		 if( !scaled )
		 {
			zoomVal = 1.003;
		 }
         break;
      }
}

void mouse( int button, int state, int x, int y )
   {
   // Left mouse click will reverse spin direction 
   if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
      {
      reverseSpin = !reverseSpin;         
      }

	if( button == 3 && focusPlanet == 0) // Scoll up
		{
			if( zoomVal > -45 )
			{
				zoomVal -= 0.2;
				view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal, height, sin(shiftVal)*zoomVal), //Eye Position
		                    spaceObjects[focusPlanet].position, //Focus point
		                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
			}
		}
	if( button == 4 && focusPlanet == 0 )
		{
			if( zoomVal < 45 )
			{
				zoomVal += 0.2;
				view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal, height, sin(shiftVal)*zoomVal), //Eye Position
		                    spaceObjects[focusPlanet].position, //Focus point
		                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
			}
		}
   }

void specialKeys( int button, int x, int y )
{
	// Special key functions
	switch( button )
	{
		// Rotate counter-clockwise
		case GLUT_KEY_LEFT:
			shiftVal += 0.2;
/*
		    view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal, height, sin(shiftVal)*zoomVal), //Eye Position
                        spaceObjects[focusPlanet].position, //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
*/
			break;
		// Rotate clockwise
		case GLUT_KEY_RIGHT:
			shiftVal -= 0.2;
/*
		    view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal, height, sin(shiftVal)*zoomVal), //Eye Position
                        spaceObjects[focusPlanet].position, //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
*/
			break;
		// Rotate clockwise
		case GLUT_KEY_UP:
			if( height < 20 )
			{


		 if( !scaled && focusPlanet != 0 )
		 {
			height += 0.01;
		 }
		else
		{
			height += 1;
		}

			}
			break;
		// Rotate clockwise
		case GLUT_KEY_DOWN:
			if( height > -20 )
			{
		 if( !scaled && focusPlanet != 0 )
		 {
			height -= 0.01;
		 }
		else
		{
			height -= 1;
		}
			}
			break;
		default:
			break;
	}

}

void menu( int selection )
   {
   switch( selection )
      {
      // Start the rotation
      case 1:
         stopSpin = false;
         break;

      // Stop the rotation
      case 2:
         stopSpin = true; 
         break; 

      // Switch between scaled/actual views  
      case 3:
         scaled = !scaled; 
         if( scaled == true )
            { 
            spaceObjects = files[0]; 
            } 
         else 
            {
            spaceObjects = files[1]; 
            }
		 height = 0;
		 zoomVal = 10;
         shiftVal = 0.0;
		 focusPlanet = 0;
         break;

      // Exit the program
      case 4:
         exit( 0 );
         break;
      }

   // Necessary for the screen to redraw itself without the menu
   glutPostRedisplay(); 
   }

bool initialize( void )
{
    // Initialize geometry with shader loader 
    Vertex *geometry = objLoader( "sphere.obj", numberOfVertices ); 
    Vertex circle[360]; 
    calculateOrbitPath( circle ); 
    text = new GLuint[spaceObjects.size()]; 
    m_blob = new Magick::Blob[spaceObjects.size()]; 

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(3, vbo_geometry);

    // Create a vertex buffer for the planet data
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry[0]);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(Vertex) * numberOfVertices, 
                 geometry, 
                 GL_STATIC_DRAW);

    // Create a vertex buffer for saturn's rings 
    Vertex *ringGeometry = objLoader( "saturn.obj", numberOfRingVertices ); 
    glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[1] );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(Vertex) * numberOfRingVertices,
                  ringGeometry,
                  GL_STATIC_DRAW ); 

    // Create a vertex buffer for the orbit path
    glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[2] );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(circle),
                  circle,
                  GL_STATIC_DRAW ); 

    //--Geometry done 
 
    // Create a texture buffer object 
    glGenTextures(spaceObjects.size(), text);
    for( unsigned int i = 0; i < spaceObjects.size(); i++ )
       {    
       // Load the image
       Magick::Image *m_pImage;
       try
          {
          m_pImage = new Magick::Image(spaceObjects[i].objectImageFile);
          m_pImage->write(&m_blob[i], "RGBA");
          }
       catch( Magick::Error& Error )
          {
          std::cout << "Error loading texture " << spaceObjects[i].objectImageFile;
          std::cout << ": " << Error.what() << std::endl;
          }

       // Create a binding for each texture 
       glActiveTexture( GL_TEXTURE0 ); 
       glBindTexture(GL_TEXTURE_2D, text[i]);
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_pImage->columns(), m_pImage->rows(), 0, 
       				GL_RGBA, GL_UNSIGNED_BYTE, m_blob[i].data() );
       glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
       glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

       // Deallocate the image
       delete m_pImage;
       m_pImage = NULL; 
       }

    // Initialize shader array 
    shader vAndFShaders[2];

    // Load data into our array of shaders 
    vAndFShaders[0].setShaderFile( "vShader.vert" );
    vAndFShaders[0].readShaderFile();
    vAndFShaders[0].setShaderType( GL_VERTEX_SHADER );
    vAndFShaders[1].setShaderFile( "fShader.frag" );
    vAndFShaders[1].readShaderFile();
    vAndFShaders[1].setShaderType( GL_FRAGMENT_SHADER );   

    // Compile and link shaders using "shader.h"
    program = vAndFShaders[0].compileAndLinkShaders( vAndFShaders );

    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_uv = glGetAttribLocation(program,
                    const_cast<const char*>("v_texture"));
    if(loc_uv == -1)
    {
        std::cerr << "[F] UV_POSITION NOT FOUND" << std::endl;
        return false;
    } 

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }

    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(cos(shiftVal)*zoomVal, height, sin			(shiftVal)*zoomVal), //Eye Position
                        glm::vec3(0.0,0.0,0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
    glDeleteBuffers(2, vbo_geometry);
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::chrono::high_resolution_clock::now();
    ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
    t1 = std::chrono::high_resolution_clock::now();
    return ret;
}

Vertex *objLoader( const char *imageFile,
                   int &numVertices )
   {
   // Initialize function/variables
   Assimp::Importer importer;
   const aiScene *pScene = importer.ReadFile( imageFile, 
                                              aiProcess_Triangulate ); 

   // Check to see if the scene loaded correctly
   if( !pScene )
      {
      printf( "Error parsing '%s': '%s'\n", imageFile, importer.GetErrorString() );
      exit( EXIT_FAILURE );  
      }

   // Find the first mesh in the scene 
   const aiMesh *mesh = pScene->mMeshes[0];
 
   // Allocate sufficient memory
   numVertices = mesh->mNumFaces * 3; 
   Vertex *outboundVertexData = new Vertex[ numVertices ]; 
   Vertex *home = outboundVertexData;

   // Iterate through each face in the current mesh
   for( unsigned int i = 0; i < mesh->mNumFaces; i++ )
      {
      // Obtain the vertices/textures that make up each face 
      const aiFace &face = mesh->mFaces[i];  
      for( unsigned int j = 0; j < 3; j++ )
         {
         // Obtain the vertices in each face 
         aiVector3D position = mesh->mVertices[ face.mIndices[j] ]; 
         outboundVertexData->position[0] = position.x; 
         outboundVertexData->position[1] = position.y; 
         outboundVertexData->position[2] = position.z;

         // Obtain the textures in each face
         aiVector3D uv = mesh->mTextureCoords[0][ face.mIndices[j] ];
         outboundVertexData->uv[0] = uv.x; 
         outboundVertexData->uv[1] = uv.y;
         outboundVertexData++;  
         }
      }

   // Function conclusion
   outboundVertexData = home;
   return outboundVertexData; 
   }

void calculateOrbitPath( Vertex circlePoints[360] )
   {
   // Calculate all points on a circle with a radius of 1
   for( int i = 0; i < 360; i++ )
      {
      // Convert degrees to radians 
      float x = i * (3.14/180); 

      // Add data to array 
      circlePoints[i].position[0] = 1.0 * cos(x);
      circlePoints[i].position[1] = 0.0; 
      circlePoints[i].position[2] = 1.0 * sin(x); 
      circlePoints[i].uv[0] = 0.5;
      circlePoints[i].uv[1] = 0.5; 
      }
   }
