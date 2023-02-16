#include "Lighting.hlsli"
// How many lights could we handle?
#define MAX_LIGHTS 128

cbuffer perLight : register(b0)
{
	float	Intensity;
	float3	Color;		
	float3	Position;	
	float3	Direction;
	float	SpotFalloff;
	float Range;
	int	type;
}

cbuffer perFrame : register(b1)
{
	float3 cameraPosition;
	int specIBLTotalMipLevels;
}
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


//IBL

Texture2D OriginalColors	: register(t0);
Texture2D Normals			: register(t1);
Texture2D Roughness			: register(t2);
Texture2D Metalness			: register(t3);
Texture2D WorldPosition		: register(t4);
SamplerState BasicSampler	: register(s0);
SamplerState ClampSampler	: register(s1);

Texture2D BrdfLookUpMap		: register(t5);
TextureCube IrradianceIBLMap: register(t6);
TextureCube SpecularIBLMap	: register(t7);

float4 main(VertexToPixel input) : SV_TARGET
{
	float4 surfaceColor = OriginalColors.Sample(BasicSampler, input.screenPosition);
	float3 normal = Normals.Sample(BasicSampler, input.screenPosition);
	float3 worldPos = Normals.Sample(BasicSampler, input.screenPosition);
	float roughness = Roughness.Sample(BasicSampler, input.screenPosition);
	float metal = Metalness.Sample(BasicSampler, input.screenPosition);

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Total color for this pixel
	float3 totalColor = float3(0,0,0);

	// Which kind of light?
	Light lightInfo;
	lightInfo.Direction = Direction;
	lightInfo.Color = Color;
	lightInfo.Position = Position;
	lightInfo.SpotFalloff = SpotFalloff;
	lightInfo.Type = type;
	lightInfo.Intensity = Intensity;
	lightInfo.Range = Range;
	lightInfo.Padding = float3(0, 0, 0);

	switch (type)
	{
	case LIGHT_TYPE_DIRECTIONAL:
		totalColor += DirLightPBR(lightInfo, input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
		break;

	case LIGHT_TYPE_POINT:
		totalColor += PointLightPBR(lightInfo, input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
		break;

	case LIGHT_TYPE_SPOT:
		totalColor += SpotLightPBR(lightInfo, input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
		break;
	}

	// Calculate requisite reflection vectors

	float3 viewToCam = normalize(cameraPosition - worldPos);

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

	float3 balancedDiff = DiffuseEnergyConserve(indirectDiffuse, indirectSpecular, metal);

	float3 fullIndirect = indirectSpecular + balancedDiff * surfaceColor.rgb;

	// Add the indirect to the direct

	totalColor += fullIndirect;
	return float4(totalColor, 1);
}