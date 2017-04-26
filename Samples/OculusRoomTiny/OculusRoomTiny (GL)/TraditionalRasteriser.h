#pragma once

#include "OVR_CAPI_GL.h"
#include "GL\CAPI_GLE.h"
#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"

class TraditionalRasteriser
{
public:
	TraditionalRasteriser();
	void Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);

private:
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;

	void Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f proj);
};