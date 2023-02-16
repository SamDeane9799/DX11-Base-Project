#pragma once

#include <memory>
#include <d3d11.h>
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
class Light
{
public:
	Light(int type);
	inline std::shared_ptr<Mesh> GetMesh() { return mesh; };
	inline Transform* GetTransform() { return &transform; };

	void RenderLights(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera);

private:
	int lightType;
	std::shared_ptr<Mesh> mesh;
	Transform transform;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;

};

