
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


Texture2D albedo	: register(t0);
SamplerState basicSampler	:	register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	return albedo.Sample(basicSampler, input.uv);
}