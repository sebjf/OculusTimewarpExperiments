/*****************************************************************************

Filename    :   main.cpp
Content     :   Simple minimal VR demo
Created     :   December 1, 2014
Author      :   Tom Heath
Copyright   :   Copyright 2012 Oculus, Inc. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

/*****************************************************************************/
/// This sample has not yet been fully assimiliated into the framework
/// and also the GL support is not quite fully there yet, hence the VR
/// is not that great!

#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"
#include "RollingShutterRasteriser.h"
#include "TraditionalRasteriser.h"
#include "renderdoc_app.h"
#include "Stimulus.h"
#include "RasterisationUtils.h"
#include "Log.h"
#include "MyUtils.h"


#include <ctime>

using namespace OVR;

/* 
Add explicit support for debugging with renderdoc, since the oculus 
requires a non-traditional render path that renderdoc has trouble 
delinating 
*/
bool hasRenderdoc = false;
long long frameIndex;
RENDERDOC_API_1_1_1* pRenderDocAPI;

/* 
Use an enum to control flow of rendering condition and decouple flags used
for input, to reduce likelihood of errors.
*/
enum RenderingCondition : int
{
	RenderingCondition_STD = 0,
	RenderingCondition_ATW = 1,
	RenderingCondition_ROL = 2
};

int	PARAM_time = 2;
float PARAM_speedbase = 0.02f;
float PARAM_speedvariance = 0.8f;

RenderingCondition renderingCondition;

/* Describes the predicted motion throughout an upcoming frame */
struct Prediction
{
	ovrPosef EyeRenderPose[2];
	double sensorSampleTime;
	long long frameindex;

	Vector3f worldOffset;


	Matrix4f GetView(int eye)
	{
		float Yaw(3.141592f);
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(EyeRenderPose[eye].Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		Vector3f shiftedEyePos = worldOffset + rollPitchYaw.Transform(EyeRenderPose[eye].Position);

		return Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
	}

	Vector3f GetForward()
	{
		float Yaw(3.141592f);
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(EyeRenderPose[0].Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		return finalForward;
	}
};

Prediction GetPrediction(ovrSession session, ovrVector3f* HmdToEyeOffset, double timeoffset)
{
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

	double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, frameIndex);

	ovrTrackingState hmdState = ovr_GetTrackingState(session, displayMidpointSeconds + timeoffset, ovrTrue);

	Prediction result;

	result.sensorSampleTime = displayMidpointSeconds + timeoffset;
	result.frameindex = frameIndex;

	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, HmdToEyeOffset, result.EyeRenderPose);

	return result;
}

float GetDP_ZX(Vector3f v1, Vector3f v2)
{
	v1.y = 0;
	v2.y = 0;
	float sign = v1.Cross(v2).Normalized().Dot(Vector3f(0, 1, 0));	// left is negative, right is positive (right handed coordinate system)
	return (1 - v1.Dot(v2)) * sign;
}

float GetDP_YZ(Vector3f v1, Vector3f v2)
{
	v1.x = 0;
	v2.x = 0;
	return v1.Dot(v2);
}

// return true to retry later (e.g. after display lost)
static bool MainLoop(bool retryCreate)
{
	// sort out renderdoc. need to start from within renderdoc to find module this way. otherwise it must
	// be loaded explicitly.
	HMODULE hRenderDoc = GetModuleHandle("renderdoc");
	if (hRenderDoc != NULL)
	{
		hasRenderdoc = true;
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(hRenderDoc, "RENDERDOC_GetAPI");
		RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_1, (void**)&pRenderDocAPI);
	}

	// Import the assets that are not hardcoded into the example

	auto stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
	aiAttachLogStream(&stream);
	const aiScene* assimpscene = aiImportFile("./Assets/box.obj", aiProcessPreset_TargetRealtime_MaxQuality);
	const aiScene* sponza = aiImportFile("./Assets/sponza_simplified.obj", aiProcessPreset_TargetRealtime_MaxQuality);

	// create the log, this will output directly to the file. the file will be closed when this is 
	// destroyed automatically when the program ends
	Logger3 log;

	Prediction prediction0;
	Prediction prediction;
	Prediction prediction1;

    TextureBuffer * eyeRenderTexture[2] = { nullptr, nullptr };
    DepthBuffer   * eyeDepthBuffer[2] = { nullptr, nullptr };
    ovrMirrorTexture mirrorTexture = nullptr;
    GLuint          mirrorFBO = 0;
    Scene         * roomScene = nullptr; 
    bool isVisible = true;
    frameIndex = 0;

	bool enableAsyncTimewarp = false;
	bool enableRenderPose = false;
	bool enableRollingShutter = false;
	bool enableClearScreen = false;

	TriggerInput inputAdvanceSymbol;
	TriggerInput inputRenderDocCaptureFrame;
	TriggerInputs userInput;

	TriggerInput  inputs[10];

    ovrSession session;
	ovrGraphicsLuid luid;
    ovrResult result = ovr_Create(&session, &luid);
    if (!OVR_SUCCESS(result))
        return retryCreate;

    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

    // Setup Window and Graphics
    // Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution

    ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };
	if (!Platform.InitDevice(windowSize.w, windowSize.h, reinterpret_cast<LUID*>(&luid)))
	{
		VALIDATE(false, "Failed to init device.");
		exit(1);
	}

	ovrSizei idealTextureSize;

	// Make eye render buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
        eyeRenderTexture[eye] = new TextureBuffer(session, true, true, idealTextureSize, 1, NULL, 1);
        eyeDepthBuffer[eye]   = new DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

        if (!eyeRenderTexture[eye]->TextureChain)
        {
            VALIDATE(false, "Failed to create texture.");
			exit(1);
        }
    }
	
    ovrMirrorTextureDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = windowSize.w;
    desc.Height = windowSize.h;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

    // Create mirror texture and an FBO used to copy mirror texture to back buffer
    result = ovr_CreateMirrorTextureGL(session, &desc, &mirrorTexture);
    if (!OVR_SUCCESS(result))
    {
        VALIDATE(false, "Failed to create mirror texture.");
		exit(1);
    }

    // Configure the mirror read buffer
    GLuint texId;
    ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId);

    glGenFramebuffers(1, &mirrorFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Turn off vsync to let the compositor do its magic
    wglSwapIntervalEXT(0);
	
    // Make scene - can simplify further if needed
	// MUST BE CALLED AFTER OPENGL INITIALISATON
    roomScene = new Scene(false);
	roomScene->models.push_back(NULL); // for the box
	auto sponza_models = ImportAssimpScene(sponza);
	for (size_t i = 0; i < sponza_models.size(); i++)
	{
		sponza_models[i]->Scale = 0.07f;
		roomScene->models.push_back(sponza_models[i]);
	}

	// replace the default box with our own
	roomScene->models[0] = ImportAssimpModel(assimpscene, 0);

	Model* handbox = ImportAssimpModel(assimpscene, 0);
	handbox->Pos.y -= 0.1f;

	//roomScene->models.push_back(handbox);

	RollingShutterRasteriser* rollingShutterRasteriser = new RollingShutterRasteriser(idealTextureSize);
	TraditionalRasteriser* traditionalRasteriser = new TraditionalRasteriser();
	Stimulus* stimuli = new Stimulus();
	BlockedConditions* conditions = new BlockedConditions(40, 5, stimuli);

	stimuli->m_model = roomScene->models[0];
	stimuli->speed1 = PARAM_speedbase;
	stimuli->speed2 = 0;

    // FloorLevel will give tracking poses where the floor height is 0
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

	renderingCondition = RenderingCondition_ROL;

	//renderingCondition = (RenderingCondition)conditions->GetNext().rasterisation;

	TimeTrigger timeTrigger;

	TextureBuffer* reticleTexture = new TextureBuffer(session, true, true, Sizei(256,256), 1, NULL, 1);
	glClearColor(0.4f, 0.4f, 0.9f, 1.0f);
	reticleTexture->SetAndClearRenderSurface(NULL);
	CLEAR();
	reticleTexture->UnsetRenderSurface();
	reticleTexture->Commit();

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    // Main loop
    while (Platform.HandleMessages())
    {
        // Keyboard inputs to adjust player orientation
        static float Yaw(0.f);

        // Keyboard inputs to adjust player position
        static Vector3f Pos2(0.0f,1.5f,-0.0f);
        if (Platform.Key['W'])		Pos2+=Matrix4f::RotationY(Yaw).Transform(Vector3f(0,0,-0.05f));
        if (Platform.Key['S'])		Pos2+=Matrix4f::RotationY(Yaw).Transform(Vector3f(0,0,+0.05f));
        if (Platform.Key['D'])      Pos2+=Matrix4f::RotationY(Yaw).Transform(Vector3f(+0.05f,0,0));
        if (Platform.Key['A'])      Pos2+=Matrix4f::RotationY(Yaw).Transform(Vector3f(-0.05f,0,0));

		if (Platform.Key['1'])      enableAsyncTimewarp = true;		else enableAsyncTimewarp = false;
		if (Platform.Key['2'])      enableRenderPose = true;		else enableRenderPose = false;
		if (Platform.Key['3'])      enableRollingShutter = true;	else enableRollingShutter = false;
		if (Platform.Key['4'])      renderingCondition = RenderingCondition_STD;
		if (Platform.Key['5'])      enableClearScreen = true;		else enableClearScreen = false;

		if (inputs[0].Update(Platform.Key[VK_NUMPAD8]).IsTriggered()) stimuli->speed2 += 0.01f;
		if (inputs[1].Update(Platform.Key[VK_NUMPAD2]).IsTriggered()) stimuli->speed2 -= 0.01f;
		if (inputs[2].Update(Platform.Key[VK_NUMPAD6]).IsTriggered()) stimuli->speed1 += 0.01f;
		if (inputs[3].Update(Platform.Key[VK_NUMPAD4]).IsTriggered()) stimuli->speed1 -= 0.01f;

		inputAdvanceSymbol.Update(Platform.Key['5']);
		inputRenderDocCaptureFrame.Update(Platform.Key['P']);

		// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
		ovrEyeRenderDesc eyeRenderDesc[2];
		eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
		eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

		// Get eye poses, feeding in correct IPD offset
		ovrVector3f               HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset,
														eyeRenderDesc[1].HmdToEyeOffset };

		if (!enableRenderPose) {
			//	ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime)
			prediction0 = GetPrediction(session, HmdToEyeOffset, -0.0065);
			prediction0.worldOffset = Pos2;
			prediction  = GetPrediction(session, HmdToEyeOffset, 0.0);
			prediction.worldOffset  = Pos2;
			prediction1 = GetPrediction(session, HmdToEyeOffset, +0.0065);
			prediction1.worldOffset = Pos2;
		}

		// Process the user input

		userInput.Update(Platform.Key[VK_LEFT], Platform.Key[VK_RIGHT], Platform.Key[VK_UP], Platform.Key[VK_DOWN]);

		static float phase = 0.0f;
		phase = (float)Platform.MouseX * -0.004f + 2.0f;
		handbox->Pos = Vector3f(stimuli->radius * (float)sin(phase), stimuli->height1, stimuli->radius * (float)cos(phase));
		handbox->Scale = 0.5f;
		handbox->Rot = Quatf();

		// -------------------------------------------------------------

		/*
		if (userInput.IsTriggered()) {
			// This is the logic for the experimental protocol - nice and simple!

			log.Step(((float)clock()) / CLOCKS_PER_SEC, userInput.Trigger(), (int)renderingCondition, stimuli);


			if (conditions->IsDone())
			{
				Platform.Running = false;
				goto Done;
			}

			auto c = conditions->GetNext();
			renderingCondition = (RenderingCondition)c.rasterisation;
			stimuli->SetCurrentCharacterId(c.character);
		}
		*/

		// -------------------------------------------------------------

		// -------------------------------------------------------------

	
		float timeoffset = ((float)clock()) / CLOCKS_PER_SEC;

		if (timeTrigger.IsTrigger(timeoffset, 6))
		{
			if (conditions->IsDone())
			{
				Platform.Running = false;
				goto Done;
			}

			auto c = conditions->GetNext();
			renderingCondition = (RenderingCondition)c.rasterisation;
		}

		if (timeTrigger.IsTrigger(timeoffset, PARAM_time))
		{
			stimuli->multiplier = 1.0f + (((float)(rand() % 100) / 100.0f) * PARAM_speedvariance);
		}		

		if (isVisible)
		{
			Vector3f v0 = prediction0.GetForward().Normalized();
			Vector3f v2 = (stimuli->m_model->Pos - (Vector3f(prediction.EyeRenderPose->Position))).Normalized();
			log.Step(timeoffset, v0, v2, (int)renderingCondition, stimuli->GetSpeed());
		}
		

		// -------------------------------------------------------------



		// Allow overriding of the condition

		if (enableAsyncTimewarp)
		{
			renderingCondition = RenderingCondition_ATW;
		}
		if (enableRollingShutter)
		{
			renderingCondition = RenderingCondition_ROL;
		}

		// Allow overriding of symbol

		if (inputAdvanceSymbol.IsTriggered())
		{
			stimuli->SetCurrentCharacterId(stimuli->GetCurrentCharacterId() + 1);
		}

		stimuli->m_model->textureId = stimuli->GetCharacterTexture(stimuli->GetCurrentCharacterId());

		// Update the stimuli (this updates the position only, texture is handled above)

		stimuli->Update();


		if (inputRenderDocCaptureFrame.IsTriggered() && hasRenderdoc)
		{
			pRenderDocAPI->StartFrameCapture(NULL, NULL);
		}

        if (isVisible)
        {
			for (int eye = 0; eye < 2; ++eye)
			{
				// Switch to eye render target
				eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);

				Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.01f, 10000.0f, ovrProjection_None);

				if (renderingCondition == RenderingCondition_STD || renderingCondition == RenderingCondition_ATW) 
				{
					// Render world (regular rasterisation or atw)
					traditionalRasteriser->Render	(roomScene, prediction.GetView(eye), prediction.GetView(eye), proj);
				}else
				if(renderingCondition == RenderingCondition_ROL)
				{
					// Rolling Shutter Rasterisation
					if (eye == 0) rollingShutterRasteriser->rollOffset = 0.5f;
					if (eye == 1) rollingShutterRasteriser->rollOffset = 0.0f;
					rollingShutterRasteriser->Render(roomScene, prediction0.GetView(eye), prediction1.GetView(eye), proj);
				}

				if (enableClearScreen) {
					// draws white quads to the screen so we can clearly identify bounds
					CLEAR();
				}

                // Avoids an error when calling SetAndClearRenderSurface during next iteration.
                // Without this, during the next while loop iteration SetAndClearRenderSurface
                // would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
                // associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
                eyeRenderTexture[eye]->UnsetRenderSurface();

                // Commit changes to the textures so they get picked up frame
                eyeRenderTexture[eye]->Commit();
            }
        }

		if (inputRenderDocCaptureFrame.IsTriggered() && hasRenderdoc)
		{
			pRenderDocAPI->EndFrameCapture(NULL, NULL);
		}

        // Do distortion rendering, Present and flush/sync
        
        ovrLayerEyeFov ld;
        ld.Header.Type  = ovrLayerType_EyeFov;
		ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

        for (int eye = 0; eye < 2; ++eye)
        {
            ld.ColorTexture[eye] = eyeRenderTexture[eye]->TextureChain;
            ld.Viewport[eye]     = Recti(eyeRenderTexture[eye]->GetSize());
            ld.Fov[eye]          = hmdDesc.DefaultEyeFov[eye];

			if (renderingCondition == RenderingCondition_ATW)
			{
				ld.RenderPose[eye] = prediction.EyeRenderPose[eye];
			}
			else
			{
				ovrPosef identityPose = ovrPosef();
				identityPose.Orientation.w = 1.0f;
				ld.RenderPose[eye] = identityPose;
				ld.Header.Flags |= ovrLayerFlag_HeadLocked;
			}

            ld.SensorSampleTime  = prediction.sensorSampleTime;
        }

		ovrLayerQuad reticle;
		reticle.Header.Type = ovrLayerType_Quad;
		reticle.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft | ovrLayerFlag_HeadLocked;
		reticle.ColorTexture = reticleTexture->TextureChain;
		reticle.QuadPoseCenter.Position.x = 0.00f;
		reticle.QuadPoseCenter.Position.y = 0.00f;
		reticle.QuadPoseCenter.Position.z = -5.f;
		reticle.QuadPoseCenter.Orientation.x = 0;
		reticle.QuadPoseCenter.Orientation.y = 0;
		reticle.QuadPoseCenter.Orientation.z = 0;
		reticle.QuadPoseCenter.Orientation.w = 1;
		// HUD is 50cm wide, 30cm tall.
		reticle.QuadSize.x = 0.05f;
		reticle.QuadSize.y = 0.05f;
		// Display all of the HUD texture.
		reticle.Viewport.Pos.x = 0;
		reticle.Viewport.Pos.y = 0;
		reticle.Viewport.Size.w = 256;
		reticle.Viewport.Size.h = 256;

		ovrLayerHeader *layerList[2];
		layerList[0] = &ld.Header;
		layerList[1] = &reticle.Header;
        ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, layerList, 2);
        // exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
        if (!OVR_SUCCESS(result))
            goto Done;

        isVisible = (result == ovrSuccess);

        ovrSessionStatus sessionStatus;
        ovr_GetSessionStatus(session, &sessionStatus);
        if (sessionStatus.ShouldQuit)
            goto Done;
        if (sessionStatus.ShouldRecenter)
            ovr_RecenterTrackingOrigin(session);

		
        // Blit mirror texture to back buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        GLint w = windowSize.w;
        GLint h = windowSize.h;
        glBlitFramebuffer(0, h, w, 0,
                          0, 0, w, h,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        SwapBuffers(Platform.hDC);
		
        frameIndex++;
    }

Done:
    delete roomScene;
    if (mirrorFBO) glDeleteFramebuffers(1, &mirrorFBO);
    if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
    for (int eye = 0; eye < 2; ++eye)
    {
        delete eyeRenderTexture[eye];
        delete eyeDepthBuffer[eye];
    }
    Platform.ReleaseDevice();
    ovr_Destroy(session);

    // Retry on ovrError_DisplayLost
    return retryCreate || OVR_SUCCESS(result) || (result == ovrError_DisplayLost);
}

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
    // Initializes LibOVR, and the Rift
    ovrResult result = ovr_Initialize(nullptr);
    VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

    VALIDATE(Platform.InitWindow(hinst, L"Oculus Room Tiny (GL)"), "Failed to open window.");

    Platform.Run(MainLoop);

    ovr_Shutdown();

    return(0);
}
