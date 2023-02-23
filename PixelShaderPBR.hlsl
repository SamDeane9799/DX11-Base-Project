
#include "Lighting.hlsli"
// How many lights could we handle?
#define MAX_LIGHTS 128

// Data that can change per material
cbuffer perMaterial : register(b0)
{
	// Surface color
	float3 colorTint;

	// UV adjustments
	float2 uvScale;
	float2 uvOffset;
};

// Data that only changes once per frame
cbuffer perFrame : register(b1)
{
	// An array of light data
	Light lights[MAX_LIGHTS];

	// The amount of lights THIS FRAME
	int lightCount;

	// Needed for specular (reflection) calculation
	float3 cameraPosition;

	int specIBLTotalMipLevels;

	float2 screenSize;
	float MotionBlurMax;
};

struct PS_Output
{
	float4 color	: SV_TARGET0;
	float4 normals	: SV_TARGET1;
	float4 roughmetal : SV_TARGET2;
	float4 depths	: SV_TARGET3;
	float2 velocity	: SV_TARGET4;
};


// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION; // The world position of this PIXEL
	float4 prevScreenPos	: SCREEN_POS0; // The world position of this PIXEL last frame
	float4 currentScreenPos	: SCREEN_POS1;
};


// Texture-related variables
Texture2D Albedo			: register(t0);
Texture2D NormalMap			: register(t1);
Texture2D RoughnessMap		: register(t2);
Texture2D MetalMap			: register(t3);
SamplerState BasicSampler	: register(s0);
SamplerState ClampSampler	: register(s1);


//IBL
Texture2D BrdfLookUpMap		: register(t4);
TextureCube IrradianceIBLMap	: register(t5);
TextureCube SpecularIBLMap	: register(t6);


// Entry point for this pixel shader
PS_Output main(VertexToPixel input)
{
	PS_Output output;

	// Always re-normalize interpolated direction vectors
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	// Apply the uv adjustments
	input.uv = input.uv * uvScale + uvOffset;

	// Sample various textures
	input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
	float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
	float metal = MetalMap.Sample(BasicSampler, input.uv).r;

	// Gamma correct the texture back to linear space and apply the color tint
	float4 surfaceColor = Albedo.Sample(BasicSampler, input.uv);
	surfaceColor.rgb = pow(surfaceColor.rgb, 2.2) * colorTint;

	float2 prevPos = input.prevScreenPos.xy / input.prevScreenPos.w;
	float2 currentPos = input.currentScreenPos.xy / input.currentScreenPos.w;
	float2 velocity = currentPos - prevPos;
	velocity.y *= -1;
	velocity *= screenSize / 2;

	if (length(velocity) > MotionBlurMax)
	{
		velocity = normalize(velocity) * MotionBlurMax;
	}

	output.color = surfaceColor;
	output.normals = float4(input.normal * .5f + 0.5f, 1);
	output.depths = input.screenPosition.w / 90.0f;
	output.roughmetal = float4(roughness, metal, 0, 0);
	output.velocity = velocity.xy;
	return output;
}