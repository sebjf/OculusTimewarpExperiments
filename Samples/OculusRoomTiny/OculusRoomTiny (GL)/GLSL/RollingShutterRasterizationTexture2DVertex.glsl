#version 400
#extension GL_EXT_gpu_shader4 : enable

/*
#include "../../GLSL/GLSLUniformSlot.glsl"
#include "../../Geometry/TextureBufferChannel.glsl"
#include "../../Framebuffer/Framebuffer.glsl"
#include "../../Framebuffer/VirtualDevice.glsl"

PLEXUS_VIRTUAL_DEVICE_TYPE("Framebuffer")
PLEXUS_VIRTUAL_DEVICE_MAPPING("2D triMesh")

PLEXUS_VIRTUAL_DEVICE_NAME("RollingShutterRasterizationTexture2D")
PLEXUS_VIRTUAL_DEVICE_DESCIRPTION("Generates a rolling shutter rasterized image")
PLEXUS_VIRTUAL_DEVICE_AUTHOR("ritschel")
PLEXUS_VIRTUAL_DEVICE_USE_CULL_FACE("true")
//PLEXUS_VIRTUAL_DEVICE_SRC_BLEND_FACTOR("GL_ONE")
//PLEXUS_VIRTUAL_DEVICE_DEST_BLEND_FACTOR("GL_ONE")
*/


uniform mat4 matW;

// match oculus shaders
in      vec3 Position;
in		vec3 Normal;
in      vec4 Color;
in      vec2 TexCoord;

out vec3 geometryPosition;
out vec3 geometryNormal;
out vec4 geometryColor;
out vec2 geometryTexCoord;

void main() {
	geometryPosition = (matW * vec4(Position,1)).xyz;
	geometryNormal = (matW * vec4(Normal,0)).xyz;
	geometryColor = Color;
	geometryTexCoord = TexCoord;
}
