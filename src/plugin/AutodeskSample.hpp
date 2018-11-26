#ifndef viewImageBlitOverride_h_
#define viewImageBlitOverride_h_

//-
// Copyright 2012 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//+


#include <maya/MString.h>
#include <maya/M3dView.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MShaderManager.h>
#include <maya/MTextureManager.h>
#include <maya/MUiMessage.h>
#include <maya/MStateManager.h>

//
// Sample plugin which will blit an image as the scene and rely on
// Maya's internal rendering for the UI only
// 
namespace viewImageBlitOverride
{
	//
	// Image blit used to perform the 'scene render'
	//
	class SceneBlit : public MHWRender::MQuadRender
	{
	public:
		SceneBlit(const MString &name);
		~SceneBlit() override;

		const MHWRender::MShaderInstance * shader() override;
		MHWRender::MClearOperation & clearOperation() override;
		const MHWRender::MDepthStencilState *depthStencilStateOverride() override;

		inline void setColorTexture(const MHWRender::MTextureAssignment &val)
		{
			mColorTexture.texture = val.texture;
			mColorTextureChanged = true;
		}
		inline void setDepthTexture(const MHWRender::MTextureAssignment &val)
		{
			mDepthTexture.texture = val.texture;
			mDepthTextureChanged = true;
		}

	protected:
		// Shader to use for the quad render
		MHWRender::MShaderInstance *mShaderInstance;
		// Texture(s) used for the quad render. Not owned by operation.
		MHWRender::MTextureAssignment mColorTexture;
		MHWRender::MTextureAssignment mDepthTexture;
		bool mColorTextureChanged;
		bool mDepthTextureChanged;

		const MHWRender::MDepthStencilState *mDepthStencilState;
	};
} //namespace

#endif
