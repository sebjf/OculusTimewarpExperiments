#pragma once

#include "Rasteriser.h"

class RollingShutterRasteriser : public Rasteriser
{
public:
	RollingShutterRasteriser(ovrSizei fb);
	void Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);

protected:

	// Properties
	ovrMatrix4f view0Matrix4;
	ovrMatrix4f view1Matrix4;
	ovrMatrix4f projectionMatrix4;
	bool useConvexHull;
	int primitiveFilterIndex;
	bool useAdaptiveBounds;
	ovrVector2i m_resolution;
	bool useRaytracing;

private:
	GLint getProgram()
	{
		return program;
	}

	void Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);
};

