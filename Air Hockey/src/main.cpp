/*
Names: Renee Iinuma, Kyle Lee, and Wesley Kepke. 

Class: CS 480 

Due Date: Monday, November 3, 2014

Project: Air Hockey

Description: This program is our take on air hockey, a classic game in which
two (usually two) players try to score points by using their paddle to knock
the puck into the opposing player's goal.  

Interaction: TODO
*/

/* Header Files 
Note: "shader.h" contains <GL/glew.h>, <GL/glut.h>, <iostream>, <fstream>, and
<string.h>.  
*/  
#include <btBulletDynamicsCommon.h> // Necessary for bullet 
#include "SDL/SDL_mixer.h"
#include "shader.h" // Necessary to load and compile shaders 
#include <vector>
#include <chrono>
#define GLM_FORCE_RADIANS // To ignore compiler warnings from assimp
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/color4.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // Makes passing matrices to shaders easier
#include <Magick++.h> // Image library used to load textures for objects 
#include <GL/freeglut.h>

// "Vertex" object - used to send object data to shaders 
struct Vertex
  {
  GLfloat position[3];
  GLfloat uv[2];
  };

/* 
"COLLISION" enumeration - for object collisions.

Bullet supports bitwise masks as a way of deciding whether or not things chould
collide with other things, or receive collisions. 
*/
#define BIT(x) ( 1 << (x) ) // Bitmask used for collisions 
enum COLLISIONS
  {
  COLLIDE_NOTHING = 0, // Collide with nothing 
  COLLIDE_MIDDLE = BIT(1), // Collide with the middle wall (pucks)
  COLLIDE_GOAL = BIT(2), // Collide with the goal (pucks)
  COLLIDE_WALL = BIT(3), // Collide with walls (pucks and paddles)
  COLLIDE_PUCK = BIT(4), // Collide with puck (paddles) 
  COLLIDE_PADDLE = BIT(5), // Collide with paddle (pucks)
  COLLIDE_TOP = BIT(6), // Collide with top and bottom of table
  COLLIDE_GROUND = BIT(7)
  }; 

// Define what our rigid bodies can collide with 
int paddleCollidesWith = COLLIDE_MIDDLE | COLLIDE_GOAL | 
                         COLLIDE_PUCK | COLLIDE_WALL | COLLIDE_GROUND |
                         COLLIDE_TOP;
int puckCollidesWith = COLLIDE_WALL | COLLIDE_PADDLE | COLLIDE_GROUND |
                       COLLIDE_TOP;
int wallCollidesWith = COLLIDE_PADDLE | COLLIDE_PUCK;
int goalWallCollidesWith = COLLIDE_PADDLE | COLLIDE_PUCK; 
int middleWallCollidesWith = COLLIDE_PADDLE; 
int groundCollidesWith = COLLIDE_PADDLE | COLLIDE_PUCK;
int topCollidesWith = COLLIDE_PADDLE | COLLIDE_PUCK;
int goalForPuck = COLLIDE_PADDLE;  

// Sound variables
Mix_Chunk *goalPtr = NULL; 

// Global Variables 
int windowWidth = 1080;
int windowHeight = 1080;
GLuint program; // The GLSL program handle
GLuint vbo_geometry[5]; // Array for VBOs (for our geometry) - WILL CHANGE
GLuint textures[12]; // Array of textures (for our geometry)
int themeOption = 0;   

// Uniform locations
GLint loc_mvpmat; // Location of the mvp matrix in the shader

// Attribute locations - Locations of variables in the shaders
GLint loc_position;
GLint loc_uv; 

// Transform matrices
glm::mat4 model[4]; // obj->world each object should have its own model matrix
glm::mat4 view; // world->eye
glm::mat4 projection; // eye->clip
glm::mat4 mvp; // premultiplied modelviewprojection matrix for vertex shader

// GLUT callbacks
void render( void );
void update( void );
void reshape( int n_w, 
              int n_h);
void keyboard( unsigned char key, 
               int x_pos, 
               int y_pos);
void mouse( int button, 
            int state, 
            int x, 
            int y ); 
void menu( int selection );
void changeThemeMenu( int selection );
void changePaddleMenu( int selection );  
void passive( int x, 
              int y );
void specialKeys( int button, int x, int y );
int mouseX = 0;
int mouseY = 0;  
bool pauseGame = false;
int p1Score = 0;
int p2Score = 0;

// Camera globals
float zoom = 40;
float rotAng = -M_PI/2;
float htAng = 21.2;
int viewMode = 0;

// Used to display winning message 
int counter = 0;

// Resource management
bool initialize( void );
void cleanUp( void );

// Random time things
float getDT( void );
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

// Assimp components
const int NUM_OBJECTS = 4; // Board, puck, and two paddles 
int numberOfBoardVertices = 0;
int numberOfCirclePuckVertices = 0;
int numberOfTriangularPuckVertices = 0; 
int numberOfCirclePaddleVertices = 0; 
int numberOfSquarePaddleVertices = 0;
int numberOfRectangularPaddleVertices = 0;
int currentPuck = 0;
int currentPaddle = 0;  
Vertex *objLoader( const char *imageFile,
                   int &numVertices );  
Magick::Blob m_blob[12]; 

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

btAlignedObjectArray<btCollisionShape*> collisionShapes: Helps keep track of
rigid bodies. 
*/
btBroadphaseInterface *broadphase = NULL; 
btDefaultCollisionConfiguration *collisionConfiguration = NULL; 
btCollisionDispatcher *dispatcher = NULL; 
btSequentialImpulseConstraintSolver *solver = NULL; 
btDiscreteDynamicsWorld *dynamicsWorld = NULL; 
btRigidBody *groundRigidBody = NULL; 

// Will be used to tell OpenGL what to render and update 
btRigidBody *puckRigidBodyArray[1];
btRigidBody *paddleRigidBodyArray[6]; 

//--Main
int main(int argc, char **argv)
  {
  // Initialize Glut/ImageMagick
  Magick::InitializeMagick( *argv ); 
  glutInit( &argc, argv );
  glutInitDisplayMode( GLUT_DOUBLE | // Double buffering 
                       GLUT_DEPTH ); // Depth testing
  glutInitWindowSize( windowWidth, windowHeight );

  // Name and create the Window
  glutCreateWindow("Assignment 09 - Air Hockey");

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
  glutMouseFunc( mouse ); // Called if there is mouse input 
  glutMotionFunc( passive );
  glutSpecialFunc( specialKeys );

  // Menu
  int subMenuTheme = glutCreateMenu( changeThemeMenu );
  glutAddMenuEntry( "Classic", 3 );
  glutAddMenuEntry( "Neon", 4 );
  glutAddMenuEntry( "Ocean", 5 ); 
  int subMenuPaddle = glutCreateMenu( changePaddleMenu );
  glutAddMenuEntry( "Make paddle circular", 6 );
  glutAddMenuEntry( "Make paddle rectangular", 7 );
  glutAddMenuEntry( "Make paddle a square", 8 ); 
  glutCreateMenu( menu );
  glutAddMenuEntry( "Continue Game", 1 );
  glutAddMenuEntry( "Pause Game", 2 );
  glutAddSubMenu( "Change Theme", subMenuTheme );
  glutAddSubMenu( "Change Paddle Shape", subMenuPaddle );
  glutAddMenuEntry( "Quit Game", 9 );  
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

// Supporting Function Implementations 
void render( void )
  {
  // Initialize function/variables 
  int numberOfVertices = 0; 
  glClearColor( 0.0, 0.0, 0.0, 1.0 ); // Black
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glUseProgram(0);
    char str[256];
    int k;
    sprintf( str, "Player 1:  %d", p2Score);
    glColor4f(1,1,1, 1.0f);
    glRasterPos3f( -.9, 0.8, 0.0 );
    for( k = 0; str[k] != '\0'; k++ )
    {
        glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, str[k]);
    }
    sprintf( str, "Player 2:  %d", p1Score);
    glColor4f(1,1,1, 1.0f);
    glRasterPos3f( .7, 0.8, 0.0 );
    for( k = 0; str[k] != '\0'; k++ )
    {
        glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, str[k]);
    }

    if( (p1Score == 7 || p2Score == 7) && counter <= 4000 )
      {
      if( counter == 1 )
        {
        goalPtr = Mix_LoadWAV( "end.wav" );
        Mix_PlayChannel( -1, goalPtr, 0 ); 
        }
      viewMode = 0;
      view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                  glm::vec3(0.0, 0.0, 0.0), // Focus point
                  glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
        pauseGame = true;
        //p1Score = 0;
        //p2Score = 0; 
        if( p1Score == 1 )
        {
          sprintf( str, "Congrats Player 1!");
        }
        else
        {
          sprintf( str, "Congrats Player 2!");
        }
        glColor4f(1,1,1, 1.0f);
        glRasterPos3f( -0.2, 0.5, 0.0 );
        for( k = 0; str[k] != '\0'; k++ )
        {
            glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, str[k]);
        };

        sprintf( str, "Press SPACE to begin a new game");
        glColor4f(1,1,1, 1.0f);
        glRasterPos3f( -0.35, 0.3, 0.0 );
        for( k = 0; str[k] != '\0'; k++ )
        {
            glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, str[k]);
        };

        if( counter < 4000 )
        {
          counter++;
        }
        else
        {
          pauseGame = false;
          counter = 0;
          p1Score = 0;
          p2Score = 0;
        }
      }

  glUseProgram( program );
  glActiveTexture( GL_TEXTURE0 );

  // Render all objects for simulation 
  for( int i = 0; i < NUM_OBJECTS; i++ )
    {
    // Premultiply the mvp matrix and send to shader 
    mvp = projection * view * model[i];
    glUniformMatrix4fv( loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp) );

    // Determine amount of vertices to render and bind correct buffer 
    switch( i )
      {
      case 0:
        numberOfVertices = numberOfBoardVertices;
        glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[0] ); 
        break;

      case 1:
        // Determine how many vertices to render for puck
        // This depends on which puck the user wants
        numberOfVertices = numberOfCirclePuckVertices;
        glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[1] ); 
        break; 

      case 2:
      case 3:
        // Determine how many vertices to render for paddle
        // This depends on which paddle the user wants
        switch( currentPaddle )
          {
          case 0: 
            numberOfVertices = numberOfCirclePaddleVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[2] );
            break; 

          case 2: 
            numberOfVertices = numberOfRectangularPaddleVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[3] );
            break;

          case 4: 
            numberOfVertices = numberOfSquarePaddleVertices;
            glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[4] );
            break; 
          } 
        break;
      }

    // Prepare textures for render 
    glBindTexture( GL_TEXTURE_2D, textures[i + themeOption] ); 

    // Determine locations of variables in the shaders 
    glEnableVertexAttribArray( loc_position );
    glEnableVertexAttribArray( loc_uv );    

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
                           (void*)offsetof( Vertex,uv ) );
       
    glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);//mode, starting index, count
    } 

  // Clean up
  glDisableVertexAttribArray(loc_position);
  glDisableVertexAttribArray(loc_uv);
                           
  // Swap the buffers
  glutSwapBuffers();
  }

void update( void )
  {
  // Initialize function/variables
  btTransform trans;
  btScalar m[16]; 
  //static float angle = 0.0;
  float dt = getDT(); // if you have anything moving, use dt 

  // Check if we need to pause the game
  if( pauseGame == true )
    {
    dt = 0; 
    }

  // Update physics world
  dynamicsWorld->stepSimulation( dt, 5 ); 
  //dynamicsWorld->stepSimulation( 1.0f/60.0f, 10 ); 

  // Rigid body for ground 
  groundRigidBody->getMotionState()->getWorldTransform(trans);
  trans.getOpenGLMatrix(m); 
  model[0] = glm::make_mat4(m);

  // Rigid body for puck
  btRigidBody **puck = &puckRigidBodyArray[currentPuck];
  btVector3 puckPosition = (*puck)->getCenterOfMassPosition();
  if( Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048 ) )
    {
    cout << "Error with audio file" << endl; 
    } 

  if( puckPosition.x() > 15.5 )
    {
    switch( themeOption )
      {
      case 0:
        goalPtr = Mix_LoadWAV( "classic.wav" );
        break; 

      case 4:
        goalPtr = Mix_LoadWAV( "neon.wav" );
        break; 

      case 8:
        goalPtr = Mix_LoadWAV( "ocean.wav" );
        break; 
      }

    Mix_PlayChannel( -1, goalPtr, 0 ); 
    p1Score++; 
    btDefaultMotionState *shapeMotionState = NULL; 
    shapeMotionState = new btDefaultMotionState( btTransform( 
                                                 btQuaternion( 0, 0, 0, 1 ),
                                                 btVector3( 0, 1, 0 ) ) );
    (*puck)->setMotionState( shapeMotionState );
    (*puck)->setLinearVelocity( btVector3( 0, -1, 0 ) );
    }

  else if( puckPosition.x() < -15.5 )
    {
    switch( themeOption )
      {
      case 0: 
        goalPtr = Mix_LoadWAV( "classic.wav" );
        break; 

      case 4:
        goalPtr = Mix_LoadWAV( "neon.wav" );
        break; 

      case 8:
        goalPtr = Mix_LoadWAV( "ocean.wav" );
        break; 
      }

    Mix_PlayChannel( -1, goalPtr, 0 ); 
    p2Score++; 
    btDefaultMotionState *shapeMotionState = NULL; 
    shapeMotionState = new btDefaultMotionState( btTransform( 
                                                 btQuaternion( 0, 0, 0, 1 ),
                                                 btVector3( 0, 1, 0 ) ) );
    (*puck)->setMotionState( shapeMotionState );
    (*puck)->setLinearVelocity( btVector3( 0, -1, 0 ) );
    }

  (*puck)->getMotionState()->getWorldTransform(trans);
  trans.getOpenGLMatrix(m); 
  model[1] = glm::make_mat4(m);

  // Rigid body for west paddle
  btRigidBody **westPaddle = &paddleRigidBodyArray[currentPaddle];
  (*westPaddle)->getMotionState()->getWorldTransform(trans);
  if( p1Score == 7 || p2Score == 7 )
    {
    btDefaultMotionState *shapeMotionState = NULL; 
    shapeMotionState = new btDefaultMotionState( btTransform( 
                                                 btQuaternion( 0, 0, 0, 1 ),
                                                 btVector3( 5, 1, 0 ) ) );
    (*westPaddle)->setMotionState( shapeMotionState );
    (*westPaddle)->setLinearVelocity( btVector3( 0, -1, 0 ) );   
    } 
  trans.getOpenGLMatrix(m); 
  model[2] = glm::make_mat4(m); 

  // Rigid body for east paddle 
  btRigidBody **eastPaddle = &paddleRigidBodyArray[currentPaddle + 1]; 
  (*eastPaddle)->getMotionState()->getWorldTransform(trans);
  if( p1Score == 7 || p2Score == 7 )
    {
    btDefaultMotionState *shapeMotionState = NULL; 
    shapeMotionState = new btDefaultMotionState( btTransform( 
                                                 btQuaternion( 0, 0, 0, 1 ),
                                                 btVector3( -5, 1, 0 ) ) );
    (*eastPaddle)->setMotionState( shapeMotionState );
    (*eastPaddle)->setLinearVelocity( btVector3( -0, -1, 0 ) );   
    } 
  trans.getOpenGLMatrix(m); 
  model[3] = glm::make_mat4(m);

  // Update the state of the scene (display callback)
  glutPostRedisplay();
  }

void reshape( int n_w, 
              int n_h )
  {
  // Initialize function/variables 
  windowWidth = n_w;
  windowHeight = n_h;

  // Change the viewport to be correct
  glViewport( 0, 0, windowWidth, windowHeight );

  // Update the projection matrix as well
  projection = glm::perspective( 45.0f, float(windowWidth)/float(windowHeight), 
                                 0.01f, 100.0f );
  }

void specialKeys( int button, int x, int y )
{
//cout << "Rot: " << rotAng << " Ht: " << htAng << " Zoom: " << zoom << endl;
  if( pauseGame )
  {
    switch( button )
    {
      case GLUT_KEY_UP:
        htAng += 0.2;
        break;
      case GLUT_KEY_DOWN:
        htAng -= 0.2;
        break;
      case GLUT_KEY_LEFT:
        rotAng += 0.2;
        break;
      case GLUT_KEY_RIGHT:
        rotAng -= 0.2;
        break;
    }
    if( viewMode == 0)
    {
    view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up" 
    }

  }
}

void passive( int x, 
              int y )
  {
  // Initialize function/variables
  paddleRigidBodyArray[currentPaddle + 1]->activate( ACTIVE_TAG );
  btVector3 tempVec;
  tempVec = paddleRigidBodyArray[currentPaddle + 1]->getLinearVelocity();

  switch( viewMode )
  {
    case 0:
      view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                      glm::vec3(0.0, 0.0, 0.0), // Focus point
                      glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
      break;
    case 1:
      view = glm::lookAt( glm::vec3(0, 40, 0), // Eye position
                      glm::vec3(0.0, 0.0, 0.0), // Focus point
                      glm::vec3(0.0, 0.0, 1.0) ); // Positive Y is "up"
      break;

    case 2:
      view = glm::lookAt( glm::vec3(0.1, 40, 0.1), // Eye position
                      glm::vec3(0.0, 0.0, 0.0), // Focus point
                      glm::vec3(1.0, 0.0, 0.0) ); // Positive Y is "up"
      break;

  }
      
  // Determine movement
  if( mouseX - x < 0 )
    {
    // Vertical view mode 
    if( viewMode == 2 )
      {
      tempVec[2] = btScalar( 20.0 );
      }

    // Horizontal view mode   
    else
      {
      tempVec[0] = btScalar( -20.0 );
      } 

    paddleRigidBodyArray[currentPaddle + 1]->setLinearVelocity( tempVec ); 
    }

  else if( mouseX - x > 0 )
    {
    // Vertical view mode 
    if( viewMode == 2 )
      {
      tempVec[2] = btScalar( -20.0 );
      }

    // Horizontal view mode        
    else
      {
      tempVec[0] = btScalar( 20.0 );
      }

    paddleRigidBodyArray[currentPaddle + 1]->setLinearVelocity( tempVec );
    } 

  if( mouseY - y < 0 )
    {
    // Vertical view mode 
    if( viewMode == 2 )
      {
      tempVec[0] = btScalar( -20.0 );
      }
        
    // Horizontal view mode        
    else
      {
      tempVec[2] = btScalar( -20.0 );
      }
    paddleRigidBodyArray[currentPaddle + 1]->setLinearVelocity( tempVec ); 
    }

  else if( mouseY - y > 0 )
    {
    // Vertical view mode 
    if( viewMode == 2 )
      {
      tempVec[0] = btScalar( 20.0 );
      }
        
    // Horizontal view mode        
    else
      {
      tempVec[2] = btScalar( 20.0 );
      }
    paddleRigidBodyArray[currentPaddle + 1]->setLinearVelocity( tempVec ); 
    } 

  // Update mouse positions
  mouseX = x;
  mouseY = y; 
  }

void keyboard( unsigned char key, 
               int x_pos, 
               int y_pos )
  {
  // Initialize function/variables
  btVector3 tempVec;
  tempVec = paddleRigidBodyArray[currentPaddle]->getLinearVelocity();

  //if( !pauseGame )
  {
    switch( viewMode )
    {
      case 0:
        view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
        break;

      case 1:
        view = glm::lookAt( glm::vec3(0, 40, 0), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 0.0, 1.0) ); // Positive Y is "up"
        break;

      case 2:
        view = glm::lookAt( glm::vec3(0.1, 40, 0.1), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(1.0, 0.0, 0.0) ); // Positive Y is "up"
        break;



    }
  }

  // Handle keyboard input
  switch( key )
    {
    // ESC
    case 27: 
      exit( 0 );
      break;

    case 'w':
    case 'W':
      paddleRigidBodyArray[currentPaddle]->activate( ACTIVE_TAG );
      // Vertical view mode 
      if( viewMode == 2 )
        {
        tempVec[0] = btScalar( 20.0 );
        }

      // Horizontal view mode   
      else
        {
        tempVec[2] = btScalar( 20.0 );
        }
      paddleRigidBodyArray[currentPaddle]->setLinearVelocity( tempVec );
      break;

    case 'a':
    case 'A':
      paddleRigidBodyArray[currentPaddle]->activate( ACTIVE_TAG );
      // Vertical view mode 
      if( viewMode == 2 )
        {
        tempVec[2] = btScalar( -20.0 );
        }

      // Horizontal view mode        
      else
        {
        tempVec[0] = btScalar( 20.0 );
        }
      paddleRigidBodyArray[currentPaddle]->setLinearVelocity( tempVec ); 
      break;

    case 's':
    case 'S':
      paddleRigidBodyArray[currentPaddle]->activate( ACTIVE_TAG );
      // Vertical view mode 
      if( viewMode == 2 )
        {
        tempVec[0] = btScalar( -20.0 );
        }
        
      // Horizontal view mode        
      else
        {
        tempVec[2] = btScalar( -20.0 );
        }
      paddleRigidBodyArray[currentPaddle]->setLinearVelocity( tempVec ); 
      break;

    case 'd':
    case 'D':
      paddleRigidBodyArray[currentPaddle]->activate( ACTIVE_TAG );
      // Vertical view mode 
      if( viewMode == 2 )
        {
        tempVec[2] = btScalar( 20.0 );
        }
        
      // Horizontal view mode        
      else
        {
        tempVec[0] = btScalar( -20.0 );
        }
      paddleRigidBodyArray[currentPaddle]->setLinearVelocity( tempVec ); 
      break;
 
    case '1':
        view = glm::lookAt( glm::vec3(0, 40, 0), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 0.0, 1.0) ); // Positive Y is "up"
        viewMode = 1;
      break;

    case '2':
        view = glm::lookAt( glm::vec3(0.1, 40, 0.1), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(1.0, 0.0, 0.0) ); // Positive Y is "up"
        viewMode = 2;
      break;   

    case '3':
      zoom = 40;
      rotAng = -M_PI/2;
      htAng = 21.2;
      //pauseGame = true;
      viewMode = 0;
      view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                  glm::vec3(0.0, 0.0, 0.0), // Focus point
                  glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
     break;

    case ' ':
      if( pauseGame )
      {
        if( viewMode == 0 )
        {
          counter = 4000;
          zoom = 40;
          rotAng = -M_PI/2;
          htAng = 21.2;
          view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                      glm::vec3(0.0, 0.0, 0.0), // Focus point
                      glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
        }
     }
     pauseGame = !pauseGame;
      break;    
    }
  }

void mouse( int button, 
            int state, 
            int x, 
            int y )
  {
  
  if( pauseGame && viewMode == 0 )
  {
    // Camera
    if( button == 3 && zoom < 50  )
    {
      zoom += 0.5;
    }

    if( button == 4 && zoom > 1.5 )
    {
      zoom -= 0.5;
    }

    view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
  }

  }

void menu( int selection )
  {
  switch( selection )
    {
    // Continue the game
    case 1:
      pauseGame = false; 
      break;

    // Pause the game
    case 2:
      pauseGame = true;
      if( viewMode == 0 )
      {
        zoom = 40;
        rotAng = -M_PI/2;
        htAng = 21.2;
        view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                    glm::vec3(0.0, 0.0, 0.0), // Focus point
                    glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"
      }
      break;

    // Allow user to change which PUCK they are using 
    case 3:
      changeThemeMenu( selection );
      break;

    // Allow user to change which PADDLE they are using 
    case 4:
      cout << "MENU" << currentPaddle << " " << ((selection - 6)*2) << endl;
      if( currentPaddle != ((selection - 6) *2))
        {
          cout << "inside if" << endl;
          changePaddleMenu( selection );
        }
      break;

    // Exit the game 
    case 9:
      //currentPaddle = 0; 
      //changePaddleMenu( 6 );
      exit( 0 ); 
      break; 
    }

  // Necessary for the screen to redraw itself without the menu
  glutPostRedisplay(); 
  }

void changeThemeMenu( int selection )
  {
  switch( selection )
    {
    case 3:
      themeOption = 0;
      break; 

    case 4:
      themeOption = 4; 
      break; 

    case 5:
      themeOption = 8;
      break; 
    }
  }

void changePaddleMenu( int selection )
  {
    if( currentPaddle != ((selection - 6) *2))
        {
  btRigidBody *paddle = NULL; 
  switch( selection )
    {
    // Circular paddle
    case 6:
      //cout << currentPaddle << endl; 
      // Remove current paddle from world
      paddle = paddleRigidBodyArray[currentPaddle];
      if( paddle != NULL )
        {
        dynamicsWorld->removeRigidBody( paddle );
        // Create a rigid body and define motion state for the paddle (WEST)
        // NOTE: THIS IS FOR THE CIRCULAR PADDLE  
          {
          btCollisionShape *shape = new btSphereShape( btScalar( 1.0f ) );
          btDefaultMotionState *shapeMotionState = NULL;
          shapeMotionState = new btDefaultMotionState( btTransform( 
                                                       btQuaternion( 0, 0, 0, 1 ),
                                                       btVector3( 5, 0, 0 ) ) );
          btScalar mass( 4.0f ); 
          btVector3 inertia( 0, 0, 0 );
          shape->calculateLocalInertia( mass, inertia ); 
          btRigidBody::btRigidBodyConstructionInfo
          shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
          btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
          shapeRigidBody->setDamping( 0.5, 5.0 );   
          shapeRigidBody->setRestitution( 0.5 );
          shapeRigidBody->setFriction( btScalar( 0.0 ) );
          dynamicsWorld->addRigidBody( shapeRigidBody,
                                       COLLIDE_PADDLE, 
                                       paddleCollidesWith );
          paddleRigidBodyArray[0] = shapeRigidBody; // INDEX: 0        
          }
        }
      else
        {
        cout << "Seg. Fault" << endl;
        exit( EXIT_FAILURE ); 
        }

      paddle = paddleRigidBodyArray[currentPaddle + 1];
      if( paddle != NULL )
        {
        dynamicsWorld->removeRigidBody( paddle );
        // Create a rigid body and define motion state for the paddle (EAST)
        // NOTE: THIS IS FOR THE CIRCULAR PADDLE  
          {
          btCollisionShape *shape = new btSphereShape( btScalar( 1.0f ) );
          btDefaultMotionState *shapeMotionState = NULL;
          shapeMotionState = new btDefaultMotionState( btTransform( 
                                                       btQuaternion( 0, 0, 0, 1 ),
                                                       btVector3( -5, 0, 0 ) ) );
          btScalar mass( 4.0f ); 
          btVector3 inertia( 0, 0, 0 );
          shape->calculateLocalInertia( mass, inertia ); 
          btRigidBody::btRigidBodyConstructionInfo
          shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
          btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );  
          shapeRigidBody->setRestitution( 0.5 );
          shapeRigidBody->setFriction( btScalar( 0.0 ) );
          shapeRigidBody->setDamping( 0.5, 5.0 ); // DAMPING?
          dynamicsWorld->addRigidBody( shapeRigidBody,
                                       COLLIDE_PADDLE, 
                                       paddleCollidesWith );
          paddleRigidBodyArray[1] = shapeRigidBody; // INDEX: 1   
          }
        }
      else
        {
        cout << "Seg. Fault" << endl;
        exit( EXIT_FAILURE ); 
        } 
      currentPaddle = 0;  
      break; 

    // Square paddle
    case 7:
    case 8:
      btCollisionShape *shape = NULL; 
      // Remove current paddle from world
      paddle = paddleRigidBodyArray[currentPaddle];
      if( paddle != NULL )
        {
        dynamicsWorld->removeRigidBody( paddle );
        // Create a rigid body and define motion state for the paddle (WEST)
        // NOTE: THIS IS FOR THE SQUARE PADDLE  
          
          if( selection == 7 )
            {
            shape = new btBoxShape( btVector3( 1, 1, 1.5 ) );
            }
          else if( selection == 8 )
            {
            shape = new btBoxShape( btVector3( 1, 1, 1 ) ); 
            }
          btDefaultMotionState *shapeMotionState = NULL;
          shapeMotionState = new btDefaultMotionState( btTransform( 
                                                       btQuaternion( 0, 0, 0, 1 ),
                                                       btVector3( 5, 0, 0 ) ) );
          btScalar mass( 4.0f ); 
          btVector3 inertia( 0, 0, 0 );
          shape->calculateLocalInertia( mass, inertia ); 
          btRigidBody::btRigidBodyConstructionInfo
          shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
          btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );  
          shapeRigidBody->setRestitution( 0.5 );
          shapeRigidBody->setFriction( btScalar( 0.0 ) );
          shapeRigidBody->setDamping( 0.5, 1.0 ); // DAMPING?
          dynamicsWorld->addRigidBody( shapeRigidBody,
                                       COLLIDE_PADDLE, 
                                       paddleCollidesWith );
          if( selection == 7 )
            {
            paddleRigidBodyArray[2] = shapeRigidBody; // INDEX: 4
            }
          else if( selection == 8 )
            {
            paddleRigidBodyArray[4] = shapeRigidBody; // INDEX: 4
            }                                        
          
          paddleRigidBodyArray[currentPaddle] = NULL;
        }
      else
        {
        cout << "Seg. Fault" << endl;
        exit( EXIT_FAILURE ); 
        }

      paddle = paddleRigidBodyArray[currentPaddle + 1];
      if( paddle != NULL )
        {
        dynamicsWorld->removeRigidBody( paddle );
        // Create a rigid body and define motion state for the paddle (EAST)
        // NOTE: THIS IS FOR THE SQUARE PADDLE  
        
        if( selection == 7 )
          {
          shape = new btBoxShape( btVector3( 1, 1, 1.5 ) );
          }
        else if( selection == 8 )
          {
          shape = new btBoxShape( btVector3( 1, 1, 1 ) ); 
          }
        btDefaultMotionState *shapeMotionState = NULL;
        shapeMotionState = new btDefaultMotionState( btTransform( 
                                                     btQuaternion( 0, 0, 0, 1 ),
                                                     btVector3( -5, 0, 0 ) ) );
        btScalar mass( 4.0f ); 
        btVector3 inertia( 0, 0, 0 );
        shape->calculateLocalInertia( mass, inertia ); 
        btRigidBody::btRigidBodyConstructionInfo
        shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
        btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );  
        shapeRigidBody->setRestitution( 0.5 );
        shapeRigidBody->setFriction( btScalar( 0.0 ) );
        shapeRigidBody->setDamping( 0.5, 1.0 ); // DAMPING?
        dynamicsWorld->addRigidBody( shapeRigidBody,
                                     COLLIDE_PADDLE, 
                                     paddleCollidesWith ); 
           paddleRigidBodyArray[currentPaddle+1] = NULL;
          if( selection == 7 )
            {
            paddleRigidBodyArray[3] = shapeRigidBody; // INDEX: 4
            currentPaddle = 2; 
            }
          else if( selection == 8 )
            {
            paddleRigidBodyArray[5] = shapeRigidBody; // INDEX: 4
            currentPaddle = 4; 
            }    
        }
        break;
      }  
    }  
  }

bool initialize( void )
  {
  // Initialize geometry - air hockey table
  Vertex *boardGeometry = objLoader( "airtable43.obj", 
                                     numberOfBoardVertices );

  // Initialize gemoetry - hockey pucks 
  Vertex *circlePuckGeometry = objLoader( "hockeypuck.obj", 
                                           numberOfCirclePuckVertices ); 

  // Initialize geometry - hockey paddles 
  Vertex *circlePaddleGeometry = objLoader( "hockeycirclepaddle.obj", 
                                            numberOfCirclePaddleVertices );
  Vertex *squarePaddleGeometry = objLoader( "hockeysquarepaddle.obj", 
                                            numberOfSquarePaddleVertices );
  Vertex *rectangularPaddleGeometry = objLoader( "hockeyrectanglepaddle.obj", 
                                                 numberOfRectangularPaddleVertices );                                 
  
  // Initialize images - THEME 1 - CLASSIC THEME
  Magick::Image *boardImage = new Magick::Image( "classic.png" );
  boardImage->write( &m_blob[0], "RGBA" ); 
  Magick::Image *puckImage = new Magick::Image( "dark.jpg" );
  puckImage->write( &m_blob[1], "RGBA" ); 
  Magick::Image *paddleOneImage = new Magick::Image( "red.jpeg" );
  paddleOneImage->write( &m_blob[2], "RGBA" );   
  Magick::Image *paddleTwoImage = new Magick::Image( "green.jpeg" );
  paddleTwoImage->write( &m_blob[3], "RGBA" );

  // Initialize images - THEME 2 - NEON THEME
  Magick::Image *boardImageTwo = new Magick::Image( "neon1.png" );
  boardImageTwo->write( &m_blob[4], "RGBA" ); 
  Magick::Image *puckImageTwo = new Magick::Image( "neon_puck.jpg" );
  puckImageTwo->write( &m_blob[5], "RGBA" ); 
  Magick::Image *paddleOneImageTwo = new Magick::Image( "neon_paddle1.jpg" );
  paddleOneImageTwo->write( &m_blob[6], "RGBA" );   
  Magick::Image *paddleTwoImageTwo = new Magick::Image( "neon_paddle2.jpg" );
  paddleTwoImageTwo->write( &m_blob[7], "RGBA" );

  // Initialize images - THEME 3 - OCEAN THEME 
  Magick::Image *boardImageThree = new Magick::Image( "ocean.png" );
  boardImageThree->write( &m_blob[8], "RGBA" ); 
  Magick::Image *puckImageThree = new Magick::Image( "ocean_puck.jpg" );
  puckImageThree->write( &m_blob[9], "RGBA" ); 
  Magick::Image *paddleOneImageThree = new Magick::Image( "ocean_paddle1.jpg" );
  paddleOneImageThree->write( &m_blob[10], "RGBA" );   
  Magick::Image *paddleTwoImageThree = new Magick::Image( "ocean_paddle2.jpg" );
  paddleTwoImageThree->write( &m_blob[11], "RGBA" );

  // Initialize bullet
  broadphase = new btDbvtBroadphase();
  collisionConfiguration = new btDefaultCollisionConfiguration(); 
  dispatcher = new btCollisionDispatcher( collisionConfiguration );
  solver = new btSequentialImpulseConstraintSolver; 
  dynamicsWorld = new btDiscreteDynamicsWorld( dispatcher, broadphase,
                                               solver, collisionConfiguration ); 
  dynamicsWorld->setGravity( btVector3( 0, -9.81, 0 ) );

  // Create a rigid body for the ground
  {
  btCollisionShape *shape = new btStaticPlaneShape( btVector3( 0, 1, 0 ), 
                                                    -1 );
  //btCollisionShape *shape = new btBoxShape( btVector3( 16.0, 1.0, 16.0 ) );
  btDefaultMotionState *shapeMotionState = NULL; 
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 0, -1, 0) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  groundRigidBody = new btRigidBody( shapeRigidBodyCI );
  groundRigidBody->setFriction( 1.0 );
  dynamicsWorld->addRigidBody( groundRigidBody,
                               COLLIDE_GROUND, 
                               groundCollidesWith ); 
  }

  // Create a rigid body and define motion state for TOP of the table
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 16.0, 1.0, 16.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 0, 3, 0) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_TOP, 
                               topCollidesWith );       
  } 

  // Create a rigid body and define motion state for MIDDLE of the table
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 0.5, 10.0, 16.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 0, 0, 0) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI ); 
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_MIDDLE,
                               middleWallCollidesWith );       
  }

  // Create a rigid body and define motion state for the upper WEST side of 
  // the table
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 1.0, 10.0, 3.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 16, 0, 6 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );  
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               goalWallCollidesWith );       
  }  

  // Create a rigid body and define motion state for the MIDDLE WEST side of 
  // the table - this is for the puck to transition through 
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 1.0, 10.0, 3.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 16, 0, 0 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );  
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               goalForPuck );       
  }  

  // Create a rigid body and define motion state for lower WEST side of 
  // the table
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 1.0, 10.0, 3.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 16, 0, -6 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );   
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               goalWallCollidesWith );       
  }

  // Create a rigid body and define motion state for the upper EAST side of 
  // the table 
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 1.0, 10.0, 3.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( -16, 0, 6 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );    
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               goalWallCollidesWith );       
  }  

  // Create a rigid body and define motion state for the MIDDLE EAST side of 
  // the table - this is for the puck to transition through 
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 1.0, 10.0, 3.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( -16, 0, 0 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );   
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               goalForPuck );       
  }  

  // Create a rigid body and define motion state for lower EAST side of 
  // the table
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 1.0, 10.0, 3.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( -16, 0, -6 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );  
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               goalWallCollidesWith );       
  }

  // Create a rigid body and define motion state for NORTH side of the table 
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 16.0, 10.0, 1.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 0, 0, 10 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI ); 
  shapeRigidBody->setRestitution( 1.0 );  
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               wallCollidesWith );     
  }

  // Create a rigid body and define motion state for SOUTH side of the table 
  {
  btCollisionShape *shape = new btBoxShape( btVector3( 16.0, 10.0, 1.0 ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 0, 0, -10 ) ) );
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( 0, shapeMotionState, shape, btVector3( 0, 0, 0 ) ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setRestitution( 1.0 );   
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_WALL,
                               wallCollidesWith );   
  }

  // Create a rigid body and define motion state for the hockey puck
  // NOTE: THIS IS FOR THE CIRCULAR PUCK  
  {
  btCollisionShape *shape = new btSphereShape( btScalar( 1.0f ) );
  btDefaultMotionState *shapeMotionState = NULL; 
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 0, 1, 0 ) ) );
  btScalar mass( 1.0f ); 
  btVector3 inertia( 0, 0, 0 );
  shape->calculateLocalInertia( mass, inertia ); 
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI ); 
  shapeRigidBody->setRestitution( 1.2 );
  shapeRigidBody->setFriction( btScalar( 0 ) );
  shapeRigidBody->setDamping( 0.5, 0.5 );   
  dynamicsWorld->addRigidBody( shapeRigidBody, 
                               COLLIDE_PUCK, 
                               puckCollidesWith );
  //puckRigidBodyArray.push_back( shapeRigidBody ); // INDEX: 0
  puckRigidBodyArray[0] = shapeRigidBody;  
  }

  // Create a rigid body and define motion state for the paddle (WEST)
  // NOTE: THIS IS FOR THE CIRCULAR PADDLE  
  {
  btCollisionShape *shape = new btSphereShape( btScalar( 1.0f ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( 5, 0, 0 ) ) );
  btScalar mass( 4.0f ); 
  btVector3 inertia( 0, 0, 0 );
  shape->calculateLocalInertia( mass, inertia ); 
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );
  shapeRigidBody->setDamping( 0.5, 5.0 );   
  shapeRigidBody->setRestitution( 0.5 );
  shapeRigidBody->setFriction( btScalar( 0.0 ) );
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_PADDLE, 
                               paddleCollidesWith );
  //paddleRigidBodyArray.push_back( shapeRigidBody ); // INDEX: 0
  paddleRigidBodyArray[0] = shapeRigidBody;         
  }

  // Create a rigid body and define motion state for the paddle (EAST)
  // NOTE: THIS IS FOR THE CIRCULAR PADDLE  
  {
  btCollisionShape *shape = new btSphereShape( btScalar( 1.0f ) );
  btDefaultMotionState *shapeMotionState = NULL;
  shapeMotionState = new btDefaultMotionState( btTransform( 
                                               btQuaternion( 0, 0, 0, 1 ),
                                               btVector3( -5, 0, 0 ) ) );
  btScalar mass( 4.0f ); 
  btVector3 inertia( 0, 0, 0 );
  shape->calculateLocalInertia( mass, inertia ); 
  btRigidBody::btRigidBodyConstructionInfo
  shapeRigidBodyCI( mass, shapeMotionState, shape, inertia ); 
  btRigidBody *shapeRigidBody = new btRigidBody( shapeRigidBodyCI );  
  shapeRigidBody->setRestitution( 0.5 );
  shapeRigidBody->setFriction( btScalar( 0.0 ) );
  shapeRigidBody->setDamping( 0.5, 5.0 ); // DAMPING?
  dynamicsWorld->addRigidBody( shapeRigidBody,
                               COLLIDE_PADDLE, 
                               paddleCollidesWith );
  //paddleRigidBodyArray.push_back( shapeRigidBody ); // INDEX: 1
  paddleRigidBodyArray[1] = shapeRigidBody;    
  }

  // Create a Vertex Buffer object to store vertex data on GPU
  glGenBuffers( 5, vbo_geometry );

  // Allocate space on GPU for air hockey table 
  glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[0] );
  glBufferData( GL_ARRAY_BUFFER, 
                sizeof(Vertex) * numberOfBoardVertices, 
                boardGeometry, 
                GL_STATIC_DRAW );

  // Allocate space on GPU for circular puck 
  glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[1] );
  glBufferData( GL_ARRAY_BUFFER, 
                sizeof(Vertex) * numberOfCirclePuckVertices, 
                circlePuckGeometry, 
                GL_STATIC_DRAW );

  // Allocate space on GPU for circular paddle 
  glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[2] );
  glBufferData( GL_ARRAY_BUFFER, 
                sizeof(Vertex) * numberOfCirclePaddleVertices, 
                circlePaddleGeometry, 
                GL_STATIC_DRAW );

  // Allocate space on GPU for rectangular paddle 
  glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[3] );
  glBufferData( GL_ARRAY_BUFFER, 
                sizeof(Vertex) * numberOfRectangularPaddleVertices, 
                rectangularPaddleGeometry, 
                GL_STATIC_DRAW );

  // Allocate space on GPU for square paddle 
  glBindBuffer( GL_ARRAY_BUFFER, vbo_geometry[4] );
  glBufferData( GL_ARRAY_BUFFER, 
                sizeof(Vertex) * numberOfSquarePaddleVertices, 
                squarePaddleGeometry, 
                GL_STATIC_DRAW );

  // Create a texture buffer object 
  glActiveTexture( GL_TEXTURE0 ); 
  glGenTextures( 12, textures );

  // Table texture - CLASSIC THEME 
  glBindTexture( GL_TEXTURE_2D, textures[0] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, boardImage->columns(), 
                boardImage->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[0].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Puck texture - CLASSIC THEME 
  glBindTexture( GL_TEXTURE_2D, textures[1] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, puckImage->columns(), 
                puckImage->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[1].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Paddle texture - CLASSIC THEME 
  glBindTexture( GL_TEXTURE_2D, textures[2] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, paddleOneImage->columns(), 
                paddleOneImage->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[2].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Paddle texture - CLASSIC THEME
  glBindTexture( GL_TEXTURE_2D, textures[3] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, paddleTwoImage->columns(), 
                paddleTwoImage->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[3].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Table texture - NEON THEME 
  glBindTexture( GL_TEXTURE_2D, textures[4] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, boardImageTwo->columns(), 
                boardImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[4].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Puck texture - NEON THEME 
  glBindTexture( GL_TEXTURE_2D, textures[5] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, puckImageTwo->columns(), 
                puckImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[5].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Paddle texture - NEON THEME 
  glBindTexture( GL_TEXTURE_2D, textures[6] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, paddleOneImageTwo->columns(), 
                paddleOneImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[6].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Paddle texture - NEON THEME
  glBindTexture( GL_TEXTURE_2D, textures[7] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, paddleTwoImageTwo->columns(), 
                paddleTwoImageTwo->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[7].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Table texture - OCEAN THEME 
  glBindTexture( GL_TEXTURE_2D, textures[8] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, boardImageThree->columns(), 
                boardImageThree->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[8].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Puck texture - OCEAN THEME 
  glBindTexture( GL_TEXTURE_2D, textures[9] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, puckImageThree->columns(), 
                puckImageThree->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[9].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Paddle texture - OCEAN THEME 
  glBindTexture( GL_TEXTURE_2D, textures[10] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, paddleOneImageThree->columns(), 
                paddleOneImageThree->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[10].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Paddle texture - OCEAN THEME
  glBindTexture( GL_TEXTURE_2D, textures[11] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, paddleTwoImageThree->columns(), 
                paddleTwoImageThree->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                m_blob[11].data() );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Initialize shader array 
  shader vAndFShaders[2];
  vAndFShaders[0].setShaderFile( "vShader.vert" );
  vAndFShaders[0].readShaderFile();
  vAndFShaders[0].setShaderType( GL_VERTEX_SHADER );
  vAndFShaders[1].setShaderFile( "fShader.frag" );
  vAndFShaders[1].readShaderFile();
  vAndFShaders[1].setShaderType( GL_FRAGMENT_SHADER );   
  program = vAndFShaders[0].compileAndLinkShaders( vAndFShaders );

  // Now we set the locations of the attributes and uniforms - this allows
  // us to access them easily while rendering
  loc_position = glGetAttribLocation(program, 
                 const_cast<const char*>( "v_position" ) );
  if(loc_position == -1)
    {
    std::cerr << "[F] POSITION NOT FOUND" << std::endl;
    return false;
    }

  loc_uv = glGetAttribLocation(program,
           const_cast<const char*>( "v_texture" ) );
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

  // Initialize the view and projection matrices
  view = glm::lookAt( glm::vec3(zoom*cos(rotAng), zoom*sin(htAng), zoom*sin(rotAng)), // Eye position
                    glm::vec3(0.0, 0.0, 0.0), // Focus point
                    glm::vec3(0.0, 1.0, 0.0) ); // Positive Y is "up"

  projection = glm::perspective( 45.0f, 
                                 float(windowWidth)/float(windowHeight),
                                 0.01f, // Distance to the near plane
                                 100.0f); // Distance to the far plane 

  // Enable depth testing
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LESS );

  // Function conclusion
  delete boardImage;
  boardImage = NULL;
  delete puckImage;
  puckImage = NULL; 
  delete paddleOneImage;
  paddleOneImage = NULL;
  delete paddleTwoImage; 
  paddleTwoImage = NULL; 
  delete []boardGeometry;
  boardGeometry = NULL; 
  delete []circlePuckGeometry;
  circlePuckGeometry = NULL; 
  delete []circlePaddleGeometry;
  circlePaddleGeometry = NULL;
  delete []rectangularPaddleGeometry;
  rectangularPaddleGeometry = NULL; 
  delete []squarePaddleGeometry;
  squarePaddleGeometry = NULL;  
  return true;
  }

void cleanUp( void )
  {
  // Clean up, Clean up
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
  delete groundRigidBody;
  groundRigidBody = NULL;
  Mix_FreeChunk( goalPtr );
  goalPtr = NULL; 
  Mix_Quit(); 
  glDeleteProgram( program );
  glDeleteBuffers( 5, vbo_geometry );
  }

float getDT( void )
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
      if( mesh->HasTextureCoords(0) )
        {
        aiVector3D uv = mesh->mTextureCoords[0][ face.mIndices[j] ]; 
        outboundVertexData->uv[0] = uv.x; 
        outboundVertexData->uv[1] = uv.y;
        outboundVertexData++;  
        }

      else
        {
        cout << "ERROR: File does not contain texture coordinates" << endl;
        exit( EXIT_FAILURE ); 
        }
      }
    }

  // Function conclusion
  outboundVertexData = home;
  return outboundVertexData; 
  }
