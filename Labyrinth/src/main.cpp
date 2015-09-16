/*/////////////////////////////////////////////////////////////////////////////
Group members: Renee Iinuma, Kyle Lee, and Wesley Kepke. 

Class: CS 480

Date: November 26, 2014 

This program is our take on Labyrinth, a game in which the user attempts to 
navigate a ball through a maze, from start to finish, without having the ball 
fall through one of the several holes scattered throughout the board.  
*//////////////////////////////////////////////////////////////////////////////

// Header Files ///////////////////////////////////////////////////////////////

/*
Note: "shader.h" contains <GL/glew.h>, <GL/glut.h>, <iostream>, <fstream>, and
<string.h>.  
*/

// Header files for bullet.  
#include <btBulletDynamicsCommon.h> 

// Header files for sound. 
#include "SDL/SDL_mixer.h"

// Header files for loading images. 
#include <Magick++.h> // Image library used to load textures for objects.

// Basic header files. 
#include "shader.h" // Necessary to load and compile shaders. 
#include <vector>
#include <chrono>
#define GLM_FORCE_RADIANS // To ignore compiler warnings from assimp.
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/color4.h>
#include <GL/freeglut.h>
#include <time.h>

// GLM header files. 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // Makes passing matrices to shaders easier
// End of Header Files ////////////////////////////////////////////////////////
 
// Global Constants/Variables/Prototypes //////////////////////////////////////

// Degrees to radians macro.
#define degreesToRadians(x) x*(3.141592f/180.0f) 

// "Vertex" object - used to send vertex data to shaders. 
struct Vertex
   {
   GLfloat position[3];
   GLfloat uv[2];
   GLfloat normal[3]; 
   };

// "Light" object - used to consolidate lighting data. 
struct Light
   {
   // Light and spotlight attributes.
   float lightX;
   float lightY;
   float lightZ;
   float lightA;
   float spotX;
   float spotY;
   float spotZ; 

   // Light position and light-material interaction vectors.  
   glm::vec4 lightPosition;
   glm::vec4 lightAmbient;
   glm::vec4 lightDiffuse;
   glm::vec4 lightSpecular; 

   // Spotlight vector and attributes.
   glm::vec3 spotDirection;   
   float spotCutoff; 
   float spotExponent;
   };

// Initialize lights.  
Light lights[3];
glm::vec4 ambientProduct;
glm::vec4 diffuseProduct;
glm::vec4 specularProduct;
glm::vec4 materialAmbient;
glm::vec4 materialDiffuse;
glm::vec4 materialSpecular;  
float materialShininess = 0.0;
float constantAttenuation = 0.0;
float linearAttenuation = 0.0;
float quadraticAttenuation = 0.0;
float *lPos = new float[12];
float *spotDir = new float[9];
float *spotCut = new float[3];
float *spotExp = new float[3];

/* 
"COLLISION" enumeration - for object collisions.

Bullet supports bitwise masks as a way of deciding whether or not things should
collide with other things, or receive collisions. 
*/
#define BIT(x) ( 1 << (x) ) // Bitmask used for collisions.  
enum COLLISIONS
  {
  COLLIDE_NOTHING = 0, // Collide with nothing. 
  COLLIDE_WALL = BIT(1), // Collide with the maze walls (balls).
  COLLIDE_FLOOR = BIT(2), // Collide with the floor (balls).
  COLLIDE_BALL = BIT(3), // Collide with ball (other ball).
  COLLIDE_BOARD = BIT(4) //  
  }; 

// Define what our rigid bodies can collide with. 
int ballCollidesWith = COLLIDE_WALL | COLLIDE_FLOOR | 
                        COLLIDE_BOARD | COLLIDE_BALL;
int boardCollidesWith = COLLIDE_BALL;
float theta = 0.0;
float phi = 0.0;    

// Sound global variables. 
Mix_Chunk *soundPtr = NULL; 

// Window variables. 
int windowWidth = 1080;
int windowHeight = 1080;

// OpenGL necessities. 
const int NUM_OBJECTS = 7; 
GLuint program; // The GLSL program handle.
GLuint vbo_geometry[NUM_OBJECTS]; // Array for VBOs (for our geometry).
GLuint textures[(NUM_OBJECTS - 1) * 2]; // Array of textures (for our geometry).

// OpenGL uniform locations.
GLint locMVP;
GLint locLightPos; 
GLint locAmbient; 
GLint locDiffuse;
GLint locSpecular;
GLint locShininess;
GLint locConstAttenuation;
GLint locLinAttenuation;
GLint locQuadAttenuation; 
GLint locSpotCutoff;
GLint locSpotExponent; 
GLint locSpotDirection;  

// OpenGL attribute locations.
GLint loc_position;
GLint loc_uv;
GLint loc_normal;  

// OpenGL transform matrices. 
glm::mat4 model[NUM_OBJECTS]; // model->world each object should have its own model matrix.
glm::mat4 view; // world->eye.
glm::mat4 projection; // eye->clip.
glm::mat4 mvp; // premultiplied modelviewprojection matrix for vertex shader.     

// Toggling options.
bool startGame = true;  
bool levelOption = true;
bool extraBall = false;
bool gameWon = false;
bool disableKeyboard = true;   
int moveState = -1;  
bool pauseGame = false;
bool spotLightOption = true;    

// Function prototypes/GLUT callbacks. 
void render( void );
void update( void );
void reshape( int n_w, 
              int n_h);
void keyboard( unsigned char key, 
               int x_pos, 
               int y_pos);
void menu( int selection ); 
void specialKeys( int button, int x, int y );
void initializeLights(void);    

// Camera globals. 
float zoom = 40;
float rotAng = -M_PI/2;
float htAng = 21.2;
int viewMode = 0; 

// Scoring globals.
float bufferTime = 0.0;  
float bestScoreEasyLevel = 99999.9;
float bestScoreHardLevel = 99999.9; 
float timeElapsed = 0.0; 
char timeStr[256];
char easyStr[256];
char hardStr[256];  

// Resource management.
bool initialize( void );
void cleanUp( void );

// Random time things.
float getDT( void );
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

// Assimp components.  
int numberOfBallVertices = 0; 
int numberOfBoardOneVertices = 0;
int numberOfBoardTwoVertices = 0;
int numberOfKnobOneVertices = 0;
int numberOfKnobTwoVertices = 0;
int numberOfOuterBoardVertices = 0;  
int numberOfBackgroundVertices = 0; 
Vertex *objLoader( const char *objFile, 
                   int &numVertices,
                   btTriangleMesh *&objTriMesh ); 
Magick::Blob m_blob[(NUM_OBJECTS - 1) * 2]; 

/* 
Bullet components:

btBroadphaseInterface broadphase: Tells bullet how to check for collisions 
between objects. This will also help eliminate object pairs that should not
collide. 
 
btDefaultCollisionConfiguration collisionConfiguration: This is a collision 
algorithm. It can be used to register a callback that filters overlapping
broadphase proxies so that the collisions are not processed by the rest of the
system. 

btCollisionDispatcher dispatcher: Used in conjunction with 
collisionConfiguration for the collision algorithm. It will send events to the
objects. 
 
btSequentialImpulseConstraintSolver *solver: This causes the objects to 
interact properly, taking into account gravity, foces, collisions, and hinge
constraints. Both the world and the objects are handled with this pointer. 

btDiscreteDynamicsworld *dynamicsWorld: Instantiate the dynamics world. 
*/
btBroadphaseInterface *broadphase = NULL; 
btDefaultCollisionConfiguration *collisionConfiguration = NULL; 
btCollisionDispatcher *dispatcher = NULL; 
btSequentialImpulseConstraintSolver *solver = NULL; 
btDiscreteDynamicsWorld *dynamicsWorld = NULL;

// Bullet rigid bodies.  
btRigidBody *boardOneRigidBody = NULL;
btRigidBody *boardTwoRigidBody = NULL;  
btRigidBody *ballOneRigidBody = NULL;
btRigidBody *ballTwoRigidBody = NULL; 

// Bullet triangle meshes. 
btTriangleMesh *ballOneTriMesh = NULL;
btTriangleMesh *ballTwoTriMesh = NULL; 
btTriangleMesh *boardOneTriMesh = NULL;
btTriangleMesh *boardTwoTriMesh = NULL;
btTriangleMesh *knobOneTriMesh = NULL;
btTriangleMesh *knobTwoTriMesh = NULL; 
btTriangleMesh *outerBoardTriMesh = NULL;
btTriangleMesh *backgroundMesh = NULL;   

// End of Global Constants/Variables/Prototypes ///////////////////////////////

// Main Program ///////////////////////////////////////////////////////////////
int main(int argc, char **argv)
   {
   // Initialize program.
   Magick::InitializeMagick( *argv ); 
   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_DOUBLE | // Double buffering. 
                        GLUT_DEPTH ); // Depth testing.
   glutInitWindowSize( windowWidth, windowHeight ); 

   // Name and create the Window
   glutCreateWindow("Assignment 11 - Labyrinth");

   // Now the window is created the GL context is fully set up
   // Because of that we can now initialize GLEW to prepare work with shaders
   GLenum status = glewInit();
   if( status != GLEW_OK)
      {
      std::cerr << "[F] GLEW NOT INITIALIZED: ";
      std::cerr << glewGetErrorString(status) << std::endl;
      return -1;
      }

   // Set all of the callbacks to GLUT that we need
   glutDisplayFunc( render ); // Called when its time to display
   glutReshapeFunc( reshape ); // Called if the window is resized
   glutIdleFunc( update ); // Called if there is nothing else to do
   glutKeyboardFunc( keyboard ); // Called if there is keyboard input
   glutSpecialFunc( specialKeys ); // Called for arrow keys

   // Main menu 
   glutCreateMenu( menu );
   glutAddMenuEntry( "Continue Game", 1 );
   glutAddMenuEntry( "Pause Game", 2 );
   glutAddMenuEntry( "Switch Levels", 3 ); 
   glutAddMenuEntry( "Restart with One Ball", 4 );
   glutAddMenuEntry( "Restart with Two Balls", 5 );
   glutAddMenuEntry( "Quit Game", 6 );  
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
// End of Main Program ////////////////////////////////////////////////////////

// Supporting Function Implementations ////////////////////////////////////////
void render( void )
   { 
   // Initialize function/variables.  
   int numberOfVertices = 0;
   int iterationNum = 0;    
   glClearColor( 0.0, 0.0, 0.0, 1.0 ); // Black
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // Keep track of player score. 
   glUseProgram(0);
   int k;
   if(gameWon == false && pauseGame == false)
      {
      timeElapsed += 1;
      }

   else if(gameWon == true)
      {
      // Easy level. 
      if( (bufferTime < bestScoreEasyLevel) &&
            levelOption == true )
         {
         bestScoreEasyLevel = bufferTime/310; 
         }

      // Hard level. 
      else if( (bufferTime < bestScoreHardLevel) &&
            levelOption == false )
         {
         bestScoreHardLevel = bufferTime/310; 
         }

      gameWon = false; 
      }

   // Print time to screen. 
   sprintf( timeStr, "Current Time:  %0.1f seconds.", timeElapsed/310);
   glColor4f(1,1,1, 1.0f);
   glRasterPos3f( -.9, 0.8, 0.0 );
   for( k = 0; timeStr[k] != '\0'; k++ )
      {
      glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, timeStr[k]);
      }

   // Print easy level time to screen. 
   sprintf( easyStr, "Lowest Easy Level Time: %0.1f seconds.", bestScoreEasyLevel );
   glColor4f(1,1,1, 1.0f);
   glRasterPos3f( 0.1, 0.8, 0.0 );
   for( k = 0; easyStr[k] != '\0'; k++ )
      {
      glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, easyStr[k]);
      }

   // Print hard level time to screen. 
   sprintf( hardStr, "Lowest Hard Level Time: %0.1f seconds.", bestScoreHardLevel );
   glColor4f(1,1,1, 1.0f);
   glRasterPos3f( 0.1, 0.7, 0.0 );
   for( k = 0; hardStr[k] != '\0'; k++ )
      {
      glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, hardStr[k]);
      }  

   // Determine amount of times to loop.
   if(extraBall == true)
      {
      iterationNum = NUM_OBJECTS; 
      }
   else if(extraBall == false)
      {
      iterationNum = NUM_OBJECTS - 1; 
      }

   // Render all objects for simulation.
   glUseProgram( program );
   glActiveTexture( GL_TEXTURE0 );
   for( int i = 0; i < iterationNum; i++ )
      {
      // Premultiply the mvp matrix and send to shader. 
      mvp = projection * view * model[i];

      // Upload data that changed to the shaders. 
      glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
      glUniform4fv(locLightPos, 3, lPos);
      glUniform3fv(locSpotDirection, 3, spotDir);

      // Determine amount of vertices to render and bind correct buffer.
      // Also determine which textures to render.   
      switch(i)
         {
         // First labyrinth ball. 
         case 0:
            numberOfVertices = numberOfBallVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[0] );
            if(levelOption == true)
               {
               glBindTexture( GL_TEXTURE_2D, textures[0] );
               }
            else if(levelOption == false)
               {
               glBindTexture( GL_TEXTURE_2D, textures[6] );
               } 
         break;

         // First of the two labyrinth boards. 
         case 1:
            if(levelOption == true)
               {
               numberOfVertices = numberOfBoardOneVertices;
               glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[1] );
               glBindTexture( GL_TEXTURE_2D, textures[1] );
               }

            else if(levelOption == false)
               {
               numberOfVertices = numberOfBoardTwoVertices;
               glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[2] ); 
               glBindTexture( GL_TEXTURE_2D, textures[7] );
               }
         break;

         // First of the two labyrinth board knobs. 
         case 2:
            numberOfVertices = numberOfKnobOneVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[3] ); 
            if(levelOption == true)
               {
               glBindTexture( GL_TEXTURE_2D, textures[2] );
               }
            else if(levelOption == false)
               {
               glBindTexture( GL_TEXTURE_2D, textures[8] );
               } 
         break;

         // Second of the two labyrinth board knobs. 
         case 3:
            numberOfVertices = numberOfKnobTwoVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[4] );
            if(levelOption == true)
               {
               glBindTexture( GL_TEXTURE_2D, textures[3] );
               }
            else if(levelOption == false)
               {
               glBindTexture( GL_TEXTURE_2D, textures[9] );
               }  
         break;

         // The outer labyrinth game board. 
         case 4: 
            numberOfVertices = numberOfOuterBoardVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[5] );
            if(levelOption == true)
               {
               glBindTexture( GL_TEXTURE_2D, textures[4] );
               }
            else if(levelOption == false)
               {
               glBindTexture( GL_TEXTURE_2D, textures[10] );
               } 
         break; 

         // The background object. 
         case 5:
            numberOfVertices = numberOfBackgroundVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[6] );
            if(levelOption == true)
               {
               glBindTexture( GL_TEXTURE_2D, textures[5] );
               }
            else if(levelOption == false)
               {
               glBindTexture( GL_TEXTURE_2D, textures[11] );
               }   
         break;

         // The extra ball (if necessary)
         case 6:
            numberOfVertices = numberOfBallVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[0] );
            if(levelOption == true)
               {
               glBindTexture( GL_TEXTURE_2D, textures[0] );
               }
            else if(levelOption == false)
               {
               glBindTexture( GL_TEXTURE_2D, textures[6] );
               } 
         break; 
         }

      // Determine locations of variables in the shaders.
      glEnableVertexAttribArray( loc_position );
      glEnableVertexAttribArray( loc_uv );
      glEnableVertexAttribArray( loc_normal ); 

      // Describe data being sent to shaders   
      glVertexAttribPointer( loc_position, // Location of attribute
                              3, // Number of elements
                              GL_FLOAT, // Data type
                              GL_FALSE, // Normalized?
                              sizeof( Vertex ), // Stride
                              0); // Offset

      glVertexAttribPointer( loc_uv,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof( Vertex ),
                              (void*)offsetof( Vertex, uv ) );
       
      glVertexAttribPointer( loc_normal,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof( Vertex ),
                              (void*)offsetof( Vertex, normal ) );

      glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);//mode, starting index, count
      } 

   // Clean up.
   glDisableVertexAttribArray(loc_position);
   glDisableVertexAttribArray(loc_uv);
   glDisableVertexAttribArray(loc_normal); 

   // Swap the buffers. 
   glutSwapBuffers();
   }

void update( void )
   {
   // Initialize function/variables.
   btTransform trans;
   btScalar m[16]; 
   float dt = getDT(); // if you have anything moving, use dt.
   bool twoBallConditionOne = false;
   bool twoBallConditionTwo = false;  

   // Prepare the sound stuff.
   if( Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) )
      {
      cout << "Error with audio file!" << endl; 
      }

   // Check if we need to pause the game.
   if( pauseGame == true )
      {
      dt = 0; 
      }

   // Update physics world.
   dynamicsWorld->stepSimulation( dt, 10 );  

   // Obtain information about the ball from the dynamics world. 
   btVector3 ballOneLoc = ballOneRigidBody->getCenterOfMassPosition(); 

   // Handle the case where there is only a single ball.  
   if(extraBall == false)
      {
      // Play initial game sound. 
      if(startGame == true)
         {
         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanStart.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanStart.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);
         startGame = false; 
         } 

      // Check to see if the ball has fallen through a hole in the maze. 
      if(ballOneLoc.y() < 0.0)
         {
         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanHole.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanHole.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);

         // Reset board to a traditional, flat orientation. 
         theta = 0.0;
         phi = 0.0;
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Reset the ball to the initial position.           
         btDefaultMotionState *shapeMotionState = NULL;
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9))); 
         ballOneRigidBody->setMotionState(shapeMotionState);
         ballOneRigidBody->setLinearVelocity(btVector3(0, 0, 0));

         // If spotlights are on, turn on all lights when ball falls 
         // through hole.
         if(spotLightOption == true)
            {   
            lights[0].lightA = 0.0;
            lights[1].lightA = 0.0; 
            lights[2].lightA = 0.0; 
            lPos[3] = lights[0].lightA;
            lPos[7] = lights[1].lightA;
            lPos[11] = lights[2].lightA;
            }

         // Temporarily disable keyboard input.
         disableKeyboard = true; 
         } 

      // Otherwise, the ball has not (yet) fallen through the maze. 
      else if(ballOneLoc.y() < 1.5)
         {
         // Re-enable keyboard input.
         disableKeyboard = false; 

         if(spotLightOption == true)
            {
            // Ensure all lights are spotlights. 
            lights[0].lightA = 1.0;
            lights[1].lightA = 1.0; 
            lights[2].lightA = 1.0;  

            // Allow the first spotlight to follow the ball. 
            lights[0].spotX = ballOneLoc.x();
            lights[0].spotY = ballOneLoc.y();
            lights[0].spotZ = ballOneLoc.z();

            // Allow the second spotlight to inversely follow the ball.
            lights[1].spotX = ballOneLoc.z();
            lights[1].spotY = ballOneLoc.y();
            lights[1].spotZ = ballOneLoc.x();

            // Update the light position vector that will be sent to the shaders. 
            lPos[0] = lights[0].spotX;
            lPos[1] = 5;
            lPos[2] = lights[0].spotZ;
            lPos[3] = lights[0].lightA;  
            lPos[4] = lights[1].spotX;
            lPos[5] = 5;
            lPos[6] = lights[1].spotZ;
            lPos[7] = lights[1].lightA; 
            lPos[11] = lights[2].lightA;
            } 
         }

      // Check to see if the ball has reached the end of the maze. 
      if( (ballOneLoc.x() > 4.5 && ballOneLoc.x() < 6.5) &&
            (ballOneLoc.z() > 8.5 && ballOneLoc.z() < 10.5) )
         {
         // Set game winning flag. 
         gameWon = true;

         // Reset board to a traditional, flat orientation. 
         theta = 0.0;
         phi = 0.0;
         bufferTime = timeElapsed; 
         timeElapsed = 0.0; 
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Reset the ball to the initial position.           
         btDefaultMotionState *shapeMotionState = NULL;
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9))); 
         ballOneRigidBody->setMotionState(shapeMotionState);
         ballOneRigidBody->setLinearVelocity(btVector3(0, 0, 0));

         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanEnd.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanEnd.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);    
         }         
      }

   // Handle the case where there are two balls. 
   else if(extraBall == true)
      {
      // Obtain information for the second ball in the dynamics world. 
      btVector3 ballTwoLoc = ballTwoRigidBody->getCenterOfMassPosition(); 

      // Check to see if either of the balls have fallen through a hole in the 
      // maze. 
      if(ballOneLoc.y() < 0.0 || ballTwoLoc.y() < 0.0)
         {
         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanHole.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanHole.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);

         // Reset board to a traditional, flat orientation.
         theta = 0.0;
         phi = 0.0;
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Reset both of the balls to their initial position.           
         btDefaultMotionState *shapeMotionState = NULL;
         btDefaultMotionState *shapeMotionStateTwo = NULL; 
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9)));
         shapeMotionStateTwo = new btDefaultMotionState(btTransform(
                                                         btQuaternion(0, 0, 0, 1), 
                                                         btVector3(-6, 15, -8))); 
         ballOneRigidBody->setMotionState(shapeMotionState);
         ballOneRigidBody->setLinearVelocity(btVector3(0, 0, 0));
         ballTwoRigidBody->setMotionState(shapeMotionStateTwo);
         ballTwoRigidBody->setLinearVelocity(btVector3(0, 0, 0));  

         // If spotlights are on, turn on all lights when ball falls 
         // through hole.
         if(spotLightOption == true)
            {   
            lights[0].lightA = 0.0;
            lights[1].lightA = 0.0; 
            lights[2].lightA = 0.0; 
            lPos[3] = lights[0].lightA;
            lPos[7] = lights[1].lightA;
            lPos[11] = lights[2].lightA;
            }

         // Temporarily disable keyboard input.
         disableKeyboard = true; 
         } 

      // Otherwise, the ball has not (yet) fallen through the maze. 
      else if(ballOneLoc.y() < 1.5 && ballTwoLoc.y() < 1.5)
         {
         // Re-enable keyboard input.
         disableKeyboard = false; 

         if(spotLightOption == true)
            {
            // Ensure all lights are spotlights. 
            lights[0].lightA = 1.0;
            lights[1].lightA = 1.0; 
            lights[2].lightA = 1.0;  

            // Allow the first spotlight to follow the first ball. 
            lights[0].spotX = ballOneLoc.x();
            lights[0].spotY = ballOneLoc.y();
            lights[0].spotZ = ballOneLoc.z();

            // Allow the second spotlight to follow the second ball. 
            lights[1].spotX = ballTwoLoc.x();
            lights[1].spotY = ballTwoLoc.y();
            lights[1].spotZ = ballTwoLoc.z();

            // Update the light position vector that will be sent to the shaders. 
            lPos[0] = lights[0].spotX;
            lPos[1] = 5;
            lPos[2] = lights[0].spotZ;
            lPos[3] = lights[0].lightA;  
            lPos[4] = lights[1].spotX;
            lPos[5] = 5;
            lPos[6] = lights[1].spotZ;
            lPos[7] = lights[1].lightA; 
            lPos[11] = lights[2].lightA;
            } 
         }

      // Check to see if the first ball has reached the end of the maze.
      if( (ballOneLoc.x() > 4.0 && ballOneLoc.x() < 7.0) &&
            (ballOneLoc.z() > 8.0 && ballOneLoc.z() < 11.0) )
         {
         twoBallConditionOne = true; 
         }

      // Check to see if the second ball has reached the end of the maze.
      if( (ballTwoLoc.x() > 4.5 && ballTwoLoc.x() < 6.5) &&
            (ballTwoLoc.z() > 8.5 && ballTwoLoc.z() < 10.5) )
         {
         twoBallConditionTwo = true; 
         }

      // Check to see if both of the two balls have reached the end of the 
      // maze.
      if( twoBallConditionOne == true &&
            twoBallConditionTwo == true )
         {
         // Set game winning flag. 
         gameWon = true;

         // Reset board to a traditional, flat orientation. 
         theta = 0.0;
         phi = 0.0;
         bufferTime = timeElapsed; 
         timeElapsed = 0.0; 
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Reset both of the balls to their initial position.           
         btDefaultMotionState *shapeMotionState = NULL;
         btDefaultMotionState *shapeMotionStateTwo = NULL; 
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9)));
         shapeMotionStateTwo = new btDefaultMotionState(btTransform(
                                                         btQuaternion(0, 0, 0, 1), 
                                                         btVector3(-6, 15, -8))); 
         ballOneRigidBody->setMotionState(shapeMotionState);
         ballOneRigidBody->setLinearVelocity(btVector3(0, 0, 0));
         ballTwoRigidBody->setMotionState(shapeMotionStateTwo);
         ballTwoRigidBody->setLinearVelocity(btVector3(0, 0, 0));  

         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanEnd.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanEnd.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);  
         }

      // Obtain the rigid body for the ball and convert it to a model matrix.
      ballTwoRigidBody->getMotionState()->getWorldTransform(trans); 
      trans.getOpenGLMatrix(m); 
      model[6] = glm::make_mat4(m);  
      }

   // Obtain the rigid body for the ball and convert it to a model matrix. 
   ballOneRigidBody->getMotionState()->getWorldTransform(trans); 
   trans.getOpenGLMatrix(m); 
   model[0] = glm::make_mat4(m);     

   // Rigid body for first labyrinth board.
   if(levelOption == true)
      { 
      boardOneRigidBody->getMotionState()->getWorldTransform(trans);
      }

   // Rigid body for second labyrinth board.
   else if(levelOption == false)
      {
      boardTwoRigidBody->getMotionState()->getWorldTransform(trans);
      }

   // Regardless of which labyrith board we are using, we need to rotate it. 
   trans.getOpenGLMatrix(m); 
   model[1] = glm::make_mat4(m);
   model[1] = glm::rotate(model[1], 
                           degreesToRadians(theta * 2), 
                           glm::vec3(0.0, 0.0, -1.0)); 
   model[1] = glm::rotate(model[1], 
                           degreesToRadians(phi * 2), 
                           glm::vec3(1.0, 0.0, 0.0)); 

   // Rotate side knob upwards. 
   if(moveState == 0)
      {
      model[2] *= glm::rotate( glm::mat4(1.0f), 
                              degreesToRadians(420 * dt), 
                              glm::vec3(1.0, 0.0, 0.0));  
      }

   // Rotate side knob backwards. 
   else if(moveState == 1)
      {
      model[2] *= glm::rotate( glm::mat4(1.0f), 
                              degreesToRadians(420 * -dt), 
                              glm::vec3(1.0, 0.0, 0.0));  
      }

   // Rotate bottom knob to the left. 
   else if(moveState == 2)
      {
      model[3] *= glm::rotate( glm::mat4(1.0f), 
                              degreesToRadians(420 * -dt), 
                              glm::vec3(0.0, 0.0, 1.0));  
      }

   // Rotate bottom knob to the right. 
   else if(moveState == 3)
      {
      model[3] *= glm::rotate( glm::mat4(1.0f), 
                              degreesToRadians(420 * dt), 
                              glm::vec3(0.0, 0.0, 1.0));  
      }

   // Reset movement state for knobs.  
   moveState = -1; 

   // Allow the background image to rotate. 
   model[5] *= glm::rotate( glm::mat4(1.0f), 
                           degreesToRadians(10.0f * dt), 
                           glm::vec3(0.0, 0.0, 1.0) );

   // Update the state of the scene (display callback)
   glutPostRedisplay();
   }

void reshape( int n_w, 
              int n_h )
   {
   // Initialize function/variables.
   windowWidth = n_w;
   windowHeight = n_h;

   // Change the viewport to be correct.
   glViewport( 0, 0, windowWidth, windowHeight );

   // Update the projection matrix as well
   projection = glm::perspective( 45.0f, 
                                    float(windowWidth)/float(windowHeight), 
                                    0.01f, 100.0f );
   }

void specialKeys( int button, int x, int y )
   {
   // Initialize function/variables. 
   float xGrav = 0.0;
   float yGrav = 0.0;
   float zGrav = 0.0; 

   // Check to see if keyboard is temporarily disabled.
   // This is really for the arrow keys only so the user doesn't 
   // move the ball around as it is initially falling onto the board. 
   if(disableKeyboard == false)
      {
      switch( button )
         {
         // Tilt board up. 
         case GLUT_KEY_UP:
            if(phi < M_PI/2)
               {
               phi += 0.1;
               moveState = 0;  
               } 
         break;

         // Tilt board down. 
         case GLUT_KEY_DOWN:
            if(phi > -M_PI/2)
               {
               phi -= 0.1;
               moveState = 1;  
               } 
         break;

         // Tilt board left. 
         case GLUT_KEY_LEFT:
            if(theta < M_PI/2)
               {
               theta += 0.1;
               moveState = 2;  
               }
         break;

         // Tilt board right. 
         case GLUT_KEY_RIGHT:
            if(theta > -M_PI/2)
               {
               theta -= 0.1;
               moveState = 3;  
               }
         break;
         };

      // X component of gravity.
      xGrav = 9.81 * sin(theta); 

      // Y component of gravity. 
      if(cos(phi) > cos(theta))
         {
         yGrav = -9.81 * cos(phi); 
         }
      else
         {
         yGrav = -9.81 * cos(theta); 
         } 

      // Z component of gravity.
      zGrav = 9.81 * sin(phi); 

      // Reset the gravity. 
      dynamicsWorld->setGravity(btVector3(xGrav, yGrav, zGrav));
      }      
   }

void keyboard( unsigned char key, 
               int x_pos, 
               int y_pos )
   {
   // Handle keyboard input
   switch( key )
      {
      // ESC
      case 27: 
         exit( 0 );
      break;

      // Spotlight toggling option.
      case 's':
      case 'S':
         // Update toggling option. 
         spotLightOption = !spotLightOption;

         // Spotlight is off. 
         if(spotLightOption == false)
            {
            lights[0].lightA = 0.0;
            lights[1].lightA = 0.0; 
            lights[2].lightA = 0.0;
            }

         // Spotlight is on.    
         else if(spotLightOption ==  true)
            {
            lights[0].lightA = 1.0;
            lights[1].lightA = 1.0; 
            lights[2].lightA = 1.0;
            } 

         // Ipdate lighting vector to be sent to shaders. 
         lPos[3] = lights[0].lightA;
         lPos[7] = lights[1].lightA;
         lPos[11] = lights[2].lightA;
      break; 

      // Standard view. 
      case '1':
         view = glm::lookAt( glm::vec3(0.0, 35.0, -20.0),
                              glm::vec3(0.0, 0.0, 0.0), // Focus point
                              glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
      break; 

      // Top-down view. 
      case '2':
         view = glm::lookAt( glm::vec3(0, 40, 0), // Eye position
                              glm::vec3(0.0, 0.0, 0.0), // Focus point
                              glm::vec3(0.0, 0.0, 1.0) ); // Positive Y is "up"
         viewMode = 1;
      break;

      // Side view. 
      case '3':
         view = glm::lookAt( glm::vec3(25.0, 0.0, -30.0),
                              glm::vec3(0.0, 0.0, 0.0), // Focus point
                              glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
      break; 

      // Space bar pauses game. 
      case ' ':
         pauseGame = !pauseGame;
      break;    
    }
  }

void menu( int selection )
   {
   // Initialize function/variables.
   btDefaultMotionState *shapeMotionState = NULL;

   // Determine option. 
   switch( selection )
      {
      // Continue the game.
      case 1:
         pauseGame = false; 
      break;

      // Pause the game.
      case 2:
         pauseGame = true;
      break;

      // Change levels.
      case 3:
         // Enable proper toggling.
         disableKeyboard = true; 

         // Sound for ocean level. 
         if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "oceanStart.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "pacmanStart.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);

         // Turn on all lights while the ball is falling. 
         lights[0].lightA = 0.0;
         lights[1].lightA = 0.0; 
         lights[2].lightA = 0.0; 
         lPos[3] = lights[0].lightA;
         lPos[7] = lights[1].lightA;
         lPos[11] = lights[2].lightA;

         // Reset board to a traditional, flat orientation. 
         theta = 0.0;
         phi = 0.0;
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Position the first ball. 
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9))); 
         ballOneRigidBody->setMotionState(shapeMotionState);
         ballOneRigidBody->setLinearVelocity(btVector3(0, -9.81, 0));

         // Position the second ball (if there is a second ball)
         if(extraBall == true)
            {
            shapeMotionState = new btDefaultMotionState(btTransform(
                                                         btQuaternion(0, 0, 0, 1), 
                                                         btVector3(-6, 15, -8))); 
            ballTwoRigidBody->setMotionState(shapeMotionState);
            ballTwoRigidBody->setLinearVelocity(btVector3(0, -9.81, 0));
            }         

         if(levelOption == false)
            {
            // Remove the second labyrinth rigid body board from the world.  
            dynamicsWorld->removeRigidBody(boardTwoRigidBody); 
            boardTwoRigidBody = NULL; 

            // Replace the previously removed rigid body with a new rigid body. 
            btCollisionShape *shape = new btBvhTriangleMeshShape(boardOneTriMesh, 
                                                                  true);
            btDefaultMotionState *shapeMotionState = NULL; 
            shapeMotionState = new btDefaultMotionState( 
                                                btTransform(
                                                   btQuaternion(0, 0, 0, 1), 
                                                   btVector3(0, 0, 0) ) ); 
            btScalar mass(0.0);
            btVector3 inertia(0, 0, 0); 
            shape->calculateLocalInertia(mass, inertia); 
            btRigidBody::btRigidBodyConstructionInfo
               shapeRigidBodyCI(mass, shapeMotionState, shape, inertia);
            boardOneRigidBody = new btRigidBody(shapeRigidBodyCI);
            boardOneRigidBody->setRestitution(1.0);
            boardOneRigidBody->setFriction(0.2);
            boardOneRigidBody->setDamping(0.5, 0.5);
            boardOneRigidBody->setActivationState(DISABLE_DEACTIVATION); 
            dynamicsWorld->addRigidBody(boardOneRigidBody, 
                                          COLLIDE_BOARD, 
                                          boardCollidesWith); 


            // Update level indicator. 
            levelOption = !levelOption;
            }

         else if(levelOption == true)
            {
            // Remove the first labyrinth rigid body board from the world.  
            dynamicsWorld->removeRigidBody(boardOneRigidBody);
            boardOneRigidBody = NULL; 

            // Replace the previously removed rigid body with a new rigid body. 
            btCollisionShape *shape = new btBvhTriangleMeshShape(boardTwoTriMesh, 
                                                                  true);
            btDefaultMotionState *shapeMotionState = NULL; 
            shapeMotionState = new btDefaultMotionState( 
                                                btTransform(
                                                   btQuaternion(0, 0, 0, 1), 
                                                   btVector3(0, 0, 0) ) );
            btScalar mass(0.0);
            btVector3 inertia(0, 0, 0); 
            shape->calculateLocalInertia(mass, inertia); 
            btRigidBody::btRigidBodyConstructionInfo
               shapeRigidBodyCI(mass, shapeMotionState, shape, inertia);
            boardTwoRigidBody = new btRigidBody(shapeRigidBodyCI);
            boardTwoRigidBody->setRestitution(1.0);
            boardTwoRigidBody->setFriction(0.2);
            boardTwoRigidBody->setDamping(0.5, 0.5);
            boardTwoRigidBody->setActivationState(DISABLE_DEACTIVATION); 
            dynamicsWorld->addRigidBody(boardTwoRigidBody, 
                                          COLLIDE_BOARD, 
                                          boardCollidesWith);

            // Update level indicator. 
            levelOption = !levelOption;
            }  
      break; 

      // Restart the game with one ball.
      case 4:
         // Reset toggling options. 
         disableKeyboard = true; 
         extraBall = false;
         timeElapsed = 0.0;  

         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanStart.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanStart.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);

         // Turn on all lights while the ball is falling. 
         lights[0].lightA = 0.0;
         lights[1].lightA = 0.0; 
         lights[2].lightA = 0.0; 
         lPos[3] = lights[0].lightA;
         lPos[7] = lights[1].lightA;
         lPos[11] = lights[2].lightA;

         // Check if we need to remove the second ball from the dynamics world. 
         if(ballTwoRigidBody != NULL)
            {
            dynamicsWorld->removeRigidBody(ballTwoRigidBody); 
            ballTwoRigidBody = NULL; 
            }

         // Reset board to a traditional, flat orientation. 
         theta = 0.0;
         phi = 0.0;
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Reset the position of the ball. 
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9))); 
         ballOneRigidBody->setMotionState(shapeMotionState); 
         ballOneRigidBody->setLinearVelocity(btVector3(0, -9.81, 0)); 
      break;

      // Restart game with second ball. 
      case 5:
         // Reset toggling options. 
         disableKeyboard = true; 
         extraBall = true;
         timeElapsed = 0.0;  

         // Sound for ocean level. 
         if(levelOption == true)
            {
            soundPtr = Mix_LoadWAV( "oceanStart.wav" );
            }

         // Sound for pacman level. 
         else if(levelOption == false)
            {
            soundPtr = Mix_LoadWAV( "pacmanStart.wav" );
            }

         // Play sound. 
         Mix_PlayChannel(-1, soundPtr, 0);

         // Turn on all lights while the ball is falling. 
         lights[0].lightA = 0.0;
         lights[1].lightA = 0.0; 
         lights[2].lightA = 0.0; 
         lPos[3] = lights[0].lightA;
         lPos[7] = lights[1].lightA;
         lPos[11] = lights[2].lightA;

         // Reset board to a traditional, flat orientation. 
         theta = 0.0;
         phi = 0.0;
         dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

         // Position both balls. 
         {
         // Position the first ball. 
         shapeMotionState = new btDefaultMotionState(btTransform(
                                                      btQuaternion(0, 0, 0, 1), 
                                                      btVector3(-6, 15, -9))); 
         ballOneRigidBody->setMotionState(shapeMotionState);
         ballOneRigidBody->setLinearVelocity(btVector3(0, -9.81, 0));

         // Create the second ball and add it to the dynamics world. 
         btCollisionShape *shape = new btSphereShape(0.3); 
         btDefaultMotionState *shapeMotionState = NULL; 
         shapeMotionState = new btDefaultMotionState( 
                                             btTransform( 
                                                btQuaternion(0, 0, 0, 1), 
                                                btVector3(-6, 15, -8) ) ); 
         btScalar mass(1.0);
         btVector3 inertia(0, 0, 0); 
         shape->calculateLocalInertia(mass, inertia);
         btRigidBody::btRigidBodyConstructionInfo
            shapeRigidBodyCI(mass, shapeMotionState, shape, inertia);
         ballTwoRigidBody = new btRigidBody(shapeRigidBodyCI);
         ballTwoRigidBody->setRestitution(0.4);
         ballTwoRigidBody->setFriction(0.5);
         ballTwoRigidBody->setDamping(0.8, 0.2);
         ballTwoRigidBody->setLinearVelocity(btVector3(0, -9.81, 0));
         ballTwoRigidBody->setActivationState( DISABLE_DEACTIVATION ); 
         dynamicsWorld->addRigidBody(ballTwoRigidBody, 
                                       COLLIDE_BALL, 
                                       ballCollidesWith);
         }       
      break;

      // Exit the game.  
      case 6:
         exit(0); 
      break; 
      }

   // Redraw screen without the menu. 
   glutPostRedisplay(); 
   }

bool initialize( void )
   {
   // Initialize lighting attributes. 
   initializeLights();
  
   // Initialize geometry - labyrinth objects.
   Vertex *ballGeometry = objLoader( "L_ball.obj", 
                                       numberOfBallVertices,
                                       ballOneTriMesh );
   ballTwoTriMesh = ballOneTriMesh;  
   Vertex *boardOneGeometry = objLoader( "L_inner1.obj", 
                                          numberOfBoardOneVertices,
                                          boardOneTriMesh );
   Vertex *boardTwoGeometry = objLoader( "L_inner2.obj", 
                                          numberOfBoardTwoVertices,
                                          boardTwoTriMesh );
   Vertex *knobOneGeometry = objLoader( "L_knob1.obj", 
                                          numberOfKnobOneVertices,
                                          knobOneTriMesh );
   Vertex *knobTwoGeometry = objLoader( "L_knob2.obj", 
                                          numberOfKnobTwoVertices,
                                          knobTwoTriMesh );
   Vertex *outerBoardGeometry = objLoader( "L_outer.obj", 
                                             numberOfOuterBoardVertices,
                                             outerBoardTriMesh );
   Vertex *backgroundGeometry = objLoader( "sphere.obj", 
                                             numberOfBackgroundVertices, 
                                             backgroundMesh ); 

   // Initialize model matrices.
   model[0] = glm::mat4(1.0f);
   model[1] = glm::mat4(1.0f); 
   model[2] = glm::mat4(1.0f); 
   model[3] = glm::mat4(1.0f);
   model[4] = glm::mat4(1.0f);
   model[5] = glm::scale( glm::mat4(1.0f), glm::vec3(6.0f) );
   model[6] = glm::mat4(1.0f);   

   // Initialize images - THEME 1 - OCEAN.
   Magick::Image *ballImageOne = new Magick::Image( "ball_b.jpg" );
   ballImageOne->write( &m_blob[0], "RGBA" ); 
   Magick::Image *innerBoxImageOne = new Magick::Image( "L_inner1_b.jpg" );
   innerBoxImageOne->write( &m_blob[1], "RGBA" ); 
   Magick::Image *firstKnobImageOne = new Magick::Image( "L_knob1_b.jpg" ); 
   firstKnobImageOne->write( &m_blob[2], "RGBA" ); 
   Magick::Image *firstKnobImageTwo = new Magick::Image( "L_knob2_b.jpg" ); 
   firstKnobImageTwo->write( &m_blob[3], "RGBA" );  
   Magick::Image *outerBoxImageOne = new Magick::Image( "L_outer_b.jpg" );
   outerBoxImageOne->write( &m_blob[4], "RGBA" );
   Magick::Image *backgroundImageOne = new Magick::Image( "L_background_b.jpg" );
   backgroundImageOne->write( &m_blob[5], "RGBA" );

   // Initialize images - THEME 2 - PACMAN.
   Magick::Image *ballImageTwo = new Magick::Image( "ball_p.jpg" );
   ballImageTwo->write( &m_blob[6], "RGBA" ); 
   Magick::Image *innerBoxImageTwo = new Magick::Image( "L_inner2_p.jpg" );
   innerBoxImageTwo->write( &m_blob[7], "RGBA" ); 
   Magick::Image *secondKnobImageOne = new Magick::Image( "L_knob1_p.jpg" ); 
   secondKnobImageOne->write( &m_blob[8], "RGBA" ); 
   Magick::Image *secondKnobImageTwo = new Magick::Image( "L_knob2_p.jpg" ); 
   secondKnobImageTwo->write( &m_blob[9], "RGBA" );  
   Magick::Image *outerBoxImageTwo = new Magick::Image( "L_outer_p.jpg" );
   outerBoxImageTwo->write( &m_blob[10], "RGBA" );
   Magick::Image *backgroundImageTwo = new Magick::Image( "L_background_p.jpg" );
   backgroundImageTwo->write( &m_blob[11], "RGBA" );

   // Initialize bullet.
   broadphase = new btDbvtBroadphase();
   collisionConfiguration = new btDefaultCollisionConfiguration(); 
   dispatcher = new btCollisionDispatcher( collisionConfiguration );
   solver = new btSequentialImpulseConstraintSolver; 
   dynamicsWorld = new btDiscreteDynamicsWorld( dispatcher, broadphase,
                                               solver, collisionConfiguration ); 
   dynamicsWorld->setGravity( btVector3(0, -9.81, 0) );

   // Create a rigid body for the labryinth ball.
   {
   btCollisionShape *shape = new btSphereShape(0.3); 
   btDefaultMotionState *shapeMotionState = NULL; 
   shapeMotionState = new btDefaultMotionState( 
                                       btTransform( 
                                          btQuaternion(0, 0, 0, 1), 
                                          btVector3(-6, 15, -9) ) ); 
   btScalar mass(1.0);
   btVector3 inertia(0, 0, 0); 
   shape->calculateLocalInertia(mass, inertia);
   btRigidBody::btRigidBodyConstructionInfo
      shapeRigidBodyCI(mass, shapeMotionState, shape, inertia);
   ballOneRigidBody = new btRigidBody(shapeRigidBodyCI);
   ballOneRigidBody->setRestitution(0.4);
   ballOneRigidBody->setFriction(0.5);
   ballOneRigidBody->setDamping(0.8, 0.2);
   ballOneRigidBody->setActivationState( DISABLE_DEACTIVATION ); 
   dynamicsWorld->addRigidBody(ballOneRigidBody, 
                                 COLLIDE_BALL, 
                                 ballCollidesWith); 
   }

   // Create a rigid body for the FIRST labyrinth game board.
   {
   btCollisionShape *shape = new btBvhTriangleMeshShape(boardOneTriMesh, true);
   btDefaultMotionState *shapeMotionState = NULL; 
   shapeMotionState = new btDefaultMotionState( 
                                       btTransform(
                                          btQuaternion(0, 0, 0, 1), 
                                          btVector3(0, 0, 0) ) ); 
   btScalar mass(0.0);
   btVector3 inertia(0, 0, 0); 
   shape->calculateLocalInertia(mass, inertia); 
   btRigidBody::btRigidBodyConstructionInfo
      shapeRigidBodyCI(mass, shapeMotionState, shape, inertia);
   boardOneRigidBody = new btRigidBody(shapeRigidBodyCI);
   boardOneRigidBody->setRestitution(1.0);
   boardOneRigidBody->setFriction(0.2);
   boardOneRigidBody->setDamping(0.5, 0.5);
   boardOneRigidBody->setActivationState(DISABLE_DEACTIVATION); 
   dynamicsWorld->addRigidBody(boardOneRigidBody, 
                                 COLLIDE_BOARD, 
                                 boardCollidesWith);  
   }  

   // Create a Vertex Buffer object to store vertex data on GPU
   glGenBuffers( NUM_OBJECTS, vbo_geometry );

   // Allocate space on GPU for labyrinth ball. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[0] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBallVertices, 
                  ballGeometry, 
                  GL_STATIC_DRAW );

   // Allocate space on GPU for FIRST labyrinth game board. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[1] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBoardOneVertices, 
                  boardOneGeometry, 
                  GL_STATIC_DRAW );

   // Allocate space on GPU for SECOND labyrinth game board. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[2] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBoardTwoVertices, 
                  boardTwoGeometry, 
                  GL_STATIC_DRAW ); 

   // Allocate space on GPU for FIRST labyrinth game board knob. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[3] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfKnobOneVertices, 
                  knobOneGeometry, 
                  GL_STATIC_DRAW ); 

   // Allocate space on GPU for SECOND labyrinth game board knob. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[4] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfKnobTwoVertices, 
                  knobTwoGeometry, 
                  GL_STATIC_DRAW ); 

   // Allocate space on GPU for OUTER labyrinth game board. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[5] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfOuterBoardVertices, 
                  outerBoardGeometry, 
                  GL_STATIC_DRAW );  

   // Allocate space on GPU for OUTER square. 
   glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[6] );
   glBufferData( GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBackgroundVertices, 
                  backgroundGeometry, 
                  GL_STATIC_DRAW );  

   // Create a texture buffer object.  
   glActiveTexture( GL_TEXTURE0 ); 
   glGenTextures((NUM_OBJECTS - 1) * 2, textures);

   // Labyrinth BALL texture - FIRST THEME.
   glBindTexture( GL_TEXTURE_2D, textures[0] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ballImageOne->columns(), 
                  ballImageOne->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[0].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth GAME BOARD texture - FIRST THEME.
   glBindTexture( GL_TEXTURE_2D, textures[1] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, innerBoxImageOne->columns(), 
                  innerBoxImageOne->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[1].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth FIRST KNOB texture - FIRST THEME.
   glBindTexture( GL_TEXTURE_2D, textures[2] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, firstKnobImageOne->columns(), 
                  firstKnobImageOne->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[2].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth SECOND KNOB texture - FIRST THEME.
   glBindTexture( GL_TEXTURE_2D, textures[3] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, firstKnobImageTwo->columns(), 
                  firstKnobImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[3].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth OUTER GAME BOARD texture - FIRST THEME.
   glBindTexture( GL_TEXTURE_2D, textures[4] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, outerBoxImageOne->columns(), 
                  outerBoxImageOne->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[4].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth BACKGROUND texture - FIRST THEME.
   glBindTexture( GL_TEXTURE_2D, textures[5] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, backgroundImageOne->columns(), 
                  backgroundImageOne->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[5].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth BALL texture - SECOND THEME.
   glBindTexture( GL_TEXTURE_2D, textures[6] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ballImageTwo->columns(), 
                  ballImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[6].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth GAME BOARD texture - SECOND THEME.
   glBindTexture( GL_TEXTURE_2D, textures[7] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, innerBoxImageTwo->columns(), 
                  innerBoxImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[7].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth FIRST KNOB texture - SECOND THEME.
   glBindTexture( GL_TEXTURE_2D, textures[8] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, secondKnobImageOne->columns(), 
                  secondKnobImageOne->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[8].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth SECOND KNOB texture - SECOND THEME.
   glBindTexture( GL_TEXTURE_2D, textures[9] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, secondKnobImageTwo->columns(), 
                  secondKnobImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[9].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth OUTER GAME BOARD texture - SECOND THEME.
   glBindTexture( GL_TEXTURE_2D, textures[10] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, outerBoxImageTwo->columns(), 
                  outerBoxImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[10].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Labyrinth BACKGROUND texture - SECOND THEME.
   glBindTexture( GL_TEXTURE_2D, textures[11] );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, backgroundImageTwo->columns(), 
                  backgroundImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  m_blob[11].data() );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   // Initialize and compile shaders. 
   shader vAndFShaders[2];
   vAndFShaders[0].setShaderFile( "vShader.vert" );
   vAndFShaders[0].readShaderFile();
   vAndFShaders[0].setShaderType( GL_VERTEX_SHADER );
   vAndFShaders[1].setShaderFile( "fShader.frag" );
   vAndFShaders[1].readShaderFile();
   vAndFShaders[1].setShaderType( GL_FRAGMENT_SHADER );   
   program = vAndFShaders[0].compileAndLinkShaders( vAndFShaders );

   // Obtain locations of variables in shaders. 
   loc_position = glGetAttribLocation(program, 
                  const_cast<const char*>( "v_position" ) ); 
   if(loc_position == -1)
      {
      std::cerr << "[F] POSITION (attribute) NOT FOUND" << std::endl;
      return false;
      } 

   loc_normal = glGetAttribLocation(program, 
                  const_cast<const char*>( "v_normal" ) ); 
   if(loc_normal == -1)
      {
      std::cerr << "[F] NORMAL (attribute) NOT FOUND" << std::endl;
      return false;
      }

   loc_uv = glGetAttribLocation(program,
            const_cast<const char*>( "v_texture" ) ); 
   if(loc_uv == -1)
      {
      std::cerr << "[F] UV_POSITION (attribute) NOT FOUND" << std::endl;
      return false;
      }

   locMVP = glGetUniformLocation(program,
                     const_cast<const char*>("mvpMatrix"));
   if(locMVP == -1)
      {
      std::cerr << "[F] MVPMATRIX (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locLightPos = glGetUniformLocation(program,
                  const_cast<const char*>("lightPosition"));
   if(locLightPos == -1)
      {
      std::cerr << "[F] lightPosition (uniform) NOT FOUND" << std::endl;
      return false;
      }    

   locAmbient = glGetUniformLocation(program,
                  const_cast<const char*>("ambientProduct"));
   if(locAmbient == -1)
      {
      std::cerr << "[F] ambientProduct (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locDiffuse = glGetUniformLocation(program,
                  const_cast<const char*>("diffuseProduct"));
   if(locDiffuse == -1)
      {
      std::cerr << "[F] diffuseProduct (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpecular = glGetUniformLocation(program,
                   const_cast<const char*>("specularProduct"));
   if(locSpecular == -1)
      {
      std::cerr << "[F] specularProduct (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locShininess = glGetUniformLocation(program,
                  const_cast<const char*>("shininess"));
   if(locShininess == -1)
      {
      std::cerr << "[F] shininess (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locConstAttenuation = glGetUniformLocation(program,
                        const_cast<const char*>("constantAttenuation"));
   if(locConstAttenuation == -1)
      {
      std::cerr << "[F] constantAttenuation (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locLinAttenuation = glGetUniformLocation(program,
                        const_cast<const char*>("linearAttenuation"));
   if(locLinAttenuation == -1)
      {
      std::cerr << "[F] linearAttenuation (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locQuadAttenuation = glGetUniformLocation(program,
                        const_cast<const char*>("quadraticAttenuation"));
   if(locQuadAttenuation == -1)
      {
      std::cerr << "[F] quadraticAttenuation (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpotCutoff = glGetUniformLocation(program,
                     const_cast<const char*>("spotCutoff"));
   if(locSpotCutoff == -1)
      {
      std::cerr << "[F] spotCutoff (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpotExponent = glGetUniformLocation(program,
                     const_cast<const char*>("spotExponent"));
   if(locSpotExponent == -1)
      {
      std::cerr << "[F] spotExponent (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpotDirection = glGetUniformLocation(program,
                     const_cast<const char*>("spotDirection"));
   if(locSpotDirection == -1)
      {
      std::cerr << "[F] spotDirection NOT FOUND" << std::endl;
      return false;
      }

   // Send initial data to the shaders. 
   glUniform4fv(locAmbient, 1, glm::value_ptr(ambientProduct));
   glUniform4fv(locDiffuse, 1, glm::value_ptr(diffuseProduct));
   glUniform4fv(locSpecular, 1, glm::value_ptr(specularProduct));
   glUniform4fv(locLightPos, 3, lPos);
   glUniform1f(locShininess, materialShininess);
   glUniform1f(locConstAttenuation, constantAttenuation);
   glUniform1f(locLinAttenuation, linearAttenuation);
   glUniform1f(locQuadAttenuation, quadraticAttenuation);
   glUniform3fv(locSpotDirection, 3, spotDir);
   glUniform1fv(locSpotCutoff, 3, spotCut);
   glUniform1fv(locSpotExponent, 3, spotExp);
  
   // Initialize the view and projection matrices.
   view = glm::lookAt( glm::vec3(0.0, 35.0, -20.0),
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"

   projection = glm::perspective( 45.0f, 
                                 float(windowWidth)/float(windowHeight),
                                 0.01f, // Distance to the near plane
                                 100.0f); // Distance to the far plane 

   // Enable depth testing. 
   glEnable( GL_DEPTH_TEST );
   glDepthFunc( GL_LESS );

   // Deallocate memory.
   delete []ballGeometry;
   ballGeometry = NULL;
   delete []boardOneGeometry;
   boardOneGeometry = NULL;
   delete []boardTwoGeometry;
   boardTwoGeometry = NULL;
   delete []knobOneGeometry;
   knobOneGeometry = NULL;
   delete []knobTwoGeometry;
   knobTwoGeometry = NULL; 
   delete []outerBoardGeometry;
   outerBoardGeometry = NULL;   

   // Function conclusion. 
   return true;
   }

void cleanUp( void )
   {
   // Clean up, Clean up.
   delete []lPos;
   lPos = NULL; 
   delete []spotDir; 
   spotDir = NULL; 
   delete []spotCut;
   spotCut = NULL; 
   delete []spotExp;
   spotExp = NULL; 
   delete dynamicsWorld;
   dynamicsWorld = NULL; 
   delete solver; 
   solver = NULL;
   delete dispatcher; 
   dispatcher = NULL;
   delete collisionConfiguration;
   collisionConfiguration = NULL;
   delete broadphase;
   broadphase = NULL;
   Mix_FreeChunk( soundPtr );
   soundPtr = NULL; 
   Mix_Quit(); 
   glDeleteProgram( program );
   glDeleteBuffers( NUM_OBJECTS, vbo_geometry );
   }

float getDT( void )
   {
   float ret;
   t2 = std::chrono::high_resolution_clock::now();
   ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
   t1 = std::chrono::high_resolution_clock::now();
   return ret;
   }

Vertex *objLoader( const char *objFile, 
                   int &numVertices,
                   btTriangleMesh *&objTriMesh )
   {
   // Initialize function/variables.
   Assimp::Importer importer;
   const aiScene *pScene = importer.ReadFile( objFile, 
                                             aiProcess_Triangulate | 
                                             aiProcess_GenNormals ); 

   // Check to see if the scene loaded correctly.
   if( !pScene )
    {
    printf( "Error parsing '%s': '%s'\n", objFile, importer.GetErrorString() );
    exit( EXIT_FAILURE );  
    }

   // Find the first mesh in the scene. 
   const aiMesh *mesh = pScene->mMeshes[0];

   // Allocate sufficient memory.
   numVertices = mesh->mNumFaces * 3;
   btVector3 triArray[3]; 
   objTriMesh = new btTriangleMesh();  
   Vertex *outboundVertexData = new Vertex[ numVertices ]; 
   Vertex *home = outboundVertexData;

   // Iterate through each face in the current mesh.
   for( unsigned int i = 0; i < mesh->mNumFaces; i++ )
      {
      // Obtain the vertices/textures that make up each face. 
      const aiFace &face = mesh->mFaces[i];
      for( unsigned int j = 0; j < 3; j++ )
         {
         // Obtain the vertices in each face. 
         aiVector3D position = mesh->mVertices[ face.mIndices[j] ]; 
         outboundVertexData->position[0] = position.x; 
         outboundVertexData->position[1] = position.y; 
         outboundVertexData->position[2] = position.z;
         triArray[j] = btVector3(position.x, position.y, position.z); 

         // Obtain the textures in each face.
         if( mesh->HasTextureCoords(0) )
            {
            aiVector3D uv = mesh->mTextureCoords[0][ face.mIndices[j] ]; 
            outboundVertexData->uv[0] = uv.x; 
            outboundVertexData->uv[1] = uv.y; 
            }

         else
            {
            cout << "ERROR: File does not contain texture coordinates!" << endl;
            exit( EXIT_FAILURE ); 
            }

         // Obtain the textures in each face. 
         if( mesh->HasNormals() )
            {
            aiVector3D normalTemp = mesh->mNormals[ face.mIndices[j] ];
            outboundVertexData->normal[0] = normalTemp.x;
            outboundVertexData->normal[1] = normalTemp.y;
            outboundVertexData->normal[2] = normalTemp.z; 
            }

         else
            {
            cout << "ERROR: File does not contain normal coordinates!" << endl;
            exit( EXIT_FAILURE ); 
            }
         outboundVertexData++; 
         }
      objTriMesh->addTriangle(triArray[0], triArray[1], triArray[2]); 
      }

   // Function conclusion.
   outboundVertexData = home;
   return outboundVertexData; 
   }

void initializeLights(void)
   {
   // Initialize light attributes. 
   // Light 0.    
   lights[0].lightX = 3.0;
   lights[0].lightY = 5.0;
   lights[0].lightZ = 5.0;
   lights[0].lightA = 0.0;
   lights[0].spotX = 0.0;
   lights[0].spotY = -1.0;
   lights[0].spotZ = 0.0;
   lights[0].lightPosition = glm::vec4(lights[0].lightX,
                                       lights[0].lightY,
                                       lights[0].lightZ,
                                       lights[0].lightA);
   lights[0].lightAmbient = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[0].lightDiffuse = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[0].lightSpecular = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[0].spotDirection = glm::vec3(lights[0].spotX,
                                       lights[0].spotY, 
                                       lights[0].spotZ);
   lights[0].spotCutoff = 15.0; 
   lights[0].spotExponent = 1.0; 

   // Light 1. 
   lights[1].lightX = 2.0;
   lights[1].lightY = 2.0;
   lights[1].lightZ = -1.0;
   lights[1].lightA = 0.0;
   lights[1].spotX = 0.0;
   lights[1].spotY = -1.0;
   lights[1].spotZ = 0.0;
   lights[1].lightPosition = glm::vec4(lights[1].lightX,
                                       lights[1].lightY,
                                       lights[1].lightZ,
                                       lights[1].lightA);
   lights[1].lightAmbient = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[1].lightDiffuse = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[1].lightSpecular = glm::vec4(0.1, 0.1, 0.1, 0.0);;
   lights[1].spotDirection = glm::vec3(lights[1].spotX,
                                       lights[1].spotY, 
                                       lights[1].spotZ);
   lights[1].spotCutoff = 15.0; 
   lights[1].spotExponent = 1.0; 

   // Light 2. 
   lights[2].lightX = 6.0;
   lights[2].lightY = 4.0;
   lights[2].lightZ = 8.0;
   lights[2].lightA = 0.0;
   lights[2].spotX = 0.0;
   lights[2].spotY = -1.0;
   lights[2].spotZ = 0.0;
   lights[2].lightPosition = glm::vec4(lights[2].lightX,
                                       lights[2].lightY,
                                       lights[2].lightZ,
                                       lights[2].lightA);
   lights[2].lightAmbient = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[2].lightDiffuse = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[2].lightSpecular = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[2].spotDirection = glm::vec3(lights[2].spotX,
                                       lights[2].spotY, 
                                       lights[2].spotZ);
   lights[2].spotCutoff = 25.0; 
   lights[2].spotExponent = 1.0; 

   // Initialize material properties for lights.
   materialAmbient = glm::vec4(0.5, 0.5, 0.5, 1.0);
   materialDiffuse = glm::vec4(0.5, 0.5, 0.5, 1.0);
   materialSpecular = glm::vec4(0.5, 0.5, 0.5, 1.0);
   materialShininess = 0.5;

   // Initialize attenuation terms for lights.
   constantAttenuation = 0.05;
   linearAttenuation = 1.0;
   quadraticAttenuation = 0.05;

   // Calculate light-material product vectors.
   ambientProduct = (lights[0].lightAmbient + 
                     lights[1].lightAmbient +
                     lights[2].lightAmbient)
                     * materialAmbient;
   diffuseProduct = (lights[0].lightDiffuse + 
                     lights[1].lightDiffuse + 
                     lights[2].lightDiffuse)
                     * materialDiffuse;
   specularProduct = (lights[0].lightSpecular + 
                        lights[1].lightSpecular + 
                        lights[2].lightSpecular)
                        * materialSpecular;   

   // Light position array to send to shaders. 
   lPos[0] = lights[0].lightPosition.x;
   lPos[1] = lights[0].lightPosition.y;
   lPos[2] = lights[0].lightPosition.z;
   lPos[3] = lights[0].lightPosition.a;
   lPos[4] = lights[1].lightPosition.x;
   lPos[5] = lights[1].lightPosition.y;
   lPos[6] = lights[1].lightPosition.z;
   lPos[7] = lights[1].lightPosition.a;
   lPos[8] = lights[2].lightPosition.x;
   lPos[9] = lights[2].lightPosition.y;
   lPos[10] = lights[2].lightPosition.z;
   lPos[11] = lights[2].lightPosition.a;

   // Spotlight position array to send to shaders.
   spotDir[0] = lights[0].spotDirection.x;  
   spotDir[1] = lights[0].spotDirection.y;  
   spotDir[2] = lights[0].spotDirection.z;  
   spotDir[3] = lights[1].spotDirection.x;  
   spotDir[4] = lights[1].spotDirection.y;  
   spotDir[5] = lights[1].spotDirection.z;
   spotDir[6] = lights[2].spotDirection.x;  
   spotDir[7] = lights[2].spotDirection.y;  
   spotDir[8] = lights[2].spotDirection.z;

   // Spotlight cutoff array.
   spotCut[0] = lights[0].spotCutoff;  
   spotCut[1] = lights[1].spotCutoff;
   spotCut[2] = lights[2].spotCutoff;

   // Spotlight exponent array. 
   spotExp[0] = lights[0].spotExponent;  
   spotExp[1] = lights[1].spotExponent;
   spotExp[2] = lights[2].spotExponent;
   }