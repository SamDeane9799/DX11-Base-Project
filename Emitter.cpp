#include "Emitter.h"


Emitter::Emitter(int NumOfParticles, int ParticlesPerEmission, float ParticleLifetime, Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context, Microsoft::WRL::ComPtr<ID3D11Device> Device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture, std::shared_ptr<SimpleVertexShader> ParticleVS, std::shared_ptr<SimplePixelShader> ParticlePS)
	:
	particlesPerEmission(ParticlesPerEmission),
	lifetimeOfParticle(ParticleLifetime),
	particleVS(ParticleVS),
	particlePS(ParticlePS),
	context(Context),
	device(Device),
	texture(Texture)
{
	particleEmissionFrequency = 1.0f / particlesPerEmission;
	particles = new Particle[NumOfParticles];

	startScale = DirectX::XMFLOAT2(0.5f, 0.5f);
	endScale = DirectX::XMFLOAT2(0.5f, 0.5f);
	startColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	endColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	startingVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	acceleration = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	velocityRange = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	totalParticlesAmount = NumOfParticles;

	firstLiveIndex = 0;
	firstDeadIndex = 0;
	amountOfLiveParticles = 0;
	timeSinceLastEmit = 0;

	myTransform = new Transform();

	unsigned int* indices = new unsigned int[NumOfParticles * 6];
	int indicesIndex = 0;
	for (int i = 0; i < NumOfParticles * 4; i += 4)
	{
		indices[indicesIndex++] = i;
		indices[indicesIndex++] = i + 1;
		indices[indicesIndex++] = i + 2;
		indices[indicesIndex++] = i;
		indices[indicesIndex++] = i + 2;
		indices[indicesIndex++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	//Make buffers and stuff
	D3D11_BUFFER_DESC particleBufferDesc = {};
	particleBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	particleBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	particleBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	particleBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	particleBufferDesc.ByteWidth = sizeof(Particle) * NumOfParticles;
	particleBufferDesc.StructureByteStride = sizeof(Particle);
	device->CreateBuffer(&particleBufferDesc, 0, particleBuffer.GetAddressOf());

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.ByteWidth = sizeof(unsigned int) * 6 * NumOfParticles;
	device->CreateBuffer(&indexBufferDesc, &indexData, indexBuffer.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC particleSRVDesc = {};
	particleSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	particleSRVDesc.Buffer.FirstElement = 0;
	particleSRVDesc.Buffer.NumElements = NumOfParticles;
	particleSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	device->CreateShaderResourceView(particleBuffer.Get(), &particleSRVDesc, particleSRV.GetAddressOf());

	delete[] indices;
}

Emitter::~Emitter()
{
	delete myTransform;

	delete[] particles;
}

void Emitter::Update(float dt)
{
	if (amountOfLiveParticles > 0)
	{
		if (firstDeadIndex < firstLiveIndex)
		{
			for (int i = firstLiveIndex; i < totalParticlesAmount; i++)
				UpdateParticle(i, dt);
			for (int i = 0; i < firstDeadIndex; i++)
				UpdateParticle(i, dt);
		}
		else if (firstDeadIndex > firstLiveIndex)
			for (int i = firstLiveIndex; i < firstDeadIndex; i++)
				UpdateParticle(i, dt);
		else
			for (int i = 0; i < totalParticlesAmount; i++)
				UpdateParticle(i, dt);
	}

	timeSinceLastEmit += dt;

	while (timeSinceLastEmit > particleEmissionFrequency)
	{
		EmitParticle();
		timeSinceLastEmit -= particleEmissionFrequency;
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (firstDeadIndex < firstLiveIndex)
	{
		memcpy(mapped.pData, particles, sizeof(Particle) * firstDeadIndex);
		memcpy((void*)((Particle*)mapped.pData + firstDeadIndex), particles + firstLiveIndex, sizeof(Particle) * (totalParticlesAmount - firstLiveIndex));
	}
	else
		memcpy(mapped.pData, particles + firstLiveIndex, sizeof(Particle) * amountOfLiveParticles);

	context->Unmap(particleBuffer.Get(), 0);
}

void Emitter::Draw(std::shared_ptr<Camera> camera)
{
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullbuffer = 0;
	context->IASetVertexBuffers(0, 1, &nullbuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	particlePS->SetShader();
	particleVS->SetShader();

	particlePS->SetShaderResourceView("Texture", texture);

	particleVS->SetShaderResourceView("ParticleData", particleSRV);
	particleVS->SetMatrix4x4("view", camera->GetView());
	particleVS->SetMatrix4x4("projection", camera->GetProjection());
	particleVS->SetFloat2("startScale", startScale);
	particleVS->SetFloat2("endScale", endScale);
	particleVS->SetFloat4("startColor", startColor);
	particleVS->SetFloat4("endColor", endColor);
	particleVS->SetFloat3("acceleration", acceleration);
	particleVS->CopyAllBufferData();

	context->DrawIndexed(amountOfLiveParticles * 6, 0, 0);
}

void Emitter::SetColor(DirectX::XMFLOAT4 newColor, DirectX::XMFLOAT4 newEndColor)
{
	startColor = newColor;
	endColor = newEndColor;
}

void Emitter::SetScale(DirectX::XMFLOAT2 newStartScale, DirectX::XMFLOAT2 newEndScale)
{
	startScale = newStartScale;
	endScale = newEndScale;
}


void Emitter::UpdateParticle(int index, float time)
{
	float lifeTime = particles[index].Time += time;
	particles[index].Age = particles[index].Time / lifetimeOfParticle;

	if (lifeTime >= lifetimeOfParticle)
	{
		firstLiveIndex++;
		firstLiveIndex %= totalParticlesAmount;
		amountOfLiveParticles--;
	}
}

float Emitter::RandomFloat(float min, float max)
{
	return (float)rand() / RAND_MAX * (max - min) + min;
}

void Emitter::EmitParticle()
{
	if (amountOfLiveParticles >= totalParticlesAmount)
		return;

	particles[firstDeadIndex].Age = 0;
	particles[firstDeadIndex].Position = myTransform->GetPosition();
	particles[firstDeadIndex].Time = 0; 
	particles[firstDeadIndex].Velocity.x = startingVelocity.x + (velocityRange.x * RandomFloat(-1.0f, 1.0f));
	particles[firstDeadIndex].Velocity.y = startingVelocity.y + (velocityRange.y * RandomFloat(-1.0f, 1.0f));
	particles[firstDeadIndex].Velocity.z = startingVelocity.z + (velocityRange.z * RandomFloat(-1.0f, 1.0f));

	firstDeadIndex++;
	firstDeadIndex %= totalParticlesAmount;
	amountOfLiveParticles++;
}
