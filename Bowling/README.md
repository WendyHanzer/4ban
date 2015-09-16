Bowling
=======

Group members: Renee Iinuma, Kyle Lee, and Wesley Kepke. 

Class: CS 480

Date: December 17, 2014 

Basic Info:
===========

This is a bowling game. The user projects the ball from the desired starting location in the desired direction at the desired speed. 

Libraries:
==========

* Assimp
* Bullet
* Magick++
* OpenGL 
* SDL_mixer

Compilation:
============

To compile: Run "make" in the "build" directory - this builds the executable. 

To run: Switch into the "bin" directory and type "./bowling" - no other command line arguments needed.

To recompile: Run "make clean" in the "build" directory followed by "make". 

Features:
=========

* Lighting
  * Spotlights - one following the bowling ball while the other is centerd on the pins. Spotlights can be turned on and off wit 's'. 

* Themes
  * Classic
  * Ocean
  * Neon

* Score is displayed with 2D text. 

User Manual:
============

Player Controls:
* When ball is moving side to side on bowling alley, press the SPACE_BAR to halt the ball at the desired starting position.
* Next, press the SPACE_BAR again to determine which angle to project the ball at.
* Next, press the SPACE_BAR again to determine how much power to project the ball at.
* Finally, the green circle with an arrow is supposed to curve the ball, BUT, this does not work yet. You still need to press the SPACE_BAR to continue. 

Camera:
* Pressing 1, 2, 3, and 4 will provide alternate camera views. 

Lighting:
* Press ‘s’ to toggle between two spotlights and all lights. 

Menu:
* Right click to Pull down the menu
  * Select “Continue Game” to continue the game if game is paused.
  * Select “Pause Game” to pause the game.
  * Select “Chnage Theme” to change the color of the alley and ball.
  * Select "Turn Bumpers On" to produce bowling bumpers. 
  * Select "Turn Bumpers Off" to remove bowling bumpers. 
  * Select “Restart Game" to start a new game. 
  * Select “Quit Game” to exit the game. 
