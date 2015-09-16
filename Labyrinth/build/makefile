# Linux
CC = g++
LIBS = -lglut -lGLEW -lGL 
LIBS += -lassimp 
LIBS += -lSDL_mixer -lSDL 
LIBS += -I/usr/include/bullet -lBulletDynamics -lBulletSoftBody 
LIBS += -lBulletCollision -lLinearMath

# For Macs uncomment the next line and comment out the previous one
#CC=clang++
#LIBS=  -framework OpenGL -framework GLUT -framework Cocoa -lGLEW -stdlib=libc++
# Assuming you want to use a recent compiler

# Compiler flags
CXXFLAGS = -g -Wall -std=c++0x
LDFLAGS = `Magick++-config --cppflags --cxxflags --ldflags --libs`

all: ../bin/labyrinth

../bin/labyrinth: ../src/shader.cpp ../src/main.cpp 
	$(CC) $(CXXFLAGS) ../src/shader.cpp ../src/main.cpp -o ../bin/labyrinth $(LDFLAGS) $(LIBS)

clean:
	rm -rf *.o ../bin/labyrinth