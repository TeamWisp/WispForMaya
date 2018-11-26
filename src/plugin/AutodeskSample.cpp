//-
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//+


#include <maya/M3dView.h>
#include "AutodeskSample.hpp"
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MImage.h>
#include <maya/MStateManager.h>
#include <maya/MGlobal.h>
#include <stdlib.h>

//
// Sample plugin which will blit an image as the scene and rely on
// Maya's internal rendering for the UI only
// 
// Classes:
//
//	RenderOverride: The main override class. Contains all the operations
//					as well as keep track of texture resources.
//	SceneBlit:		A simple quad render responsible for blitting a
//					colour and depth image. Will also clear the background depth
//	UIDraw:			A scene override which filters out all but UI drawing
//
//  A stock "present" operation is also queued so that the results go to the viewport
//
namespace viewImageBlitOverride
{
	///////////////////////////////////////////////////////////////
	//
	// Image blit used to perform the 'scene render'
	//
	SceneBlit::SceneBlit(const MString &name)
		: MQuadRender(name)
		, mShaderInstance(NULL)
		, mColorTextureChanged(false)
		, mDepthTextureChanged(false)
		, mDepthStencilState(NULL)
	{
		mColorTexture.texture = NULL;
		mDepthTexture.texture = NULL;
	}

	SceneBlit::~SceneBlit()
	{
		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		if (!renderer)
			return;

		// Release any shader used
		if (mShaderInstance)
		{
			const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
			if (shaderMgr)
			{
				shaderMgr->releaseShader(mShaderInstance);
			}
			mShaderInstance = NULL;
		}

		// Release any state
		if (mDepthStencilState)
		{
			MHWRender::MStateManager::releaseDepthStencilState(mDepthStencilState);
			mDepthStencilState = NULL;
		}
	}

	const MHWRender::MShaderInstance * SceneBlit::shader()
	{
		if (!mShaderInstance)
		{
			MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
			const MHWRender::MShaderManager* shaderMgr = renderer ? renderer->getShaderManager() : NULL;
			if (shaderMgr)
			{
				// Create the shader.
				//
				// The default shader technique will blit color and depth textures to the
				// the output color and depth buffers respectively. The values in the depth
				// texture are expected to be normalized. 
				// 
				// The flag 'showDepthAsColor' can be set to switch to the "DepthToColor"
				// technique which will route the depth texture to the color buffer.
				// This can be used for visualizing or debugging the contents of the depth texture.
				//
				bool showDepthAsColor = false;
				mShaderInstance = shaderMgr->getEffectsFileShader("mayaBlitColorDepth", showDepthAsColor ? "DepthToColor" : "");
			}
		}

		MStatus status = MStatus::kFailure;
		if (mShaderInstance)
		{
			// If texture changed then bind new texture to the shader
			// 
			status = MStatus::kSuccess;
			if (mColorTextureChanged)
			{
				status = mShaderInstance->setParameter("gColorTex", mColorTexture);
				mColorTextureChanged = false;
			}

			if (status == MStatus::kSuccess && mDepthTextureChanged)
			{
				status = mShaderInstance->setParameter("gDepthTex", mDepthTexture);
				mDepthTextureChanged = false;
			}
		}
		if (status != MStatus::kSuccess)
		{
			return NULL;
		}
		return mShaderInstance;
	}

	MHWRender::MClearOperation & SceneBlit::clearOperation()
	{
		mClearOperation.setClearGradient(false);
		mClearOperation.setMask((unsigned int)MHWRender::MClearOperation::kClearAll);
		return mClearOperation;
	}

	//
	// Want to have this state override set to override the default
	// depth stencil state which disables depth writes.
	//
	const MHWRender::MDepthStencilState *SceneBlit::depthStencilStateOverride()
	{
		if (!mDepthStencilState)
		{
			MHWRender::MDepthStencilStateDesc desc;
			desc.depthEnable = true;
			desc.depthWriteEnable = true;
			desc.depthFunc = MHWRender::MStateManager::kCompareAlways;
			mDepthStencilState = MHWRender::MStateManager::acquireDepthStencilState(desc);
		}
		return mDepthStencilState;
	}

	///////////////////////////////////////////////////////////////
	// Maya UI draw operation. Draw all UI except for a few exclusion
	// 
	UIDraw::UIDraw(const MString& name)
		: MHWRender::MSceneRender(name)
	{
	}

	UIDraw::~UIDraw()
	{
	}

	MHWRender::MSceneRender::MSceneFilterOption
		UIDraw::renderFilterOverride()
	{
		return MHWRender::MSceneRender::kRenderNonShadedItems;
	}

	MUint64
		UIDraw::getObjectTypeExclusions()
	{
		// Exclude drawing the grid and image planes
		return (MHWRender::MFrameContext::kExcludeGrid | MHWRender::MFrameContext::kExcludeImagePlane);
	}

	MHWRender::MClearOperation &
		UIDraw::clearOperation()
	{
		// Disable clear since we don't want to clobber the scene colour blit.
		mClearOperation.setMask((unsigned int)MHWRender::MClearOperation::kClearNone);
		return mClearOperation;
	}

} //namespace

