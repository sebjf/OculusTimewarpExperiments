#version 150

uniform sampler2D Texture0;
in      vec4    oColor;
in      vec2	oTexCoord;
in		vec4	v;
in		vec4	n;

out     vec4      FragColor;

void main()
{
   FragColor = oColor * max(0.1, dot(normalize(n.xyz), -normalize(v.xyz)) ) * texture2D(Texture0, oTexCoord).x;;
   FragColor.a = 1.0;  
};