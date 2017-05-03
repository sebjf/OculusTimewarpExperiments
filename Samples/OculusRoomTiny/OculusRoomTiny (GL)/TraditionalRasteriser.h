#pragma once

#include "Rasteriser.h"

class TraditionalRasteriser : public Rasteriser
{
public:
	TraditionalRasteriser();
	void Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection);

private:
	void Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f proj);
};