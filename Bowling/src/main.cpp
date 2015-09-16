/*/////////////////////////////////////////////////////////////////////////////
Group members: Renee Iinuma, Kyle Lee, and Wesley Kepke. 

Class: CS 480

Date: December 17, 2014 

This program is our recreation of Bowling, a popular game that is currently 
sweeping the nation.
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

// GLM header files. 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // Makes passing matrices to shaders easier. 
// End of Header Files ////////////////////////////////////////////////////////
 
// Global Constants/Variables/Prototypes //////////////////////////////////////

// Macros.
#define degreesToRadians(x) x*(3.141592f/180.0f) 
#define radiansToDegrees(x) x*(180.0f/3.141592f)

// "Vertex" object - used to send vertex data to shaders. 
struct Vertex
   {
   GLfloat position[3];
   GLfloat uv[2];
   GLfloat normal[3]; 
   };

// "Pin" object - used to consolidate bowling pin data. 
struct Pin
   {
   int pinNum;  
   bool pinKnockedDown; 
   glm::mat4 pinModel;
   btVector3 pinOriginalPos; 
   btRigidBody *pinRigidBody;
   btTriangleMesh* pinTriMesh;  // May only need one instance of this
   }; 

// "Light" object - used to consolidate lighting data. 
struct Light
   {
   // Light attributes.
   float lightX;
   float lightY;
   float lightZ;
   float lightA;

   // Light position and light-material interaction vectors.  
   glm::vec4 lightPosition;
   glm::vec4 lightAmbient;
   glm::vec4 lightDiffuse;
   glm::vec4 lightSpecular; 

   // Spotlight vector and attributes.
   glm::vec3 spotDirection;
   float spotX;
   float spotY;
   float spotZ;    
   float spotCutoff; 
   float spotExponent;
   };

// "score" object - used to consolidate scoring data. 
struct score
   {
   int firstScore;
   int secondScore;

   bool strike;
   bool spare;
   bool gutter;
   int frameScore;
   int pinsLeft;
   };


// Initialize lights.
const int NUM_LIGHTS = 2;   
Light lights[NUM_LIGHTS];
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
float *lPos = new float[8];
float *spotDir = new float[6];
float *spotCut = new float[2];
float *spotExp = new float[2];

/* 
"COLLISION" enumeration - for object collisions.

Bullet supports bitwise masks as a way of deciding whether or not things should
collide with other things, or receive collisions. 
*/
#define BIT(x) (1 << (x)) // Bitmask used for collisions.  
enum COLLISIONS
  {
  COLLIDE_NOTHING = 0, // Collide with nothing. 
  COLLIDE_BUMPER = BIT(1), // Collide with the bumpers (if there are bumpers).
  COLLIDE_FLOOR = BIT(2), // Collide with the floor (ball and pins). 
  COLLIDE_BALL = BIT(3), // Collide with ball (pins).
  COLLIDE_PIN = BIT(4), // Collide with pins (ball).  
  }; 

// Define what our rigid bodies can collide with.
int bowlingBallCollidesWithBumper = COLLIDE_FLOOR | COLLIDE_PIN; 
int bowlingBallCollidesWithNoBumper = COLLIDE_BUMPER | COLLIDE_FLOOR |
                                    COLLIDE_PIN;  
int bowlingPinsCollideWith = COLLIDE_FLOOR | COLLIDE_BALL | COLLIDE_PIN;
int bowlingLaneCollidesWith = COLLIDE_BALL | COLLIDE_PIN;
int wallCollidesWith = COLLIDE_BALL;     

// Sound global variables. 
Mix_Chunk *soundPtr = NULL; 

// Window variables.
int windowWidth = 1080;
int windowHeight = 1080;

// Number of objects/attributes constants.
const int NUM_PINS = 10; 
const int NUM_DISTINCT_OBJECTS = 5;  
const int BOWLING_LANE = 0;
const int BOWLING_BALL = 1;
const int BOWLING_PIN = 2;
const int POINTER = 3;
const int CURVE = 4;   
const int POWER_CONST = 15;
const int LANE_LENGTH = 47;   
Pin bowlingPins[NUM_PINS];   

// OpenGL and texture necessities. 
/*
Objects:
10 bowling pins.
1 bowling ball.
1 bowling lane. 
*/
GLuint programNoGeoShader; // The GLSL program handle without geometry shader.
GLuint programGeoShader; // The GLSL program handle with geometry shader. 
GLuint vboGeometry[NUM_DISTINCT_OBJECTS + 1]; // Array for VBOs (for our geometry).
GLuint textures[10]; // Array of textures (for our geometry).
Magick::Blob blobArr[10]; // Array of images (for our geometry). 

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
GLint locPosition;
GLint locUV;
GLint locNormal;  

// OpenGL transform matrices.
glm::mat4 model[NUM_DISTINCT_OBJECTS + 1];
glm::mat4 view; 
glm::mat4 projection; 
glm::mat4 mvp; // premultiplied modelviewprojection matrix for vertex shader.     

// Toggling options.
bool startGame = true;  
bool disableKeyboard = true;   
int playerTurn = 0;
int frameNumber = 0;
int themeOption = 0;    
bool pauseGame = false;
bool spotLightOption = false;
bool startPositionFound = false;
bool startProjectionAngleFound = false;
bool startPowerFound = false;
bool startCurveFound = false;
bool doNotPrintCurve = true;
bool bumpersOn = false;     
bool endGame = false;  

// Function prototypes/GLUT callbacks. 
void render(void);
void update(void);
void reshape(int n_w, 
               int n_h);
void keyboard(unsigned char key, 
               int x_pos, 
               int y_pos);
void menu(int selection); 
void initializeGeometry(void);
void initializeTextures(void); 
void initializeLights(void);
void initializeBullet(void);
void initializeGPU(void);
bool initializeShaders(void);      
void chooseBowlingBallAttributes(float currentTime,
                                 const btVector3 &currentBowlingBallPos,
                                 float &xzPlaneAngle,
                                 float &powerOfLaunch,
                                 float &curveAngle);  
void resetBowlingBall(const btVector3 &currentBowlingBallPos,
                        bool &needToRecordScore);
void removeBowlingPins(const btVector3 &currentBowlingBallPos,
                        int &numPinsKnockedDown,
                        float currentTime);
void resetBowlingPins(void);
void recordFirst(int pinsDown, int frame, score arr[]);
void recordSecond(int pinsDown, int frame, score arr[]);
int calculateScore(score arr[]);

// Scoring globals.
score scoreArr[3];

// Resource management.
bool initialize(void);
void cleanUp(void); 

// Random time things.
float getDT(void);
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

// Assimp components.
Vertex *bowlingLaneGeometryBumpers = NULL; 
Vertex *bowlingLaneGeometryNoBumpers = NULL; 
Vertex *bowlingBallGeometry = NULL; 
Vertex *bowlingPinGeometry = NULL;
Vertex *pointerGeometry = NULL;
Vertex *curveGeometry = NULL;      
int numberOfBowlingBallVertices = 0;
int numberOfBowlingPinVertices = 0; 
int numberOfBowlingLaneVerticesNoBumpers = 0; 
int numberOfBowlingLaneVerticesBumpers = 0; 
int numberOfCurveVertices = 0; 
int numberOfPointerVertices = 0;
int numKnockedOverPins = 0;     
int endCounter = 0;

Vertex *objLoader(const char *objFile, 
                  int &numVertices,
                  btTriangleMesh *&objTriMesh);  

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
btRigidBody *bowlingBallRigidBody = NULL; 
btRigidBody *bowlingLaneRigidBodyBumpers = NULL; 
btRigidBody *bowlingLaneRigidBodyNoBumpers = NULL; 
btRigidBody *curveRigidBody = NULL;
btRigidBody *wallLeft = NULL;
btRigidBody *wallRight = NULL;   

// Bullet triangle meshes. 
btTriangleMesh *bowlingBallTriMesh = NULL;
btTriangleMesh *bowlingPinTriMesh = NULL;  
btTriangleMesh *bowlingLaneTriMeshBumpers = NULL; 
btTriangleMesh *bowlingLaneTriMeshNoBumpers = NULL; 
btTriangleMesh *pointerTriMesh = NULL;
btTriangleMesh *curveTriMesh = NULL;     
// End of Global Constants/Variables/Prototypes ///////////////////////////////

// Main Program ///////////////////////////////////////////////////////////////
int main(int argc, char **argv)
   {
   // Initialize program.
   Magick::InitializeMagick(*argv); 
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | // Double buffering. 
                        GLUT_DEPTH ); // Depth testing.
   glutInitWindowSize(windowWidth, windowHeight); 

   // Name and create the Window
   glutCreateWindow("Final Project - Bowling");

   // Now the window is created the GL context is fully set up. 
   // Because of that we can now initialize GLEW to prepare work with shaders. 
   GLenum status = glewInit();
   if( status != GLEW_OK)
      {
      std::cerr << "[F] GLEW NOT INITIALIZED: ";
      std::cerr << glewGetErrorString(status) << std::endl;
      return -1;
      }

   // Set all of the callbacks to GLUT that we need.
   glutDisplayFunc(render); // Called when its time to display.
   glutReshapeFunc(reshape); // Called if the window is resized.
   glutIdleFunc(update); // Called if there is nothing else to do.
   glutKeyboardFunc(keyboard); // Called if there is keyboard input.

   // Main menu. 
   glutCreateMenu(menu);
   glutAddMenuEntry("Continue Game", 1);
   glutAddMenuEntry("Pause Game", 2);
   glutAddMenuEntry("Change Theme", 3);
   glutAddMenuEntry("Turn Bumpers On", 4);
   glutAddMenuEntry("Turn Bumpers Off", 5);
   glutAddMenuEntry("Restart Game", 6); 
   glutAddMenuEntry("Quit Game", 7);  
   glutAttachMenu(GLUT_RIGHT_BUTTON);   

   // Initialize all of our resources(shaders, geometry). 
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
void render(void)
   { 
   // Initialize function/variables.  
   int numberOfVertices = 0;
   int offset = 0; 
   bool needToPrintInstructions = false;   
   glClearColor(0.0, 0.0, 0.0, 1.0); // Black
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Display data for the user.
   glUseProgram(0);  
   int k;
   char temp[500];
   char score[500];

   // Determine which instructions to screen.
   if(startPositionFound == false)
      {
      sprintf(temp, "Choose starting position...");
      needToPrintInstructions = true;  
      }

   else if(startPositionFound == true &&
            startProjectionAngleFound == false)
      {
      sprintf(temp, "Choose starting projection angle...");
      needToPrintInstructions = true;               
      }

   else if(startPositionFound == true &&
            startProjectionAngleFound == true &&
            startPowerFound == false)
      {
      sprintf(temp, "Choose launch power..."); 
      needToPrintInstructions = true;              
      }

   else if(startPositionFound == true &&
            startProjectionAngleFound == true &&
            startPowerFound == true &&
            startCurveFound == false)
      {
      sprintf(temp, "Choose curvature for ball..."); 
      needToPrintInstructions = true;              
      }

   // If instructions need to be printed, print them. 
   if(needToPrintInstructions == true)
      { 
      glColor4f(1,1,1, 1.0f);
      glRasterPos3f( -.9, 0.8, 0.0 );
      for(k = 0; temp[k] != '\0'; k++)
         {
         glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, temp[k]);
         }
      }

   if(frameNumber == 2)
      {
         endGame = true;

      }
   if( endGame == false )
   {
      // Continuously print the player's score to the screen. 
      sprintf(score, "Pins this frame: %i", numKnockedOverPins);
      glColor4f(1,1,1, 1.0f);
      glRasterPos3f( 0.1, 0.8, 0.0 );
      for(k = 0; score[k] != '\0'; k++)
         {
         glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, score[k]);
         }         

      // Continuously print the player's score to the screen. 
      score[0] = '\0';
      sprintf(score, "Frame: %i", frameNumber + 1);
      glColor4f(1,1,1, 1.0f);
      glRasterPos3f( 0.1, 0.9, 0.0 );
      for(k = 0; score[k] != '\0'; k++)
         {
         glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, score[k]);
         }   
      //}
   }
   else
   {
      // Continuously print the player's score to the screen. 
      sprintf(score, "Game Over");
      glColor4f(1,1,1, 1.0f);
      glRasterPos3f( -0.15, 0.6, 0.0 );
      for(k = 0; score[k] != '\0'; k++)
         {
         glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, score[k]);
         }      
      endCounter++;
      if( endCounter > 2000 )
      {
         endCounter = 0;
         endGame = false;
         frameNumber = 0;
      }
      score[0] = '\0';
      int endScore = calculateScore(scoreArr);
         //cout << "SCORE: " << score << endl; 
      // Continuously print the player's score to the screen. 
      sprintf(score, "Score: %i", endScore);
      glColor4f(1,1,1, 1.0f);
      glRasterPos3f( -0.1, 0.45, 0.0 );
      for(k = 0; score[k] != '\0'; k++)
         {
         glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, score[k]);
         }      
      endCounter++;
      if( endCounter > 3000 )
      {
         endCounter = 0;
         endGame = false;
         frameNumber = 0;
      }
   }
   // Render all objects for simulation.
   glUseProgram(programNoGeoShader);
   glActiveTexture(GL_TEXTURE0);
   int pinNum = 0; 
   for(int i = 0; 
         i < (NUM_PINS + NUM_DISTINCT_OBJECTS); 
         i++)
      {
      // Only display pointer when user has determined start position. 
      if(startPositionFound == false && i == POINTER)
         {
         continue; 
         }

      if(doNotPrintCurve == true && i == CURVE)
         {
         continue; 
         }

      // Only display curve meter after certain ball attributes have
      // been determined.
      if(startPositionFound == true &&
         startProjectionAngleFound == true &&
         startPowerFound == true)
         {
         model[POINTER] = glm::translate(glm::mat4(1.0f),
                                          glm::vec3(50, 50, 50));
         doNotPrintCurve = false; 
         }

      // Premultiply the mvp matrix for non-pin objects and send to shader.
      if(i == BOWLING_BALL ||
         i == BOWLING_LANE ||
         i == POINTER ||
         i == CURVE)
         {
         // Determine which bowling lane to render. 
         if(i == BOWLING_LANE)
            {
            // Bowling lane with no bumpers. 
            if(bumpersOn == false)
               {
               mvp = projection * view * model[i];
               }

            // Bowling lane with bumpers. 
            else if(bumpersOn == true)
               {
               mvp = projection * view * model[NUM_DISTINCT_OBJECTS];
               }
            }

         // Otherwise, we are not currently rendering the bowling lane. 
         else
            {
            mvp = projection * view * model[i];
            pinNum = 0; 
            }
         }

      // Premultiply the mvp matrix for pins.  
      else
         {
         mvp = projection * 
               view * 
               bowlingPins[pinNum].pinModel;
         pinNum++; 
         }

      // Upload data that changed to the shaders. 
      glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
      glUniform4fv(locLightPos, 2, lPos);
      glUniform3fv(locSpotDirection, 2, spotDir);

      if(themeOption == 1)
         {
         offset = 4;
         }
      else if(themeOption == 2)
         {
         offset = 7; 
         }

      // Determine amount of vertices to render and bind correct buffer.
      // Also determine which textures to render.
      if(i == BOWLING_LANE)
         {
         // No bumper bowling lane. 
         if(bumpersOn == false)
            {
            numberOfVertices = numberOfBowlingLaneVerticesNoBumpers;
            glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[BOWLING_LANE]); 
            glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE + offset]);
            }

         // Bumper bowling lane. 
         else if(bumpersOn == true)
            {
            numberOfVertices = numberOfBowlingLaneVerticesBumpers;
            glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[NUM_DISTINCT_OBJECTS]); 
            glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE + offset]);              
            } 
         }

      else if(i == BOWLING_BALL)
         {
         numberOfVertices = numberOfBowlingBallVertices;
         glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[BOWLING_BALL]); 
         glBindTexture(GL_TEXTURE_2D, textures[BOWLING_BALL + offset]);  
         }

      else if(i == POINTER)
         {
         numberOfVertices = numberOfPointerVertices;
         glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[POINTER]); 
         glBindTexture(GL_TEXTURE_2D, textures[POINTER]);  
         }

      else if(i == CURVE)
         {
         numberOfVertices = numberOfCurveVertices;
         glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[CURVE]); 
         glBindTexture(GL_TEXTURE_2D, textures[POINTER]);  
         }         

      else
         {
         numberOfVertices = numberOfBowlingPinVertices;
         glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[BOWLING_PIN]); 
         glBindTexture(GL_TEXTURE_2D, textures[BOWLING_PIN + offset]);  
         }

      // Determine locations of variables in the shaders.
      glEnableVertexAttribArray(locPosition);
      glEnableVertexAttribArray(locUV);
      glEnableVertexAttribArray(locNormal); 

      // Describe data being sent to shaders.   
      glVertexAttribPointer(locPosition, // Location of attribute.
                              3, // Number of elements.
                              GL_FLOAT, // Data type.
                              GL_FALSE, // Normalized?
                              sizeof(Vertex), // Stride.
                              0); // Offset.

      glVertexAttribPointer(locUV,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (void*)offsetof( Vertex, uv));
       
      glVertexAttribPointer(locNormal,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (void*)offsetof(Vertex, normal));

      glDrawArrays(GL_TRIANGLES, 0, numberOfVertices);//mode, starting index, count
      } 

   // Clean up.
   glDisableVertexAttribArray(locPosition);
   glDisableVertexAttribArray(locUV);
   glDisableVertexAttribArray(locNormal); 

   // Swap the buffers. 
   glutSwapBuffers();
   }

void update(void)
   {
   // Initialize function/variables.
   btTransform trans;
   btScalar m[16]; 
   float dt = getDT(); // if you have anything moving, use dt.
   btVector3 currentPos;
   static float projectionAngle = 0.0; 
   static float powerValue = 0.0;
   static float curveValue = 0.0; 
   static int waitToResetTimer = 0;
   bool needToRecordScore = false;

   // If game is paused, do not allow anything to move. 
   if(pauseGame == true)
      {
      return; 
      }

   // Update the physics world.
   dynamicsWorld->stepSimulation(dt, 10);

   // Rigid body for the bowling lane.
   if(bumpersOn == true)
      {
      bowlingLaneRigidBodyBumpers->getMotionState()->getWorldTransform(trans);
      trans.getOpenGLMatrix(m);
      model[NUM_DISTINCT_OBJECTS] = glm::make_mat4(m);
      }
   else if(bumpersOn == false)
      {
      bowlingLaneRigidBodyNoBumpers->getMotionState()->getWorldTransform(trans);
      trans.getOpenGLMatrix(m);
      model[BOWLING_LANE] = glm::make_mat4(m);
      }

   // Rigid body for the bowling ball.
   currentPos = bowlingBallRigidBody->getCenterOfMassPosition();
   if(currentPos.y() < -7.0 && currentPos.z() == 30)
      {
      // Once bowling ball has reached the ground, allow user to choose 
      // bowling ball attributes.
      disableKeyboard = false; 
      chooseBowlingBallAttributes(dt, currentPos, projectionAngle, 
                                    powerValue, curveValue);
      }

   // Check to see if user has selected all initial bowling ball attributes. 
   if(startPositionFound == true &&
      startProjectionAngleFound == true && 
      startPowerFound == true &&
      startCurveFound == true) 
      {
      // If so, launch bowling ball in desired direction with desired
      // power.
      disableKeyboard = true;
      doNotPrintCurve = true;  
      float theta = 90.0 + projectionAngle;
      btVector3 force(powerValue * POWER_CONST * cos(degreesToRadians(theta)), 
                        0, 
                        -powerValue * POWER_CONST * sin(degreesToRadians(theta)));
      bowlingBallRigidBody->applyCentralForce(force);  
      btVector3 angVel((powerValue/15)*cos(degreesToRadians(curveValue-90)), 
                        0, 
                        (-powerValue/15)*sin(degreesToRadians(curveValue-90)));
      bowlingBallRigidBody->setAngularVelocity( angVel );  
      }

   // Reset the bowling ball back to its original position. 
   resetBowlingBall(currentPos, needToRecordScore);
   if(needToRecordScore == true)
      {
      // If we need to record the score, then do it! 
      removeBowlingPins(currentPos, numKnockedOverPins, dt);
      if(playerTurn == 1)
         {
         // Check for strike. 
         if(numKnockedOverPins == 10)
            {
            resetBowlingPins();
            frameNumber++; 
            }
         recordFirst(numKnockedOverPins, frameNumber, scoreArr);
         }

      else if(playerTurn == 2)
         {
         //numKnockedOverPins = pinNum;
         recordSecond(numKnockedOverPins, frameNumber, scoreArr);
         }
      }

   // If player has bowled both frames, we need to reset the frames.
   //cout << "Wait to reset timer: " << waitToResetTimer << endl; 
   if(playerTurn == 2)
      {
      waitToResetTimer++;
      }  
   if((waitToResetTimer > 3000 || currentPos.z() < -LANE_LENGTH) && playerTurn == 2) 
      {
      waitToResetTimer = 0; 
      resetBowlingPins();
      cout << frameNumber << endl;
      frameNumber++;
      numKnockedOverPins = 0; 
      }

   // Obtain our model matrix for the bowling ball. 
   bowlingBallRigidBody->getMotionState()->getWorldTransform(trans);
   trans.getOpenGLMatrix(m);
   model[BOWLING_BALL] = glm::make_mat4(m);

   // Debugging output code
   /*
   cout << "X: " << currentPos.x() << endl; 
   cout << "Y: " << currentPos.y() << endl;
   cout << "Z: " << currentPos.z() << endl; */

   if(spotLightOption == true)
      {
      //lights[0].lightX = 1.0; 
      lights[0].lightA = 1.0; 
      lights[0].spotX = currentPos.x();
      lights[0].spotY = currentPos.y();
      lights[0].spotZ = currentPos.z();
      lPos[0] = lights[0].spotX;
      lPos[1] = lights[0].spotY;
      lPos[2] = lights[0].spotZ;
      lPos[3] = lights[0].lightA;
      }   

   // Rigid bodies for the bowling pins.
   for(int i = 0; i < NUM_PINS; i++)
      {
      bowlingPins[i].pinRigidBody->getMotionState()->getWorldTransform(trans);
      trans.getOpenGLMatrix(m);
      bowlingPins[i].pinModel = glm::make_mat4(m); 
      }

   // Update the state of the scene (display callback)
   glutPostRedisplay();
   }

bool initialize(void)
   {
   // Initialize geometry - bowling objects.
   initializeGeometry(); 

   // Initialize images.
   initializeTextures();  

   // Initialize bullet.
   initializeBullet(); 

   // Initialize buffers on GPU.
   initializeGPU(); 

   // Initialize lighting attributes.
   initializeLights();

   // Initialize scoring array. 
   for( int i = 0; i < 3; i++ )
      {
      scoreArr[i].firstScore = 0;
      scoreArr[i].secondScore = 0;
      scoreArr[i].strike = false;
      scoreArr[i].spare = false;
      scoreArr[i].gutter = false;
      scoreArr[i].frameScore = 0;
      scoreArr[i].pinsLeft = 0;
      }

   // Initialize and compile shaders.
   bool shadersLoaded = initializeShaders(); 
   if(shadersLoaded == false)
      {
      return false; 
      }

   // Initialize the view and projection matrices.
   view = glm::lookAt(glm::vec3(0.0, -1, 47.0),
                        glm::vec3(0.0, 0.0, 0.0), // Focus point
                        glm::vec3(0.0, 1.0, 0.0)); // Positive Y is "up"

   projection = glm::perspective(45.0f, 
                                 float(windowWidth)/float(windowHeight),
                                 0.01f, // Distance to the near plane
                                 100.0f); // Distance to the far plane 

   // Enable depth testing. 
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);  

   // Function conclusion. 
   return true;
   }

void resetBowlingBall(const btVector3 &currentBowlingBallPos,
                        bool &needToRecordScore)
   {
   // Initialize variables.
   btVector3 currentPinPos;

   // Check to see if the bowling ball has reached the end of the bowling lane.
   if(currentBowlingBallPos.z() < -LANE_LENGTH)
      {
      // Reset bowling ball to the initial position.
      btDefaultMotionState *shapeMotionState = NULL;
      shapeMotionState = new btDefaultMotionState( 
                                          btTransform( 
                                             btQuaternion(0, 0, 0, 1), 
                                             btVector3(0, 5, 30))); 
      bowlingBallRigidBody->setMotionState(shapeMotionState);
      bowlingBallRigidBody->clearForces(); 
      bowlingBallRigidBody->setAngularVelocity(btVector3(0, 0, 0));
      bowlingBallRigidBody->setLinearVelocity(btVector3(0, 0, 0));
      startPositionFound = false;
      startProjectionAngleFound = false;
      startPowerFound = false;
      startCurveFound = false; 
      disableKeyboard = true;
      needToRecordScore = true; 
      }
   else
      {
      needToRecordScore = false; 
      }
   }

void removeBowlingPins(const btVector3 &currentBowlingBallPos,
                        int &numPinsKnockedDown,
                        float currentTime)
   {
   // Initialize variables.
   btVector3 currentPinPos;

   // Only remove the bowling pins if the ball has passed a particular
   // point in the bowling lane. 
   //if(currentBowlingBallPos.z() < -LANE_LENGTH)
      //{ 
      // Determine which pins have fallen down.
      for(int i = 0; i < NUM_PINS; i++)
         {
         currentPinPos = bowlingPins[i].pinRigidBody->getCenterOfMassPosition();
         if((abs(currentPinPos.x() - bowlingPins[i].pinOriginalPos.x()) > 0.4) ||
            (abs(currentPinPos.z() - bowlingPins[i].pinOriginalPos.z()) > 0.4) )
            {
            if(bowlingPins[i].pinKnockedDown == false)
               { 
               numPinsKnockedDown++;
               bowlingPins[i].pinKnockedDown = true;
               bowlingPins[i].pinRigidBody->translate(btVector3(50, 0, 50));
               } 
            }
         }
      //} 
   }

void resetBowlingPins(void)
   {
   // Reset the number of times that the player has rolled their bowling ball.  
   playerTurn = 0;
   dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); 

   // Remove all of the pins that still happen to be on the bowling alley.
   for(int i = 0; i < NUM_PINS; i++)
      {
      if(bowlingPins[i].pinKnockedDown == false)
         {
         dynamicsWorld->removeRigidBody(bowlingPins[i].pinRigidBody); 
         }

      // Reset all pins back to their original position. 
      btCollisionShape *shape = new btCylinderShape(
                                          btVector3(0.7, 1.5, 0.7)); 
      btDefaultMotionState *shapeMotionState = NULL;
      shapeMotionState = new btDefaultMotionState( 
                                          btTransform(
                                             btQuaternion(0, 0, 0, 1), 
                                             bowlingPins[i].pinOriginalPos));
      btScalar mass(0.2);
      btVector3 inertia(0, 0, 0); 
      shape->calculateLocalInertia(mass, inertia); 
      btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(mass, 
                                                      shapeMotionState, 
                                                      shape, 
                                                      inertia);
      bowlingPins[i].pinRigidBody = new btRigidBody(shapeRigidBodyCI);
      bowlingPins[i].pinRigidBody->setRestitution(0);
      bowlingPins[i].pinRigidBody->setFriction(0.2);
      bowlingPins[i].pinRigidBody->setDamping(0.5, 0.5);
      bowlingPins[i].pinRigidBody->setActivationState(DISABLE_DEACTIVATION); 
      dynamicsWorld->addRigidBody(bowlingPins[i].pinRigidBody, 
                                    COLLIDE_PIN, 
                                    bowlingPinsCollideWith);  
      }
   }

void chooseBowlingBallAttributes(float currentTime,
                                 const btVector3 &currentBowlingBallPos,
                                 float &xzPlaneAngle,
                                 float &powerOfLaunch,
                                 float &curveAngle)
   {
   // Initialize variables. 
   static bool moveLeft = true; 
   static bool rotateClockwise = true;
   static bool scaleUp = true;
   static float powerMeter = degreesToRadians(45.0);    
   static float rotAngle = degreesToRadians(45.0);
   static float curveMeter = degreesToRadians(45.0); 
   static glm::mat4 finalYAxisRotationOfPointer; 
   float bowlingBallPositionZ = currentBowlingBallPos.z(); 
   btVector3 newBowlingBallPos = currentBowlingBallPos;
   newBowlingBallPos[1] = 0;
   newBowlingBallPos[2] = 0;

   // If game is paused, do not allow anything to move. 
   if(pauseGame == true)
      {
      return; 
      }

   // Scale the pointer so it looks bigger.
   glm::mat4 pointerScale = glm::scale(glm::mat4(1.0f),
                                       glm::vec3(3.0f));

   // Obtain translation matrix so pointer is close to bowling ball.  
   glm::mat4 pointerTrans = glm::translate(glm::mat4(1.0f),
                                             glm::vec3(newBowlingBallPos.x(),
                                             -5.0, 
                                             bowlingBallPositionZ));

   // Calculate an intermediate matrix.
   glm::mat4 intermediateMatrix = pointerTrans * pointerScale; 

   // Translate ball left to right (and back) until starting position is found.
   if(startPositionFound == false)
      {
      // Determine which way to translate the ball.
      if(newBowlingBallPos.x() < -5.0)
         {
         moveLeft = false; 
         }
      else if(newBowlingBallPos.x() > 5.0)
         { 
         moveLeft = true; 
         }

      // Move the ball left. 
      if(moveLeft == true)
         {
         newBowlingBallPos[0] = -0.01; 
         bowlingBallRigidBody->translate(newBowlingBallPos);
         }

      // Move the ball right. 
      else if(moveLeft == false)
         {
         newBowlingBallPos[0] = 0.01; 
         bowlingBallRigidBody->translate(newBowlingBallPos);
         } 
      }

   // Once starting position is found, determine angle to project ball. 
   if(startPositionFound == true && 
      startProjectionAngleFound == false)
      {
      // Determine which way to rotate our pointer.
      if(radiansToDegrees(rotAngle) > 45.0)
         { 
         rotateClockwise = false;  
         }
      else if(radiansToDegrees(rotAngle) < -45.0)
         {
         rotateClockwise = true;
         }

      // Rotate pointer counter-clockwise. 
      if(rotateClockwise == true)
         {
         rotAngle += (currentTime * M_PI/2) / 4;
         }

      // Rotate pointer clockwise. 
      else if(rotateClockwise == false)
         {
         rotAngle -= (currentTime * M_PI/2) / 4;
         }

      // Calculate rotation matrix and rotate the pointer through degrees 
      // of freedom.  
      glm::mat4 pointerRot = glm::rotate(glm::mat4(1.0f),
                                          rotAngle,
                                          glm::vec3(0.0, 1.0, 0.0));
      finalYAxisRotationOfPointer = pointerRot; 
      model[POINTER] = intermediateMatrix * pointerRot;
      xzPlaneAngle = radiansToDegrees(rotAngle);  
      }

   // Once starting position and projection angle are found, determine how hard
   // the user wants to project the bowling ball.
   if(startPositionFound == true &&
      startProjectionAngleFound == true &&
      startPowerFound == false)
      {
      // Determine how much to scale our pointer. 
      if(radiansToDegrees(powerMeter) > 45.0)
         { 
         scaleUp = false;  
         }
      else if(radiansToDegrees(powerMeter) < 15.0)
         {
         scaleUp = true;
         }

      // Increase scaling of pointer. 
      if(scaleUp == true)
         {
         powerMeter += (currentTime * M_PI/2) / 6;
         }

      // Decrease scaling of pointer. 
      else if(scaleUp == false)
         {
         powerMeter -= (currentTime * M_PI/2) / 6;
         } 

      //cout << "Power: " << radiansToDegrees(powerMeter) << endl; 

      // Calculate scaling matrix and scale the pointer through degrees 
      // of freedom. 
      glm::mat4 pointerScale = glm::scale(finalYAxisRotationOfPointer,
                                          glm::vec3(1.0, 1.0, powerMeter));
      model[POINTER] = intermediateMatrix * pointerScale;
      powerOfLaunch = radiansToDegrees(powerMeter);
      }

   // Once starting position, projection angle, and power of launch are found, 
   // determine how much of a curve to apply to the ball.
   if(startPositionFound == true &&
      startProjectionAngleFound == true &&
      startPowerFound == true &&
      startCurveFound == false)
      {
      // Determine which way to rotate our curve meter. 
      if(radiansToDegrees(curveMeter) > 45.0)
         { 
         //rotateClockwise = false;  
         }
      else if(radiansToDegrees(curveMeter) < -45.0)
         {
         //rotateClockwise = true;
         }

      // Rotate pointer counter-clockwise. 
      if(rotateClockwise == true)
         {
         curveMeter += (currentTime * M_PI/2) / 4;
         }

      // Rotate pointer clockwise. 
      else if(rotateClockwise == false)
         {
         curveMeter -= (currentTime * M_PI/2) / 4;
         }

      // Calculate rotation matrix and rotate the pointer through degrees 
      // of freedom.  
      glm::mat4 curveRot = glm::rotate(glm::mat4(1.0f),
                                          curveMeter,
                                          glm::vec3(0.0, 1.0, 0.0));
      model[CURVE] = pointerTrans * curveRot;
      curveAngle = radiansToDegrees(curveMeter); 
      }         
   }

void reshape(int n_w, 
               int n_h)
   {
   // Initialize function/variables.
   windowWidth = n_w;
   windowHeight = n_h;

   // Change the viewport to be correct.
   glViewport(0, 0, windowWidth, windowHeight);

   // Update the projection matrix as well
   projection = glm::perspective(45.0f, 
                                 float(windowWidth)/float(windowHeight), 
                                 0.01f, 
                                 100.0f );
   }

void keyboard(unsigned char key, 
               int x_pos, 
               int y_pos)
   {
   // Initialize variables.
   static int numTimesSpaceBarPressed = 0; 

   // Handle keyboard input.
   switch(key)
      {
      // ESC
      case 27: 
         exit(0);
      break;

      // Ball options.
      case ' ':
         if(disableKeyboard == false)
            {
            switch(numTimesSpaceBarPressed)
               {
               case 0:
                  startPositionFound = !startPositionFound;
                  numTimesSpaceBarPressed++;
               break;

               case 1:
                  startProjectionAngleFound = !startProjectionAngleFound;
                  numTimesSpaceBarPressed++;
               break; 

               case 2:
                  startPowerFound = !startPowerFound;
                  numTimesSpaceBarPressed++;
                  //playerTurn++;  
                 //cout << "Player Turn: " << playerTurn << endl; 
               break;

               case 3:
                  startCurveFound = !startCurveFound;
                  doNotPrintCurve = true; 
                  numTimesSpaceBarPressed = 0;
                  //numKnockedOverPins = 0; 
                  playerTurn++; 
               break;
               }
            } 
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
         view = glm::lookAt(glm::vec3(0.0, -1, 47.0),
                              glm::vec3(0.0, 0.0, 0.0), // Focus point
                              glm::vec3(0.0, 1.0, 0.0)); // Positive Y is "up"
      break; 

      // Reverse view.  
      case '2':
         view = glm::lookAt(glm::vec3(0.0, 0, -51.0),
                              glm::vec3(0.0, -2, -40.0), // Focus point
                              glm::vec3(0.0, 1.0, 0.0)); // Positive Y is "up"
      break;

      // Close-up pin view.  
      case '3':
         view = glm::lookAt(glm::vec3(0.0, 2, -10),
                              glm::vec3(0.0, -2, -40), // Focus point
                              glm::vec3(0.0, 1.0, 0.0)); // Positive Y is "up"
      break; 

      // Top-down view.  
      case '4':
         view = glm::lookAt(glm::vec3(25.0, 25.0, 47.0),
                              glm::vec3(0.0, 0.0, 0.0), // Focus point
                              glm::vec3(0.0, 1.0, 0.0)); // Positive Y is "up"
      break; 

      // Space bar pauses game. 
      case 'p':
      case 'P':
         pauseGame = !pauseGame;
      break;    
    }
  }

void menu(int selection)
   {
   // Initialize function/variables.
   btVector3 currentBumperPos;

   // Determine option. 
   switch(selection)
      {
      // Continue the game.
      case 1:
         pauseGame = false; 
      break;

      // Pause the game.
      case 2:
         pauseGame = true;
      break;

      // Change the game.
      case 3:
         themeOption++;
         if(themeOption == 3)
            {
            themeOption = 0; 
            } 
      break;

      // Turn bumpers on. 
      case 4:
         if(bumpersOn != true)
            {
            dynamicsWorld->removeRigidBody(bowlingLaneRigidBodyNoBumpers);
            btCollisionShape *shape = NULL;
            shape = new btBvhTriangleMeshShape(bowlingLaneTriMeshBumpers, true); 
            btDefaultMotionState *shapeMotionState = NULL; 
            shapeMotionState = new btDefaultMotionState( 
                                                btTransform( 
                                                   btQuaternion(0, 0, 0, 1), 
                                                   btVector3(0, 0, 0))); 
            btScalar mass(0.0);
            btVector3 inertia(0, 0, 0); 
            shape->calculateLocalInertia(mass, inertia);
            btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(mass, 
                                                            shapeMotionState, 
                                                            shape, 
                                                            inertia);
            bowlingLaneRigidBodyBumpers = new btRigidBody(shapeRigidBodyCI);
            bowlingLaneRigidBodyBumpers->setRestitution(0.4);
            bowlingLaneRigidBodyBumpers->setFriction(2.0);
            bowlingLaneRigidBodyBumpers->setDamping(0.8, 0.2);
            bowlingLaneRigidBodyBumpers->setActivationState(DISABLE_DEACTIVATION);
            dynamicsWorld->addRigidBody(bowlingLaneRigidBodyBumpers,
                                          COLLIDE_FLOOR, 
                                          bowlingLaneCollidesWith);    
            bumpersOn = true;
            } 
      break;

      // Turn bumpers off.
      case 5:
         if(bumpersOn != false)
            {
            dynamicsWorld->removeRigidBody(bowlingLaneRigidBodyBumpers);
            btCollisionShape *shape = NULL;
            shape = new btBvhTriangleMeshShape(bowlingLaneTriMeshNoBumpers, true); 
            btDefaultMotionState *shapeMotionState = NULL; 
            shapeMotionState = new btDefaultMotionState( 
                                                btTransform( 
                                                   btQuaternion(0, 0, 0, 1), 
                                                   btVector3(0, 0, 0))); 
            btScalar mass(0.0);
            btVector3 inertia(0, 0, 0); 
            shape->calculateLocalInertia(mass, inertia);
            btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(mass, 
                                                            shapeMotionState, 
                                                            shape, 
                                                            inertia);
            bowlingLaneRigidBodyNoBumpers = new btRigidBody(shapeRigidBodyCI);
            bowlingLaneRigidBodyNoBumpers->setRestitution(0.4);
            bowlingLaneRigidBodyNoBumpers->setFriction(2.0);
            bowlingLaneRigidBodyNoBumpers->setDamping(0.8, 0.2);
            bowlingLaneRigidBodyNoBumpers->setActivationState(DISABLE_DEACTIVATION);
            dynamicsWorld->addRigidBody(bowlingLaneRigidBodyNoBumpers, 
                                          COLLIDE_FLOOR, 
                                          bowlingLaneCollidesWith);
            bumpersOn = false;
            }  
      break;

      // Restart game.
      case 6:
      break;

      // Quit the game.  
      case 7:
         exit(0); 
      break; 
      }

   // Redraw screen without the menu. 
   glutPostRedisplay(); 
   }

void initializeLights(void)
   {
   // Spotlight that follows the ball.  
   lights[0].lightX = 0.0;
   lights[0].lightY = 3.0;
   lights[0].lightZ = 0.0;
   lights[0].lightA = 0.0;
   lights[0].spotX = 0; 
   lights[0].spotY = -1; 
   lights[0].spotZ = 0; 
   lights[0].lightPosition = glm::vec4(lights[0].lightX,
                                       lights[0].lightY,
                                       lights[0].lightZ,
                                       lights[0].lightA);
   lights[0].lightAmbient = glm::vec4(0.4, 0.4, 0.4, 0.0);
   lights[0].lightDiffuse = glm::vec4(0.4, 0.4, 0.4, 0.0);
   lights[0].lightSpecular = glm::vec4(0.4, 0.4, 0.4, 0.0);
   lights[0].spotDirection = glm::vec3(lights[0].spotX,
                                       lights[0].spotY, 
                                       lights[0].spotZ);
   lights[0].spotCutoff = 55.0; 
   lights[0].spotExponent = 1.0; 

   // Spotlight surrounding the pins. 
   lights[1].lightX = 0.0;
   lights[1].lightY = 3.0;
   lights[1].lightZ = 0.0;
   lights[1].lightA = 0.0;
   lights[1].spotX = bowlingPins[0].pinOriginalPos.x();
   lights[1].spotY = bowlingPins[0].pinOriginalPos.y();
   lights[1].spotZ = bowlingPins[0].pinOriginalPos.z();
   lights[1].lightPosition = glm::vec4(lights[1].lightX,
                                       lights[1].lightY,
                                       lights[1].lightZ,
                                       lights[1].lightA);
   lights[1].lightAmbient = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[1].lightDiffuse = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[1].lightSpecular = glm::vec4(0.1, 0.1, 0.1, 0.0);
   lights[1].spotDirection = glm::vec3(lights[1].spotX,
                                       lights[1].spotY, 
                                       lights[1].spotZ);
   lights[1].spotCutoff = 12.0; 
   lights[1].spotExponent = 1.0; 

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
                     lights[1].lightAmbient)
                     * materialAmbient;
   diffuseProduct = (lights[0].lightDiffuse + 
                     lights[1].lightDiffuse)
                     * materialDiffuse;
   specularProduct = (lights[0].lightSpecular + 
                        lights[1].lightSpecular)
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

   // Spotlight position array to send to shaders.
   spotDir[0] = lights[0].spotDirection.x;  
   spotDir[1] = lights[0].spotDirection.y;  
   spotDir[2] = lights[0].spotDirection.z;  
   spotDir[3] = lights[1].spotDirection.x;  
   spotDir[4] = lights[1].spotDirection.y;  
   spotDir[5] = lights[1].spotDirection.z;

   // Spotlight cutoff array.
   spotCut[0] = lights[0].spotCutoff;  
   spotCut[1] = lights[1].spotCutoff;

   // Spotlight exponent array. 
   spotExp[0] = lights[0].spotExponent;  
   spotExp[1] = lights[1].spotExponent;
   }

void initializeGeometry(void)
   {
   // Initialize geometry - bowling objects. 
   bowlingBallGeometry = objLoader("bball.obj", 
                                    numberOfBowlingBallVertices, 
                                    bowlingBallTriMesh);
   bowlingPinGeometry = objLoader("pin.obj", 
                                    numberOfBowlingPinVertices, 
                                    bowlingPinTriMesh);
   bowlingLaneGeometryNoBumpers = objLoader("lane_nobumper.obj",
                                             numberOfBowlingLaneVerticesNoBumpers,
                                             bowlingLaneTriMeshNoBumpers);   
   bowlingLaneGeometryBumpers = objLoader("lane_bumper.obj", 
                                          numberOfBowlingLaneVerticesBumpers, 
                                          bowlingLaneTriMeshBumpers);
   pointerGeometry = objLoader("pointer.obj",
                                 numberOfPointerVertices,
                                 pointerTriMesh);
   curveGeometry = objLoader("curve.obj",
                              numberOfCurveVertices,
                              curveTriMesh);

   // Initialize model matrices for bowling ball and bowling lane. 
   model[BOWLING_BALL] = glm::mat4(1.0);
   model[BOWLING_LANE] = glm::mat4(1.0);
   model[POINTER] = glm::mat4(1.0); 
   model[CURVE] = glm::mat4(1.0f);
   model[NUM_DISTINCT_OBJECTS] = glm::mat4(1.0f);

   // Initialize attributes associated with bowling pins. 
   for(int i = 0; i < NUM_PINS; i++)
      {
      bowlingPins[i].pinNum = i; 
      bowlingPins[i].pinKnockedDown = false; 
      bowlingPins[i].pinModel = glm::mat4(1.0); 
      bowlingPins[i].pinRigidBody = NULL; 
      bowlingPins[i].pinTriMesh = bowlingPinTriMesh; 
      }

   // Initialize bowling pin locations. 
   bowlingPins[0].pinOriginalPos = btVector3(0, -8, -36);
   bowlingPins[1].pinOriginalPos = btVector3(-1.5, -8, -38); 
   bowlingPins[2].pinOriginalPos = btVector3(1.5, -8, -38);
   bowlingPins[3].pinOriginalPos = btVector3(-3, -8, -40);
   bowlingPins[4].pinOriginalPos = btVector3(0, -8, -40); 
   bowlingPins[5].pinOriginalPos = btVector3(3, -8, -40);
   bowlingPins[6].pinOriginalPos = btVector3(-4.5, -8, -42); 
   bowlingPins[7].pinOriginalPos = btVector3(-1.5, -8, -42);
   bowlingPins[8].pinOriginalPos = btVector3(1.5, -8, -42);
   bowlingPins[9].pinOriginalPos = btVector3(4.5, -8, -42);    
   }

void initializeTextures(void)
   {
   // Create a texture buffer object.  
   glActiveTexture(GL_TEXTURE0); 
   glGenTextures(10, textures);

   // Initialize bowling lane NO BUMPERS image - CLASSIC THEME - THEME 1. 
   Magick::Image *classicBowlingLaneImage = new Magick::Image("lane_c.jpg");
   classicBowlingLaneImage->write(&blobArr[BOWLING_LANE], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, classicBowlingLaneImage->columns(), 
                  classicBowlingLaneImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_LANE].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize bowling lane BUMPERS image - CLASSIC THEME - THEME 1. 
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, classicBowlingLaneImage->columns(), 
                  classicBowlingLaneImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_LANE].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

   // Initialize bowling ball image - CLASSIC THEME - THEME 1.
   Magick::Image *classicBowlingBallImage = new Magick::Image("ball_c.jpg"); 
   classicBowlingBallImage->write(&blobArr[BOWLING_BALL], "RGBA"); 
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_BALL]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, classicBowlingBallImage->columns(), 
                  classicBowlingBallImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_BALL].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize bowling pin image - CLASSIC THEME - THEME 1.
   Magick::Image *classicBowlingPinImage = new Magick::Image("pin_c.jpg"); 
   classicBowlingPinImage->write(&blobArr[BOWLING_PIN], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_PIN]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, classicBowlingPinImage->columns(), 
                  classicBowlingPinImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_PIN].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize pointer image - CLASSIC THEME - THEME 1.
   Magick::Image *classicPointerImage = new Magick::Image("green.jpeg"); 
   classicPointerImage->write(&blobArr[POINTER], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[POINTER]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, classicPointerImage->columns(), 
                  classicPointerImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[POINTER].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize curve image - CLASSIC THEME - THEME 1.
   glBindTexture(GL_TEXTURE_2D, textures[POINTER]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, classicPointerImage->columns(), 
                  classicPointerImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[POINTER].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// New texture.. 

   // Initialize bowling lane NO BUMPERS image - OCEAN THEME - THEME 2. 
   Magick::Image *oceanBowlingLaneImage = new Magick::Image("lane_o.jpg");
   oceanBowlingLaneImage->write(&blobArr[BOWLING_LANE + 4], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE + 4]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oceanBowlingLaneImage->columns(), 
                  oceanBowlingLaneImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_LANE + 4].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize bowling lane BUMPERS image - OCEAN THEME - THEME 2. 
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE + 4]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oceanBowlingLaneImage->columns(), 
                  oceanBowlingLaneImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_LANE + 4].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

   // Initialize bowling ball image - OCEAN THEME - THEME 2. 
   Magick::Image *oceanBowlingBallImage = new Magick::Image("ball_o.jpg"); 
   oceanBowlingBallImage->write(&blobArr[BOWLING_BALL + 4], "RGBA"); 
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_BALL + 4]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oceanBowlingBallImage->columns(), 
                  oceanBowlingBallImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_BALL + 4].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize bowling pin image - OCEAN THEME - THEME 2. 
   Magick::Image *oceanBowlingPinImage = new Magick::Image("pin_o.jpg"); 
   oceanBowlingPinImage->write(&blobArr[BOWLING_PIN + 4], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_PIN + 4]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oceanBowlingPinImage->columns(), 
                  oceanBowlingPinImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_PIN + 4].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// New texture..

   // Initialize bowling lane NO BUMPERS image - NEON THEME - THEME 3. 
   Magick::Image *neonBowlingLaneImage = new Magick::Image("lane_n.jpg");
   neonBowlingLaneImage->write(&blobArr[BOWLING_LANE + 7], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE + 7]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, neonBowlingLaneImage->columns(), 
                  neonBowlingLaneImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_LANE + 8].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize bowling lane BUMPERS image - NEON THEME - THEME 3. 
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_LANE + 7]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, neonBowlingLaneImage->columns(), 
                  neonBowlingLaneImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_LANE + 7].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

   // Initialize bowling ball image - NEON THEME - THEME 3.  
   Magick::Image *neonBowlingBallImage = new Magick::Image("ball_n.jpg"); 
   neonBowlingBallImage->write(&blobArr[BOWLING_BALL + 7], "RGBA"); 
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_BALL + 7]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, neonBowlingBallImage->columns(), 
                  neonBowlingBallImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_BALL + 7].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Initialize bowling pin image - NEON THEME - THEME 3.  
   Magick::Image *neonBowlingPinImage = new Magick::Image("pin_n.jpg"); 
   neonBowlingPinImage->write(&blobArr[BOWLING_PIN + 7], "RGBA");
   glBindTexture(GL_TEXTURE_2D, textures[BOWLING_PIN + 7]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, neonBowlingPinImage->columns(), 
                  neonBowlingPinImage->rows(), 0, GL_RGBA, 
                  GL_UNSIGNED_BYTE, blobArr[BOWLING_PIN + 7].data());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
   }

void initializeBullet(void)
   {
   // Initialize main bullet components.
   broadphase = new btDbvtBroadphase();
   collisionConfiguration = new btDefaultCollisionConfiguration(); 
   dispatcher = new btCollisionDispatcher(collisionConfiguration);
   solver = new btSequentialImpulseConstraintSolver; 
   dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, 
                                                broadphase,
                                                solver, 
                                                collisionConfiguration ); 
   dynamicsWorld->setGravity(btVector3(0, -9.81, 0));

   // Create a rigid body for the bowling lane WITH NO BUMPERS.
   {
   btCollisionShape *shape = NULL;
   shape = new btBvhTriangleMeshShape(bowlingLaneTriMeshNoBumpers, true); 
   btDefaultMotionState *shapeMotionState = NULL; 
   shapeMotionState = new btDefaultMotionState( 
                                       btTransform( 
                                          btQuaternion(0, 0, 0, 1), 
                                          btVector3(0, 0, 0))); 
   btScalar mass(0.0);
   btVector3 inertia(0, 0, 0); 
   shape->calculateLocalInertia(mass, inertia);
   btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(mass, 
                                                   shapeMotionState, 
                                                   shape, 
                                                   inertia);
   bowlingLaneRigidBodyNoBumpers = new btRigidBody(shapeRigidBodyCI);
   bowlingLaneRigidBodyNoBumpers->setRestitution(0.4);
   bowlingLaneRigidBodyNoBumpers->setFriction(2.0);
   bowlingLaneRigidBodyNoBumpers->setDamping(0.8, 0.2);
   bowlingLaneRigidBodyNoBumpers->setActivationState(DISABLE_DEACTIVATION);
   dynamicsWorld->addRigidBody(bowlingLaneRigidBodyNoBumpers, 
                                 COLLIDE_FLOOR, 
                                 bowlingLaneCollidesWith);
   }

   // Create a rigid body for the bowling ball.
   {
   btCollisionShape *shape = new btSphereShape(1.0); 
   btDefaultMotionState *shapeMotionState = NULL; 
   shapeMotionState = new btDefaultMotionState( 
                                       btTransform( 
                                          btQuaternion(0, 0, 0, 1), 
                                          btVector3(0, 5, 30))); 
   btScalar mass(10.0);
   btVector3 inertia(0, 0, 0); 
   shape->calculateLocalInertia(mass, inertia);
   btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(mass, 
                                                   shapeMotionState, 
                                                   shape, 
                                                   inertia);
   bowlingBallRigidBody = new btRigidBody(shapeRigidBodyCI);
   bowlingBallRigidBody->setRestitution(0.4);
   bowlingBallRigidBody->setFriction(0.7);
   bowlingBallRigidBody->setDamping(0.8, 0.2);
   bowlingBallRigidBody->setActivationState(DISABLE_DEACTIVATION);

   // Add bowling ball to dynamics world - since game does not start with
   // bumpers, the rigid body will not act as if there are bumpers.
   dynamicsWorld->addRigidBody(bowlingBallRigidBody, 
                                 COLLIDE_BALL, 
                                 bowlingBallCollidesWithNoBumper);
   }

   // Create rigid bodies for each of the ten bowling pins. 
   {
   // NOTE: This may change from a cylinder to a tri. mesh..
   btCollisionShape *shape = new btCylinderShape(
                                       btVector3(0.5, 1.5, 0.5)); 
   btDefaultMotionState *shapeMotionState = NULL;
   for(int i = 0; i < NUM_PINS; i++) 
      {
      shapeMotionState = new btDefaultMotionState( 
                                          btTransform(
                                             btQuaternion(0, 0, 0, 1), 
                                             bowlingPins[i].pinOriginalPos));
      btScalar mass(1);
      btVector3 inertia(0, 0, 0); 
      shape->calculateLocalInertia(mass, inertia); 
      btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI(mass, 
                                                      shapeMotionState, 
                                                      shape, 
                                                      inertia);
      bowlingPins[i].pinRigidBody = new btRigidBody(shapeRigidBodyCI);
      bowlingPins[i].pinRigidBody->setRestitution(0);
      bowlingPins[i].pinRigidBody->setFriction(0.2);
      bowlingPins[i].pinRigidBody->setDamping(0.5, 0.5);
      bowlingPins[i].pinRigidBody->setActivationState(DISABLE_DEACTIVATION); 
      dynamicsWorld->addRigidBody(bowlingPins[i].pinRigidBody, 
                                    COLLIDE_PIN, 
                                    bowlingPinsCollideWith);  
      } 
   }

   // Create rigid body for left wall.
   {
   btCollisionShape *shape = new btBoxShape(btVector3(1, 10, LANE_LENGTH));
   btDefaultMotionState *shapeMotionState = NULL;
   shapeMotionState = new btDefaultMotionState(btTransform( 
                                               btQuaternion(0, 0, 0, 1),
                                               btVector3(-12, 0, 0)));
   btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI( 0, 
                                                      shapeMotionState, 
                                                      shape, 
                                                      btVector3(0, 0, 0)); 
   btRigidBody *shapeRigidBody = new btRigidBody(shapeRigidBodyCI);
   shapeRigidBody->setRestitution(1.0);
   shapeRigidBody->setActivationState(DISABLE_DEACTIVATION);  
   dynamicsWorld->addRigidBody(shapeRigidBody,
                                 COLLIDE_BUMPER,
                                 wallCollidesWith); 
   }

   // Create rigid body for right wall.
   {
   btCollisionShape *shape = new btBoxShape(btVector3(1, 10, LANE_LENGTH));
   btDefaultMotionState *shapeMotionState = NULL;
   shapeMotionState = new btDefaultMotionState(btTransform( 
                                               btQuaternion(0, 0, 0, 1),
                                               btVector3(12, 0, 0)));
   btRigidBody::btRigidBodyConstructionInfo shapeRigidBodyCI( 0, 
                                                      shapeMotionState, 
                                                      shape, 
                                                      btVector3(0, 0, 0)); 
   btRigidBody *shapeRigidBody = new btRigidBody(shapeRigidBodyCI);
   shapeRigidBody->setRestitution(1.0);
   shapeRigidBody->setActivationState(DISABLE_DEACTIVATION);  
   dynamicsWorld->addRigidBody(shapeRigidBody,
                                 COLLIDE_BUMPER,
                                 wallCollidesWith); 
   } 
   }

void initializeGPU(void)
   {
   // Create vertex buffer objects (VBOs) to store vertex data on GPU. 
   glGenBuffers(NUM_DISTINCT_OBJECTS + 1, vboGeometry);

   // Allocate space on GPU for bowling lane WITH NO BUMPERS. 
   glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[BOWLING_LANE]);
   glBufferData(GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBowlingLaneVerticesNoBumpers, 
                  bowlingLaneGeometryNoBumpers, 
                  GL_STATIC_DRAW);

   // Allocate space on GPU for the bowling ball. 
   glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[BOWLING_BALL]);
   glBufferData(GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBowlingBallVertices, 
                  bowlingBallGeometry, 
                  GL_STATIC_DRAW);

   // Allocate space on GPU for the bowling pins. 
   glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[BOWLING_PIN]);
   glBufferData(GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBowlingPinVertices, 
                  bowlingPinGeometry, 
                  GL_STATIC_DRAW);

   // Allocate space on GPU for the pointer. 
   glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[POINTER]);
   glBufferData(GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfPointerVertices, 
                  pointerGeometry, 
                  GL_STATIC_DRAW);

   // Allocate space on GPU for the curve symbol. 
   glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[CURVE]);
   glBufferData(GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfCurveVertices, 
                  curveGeometry, 
                  GL_STATIC_DRAW);

   // Allocate space on GPU for the the alley WITH BUMPERS.  
   glBindBuffer(GL_ARRAY_BUFFER, vboGeometry[NUM_DISTINCT_OBJECTS]);
   glBufferData(GL_ARRAY_BUFFER, 
                  sizeof(Vertex) * numberOfBowlingLaneVerticesBumpers,
                  bowlingLaneGeometryBumpers, 
                  GL_STATIC_DRAW);

   // Deallocate vertex data - no longer needed because it's on the GPU.
   delete []bowlingLaneGeometryBumpers;
   bowlingLaneGeometryBumpers = NULL;
   delete []bowlingLaneGeometryNoBumpers;
   bowlingLaneGeometryNoBumpers = NULL;      
   delete []bowlingBallGeometry; 
   bowlingBallGeometry = NULL;
   delete []bowlingPinGeometry; 
   bowlingPinGeometry = NULL;
   delete []pointerGeometry;
   pointerGeometry = NULL;
   delete []curveGeometry;
   curveGeometry = NULL;    
   }

bool initializeShaders(void)
   {
   shader vAndFShaders[2];
   vAndFShaders[0].setShaderFile("vShader.vert");
   vAndFShaders[0].readShaderFile();
   vAndFShaders[0].setShaderType(GL_VERTEX_SHADER);
   vAndFShaders[1].setShaderFile("fShader.frag");
   vAndFShaders[1].readShaderFile();
   vAndFShaders[1].setShaderType(GL_FRAGMENT_SHADER);   
   programNoGeoShader = vAndFShaders[0].compileAndLinkShaders(vAndFShaders);

   // Obtain locations of variables in shaders. 
   locPosition = glGetAttribLocation(programNoGeoShader, 
                  const_cast<const char*>("v_position")); 
   if(locPosition == -1)
      {
      std::cerr << "[F] POSITION (attribute) NOT FOUND" << std::endl;
      return false;
      } 

   locNormal = glGetAttribLocation(programNoGeoShader, 
                  const_cast<const char*>("v_normal")); 
   if(locNormal == -1)
      {
      std::cerr << "[F] NORMAL (attribute) NOT FOUND" << std::endl;
      return false;
      }

   locUV = glGetAttribLocation(programNoGeoShader,
            const_cast<const char*>("v_texture")); 
   if(locUV == -1)
      {
      std::cerr << "[F] UV_POSITION (attribute) NOT FOUND" << std::endl;
      return false;
      }

   locMVP = glGetUniformLocation(programNoGeoShader,
                     const_cast<const char*>("mvpMatrix"));
   if(locMVP == -1)
      {
      std::cerr << "[F] MVPMATRIX (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locLightPos = glGetUniformLocation(programNoGeoShader,
                  const_cast<const char*>("lightPosition"));
   if(locLightPos == -1)
      {
      std::cerr << "[F] lightPosition (uniform) NOT FOUND" << std::endl;
      return false;
      } 

   locAmbient = glGetUniformLocation(programNoGeoShader,
                  const_cast<const char*>("ambientProduct"));
   if(locAmbient == -1)
      {
      std::cerr << "[F] ambientProduct (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locDiffuse = glGetUniformLocation(programNoGeoShader,
                  const_cast<const char*>("diffuseProduct"));
   if(locDiffuse == -1)
      {
      std::cerr << "[F] diffuseProduct (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpecular = glGetUniformLocation(programNoGeoShader,
                   const_cast<const char*>("specularProduct"));
   if(locSpecular == -1)
      {
      std::cerr << "[F] specularProduct (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locShininess = glGetUniformLocation(programNoGeoShader,
                  const_cast<const char*>("shininess"));
   if(locShininess == -1)
      {
      std::cerr << "[F] shininess (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locConstAttenuation = glGetUniformLocation(programNoGeoShader,
                        const_cast<const char*>("constantAttenuation"));
   if(locConstAttenuation == -1)
      {
      std::cerr << "[F] constantAttenuation (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locLinAttenuation = glGetUniformLocation(programNoGeoShader,
                        const_cast<const char*>("linearAttenuation"));
   if(locLinAttenuation == -1)
      {
      std::cerr << "[F] linearAttenuation (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locQuadAttenuation = glGetUniformLocation(programNoGeoShader,
                        const_cast<const char*>("quadraticAttenuation"));
   if(locQuadAttenuation == -1)
      {
      std::cerr << "[F] quadraticAttenuation (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpotCutoff = glGetUniformLocation(programNoGeoShader,
                     const_cast<const char*>("spotCutoff"));
   if(locSpotCutoff == -1)
      {
      std::cerr << "[F] spotCutoff (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpotExponent = glGetUniformLocation(programNoGeoShader,
                        const_cast<const char*>("spotExponent"));
   if(locSpotExponent == -1)
      {
      std::cerr << "[F] spotExponent (uniform) NOT FOUND" << std::endl;
      return false;
      }

   locSpotDirection = glGetUniformLocation(programNoGeoShader,
                     const_cast<const char*>("spotDirection"));
   if(locSpotDirection == -1)
      {
      std::cerr << "[F] spotDirection (uniform) NOT FOUND" << std::endl;
      return false;
      }

   // Send initial data to the shaders. 
   glUniform4fv(locAmbient, 1, glm::value_ptr(ambientProduct));
   glUniform4fv(locDiffuse, 1, glm::value_ptr(diffuseProduct));
   glUniform4fv(locSpecular, 1, glm::value_ptr(specularProduct));
   glUniform4fv(locLightPos, 2, lPos);
   glUniform1f(locShininess, materialShininess);
   glUniform1f(locConstAttenuation, constantAttenuation);
   glUniform1f(locLinAttenuation, linearAttenuation);
   glUniform1f(locQuadAttenuation, quadraticAttenuation);
   glUniform3fv(locSpotDirection, 2, spotDir);
   glUniform1fv(locSpotCutoff, 2, spotCut);
   glUniform1fv(locSpotExponent, 2, spotExp);

   // All shader variables were loaded correctly. 
   return true; 
   }  

void cleanUp(void)
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
   Mix_FreeChunk(soundPtr);
   soundPtr = NULL; 
   Mix_Quit(); 
   glDeleteProgram(programNoGeoShader);
   glDeleteBuffers(NUM_DISTINCT_OBJECTS, vboGeometry);
   }

float getDT(void)
   {
   float ret;
   t2 = std::chrono::high_resolution_clock::now();
   ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
   t1 = std::chrono::high_resolution_clock::now();
   return ret;
   }

Vertex *objLoader(const char *objFile, 
                  int &numVertices,
                  btTriangleMesh *&objTriMesh)
   {
   // Initialize function/variables.
   Assimp::Importer importer;
   const aiScene *pScene = importer.ReadFile(objFile, 
                                             aiProcess_Triangulate | 
                                             aiProcess_GenNormals); 

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
            cout << "ERROR: " << objFile;
            cout << " does not contain texture coordinates!" << endl;
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
            cout << "ERROR: " << objFile;
            cout << " does not contain normal coordinates!" << endl;            
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

void recordFirst(int pinsDown, int frame, score arr[])
   {
   arr[frame].pinsLeft = 10 - pinsDown;
   if( arr[frame].pinsLeft == 10 )
      {
      arr[frame].gutter = true;
      }

   else if( arr[frame].pinsLeft == 0 )
      {
      arr[frame].strike = true;
      }

   if( arr[frame].gutter == true )
      {
      arr[frame].firstScore = 0;
      }

   else if( arr[frame].strike == true )
      {
      arr[frame].firstScore = 10;
      arr[frame].secondScore = 0;
      }

   else
      {
      arr[frame].firstScore = pinsDown;
      }
   }

void recordSecond(int pinsDown, int frame, score arr[])
   {
   arr[frame].pinsLeft -= pinsDown;
   if( arr[frame].pinsLeft == 0 && arr[frame].strike == false )
      {
      arr[frame].spare = true;
      }

   arr[frame].secondScore = pinsDown;
   }

int calculateScore(score arr[])
   {
   int score = 0; 
   for( int i = 0; i < 3; i++ )
      {
      if( i == 0 )
         {
         if( arr[i].strike == true )
            {
            if( arr[i+1].strike == true )
               {
               if( arr[i+2].strike == true )
                  {
                  arr[i].frameScore = arr[i].firstScore + arr[i+1].firstScore +
                                    arr[i+2].firstScore;
                  }
               }

            else
               {
               arr[i].frameScore = arr[i].firstScore + arr[i+1].firstScore +
                              arr[i+1].secondScore;
               }
            }

            else if( arr[i].spare == true )
               {
               if( arr[i+1].strike == true )
                  {
                     arr[i].frameScore = arr[i].firstScore + arr[i].secondScore +
                                    arr[i+1].firstScore;
                  }
               }

            else
               {
               arr[i].frameScore = arr[i].firstScore + arr[i].secondScore;
               }
         }

      else if( i == 1 )
         {
         if( arr[i].strike == true )
            {
            if( arr[i+1].strike == true )
               {
               arr[i].frameScore = arr[i].firstScore + arr[i+1].firstScore;
               }

            else
               {
               arr[i].frameScore = arr[i].firstScore + arr[i+1].firstScore +
                              arr[i+1].secondScore;
               }
            }

         else if( arr[i].spare == true )
            {
            arr[i].frameScore = arr[i].firstScore + arr[i].secondScore +
                           arr[i+1].firstScore;
            }

         else
            {
            arr[i].frameScore = arr[i].firstScore + arr[i].secondScore;
            }
         }

         else
            {
            if( arr[i].strike == true )
               {
               arr[i].frameScore = 10;
               }

            else
               {
               arr[i].frameScore = arr[i].firstScore + arr[i].secondScore;
               }
         }  
      }

   for( int i = 0; i < 3; i++ )
      {
      score += arr[i].frameScore;
      }

   return score;
   }
// End of Supporting Function Implementations /////////////////////////////////