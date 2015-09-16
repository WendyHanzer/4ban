// Vertex Shader.

// Global constants.
const int numLights = 3; 

// The vertex shader will be given the following information from the 
// application program.
attribute vec3 v_position;
attribute vec2 v_texture;  
attribute vec3 v_normal;  

// The vertex shader will pass the following information to the fragment
// shader. 
varying vec2 texture;
varying vec3 N;
varying vec3 E;
varying vec3 L[numLights];
varying float W[numLights]; 

// Uniform variables - held constant for each incoming vertex.
// This data is acquired from the application program. 
uniform mat4 mvpMatrix;
uniform vec4 lightPosition[numLights]; 

// Main.
void main( void )
	{
	// Initialize variables. 
	gl_Position = mvpMatrix * vec4(v_position, 1.0);
	texture = v_texture;

	// Calculate the normal vector - not yet normalized.  
	N = v_normal;

	// Calculate the vector in the direction of the viewer - not yet normalized.
	E = v_position;

	// Calculate the light vectors - not yet normalized. 
	for(int i = 0; i < numLights; i++)
		{
		L[i] = lightPosition[i]; 
		W[i] = 0.0; 

		// If the fourth component of the light position is 0.0, this means that 
		// the lightPosition is a directional source (a vector). 
		if(lightPosition[i].w != 0.0)
			{
			L[i] = lightPosition[i] - v_position;
			W[i] = 1.0;  
			}
		}
	} 