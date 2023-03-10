
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include "Mesh.h"
#include "Material.h"
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "SpriteFont.h"
#include "SpriteBatch.h"
#include "Light.h"
#include "Emitter.h"
#include "Sky.h"
#include "Settings.h"

enum RenderTargetType
{ 
	ALBEDO,
	NORMALS,
	ROUGHMETAL,
	DEPTHS,
	VELOCITY,
	NEIGHBORHOOD_MAX,
	LIGHT_OUTPUT,
	SCENE,
	SCENE_AMBIENT,
	RENDER_TARGETS_COUNT
};

class Renderer
{
public:
	Renderer(Microsoft::WRL::ComPtr<ID3D11Device> Device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context, Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> BackBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthBufferDSV, unsigned int WindowWidth, unsigned int WindowHeight, std::shared_ptr<Sky> SkyPTR, std::vector<std::shared_ptr<GameEntity>>& Entities, std::vector<std::shared_ptr<Emitter>>& Emitters,
		std::vector<std::shared_ptr<Light>>& Lights, Microsoft::WRL::ComPtr<ID3D11SamplerState> ClampSampler, HWND hWnd);
	~Renderer();
	void PreResize();

	void PostResize(unsigned int windowWidth, unsigned int windowHeight,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);

	void Render(std::shared_ptr<Camera> camera, std::vector<std::shared_ptr<Material>> materials, float deltaTime);
private:

	void DrawPointLights(std::shared_ptr<Camera> camera);

	void DrawUI(std::vector<std::shared_ptr<Material>> materials, float deltaTime);

	void CreatePostProcessResources(int width, int height);

	void CreateRTV(int width, int height, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT format);

	void ToggleLightsOfAllTypes(int type, bool toggleTo);

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDSS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> directionalLightDSS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pointLightDSS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> pointLightRS;
	Microsoft::WRL::ComPtr<ID3D11BlendState> particleBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState> deferredBS;
	std::shared_ptr<DirectX::SpriteFont> arial;
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	unsigned int windowWidth;
	unsigned int windowHeight;
	int motionBlurNeighborhoodSamples;
	int motionBlurMax;
	int previousLightType;
	std::shared_ptr<Sky> sky;
	std::vector<std::shared_ptr<GameEntity>>& entities;
	std::vector<std::shared_ptr<Emitter>>& emitters;
	std::vector<std::shared_ptr<Light>>& lights;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>* renderTargetsRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* renderTargetsSRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;

	DirectX::XMFLOAT4X4 prevView;
	DirectX::XMFLOAT4X4 prevProj;

	Settings settings;
};

