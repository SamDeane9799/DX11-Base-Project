struct VertexToPixel
{
	float4 position : SV_POSITION;

	float2 uv : TEXCOORD0;
};

cbuffer ExternalData
{
	int numOfSamples;
};

Texture2D Velocities	: register (t0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float2 index = input.position.xy;

	float2 maxVelocity = float2(0, 0);
	float maxMagnitude = 0;

	for (int i = -numOfSamples/2; i < numOfSamples/2; i++)
	{
		for (int j = -numOfSamples/2; j < numOfSamples/2; j++)
		{
			float2 currentVel = Velocities.Load(int3(index + float2(i, j), 0)).xy;
			float currentMag = length(currentVel);

			if (currentMag > maxMagnitude)
			{
				maxVelocity = currentVel;
				maxMagnitude = currentMag;
			}
		}
	}

	return float4(maxVelocity,0 , 0);
}