// Fragment Shader.

// Global constants.
const int numLights = 2; 

// The fragment shader will be given the following information from the vertex
// shader.
varying vec2 texture;
varying vec3 N;
varying vec3 E;
varying vec3 L[numLights];
varying float W[numLights];   

// Uniform variables - held constant for each incoming fragment.
// This data is acquired from the application program.  
uniform sampler2D gSampler;
uniform vec4 lightPosition[numLights];
uniform vec4 ambientProduct;
uniform vec4 diffuseProduct;
uniform vec4 specularProduct;
uniform float shininess;
uniform float constantAttenuation;
uniform float linearAttenuation;
uniform float quadraticAttenuation;  
uniform float spotCutoff[numLights];
uniform float spotExponent[numLights]; 
uniform vec3 spotDirection[numLights];  

// Main. 
void main( void )
   {
   // Initialize shader code. 
   vec3 NN = normalize(N);
   vec3 EE = normalize(E);
   vec4 ambient, diffuse, specular;
   //vec4 finalLight;
   vec4 finalLight = ambientProduct; 
   float attenuationVal = 1.0; 

   // Iterate through all lights in our scene.
   for(int i = 0; i < numLights; i++)
      { 
      // Calculate the light vector.   
      vec3 LL = normalize(L[i]);

      // Calculate the halfway vector.
      vec3 HH = normalize(LL + EE);

      // Point and spotlight. 
      if(W[i] != 0.0)
         {
         // Calculate attenuation. 
         float distance = length(L[i]);
         float constAttenuation = constantAttenuation; 
         float linAttenuation = linearAttenuation * distance;
         float quadAttenuation = quadraticAttenuation * distance * distance; 
         attenuationVal = 1.0 / (constantAttenuation + 
                                 linearAttenuation + 
                                 quadraticAttenuation);

         // Determine if the light is a spotlight. 
         if(spotCutoff[i] <= 90.0)
            {
            // Calculate cosine value. 
            float clampedCosine = max(0.0, dot(-LL, normalize(spotDirection[i])));

            // Compare cosine values to determine if location is outside spotlight.  
            if(clampedCosine < cos(radians(spotCutoff[i])))
               {
               attenuationVal = 0.0; 
               }

            else
               {
               attenuationVal *= pow(clampedCosine, spotExponent[i]); 
               }
            }
         }

      // Calculate the diffuse term.
      float kD = max(dot(LL,NN), 0.0);
      diffuse = kD * diffuseProduct * attenuationVal;

      // Calculate the specular term.
      //float kS = pow(max(dot(NN, HH), 0.0), shininess);
      float kS = pow(max(dot(reflect(-LL, NN), EE), 0.0), shininess);  
      specular = kS * specularProduct * attenuationVal;

      // Check if the light is behind the vertex.
      if(dot(LL,N) < 0.0)
         {
         specular = vec4(0.0, 0.0, 0.0, 1.0); 
      	}

      // Calculate the ambient term.
      ambient = ambientProduct; 

      // Sum all the light-material interactions together.
      // This also sums all lights in a scene.  
      //finalLight += ambient + diffuse + specular;
      finalLight += diffuse + specular;    
      }

   // Determine final color values.
   gl_FragColor = finalLight;  
   gl_FragColor.a = 1.0;  
   gl_FragColor *= texture2D( gSampler, texture.xy ); 
   }