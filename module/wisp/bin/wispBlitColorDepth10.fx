//**************************************************************************/
// Copyright 2012 Autodesk, Inc.  
// All rights reserved.
// Use of this software is subject to the terms of the Autodesk license 
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.   
//**************************************************************************/

#include "Common10.fxh"

// Color texture. Assumed to be 4 channel
Texture2D gColorTex : SourceTexture
<
    string UIName = "Color Texture";
> = NULL;
// Color texture sampler.
SamplerState gColorSampler 
{
   FILTER = MIN_MAG_MIP_POINT;
};

// Disable alpha output.
bool gDisableAlpha;

// Depth texture. Assumed to be 1 channel normalized between 0 and 1
Texture2D gDepthTex : SourceTexture2
<
    string UIName = "Depth Texture";
> = NULL;
// Depth texture sampler.
SamplerState gDepthSampler
{
    FILTER = MIN_MAG_MIP_POINT;
};


// Pixel shader outputs both color and depth
struct PS_OUTPUT
{
    float4 color : SV_TARGET;
    float  depth : SV_DEPTH;
};

// Pixel shader for color + depth
//
PS_OUTPUT PS_BlitColorDepth(VS_TO_PS_ScreenQuad In)
{
	PS_OUTPUT outputStruct;

	// Output color
	//
	float4 output = gColorTex.Sample(gColorSampler, In.UV);

	// Gamma correction (inverted, so 1/2.2=0.45...)
	float gamma = 0.45454545;
    output.xyz = pow(output.xyz, float3(1.0/gamma,1.0/gamma,1.0/gamma));

	if (gDisableAlpha)
	{
	    outputStruct.color = float4( output.xyz, 1.0 );
	}
	else
	{
	    outputStruct.color = output;
	}

	// Output depth
	//
	float4 outputDepth = gDepthTex.Sample(gDepthSampler, In.UV); 
	outputStruct.depth = outputDepth.r;
	 
	return outputStruct;
}

// Debug depth to color. Assumes a R32 single channel texture.
//
float4 PS_BlitDepthToColor(VS_TO_PS_ScreenQuad In) : SV_TARGET
{
	In.UV.y = 1.0f - In.UV.y;
	float4 outputDepth = gDepthTex.Sample(gDepthSampler, In.UV); 
	return float4(outputDepth.rrr, 1.0f);
}

// The main technique
technique10 Main
{
    pass p0
    {
		SetVertexShader(CompileShader(vs_4_0, VS_ScreenQuad()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS_BlitColorDepth()));
    }
}

// Debug by putting depth to color
technique10 DepthToColor
{
    pass p0
    {
		SetVertexShader(CompileShader(vs_4_0, VS_ScreenQuad()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS_BlitDepthToColor()));
    }
}
