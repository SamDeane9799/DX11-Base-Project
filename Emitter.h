#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include "Transform.h"
#include "Camera.h"
#include <wrl/client.h>
#include "SimpleShader.h"
#include <memory>

struct Particle
{
	float				Age;
	DirectX::XMFLOAT3	Position;	// 32 bytes
	float				Time;
	DirectX::XMFLOAT3   Velocity;
};
class Emitter
{
private:
	void EmitParticle();
	void UpdateParticle(int index, float time);
	float RandomFloat(float min, float max);

	Particle* particles;
	int firstLiveIndex;
	int firstDeadIndex;
	int amountOfLiveParticles;
	int totalParticlesAmount;

	int particlesPerEmission;
	float particleEmissionFrequency;
	float timeSinceLastEmit;

	float lifetimeOfParticle;

	Transform* myTransform;

	DirectX::XMFLOAT2 startScale;
	DirectX::XMFLOAT2 endScale;
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;
	DirectX::XMFLOAT3 startingVelocity;
	DirectX::XMFLOAT3 acceleration;
	DirectX::XMFLOAT3 velocityRange;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	Microsoft::WRL::ComPtr<ID3D11Buffer> particleBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	std::shared_ptr<SimpleVertexShader> particleVS;
	std::shared_ptr<SimplePixelShader> particlePS;
public:
	Emitter(int NumOfParticles, int ParticlesPerEmission, float ParticleLifetime, Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context, Microsoft::WRL::ComPtr<ID3D11Device> Device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture, std::shared_ptr<SimpleVertexShader> ParticleVS, std::shared_ptr<SimplePixelShader> ParticlePS);
	~Emitter();

	void Update(float dt);
	void Draw(std::shared_ptr<Camera> camera);

	void SetColor(DirectX::XMFLOAT4 newStartColor, DirectX::XMFLOAT4 newEndColor);
	void SetScale(DirectX::XMFLOAT2 newStartScale, DirectX::XMFLOAT2 newEndScale);
	void SetStartingVelocity(DirectX::XMFLOAT3 newStartingVel) { startingVelocity = newStartingVel; }
	void SetAcceleration(DirectX::XMFLOAT3 newAcceleration) { acceleration = newAcceleration; }
	void SetVelocityRange(DirectX::XMFLOAT3 newVelocityRange) { velocityRange = newVelocityRange; }

	Transform* GetTransform() { return myTransform; }
};

