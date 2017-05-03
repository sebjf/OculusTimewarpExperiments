#version 150

uniform mat4 matWVP;
uniform mat4 matWV;

in      vec3 Position;
in		vec3 Normal;
in      vec4 Color;
in      vec2 TexCoord;

out     vec2 oTexCoord;
out     vec4 oColor;
out		vec4 v;
out		vec4 n;

void main()
{
   gl_Position = (matWVP * vec4(Position,1));
   v = (matWV * vec4(Position,1));
   n = (matWV * vec4(Normal,0));
   oTexCoord   = TexCoord;
   oColor.rgb  = pow(Color.rgb, vec3(2.2));   // convert from sRGB to linear
   oColor.a    = Color.a;
};