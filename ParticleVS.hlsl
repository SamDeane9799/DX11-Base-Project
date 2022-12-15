cbuffer externalData	: register(b0)
{
	matrix view;
	matrix projection;
	float2 startScale;
	float2 endScale;
	float4 startColor;
	float4 endColor;
	float3 acceleration;
}

struct Particle
{
	float EmitTime;
	float3 StartPos;
	float TotalLife;
	float3 Velocity;
};


struct VertexToPixel

{

	float4 position : SV_POSITION;

	float2 uv : TEXCOORD0;

	float4 color :		COLOR0;
};

StructuredBuffer<Particle> ParticleData	: register(t0);

VertexToPixel main(uint id : SV_VertexID)
{
	VertexToPixel output;

	uint particleID = id / 4;
	uint cornerID = id % 4;

	Particle part = ParticleData.Load(particleID);
	float age = part.EmitTime;
	float3 pos = acceleration * part.EmitTime * part.EmitTime /2.0f + part.Velocity * part.EmitTime + part.StartPos;

	float xScale = lerp(startScale.x, endScale.x, age);
	float yScale = lerp(startScale.y, endScale.y, age);;
	float2 offsets[4];
	offsets[0] = float2(-xScale, +yScale);//top left
	offsets[1] = float2(+xScale, +yScale);//top right
	offsets[2] = float2(+xScale, -yScale);//bottom right
	offsets[3] = float2(-xScale, -yScale);//bottom left

	pos += float3(view._11, view._12, view._13) * offsets[cornerID].x;
	pos += float3(view._21, view._22, view._23) * offsets[cornerID].y;


	matrix viewProj = mul(projection, view);
	output.position = mul(viewProj, float4(pos, 1.0f));
	output.color = lerp(startColor, endColor, age);
	float2 UVs[4];
	UVs[0] = float2(0, 0);
	UVs[1] = float2(1, 0);
	UVs[2] = float2(1, 1);
	UVs[3] = float2(0, 1);

	output.uv = UVs[cornerID];

	return output;
}