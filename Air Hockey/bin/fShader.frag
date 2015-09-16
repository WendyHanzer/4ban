// Fragment Shader

varying vec2 texture; 
uniform sampler2D gSampler; 

void main( void )
   {
   gl_FragColor = texture2D( gSampler, texture.xy );
   }
