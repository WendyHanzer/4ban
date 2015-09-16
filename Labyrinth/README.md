Labyrinth
=========

Group members: Renee Iinuma, Kyle Lee, and Wesley Kepke. 

Class: CS 480

Date: November 24, 2014 

Basic Info:
===========

This is a labyrinth game that can be changed between multiple themes and number of balls. It keeps track of players' time and the game ends if the player successfully reaches the end platform. The game can be played in 2 preset views.

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

To run: Switch into the "bin" directory and type "./labyrinth" - no other command line arguments needed.

To recompile: Run "make clean" in the "build" directory followed by "make". 

Extra Credit:
=============

Multiple balls: 
* For normal mode, only 1 ball is placed on the board. 
* For added fun, user can allow for a 2nd ball to be placed on the board. 

Extra Credit for 480:

* Switch Balls
  * One
  * Two

* Levels
  * Easy
  * Hard

Extra Credit for All:

* Sound Effects for Both Themes
    * Start of game
    * Ball falling in a hole
    * End of game

Other:
* Lighting
  * Spotlight/Spotlights follows ball/balls

* Themes
  * Ocean
  * Pac Man

* Rotating knobs

* Displaying score with 2D Text

User Manual:
============

Player Controls:
* Use the arrow keys to tilt the board to navigate the ball
* Press SPACE BAR in order to pause or resume the game

Camera:
* Press 1 or 2 to change between the preset play views
* Press 3 to see the knobs rotate relative to the board

Lighting:
* Press ‘s’ to toggle between spotlight and all light

Menu:
* Right click to Pull down the menu
  * Select “Continue Game” to continue the game if game is paused
  * Select “Pause Game” to pause the game
  * Select “Switch Levels” to change between two levels/themes
  * Select “Restart with one ball” to restart the game with one ball
  * Select “Restart with two balls” to restart the game with two balls
  * Select “Quit Game” to exit the game
