# Linux
CC=g++
LIBS= -lglut -lGLEW -lGL -lassimp

# For Macs uncomment the next line and comment out the previous one
#CC=clang++
#LIBS=  -framework OpenGL -framework GLUT -framework Cocoa -lGLEW -stdlib=libc++
# Assuming you want to use a recent compiler

# Compiler flags
CXXFLAGS= -g -Wall -std=c++0x
LDFLAGS= `Magick++-config --cppflags --cxxflags --ldflags --libs`

all: ../bin/model_Loader

../bin/model_Loader: ../src/shader.cpp ../src/main.cpp
	$(CC) $(CXXFLAGS) ../src/shader.cpp ../src/main.cpp -o ../bin/model_Loader $(LDFLAGS) $(LIBS)

clean:
	rm -rf *.o ../bin/model_Loader
