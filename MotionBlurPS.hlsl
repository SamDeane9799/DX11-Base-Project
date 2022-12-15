
struct VertexToPixel
{
	float4 position : SV_POSITION;

	float2 uv : TEXCOORD0;
};

cbuffer ExternalData
{
	int numOfSamples;
	float2 screenSize;
};

Texture2D OriginalColors	: register(t0);
Texture2D Velocities		: register(t1);
SamplerState ClampSampler	: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float2 velocity = Velocities.Load(int3(input.position.xy, 0)).xy * 1.5f;
	float4 totalColor = OriginalColors.Sample(ClampSampler, input.uv);

	float2 stepSize = (velocity / screenSize) / numOfSamples;
	float2 texCoord = input.uv + stepSize;


	for (int i = 0; i < numOfSamples; i++, texCoord += stepSize)
	{
		float4 color = OriginalColors.Sample(ClampSampler, texCoord);
		totalColor += color;
	}

	float4 finalColor = totalColor / numOfSamples;
	return finalColor;
}