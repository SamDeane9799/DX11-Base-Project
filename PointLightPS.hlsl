#include "lighting.hlsli"

cbuffer perLight : register(b0)
{
	Light lightInfo;
}

cbuffer perFrame : register(b1)
{
	float3 cameraPosition;
	matrix invViewProj;
	int specIBLTotalMipLevels;
	float2 screenSize;
}

struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float3 worldPos			: POSITION;
};

Texture2D OriginalColors	: register(t0);
Texture2D Normals			: register(t1);
Texture2D RoughMetal		: register(t2);
Texture2D Depths			: register(t3);

SamplerState BasicSampler	: register(s0);
SamplerState ClampSampler	: register(s1);

float4 main(VertexToPixel input) : SV_TARGET
{
	float2 uv = input.screenPosition / screenSize;
	
	float3 surfaceColor = OriginalColors.Sample(BasicSampler, uv).rgb;
	float3 normal = normalize(Normals.Sample(BasicSampler, uv).rgb * 2 - 1);
	float depth = Depths.Sample(BasicSampler, uv).r;
	float3 pixelWorldPos = WorldSpaceFromDepth(depth, uv, invViewProj);
	float roughness = RoughMetal.Sample(BasicSampler, uv).r;
	float metal = RoughMetal.Sample(BasicSampler, uv).g;

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Total color for this pixel
	float3 totalColor = float3(0,0,0);

	totalColor += PointLightPBR(lightInfo, normal, pixelWorldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
	return float4(totalColor, 1);
}