
// Out of the vertex shader (and eventually input to the PS)
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION; // The world position of this vertex
	float4 prevScreenPos	: SCREEN_POS0;// The world position of this vertex last frame
	float4 currentScreenPos	: SCREEN_POS1;
};
cbuffer ScreenInfo
{
	float2 screenSize;
	float refractionScale;
	float padding;
};

Texture2D OriginalColors	: register(t0);
Texture2D NormalMap			: register(t1);
SamplerState ClampSampler	: register(s0);
SamplerState BasicSampler	: register(s1);

float4 main(VertexToPixel input)	: SV_TARGET
{
	float2 screenUV = input.screenPosition.xy / screenSize;

	float2 offset = NormalMap.Sample(BasicSampler, input.uv).xy * 2 - 1;
	offset.y *= -1;

	float2 refractedUV = screenUV + offset * refractionScale;
	return OriginalColors.Sample(ClampSampler, refractedUV);
}