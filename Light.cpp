#include "Light.h"
#include "Assets.h"

Light::Light(int type, LightInfo lightInfo)
{
	Assets& instance = Assets::GetInstance();
	lightType = type;
	info = lightInfo;
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
		transform.Rotate(0, 0, 180);
		vs = instance.GetVertexShader("VertexShader");
		default:
			break;
	}

}

void Light::SetRange(int range)
{
	switch (lightType)
	{
	case LIGHT_TYPE_POINT || LIGHT_TYPE_SPOT:
		transform.Scale(range, range, range);
		info.Range = range;
		break;
	default:
		break;
	}
}

void Light::SetDirection(DirectX::XMFLOAT3 direction)
{
	switch (lightType)
	{
	case LIGHT_TYPE_SPOT :
		transform.SetRotation(direction.x + 180, direction.y, direction.z);
		info.Direction = direction;
		break;
	case LIGHT_TYPE_DIRECTIONAL: 
		info.Direction = direction;
		break;
	default:
		break;
	}
}

void Light::SetPosition(DirectX::XMFLOAT3 position)
{
	switch (lightType)
	{
	case LIGHT_TYPE_SPOT || LIGHT_TYPE_POINT:
		transform.SetPosition(position.x, position.y, position.z);
		info.Position = position;
		break;
	default:
		break;
	}
}

void Light::RenderLight(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera)
{
	vs->SetShader();
	ps->SetShader();
	
	// Send data to the vertex shader
	if (lightType != LIGHT_TYPE_DIRECTIONAL)
	{
		vs->SetMatrix4x4("world", transform.GetWorldMatrix());
		vs->SetMatrix4x4("view", camera->GetView());
		vs->SetMatrix4x4("projection", camera->GetProjection());
		vs->CopyAllBufferData();

		ps->SetMatrix4x4("invViewProj", camera->GetInvViewProj());
	}

	ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
	ps->SetData("lightInfo", (void*)(&info), sizeof(LightInfo));
	if (lightType == LIGHT_TYPE_SPOT) {
		ps->SetFloat("SpotFalloff", info.SpotFalloff);
	}
	ps->CopyAllBufferData();
	//Send input to pixel shader
	if (lightType != LIGHT_TYPE_DIRECTIONAL)
		mesh->SetBuffersAndDraw(context);
	else
		context->Draw(3, 0);
}
