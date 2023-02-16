#include "Light.h"
#include "Assets.h"

Light::Light(int type)
{
	Assets& instance = Assets::GetInstance();
	switch (type)
	{
	case 0:
		break;
	case 1:
		mesh = instance.GetMesh("sphere");
		break;
	case 2:
		mesh = instance.GetMesh("cone");
		default:
			break;
	}

	vs = instance.GetVertexShader("VertexShader");
	ps = instance.GetPixelShader("LightRender");
}

void Light::RenderLights(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera)
{
	vs->SetShader();
	ps->SetShader();
	
	// Send data to the vertex shader
	vs->SetMatrix4x4("world", transform.GetWorldMatrix());
	vs->SetMatrix4x4("worldInverseTranspose", transform.GetWorldInverseTransposeMatrix());
	vs->SetMatrix4x4("view", camera->GetView());
	vs->SetMatrix4x4("projection", camera->GetProjection());
	vs->CopyAllBufferData();

	//Send input to pixel shader

	mesh->SetBuffersAndDraw(context);
}
