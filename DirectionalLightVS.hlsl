// Defines the output data of our vertex shader

struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
};

VertexToPixel main(uint id : SV_VertexID)

{

	// Set up output

	VertexToPixel output;


	// Calculate the UV (0,0) to (2,2) via the ID

	output.uv = float2(

		(id << 1) & 2, // id % 2 * 2

		id & 2);


	// Adjust the position based on the UV

	output.screenPosition = float4(output.uv, 0, 1);

	output.screenPosition.x = output.screenPosition.x * 2 - 1;

	output.screenPosition.y = output.screenPosition.y * -2 + 1;

	output.screenPosition.z = 1;


	return output;

}