#pragma once

#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "SimpleShader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Transform.h"

// Light types
// Must match definitions in shader
#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2


struct LightInfo
{
	float				SpotFalloff;
	DirectX::XMFLOAT3	Direction;
	float				Range;
	DirectX::XMFLOAT3	Position;
	float				Intensity;
	DirectX::XMFLOAT3	Color;
};

class Light
{
public:
	Light(int type, LightInfo lightInfo);
	inline std::shared_ptr<Mesh> GetMesh() { return mesh; };
	inline Transform* GetTransform() { return &transform; };
	inline std::shared_ptr<SimplePixelShader> GetPixelShader() { return ps; }
	inline int GetType() { return lightType; }
	void SetRange(int range);
	void SetDirection(DirectX::XMFLOAT3 direction);

	void RenderLight(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera);

	LightInfo info;

private:
	int lightType;
	std::shared_ptr<Mesh> mesh;
	Transform transform;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;

};

