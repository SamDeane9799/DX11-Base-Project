
struct VertexToPixel

{

	float4 position : SV_POSITION;

	float2 uv : TEXCOORD0;
};


Texture2D albedo	: register(t0);
SamplerState basicSampler	:	register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	return albedo.Sample(basicSampler, input.uv);
}