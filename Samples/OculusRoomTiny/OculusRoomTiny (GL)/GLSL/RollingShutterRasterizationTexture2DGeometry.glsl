#version 430
#extension GL_EXT_gpu_shader4 : enable

/*
#include "../../GLSL/Tools.glsl"
#include "../../GLSL/GLSLUniformSlot.glsl"
#include "RollingShutter.glsl"
*/

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

struct Ray {
	vec3 origin;
	vec3 direction;
};

#define POINT_COUNT 6

layout(triangles) in;
layout(triangle_strip, max_vertices = POINT_COUNT) out;

in vec3 geometryPosition[];
in vec4 geometryColor[];
in vec2 geometryTexCoord[];

flat out vec3 triPositions[3];
flat out vec4 triColors[3];
flat out vec2 triTexCoords[3];

PLEXUS_SLOT_UNIFORM(mat4, view0Matrix4)
PLEXUS_SLOT_UNIFORM(mat4, view1Matrix4)
PLEXUS_SLOT_UNIFORM(mat4, projectionMatrix4)

PLEXUS_SLOT_UNIFORM(bool, useConvexHull)
PLEXUS_SLOT_UNIFORM(int,  primitiveFilterIndex)
PLEXUS_SLOT_UNIFORM(bool, useAdaptiveBounds)

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

vec3 project(const vec4 v) {
	return v.xyz / v.w;
}

// Tests, if the direciton from point to start is on th eleft side of the direction from oldPoint to start
bool isLeftOf(const vec2 point, const vec2 oldPoint, const vec2 start) {
	vec2 perpendicular = normalize(point - start).yx * vec2(1, -1);
	return dot(normalize(oldPoint - start), perpendicular) > 0;
}

// Simple gift wrap-based convex hull
void giftWarp(
	in const vec2 inputPositions[POINT_COUNT], 
	out vec2 outputPositions[POINT_COUNT], 
	out int pointCount)
{
	// Find the start point
	// If the rest works, this shouldbe pretty irrelevant and independent of the rest, as long as it is on the hull
	int lowestIndex = -1;
	float lowestX = -1000;
	for (int i = 0; i < POINT_COUNT; i++) {
		float x = inputPositions[i].y;
		if (x > lowestX) {
			lowestX = x;
			lowestIndex = i;
		}
	}

	pointCount = 0;

	int pointOnHull = lowestIndex;
	for (int i = 0; i < POINT_COUNT; i++) {
		outputPositions[pointCount] = inputPositions[pointOnHull];

		int endPoint = 0;
		for (int j = 1; j < POINT_COUNT; j++) {
			if ((endPoint == pointOnHull) || isLeftOf(
				inputPositions[j], 
				inputPositions[endPoint],
				outputPositions[pointCount]))
			{
				endPoint = j;
			}
		}

		pointCount++;
		pointOnHull = endPoint;

		if (endPoint == lowestIndex) return;
	}

	pointCount = 0;
}

void emitPolygon(const vec2 positions[POINT_COUNT], in const int pointCount) {
	gl_Position.z = 0.5;
	gl_Position.w = 1;

	for (int i = 0; i <pointCount; ++i) {
		if (i % 2 == 0)
			gl_Position.xy = positions[i / 2];
		else
			gl_Position.xy = positions[pointCount - 1 - i / 2];
		EmitVertex();
	}
}

void emitConvexHull(const vec2 positions[POINT_COUNT]) {
	int outputPointCount;
	vec2 hullPositions[POINT_COUNT];
	giftWarp(positions, hullPositions, outputPointCount);
	
	emitPolygon(hullPositions, outputPointCount);
}

void emitQuadHull(const vec2 positions[POINT_COUNT]) {
	vec2 minPosition = vec2(+10000);
	vec2 maxPosition = vec2(-10000);
	for (int i = 0; i < POINT_COUNT; i++) {
		minPosition = min(minPosition, positions[i]);
		maxPosition = max(maxPosition, positions[i]);
	}

	vec2 quadPositions[POINT_COUNT];
	quadPositions[3] = vec2(minPosition.x, minPosition.y);
	quadPositions[2] = vec2(maxPosition.x, minPosition.y);	
	quadPositions[1] = vec2(maxPosition.x, maxPosition.y);
	quadPositions[0] = vec2(minPosition.x, maxPosition.y);

	emitPolygon(quadPositions, 4);
}

void getPositionsSimple(out vec2 positions[POINT_COUNT]) {
	mat4 m0 = projectionMatrix4 * view0Matrix4;
	mat4 m1 = projectionMatrix4 * view1Matrix4;

	positions[0] = project(m0 * vec4(geometryPosition[0], 1)).xy;
	positions[1] = project(m0 * vec4(geometryPosition[1], 1)).xy;
	positions[2] = project(m0 * vec4(geometryPosition[2], 1)).xy;
	positions[3] = project(m1 * vec4(geometryPosition[0], 1)).xy;
	positions[4] = project(m1 * vec4(geometryPosition[1], 1)).xy;
	positions[5] = project(m1 * vec4(geometryPosition[2], 1)).xy;
}

void getPositionsAdaptive(out vec2 positions[POINT_COUNT]) {
	getPositionsSimple(positions);

	// Get the maximal roll
	float minRoll = 1;
	float maxRoll = -1;
	for (int i = 0; i < POINT_COUNT; i++) {
		minRoll = min(minRoll, positions[i].x * 0.5 + 0.5);
		maxRoll = max(maxRoll, positions[i].x * 0.5 + 0.5);
	}
	
	mat4 m0 = projectionMatrix4 * getRollingViewMatrix(minRoll);
	mat4 m1 = projectionMatrix4 * getRollingViewMatrix(maxRoll);

	positions[0] = project(m0 * vec4(geometryPosition[0], 1)).xy;
	positions[1] = project(m0 * vec4(geometryPosition[1], 1)).xy;
	positions[2] = project(m0 * vec4(geometryPosition[2], 1)).xy;
	positions[3] = project(m1 * vec4(geometryPosition[0], 1)).xy;
	positions[4] = project(m1 * vec4(geometryPosition[1], 1)).xy;
	positions[5] = project(m1 * vec4(geometryPosition[2], 1)).xy;
}

bool cull() {
	mat4 m0 = view0Matrix4;
	mat4 m1 = view1Matrix4;

	for (int i = 0; i < 6; i++) {
		if (project(m0 * vec4(geometryPosition[i], 1)).z > 0) return true;
		if (project(m1 * vec4(geometryPosition[i], 1)).z > 0) return true;
	}
	return false;
}


void main() {

	if (primitiveFilterIndex > -1 && gl_PrimitiveIDIn != primitiveFilterIndex) return;

	if (cull()) return;

	triPositions[0] = geometryPosition[0];
	triPositions[1] = geometryPosition[1];
	triPositions[2] = geometryPosition[2];
	triColors[0] = geometryColor[0];
	triColors[1] = geometryColor[1];
	triColors[2] = geometryColor[2];
	triTexCoords[0] = geometryTexCoord[0];
	triTexCoords[1] = geometryTexCoord[1];
	triTexCoords[2] = geometryTexCoord[2];

	vec2 positions[POINT_COUNT];
	if (useAdaptiveBounds) {
		getPositionsAdaptive(positions);
	} else {
		getPositionsSimple(positions);
	}	

	if (useConvexHull)  {
		emitConvexHull(positions);
	} else {
		emitQuadHull(positions);
	}
}
