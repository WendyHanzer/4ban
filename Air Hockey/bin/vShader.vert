// Vertex Shader 
 
attribute vec3 v_position;
attribute vec2 v_texture; 

varying vec2 texture; 
uniform mat4 mvpMatrix;

void main( void )
   {
   gl_Position = mvpMatrix * vec4(v_position, 1.0);
   texture = v_texture; 
   }
