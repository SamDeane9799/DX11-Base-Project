#include "Light.h"
#include "Assets.h"

Light::Light(int type, LightInfo lightInfo)
{
	Assets& instance = Assets::GetInstance();
	lightType = type;
	info = lightInfo;
	enabled = true;
	transform = new Transform();
	switch (type)
	{
	case LIGHT_TYPE_DIRECTIONAL:
		vs = instance.GetVertexShader("DirectionalLightVS");
		ps = instance.GetPixelShader("DirectionalLightPS");
		break;
	case LIGHT_TYPE_POINT:
		mesh = instance.GetMesh("sphere");
		vs = instance.GetVertexShader("PointLightVS");
		ps = instance.GetPixelShader("PointLightPS");
		break;
	case LIGHT_TYPE_SPOT:
		mesh = instance.GetMesh("cone");
		vs = instance.GetVertexShader("VertexShader");
		default:
			break;
	}

}

Light::~Light()
{
	delete transform;
}

void Light::RenderLight(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera)
{
	vs->SetShader();
	if (lightType != LIGHT_TYPE_POINT)
	{
		transform->SetRotation(info.Direction.x, info.Direction.y, info.Direction.z);
	}
	// Send data to the vertex shader
	if (lightType != LIGHT_TYPE_DIRECTIONAL)
	{
		transform->SetPosition(info.Position.x, info.Position.y, info.Position.z);
		transform->SetScale(info.Range, info.Range, info.Range);
		vs->SetMatrix4x4("world", transform->GetWorldMatrix());
		vs->SetMatrix4x4("view", camera->GetView());
		vs->SetMatrix4x4("projection", camera->GetProjection());
		vs->CopyAllBufferData();

	}
	if (lightType == LIGHT_TYPE_SPOT) {
		ps->SetFloat("SpotFalloff", info.SpotFalloff);
	}

	ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
	ps->SetMatrix4x4("invViewProj", camera->GetInvViewProj());
	ps->SetData("lightInfo", (void*)(&info), sizeof(LightInfo));
	ps->CopyAllBufferData();
	//Send input to pixel shader
	if (lightType != LIGHT_TYPE_DIRECTIONAL)
		mesh->SetBuffersAndDraw(context);
	else
		context->Draw(3, 0);
}
