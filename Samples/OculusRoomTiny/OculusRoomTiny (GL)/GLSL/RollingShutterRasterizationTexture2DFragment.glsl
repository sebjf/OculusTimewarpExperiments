#version 430 compatibility
#extension GL_EXT_gpu_shader4 : enable

/*
#include "../../GLSL/Tools.glsl"
#include "../../Framebuffer/Framebuffer.glsl"
#include "../../GLSL/GLSLUniformSlot.glsl"
#include "../../Raytracing/Raytracing.glsl"
#include "../../Raytracing/FragCoordRay.glsl"
#include "../../Raytracing/PerTriangleInterpolation.glsl"
#include "RollingShutter.glsl"
*/

/* Requirements from "../../GLSL/GLSLUniformSlot.glsl" */

#define PLEXUS_SLOT_UNIFORM_NAMED(UNIFORM_TYPE_NAME, UNIFORM_NAME, SLOT_NAME) uniform UNIFORM_TYPE_NAME UNIFORM_NAME;
#define PLEXUS_SLOT_UNIFORM_NAMED_STATE(UNIFORM_TYPE_NAME, UNIFORM_NAME, SLOT_NAME, STATE) uniform UNIFORM_TYPE_NAME UNIFORM_NAME;
#define PLEXUS_SLOT_UNIFORM_NAMED_LAYOUT(UNIFORM_TYPE_NAME, UNIFORM_NAME, SLOT_NAME, LAYOUT) PLEXUS_SLOT_UNIFORM_NAMED(layout(LAYOUT) UNIFORM_TYPE_NAME, UNIFORM_NAME, SLOT_NAME)
#define PLEXUS_SLOT_UNIFORM_NAMED_SUBROUTINE(UNIFORM_TYPE_NAME, UNIFORM_NAME, SLOT_NAME) subroutine PLEXUS_SLOT_UNIFORM_NAMED(UNIFORM_TYPE_NAME, UNIFORM_NAME, SLOT_NAME)

// To actually use the atomic counter definition call only PLEXUS_SLOT_UNIFORM_ATOMIC_UINT_NAMED(UNIFORM_NAME, SLOT_NAME), BINDING will be added in Shader.cpp
#define PLEXUS_SLOT_UNIFORM_ATOMIC_UINT_NAMED(UNIFORM_NAME, SLOT_NAME, BINDING) layout(binding = BINDING, offset = 0) uniform atomic_uint UNIFORM_NAME;


#define PLEXUS_SLOT_UNIFORM(UNIFORM_TYPE_NAME, UNIFORM_NAME) PLEXUS_SLOT_UNIFORM_NAMED(UNIFORM_TYPE_NAME, UNIFORM_NAME, "") 
#define PLEXUS_SLOT_UNIFORM_STATE(UNIFORM_TYPE_NAME, UNIFORM_NAME, STATE) PLEXUS_SLOT_UNIFORM_NAMED_STATE(UNIFORM_TYPE_NAME, UNIFORM_NAME, "", STATE) 
#define PLEXUS_SLOT_UNIFORM_LAYOUT(UNIFORM_TYPE_NAME, UNIFORM_NAME, LAYOUT) PLEXUS_SLOT_UNIFORM_NAMED_LAYOUT(UNIFORM_TYPE_NAME, UNIFORM_NAME, "", LAYOUT) 
#define PLEXUS_SLOT_UNIFORM_SUBROUTINE(UNIFORM_TYPE_NAME, UNIFORM_NAME) PLEXUS_SLOT_UNIFORM_NAMED_SUBROUTINE(UNIFORM_TYPE_NAME, UNIFORM_NAME, "") 

// To actually use the atomic counter definition call only PLEXUS_SLOT_UNIFORM_ATOMIC_UINT(UNIFORM_NAME), BINDING will be added in Shader.cpp
#define PLEXUS_SLOT_UNIFORM_ATOMIC_UINT(UNIFORM_NAME, BINDING) PLEXUS_SLOT_UNIFORM_ATOMIC_UINT_NAMED(UNIFORM_NAME, "", BINDING)

/* Requirements from "../../Framebuffer/Framebuffer.glsl" */

#extension GL_ARB_fragment_layer_viewport : enable
#extension GL_AMD_vertex_shader_layer : enable

#define PLEXUS_FRAMEBUFFER_DEVICE true

#define PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, ATTACHMENT_NAME) out GLSL_TYPE NAME;
#define PLEXUS_FRAMEBUFFER_ATTACHMENT(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT) PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, "")

#define PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED_MIP_STATE_SAMPLER(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, ATTACHMENT_NAME, STATE, SAMPLER) out GLSL_TYPE NAME; uniform SAMPLER NAME##Texture2DFramebufferTexture2D;
#define PLEXUS_FRAMEBUFFER_ATTACHMENT_MIP_STATE_SAMPLER(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, STATE, SAMPLER) PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED_MIP_STATE_SAMPLER(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, "", STATE, SAMPLER)
#define PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED_MIP_STATE(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, ATTACHMENT_NAME, STATE) PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED_MIP_STATE_SAMPLER(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, ATTACHMENT_NAME, STATE, sampler2D)
#define PLEXUS_FRAMEBUFFER_ATTACHMENT_MIP_STATE(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, STATE) PLEXUS_FRAMEBUFFER_ATTACHMENT_NAMED_MIP_STATE(GLSL_TYPE, NAME, OPENGL_INTERNAL_FORMAT, "", STATE)

#define INTERPOLATE_BARYCENTRIC(VALUE0, VALUE1, VALUE2, COORD) ((VALUE0) + (COORD).x * ((VALUE1) - (VALUE0)) + (COORD).y * ((VALUE2) - (VALUE0)))

struct Ray {
	vec3 origin;
	vec3 direction;
};

flat in vec3 triPositions[3];
flat in vec4 triColors[3];
flat in vec2 triTexCoords[3];

uniform sampler2D Texture0;

PLEXUS_SLOT_UNIFORM(mat4, view0Matrix4)
PLEXUS_SLOT_UNIFORM(mat4, view1Matrix4)
PLEXUS_SLOT_UNIFORM(mat4, projectionMatrix4)

PLEXUS_SLOT_UNIFORM(ivec2, resolution)
PLEXUS_SLOT_UNIFORM(bool, useRaytracing)

layout(location = 0) out vec4 FragColor; //http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/

PLEXUS_FRAMEBUFFER_ATTACHMENT(vec4, position, GL_RGBA32F)
PLEXUS_FRAMEBUFFER_ATTACHMENT(vec3, normal, GL_RGB16F)
PLEXUS_FRAMEBUFFER_ATTACHMENT(float, depth, GL_DEPTH_COMPONENT32)


mat4 mix(mat4 m0, mat4 m1, const float t) {
	return m0 + t * (m1 - m0);
}

mat4 getRollingViewMatrix(const float roll) {
	return mix(view0Matrix4, view1Matrix4, roll);
}

vec3 getRollingPositionObjectSpace(vec4 positionScreenSpace, const float roll) {
	mat4 projViewModelInverted = inverse(projectionMatrix4 * getRollingViewMatrix(roll));
	vec4 positionObjectSpace = projViewModelInverted * positionScreenSpace;
	return positionObjectSpace.xyz / positionObjectSpace.w;
}

vec3 getRollingCameraPosition(const float roll) {
	return (inverse(getRollingViewMatrix(roll)) * vec4(0, 0, 0, 1)).xyz;
}

Ray getRollingRay(const in vec2 fragCoord, const in ivec2 resolution, const in float rayBias, const float roll) {

	Ray ray;
	ray.origin = getRollingCameraPosition(roll);
	// Where would I end up on the screen (NDC) being rasterized? (depth in Z is chosen to get reasonable values and minimize precision lost)
	vec4 targetPointInSS = vec4(fragCoord / resolution * 2 - 1, 0, 1);
	// Where whould I have to be in object space to end up there?
	vec3 targetPointOS = getRollingPositionObjectSpace(targetPointInSS, roll);
	// Which direction do I have to go to get there from the Origin(TM)?
	ray.direction = normalize(targetPointOS - ray.origin);
	ray.origin += rayBias * ray.direction;
	return ray;
}

bool intersectTri(const vec3 vertex0, const vec3 vertex1, const vec3 vertex2, const Ray ray, inout float tBest, inout vec2 coordBest) {
	vec3 e1 = vertex1 - vertex0;
	vec3 e2 = vertex2 - vertex0;
	vec3 p = cross(ray.direction,e2); 
	float det = dot(p,e1); 
	if(det < -0.000001) return false;

	float invdet = 1.0 / det;

	vec3 tvec = ray.origin - vertex0;
	float u = dot(p,tvec) * invdet;  
	vec3 q = cross(tvec,e1); 
	float v = dot(q,ray.direction) * invdet; 
	float t = dot(q,e2) * invdet; 	
	if((u >= 0.0) && (v >= 0.0) && (u + v <= 1.0) && t > 0 && t < tBest) {
		tBest = t;
		coordBest = vec2(u, v);		
		return true;
	}
	return false;
}

void main() {

	float roll = float(gl_FragCoord.x) / float(resolution.x);

	Ray ray = getRollingRay(gl_FragCoord.xy, resolution, 0, roll);

	float tBest = 1000000;
	vec2 coordBest = vec2(0);
	bool hit = intersectTri(
		triPositions[0], triPositions[1], triPositions[2], ray, tBest, coordBest);
	hit = hit || intersectTri(
		triPositions[2], triPositions[0], triPositions[1], ray, tBest, coordBest);

	if (useRaytracing) {
		if (!hit) discard;

		position.xzy = INTERPOLATE_BARYCENTRIC(triPositions[0], triPositions[1], triPositions[1], coordBest);
		position.a = 1;

		vec4 color		= INTERPOLATE_BARYCENTRIC(triColors[0], triColors[1], triColors[2], coordBest);
		vec2 texcoord	= INTERPOLATE_BARYCENTRIC(triTexCoords[0], triTexCoords[1], triTexCoords[2], coordBest);

		gl_FragDepth = tBest / 100.0;
		FragColor = color * texture2D(Texture0, texcoord).x;

	} else {
		gl_FragDepth = 0.5;
		position = vec4(1);
		normal = vec3(0);
		FragColor = vec4(1,1,1,1);
	}
	
}
