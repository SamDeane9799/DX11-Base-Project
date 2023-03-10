#include "Lighting.hlsli"

cbuffer perLight : register(b0)
{
	Light lightInfo;
}

cbuffer perFrame : register(b1)
{
	float3 cameraPosition;
	matrix invViewProj;
	float2 screenSize;
}

struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
};

Texture2D OriginalColors	: register(t0);
Texture2D Normals			: register(t1);
Texture2D RoughMetal		: register(t2);
Texture2D Depths			: register(t3);

SamplerState BasicSampler	: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 uv = float3(input.uv, 0);

	float3 surfaceColor = OriginalColors.Sample(BasicSampler, uv).rgb;
	float3 normal = normalize(Normals.Sample(BasicSampler, uv).rgb * 2 - 1);
	float depth = Depths.Sample(BasicSampler, uv).r;
	float3 pixelWorldPos = WorldSpaceFromDepth(depth, input.uv, invViewProj);
	float3 roughMetal = RoughMetal.Sample(BasicSampler,uv).rgb;
	float roughness = roughMetal.r;
	float metal = roughMetal.g;

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor, metal);

	// Total color for this pixel
	float3 totalColor = DirLightPBR(lightInfo, normal, pixelWorldPos, cameraPosition, roughness, metal, surfaceColor, specColor);
	return float4(totalColor, 1);
}