#pragma once

#include "OVR_CAPI_GL.h"
#include "GL\CAPI_GLE.h"
#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"

class RollingShutterRasteriser
{
public:
	RollingShutterRasteriser(ovrSizei fb);

	void Begin();
	void End();

	GLint getProgram()
	{
		return program;
	}

	void Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);
	void Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);

protected:

	GLuint vertexShader;
	GLuint geometryShader;
	GLuint fragmentShader;
	GLuint program;

	// Properties

	ovrMatrix4f view0Matrix4;
	ovrMatrix4f view1Matrix4;
	ovrMatrix4f projectionMatrix4;
	bool useConvexHull;
	int primitiveFilterIndex;
	bool useAdaptiveBounds;
	ovrVector2i m_resolution;
	bool useRaytracing;
};

struct Texture;

class RollingShutterModel : public RollingShutterRasteriser, public Model
{
public:
	RollingShutterModel(ShaderFill* fill, ovrSizei fb) : Model(ovrVector3f(), fill), RollingShutterRasteriser(fb)
	{
		m_resolution.x = fb.w;
		m_resolution.y = fb.h;
		texture = NULL;
	}

	virtual void Render(ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);

	void SetTexture(const char* filename);

private:
	Texture* texture;
};