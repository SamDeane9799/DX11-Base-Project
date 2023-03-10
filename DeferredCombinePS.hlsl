#include "Lighting.hlsli"

cbuffer perFrame : register(b1)
{
	float3 cameraPosition;
	matrix invViewProj;
	int specIBLTotalMipLevels;
}

struct PS_Output
{
	float4 scene	: SV_TARGET0;
	float4 sceneAmbient: SV_TARGET1;
};
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
};

Texture2D OriginalColors	: register(t0);
Texture2D Normals			: register(t1);
Texture2D RoughMetal		: register(t2);
Texture2D Depths			: register(t3);
Texture2D LightOutput		: register(t4);
//IBL
Texture2D BrdfLookUpMap		: register(t5);
TextureCube IrradianceIBLMap: register(t6);
TextureCube SpecularIBLMap	: register(t7);

SamplerState BasicSampler	: register(s0);
SamplerState ClampSampler	: register(s1);

PS_Output main(VertexToPixel input) : SV_TARGET
{
	PS_Output output;
	float3 uv = float3(input.uv, 0);

	float3 surfaceColor = OriginalColors.Sample(BasicSampler, uv).rgb;
	float3 normal = normalize(Normals.Sample(BasicSampler, uv).rgb * 2 - 1);
	float depth = Depths.Sample(BasicSampler, uv).r;
	float3 pixelWorldPos = WorldSpaceFromDepth(depth, input.uv, invViewProj);
	float2 roughMetal = RoughMetal.Sample(BasicSampler, uv).r;
	float roughness = roughMetal.r;
	float metal = roughMetal.g;

	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Calculate requisite reflection vectors

	float3 viewToCam = normalize(cameraPosition - pixelWorldPos);

	float3 viewRefl = normalize(reflect(-viewToCam, normal));

	float NdotV = saturate(dot(normal, viewToCam));


	// Indirect lighting

	float3 indirectDiffuse = IndirectDiffuse(IrradianceIBLMap, BasicSampler, normal);

	float3 indirectSpecular = IndirectSpecular(

		SpecularIBLMap, specIBLTotalMipLevels,

		BrdfLookUpMap, ClampSampler, // MUST use the clamp sampler here!

		viewRefl, NdotV,

		roughness, specColor);


	// Balance indirect diff/spec
	
	float3 balancedDiff = DiffuseEnergyConserve(indirectDiffuse, indirectSpecular, metal) * surfaceColor.rgb;

	float3 fullIndirect = indirectSpecular + balancedDiff;
	
	output.scene = float4(LightOutput.Sample(BasicSampler, uv).rgb + fullIndirect, 1);
	output.sceneAmbient = float4(fullIndirect, 1);
	return output;
}