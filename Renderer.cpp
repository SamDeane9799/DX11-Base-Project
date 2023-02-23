#include "Renderer.h"
#include "Assets.h"
#include <DirectXMath.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

using namespace std;
using namespace DirectX;

Renderer::Renderer(Microsoft::WRL::ComPtr<ID3D11Device> Device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context, Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> BackBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthBufferDSV, unsigned int WindowWidth, unsigned int WindowHeight, std::shared_ptr<Sky> SkyPTR, std::vector<std::shared_ptr<GameEntity>>& Entities, std::vector<std::shared_ptr<Emitter>>& Emitters,
	std::vector<std::shared_ptr<Light>>& Lights, Microsoft::WRL::ComPtr<ID3D11SamplerState> ClampSampler, HWND hWnd)
	:
		lights(Lights),
		entities(Entities),
		emitters(Emitters)
{
	device = Device;
	context = Context;
	swapChain = SwapChain;
	backBufferRTV = BackBufferRTV;
	depthBufferDSV = DepthBufferDSV;
	windowWidth = WindowWidth;
	windowHeight = WindowHeight;
	sky = SkyPTR;
	clampSampler = ClampSampler;
	
	motionBlurNeighborhoodSamples = 16;
	motionBlurMax = 16;

	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	renderTargetsRTV = new Microsoft::WRL::ComPtr<ID3D11RenderTargetView>[RENDER_TARGETS_COUNT];
	renderTargetsSRV = new Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>[RENDER_TARGETS_COUNT];

	CreatePostProcessResources();

	D3D11_SAMPLER_DESC ppSampleDesc = {};
	ppSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&ppSampleDesc, ppSampler.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC particleDepthDesc = {};
	particleDepthDesc.DepthEnable = true;
	particleDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	particleDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&particleDepthDesc, particleDSS.GetAddressOf());

	D3D11_BLEND_DESC additiveBlendDesc = {};
	additiveBlendDesc.RenderTarget[0].BlendEnable = true;
	additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	device->CreateBlendState(&additiveBlendDesc, particleBS.GetAddressOf());

	D3D11_BLEND_DESC deferredBlendDesc = {};
	deferredBlendDesc.AlphaToCoverageEnable = false;
	deferredBlendDesc.IndependentBlendEnable = false;
	deferredBlendDesc.RenderTarget[0].BlendEnable = true;
	deferredBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	deferredBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	deferredBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	deferredBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	deferredBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	deferredBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	deferredBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&deferredBlendDesc, deferredBS.GetAddressOf());


	D3D11_RASTERIZER_DESC rDesc = {};
	rDesc.DepthClipEnable = true;
	rDesc.CullMode = D3D11_CULL_FRONT;
	rDesc.FillMode = D3D11_FILL_SOLID;
	device->CreateRasterizerState(&rDesc, pointLightRS.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};

	dsDesc.DepthEnable = true;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	device->CreateDepthStencilState(&dsDesc, directionalLightDSS.GetAddressOf());

	dsDesc.DepthEnable = true;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	device->CreateDepthStencilState(&dsDesc, pointLightDSS.GetAddressOf());
}

Renderer::~Renderer()
{
}

void Renderer::PreResize()
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();
}

void Renderer::PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV)
{
	this->windowHeight = windowHeight;
	this->windowWidth = windowWidth;

	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	for (int i = 0; i < RENDER_TARGETS_COUNT - 1; i++)
	{
		renderTargetsRTV[i].Reset();
		renderTargetsSRV[i].Reset();
	}

	CreatePostProcessResources();
}



void Renderer::Render(shared_ptr<Camera> camera, vector<shared_ptr<Material>> materials, float deltaTime)
{
	// Background color for clearing
	const float color[4] = { 0, 0, 0, 1 };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
	for (int i = 0; i < RENDER_TARGETS_COUNT; i++)
	{
		context->ClearRenderTargetView(renderTargetsRTV[i].Get(), color);
	}
	/*const float clear[4] = {1, 0, 0, 0};
	context->ClearRenderTargetView(renderTargetsRTV[DEPTHS].Get(), clear);*/


	ID3D11RenderTargetView* renderTargets[RENDER_TARGETS_COUNT] = {};
	for (int i = 0; i < RENDER_TARGETS_COUNT - 1; i++)
	{
		renderTargets[i] = renderTargetsRTV[i].Get();
	}

	context->OMSetRenderTargets(RENDER_TARGETS_COUNT - 1, renderTargets, depthBufferDSV.Get());
	// Draw all of the entities
	for (auto& ge : entities)
	{
		if (ge->GetMaterial()->GetRefractive())
			continue;
		std::shared_ptr<SimpleVertexShader> vs = ge->GetMaterial()->GetVertexShader();
		vs->SetMatrix4x4("prevProjection", prevProj);
		vs->SetMatrix4x4("prevView", prevView);
		vs->SetMatrix4x4("prevWorld", ge->GetTransform()->GetPreviousWorldMatrix());
		vs->CopyAllBufferData();

		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetFloat2("screenSize", XMFLOAT2(windowWidth, windowHeight));
		ps->SetFloat("MotionBlurMax", motionBlurMax);
		ps->CopyBufferData("perFrame");

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the light sources
	//DrawPointLights(camera);

	//Do light rendering
	context->OMSetRenderTargets(1, renderTargetsRTV[LIGHT_OUTPUT].GetAddressOf(), 0);
	context->OMSetBlendState(deferredBS.Get(), 0, 0xFFFFFFFF);
	previousLightType = -1;
	for (auto& l : lights)
	{
		std::shared_ptr<SimplePixelShader> ps = l->GetPixelShader();
		ps->SetShader();

		if (l->GetType() == LIGHT_TYPE_DIRECTIONAL) {
			context->OMSetDepthStencilState(directionalLightDSS.Get(), 0);
			context->RSSetState(0);
		}
		else if (l->GetType() == LIGHT_TYPE_POINT) {
			context->OMSetDepthStencilState(pointLightDSS.Get(), 0);
			context->RSSetState(pointLightRS.Get());

			ps->SetData("lightInfo", (void*)&l->info, sizeof(Light));
		}
		//Set Per frame info (should be moved before the for loop)
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->SetMatrix4x4("invViewProj", l->GetTransform()->GetWorldInverseTransposeMatrix());
		ps->SetFloat2("screenSize", DirectX::XMFLOAT2(windowWidth, windowHeight));

		//Setting all our lighting information
		ps->SetShaderResourceView("OriginalColors", renderTargetsSRV[ALBEDO].Get());
		ps->SetShaderResourceView("Normals", renderTargetsSRV[NORMALS].Get());
		ps->SetShaderResourceView("Depths", renderTargetsSRV[DEPTHS].Get());
		ps->SetShaderResourceView("RoughMetal", renderTargetsSRV[ROUGHMETAL].Get());
		ps->CopyAllBufferData();

		l->RenderLight(context, camera);
	}
	context->OMSetDepthStencilState(0, 0);
	context->RSSetState(0);
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);


	context->OMSetRenderTargets(1, renderTargetsRTV[SCENE].GetAddressOf(), 0);
	Assets::GetInstance().GetVertexShader("FullscreenVS")->SetShader();
	//Final Light Combine
	{
		std::shared_ptr<SimplePixelShader> ps = Assets::GetInstance().GetPixelShader("DeferredCombinePS");
		ps->SetShader();

		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->SetInt("specIBLTotalMipLevels", sky->GetNumOfMipLevels());
		ps->SetMatrix4x4("invViewProj", camera->GetInvViewProj());

		//Set Clamp sampler
		ps->SetSamplerState("ClampSampler", clampSampler);
		//Setting all our lighting information
		ps->SetShaderResourceView("OriginalColors", renderTargetsSRV[ALBEDO].Get());
		ps->SetShaderResourceView("Normals", renderTargetsSRV[NORMALS].Get());
		ps->SetShaderResourceView("Depths", renderTargetsSRV[DEPTHS].Get());
		ps->SetShaderResourceView("RoughMetal", renderTargetsSRV[ROUGHMETAL].Get());
		ps->SetShaderResourceView("LightOutput", renderTargetsSRV[LIGHT_OUTPUT].Get());
		ps->SetShaderResourceView("BrdfLookUpMap", sky->GetBrdfLookUp());
		ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->getConvolvedSpecularMap());

		context->Draw(3, 0);
	}
	// Draw the sky
	context->OMSetRenderTargets(1, renderTargetsRTV[SCENE].GetAddressOf(), depthBufferDSV.Get());
	sky->Draw(camera);



	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	ID3D11Buffer* nothing = 0;
	context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
	context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);

	context->OMSetRenderTargets(1, renderTargetsRTV[NEIGHBORHOOD_MAX].GetAddressOf(), 0);
	Assets::GetInstance().GetVertexShader("FullscreenVS")->SetShader();
	//Doing the motion blur B)
	{
		std::shared_ptr<SimplePixelShader> ps = Assets::GetInstance().GetPixelShader("MotionBlurNeighborhoodPS");
		ps->SetShader();
		ps->SetInt("numOfSamples", motionBlurNeighborhoodSamples);
		ps->SetShaderResourceView("Velocities", renderTargetsSRV[VELOCITY].Get());
		ps->CopyAllBufferData();

		context->Draw(3, 0);
	}


	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);
	{
		std::shared_ptr<SimplePixelShader> ps = Assets::GetInstance().GetPixelShader("MotionBlurPS");
		ps->SetShader();
		ps->SetInt("numOfSamples", 16);
		ps->SetShaderResourceView("OriginalColors", renderTargetsSRV[SCENE].Get());
		ps->SetShaderResourceView("Velocities", renderTargetsSRV[NEIGHBORHOOD_MAX].Get());
		ps->SetSamplerState("ClampSampler", ppSampler);
		ps->SetFloat2("screenSize", DirectX::XMFLOAT2(windowWidth, windowHeight));
		ps->CopyAllBufferData();

		context->Draw(3, 0);
	}

	//Refractive objects rendering
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	for (auto& ge : entities)
	{
		ge->GetTransform()->SetPreviousWorldMatrix(ge->GetTransform()->GetWorldMatrix());
		if (!ge->GetMaterial()->GetRefractive())
			continue;
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetShader();
		ps->SetShaderResourceView("OriginalColors", renderTargetsSRV[SCENE].Get());
		ps->SetFloat2("screenSize", DirectX::XMFLOAT2(windowWidth, windowHeight));

		// Draw the entity
		ge->Draw(context, camera);
	}

	//Particle drawing
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	context->OMSetDepthStencilState(particleDSS.Get(), 0);

	for (auto& e : emitters)
	{
		context->OMSetBlendState(particleBS.Get(), 0, 0xFFFFFFFF);
		e->Draw(camera);
	}

	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->OMSetDepthStencilState(0, 0);
	// Draw some UI
	DrawUI(materials, deltaTime);
	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)

	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);

	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	prevView = camera->GetView();
	prevProj = camera->GetProjection();
}

void Renderer::DrawUI(vector<shared_ptr<Material>> materials, float deltaTime)
{
	// Reset render states, since sprite batch changes these!
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->OMSetDepthStencilState(0, 0);

	ImGui::SetNextWindowSize(ImVec2(500, 300));
	ImGui::Begin("Program Info");
	ImGui::Text("FPS = %f", 1.0f / deltaTime);
	ImGui::Text("Width = %i", windowWidth);
	ImGui::Text("Height = %i", windowHeight);
	ImGui::Text("Aspect ratio = %f", (float)windowWidth / (float)windowHeight);
	ImGui::Text("Number of Entities = %i", entities.size());
	ImGui::Text("Number of Lights = %i", lights.size());

	if (ImGui::CollapsingHeader("Lights")) {
		for (int i = 0; i < lights.size(); i++)
		{
			ImGui::PushID(i);
			std::string label = "Light " + std::to_string(i + 1);
			if (ImGui::TreeNode(label.c_str()))
			{
				ImGui::ColorEdit3("Light Color", &lights[i]->info.Color.x);
				ImGui::DragFloat3("Light Direction", &lights[i]->info.Direction.x);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	if (ImGui::CollapsingHeader("Entities")) {
		for (int i = 0; i < entities.size(); i++)
		{
			ImGui::PushID(i);
			std::string label = "Entity " + std::to_string(i + 1);
			if (ImGui::TreeNode(label.c_str()))
			{
				XMFLOAT3 pos = entities[i]->GetTransform()->GetPosition();
				XMFLOAT3 rot = entities[i]->GetTransform()->GetPitchYawRoll();
				XMFLOAT3 scale = entities[i]->GetTransform()->GetScale();
				if (ImGui::DragFloat3("Position", &pos.x))
				{
					entities[i]->GetTransform()->SetPosition(pos.x, pos.y, pos.z);
				}
				if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
				{
					entities[i]->GetTransform()->SetRotation(rot.x, rot.y, rot.z);
				}
				if (ImGui::DragFloat3("Scale", &scale.x, 0.05f))
				{
					entities[i]->GetTransform()->SetScale(scale.x, scale.y, scale.z);
				}

				int currentMat = -1;
				for (int j = 0; j < materials.size(); j++)
				{
					if (materials[j] == entities[i]->GetMaterial()) {

						currentMat = j;
					}
				}

				for (int j = 0; j < materials.size(); j++)
				{
					if (ImGui::RadioButton(materials[j]->GetName(), currentMat == j))
					{
						entities[i]->SetMaterial(materials[j]);
					}
					if ((j + 1) % 4 != 0)
						ImGui::SameLine();
				}
				ImGui::NewLine();
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	if (ImGui::CollapsingHeader("Materials")) {
		for (int i = 0; i < materials.size(); i++)
		{
			ImGui::PushID(i);
			std::string label = "Material " + std::to_string(i + 1);
			if (ImGui::TreeNode(label.c_str()))
			{
				XMFLOAT3 colorTint = materials[i]->GetColorTint();
				if (ImGui::ColorEdit3("Color Tint", &colorTint.x)) {
					materials[i]->SetColorTint(colorTint);
				}
				ImGui::Text("Albedo Map");
				ImGui::Image((void*)materials[i]->GetTextureSRV("Albedo").Get(), ImVec2(256, 256));
				ImGui::Text("Normal Map");
				ImGui::Image((void*)materials[i]->GetTextureSRV("NormalMap").Get(), ImVec2(256, 256));
				ImGui::Text("Roughness Map");
				ImGui::Image((void*)materials[i]->GetTextureSRV("RoughnessMap").Get(), ImVec2(256, 256));
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	if (ImGui::CollapsingHeader("SRV's")) {
		ImGui::Image((void*)sky->GetBrdfLookUp().Get(), ImVec2(256, 256));
		for (int i = 0; i < RENDER_TARGETS_COUNT; i++) {
			ImGui::Image((void*)renderTargetsSRV[i].Get(), ImVec2(256, 256));
		}
	}

	if (ImGui::CollapsingHeader("Motion Blur"))
	{
		ImGui::DragInt("Motion Blur Samples", &motionBlurNeighborhoodSamples, 1, 0, 64);
		ImGui::DragInt("Max Motion Blur", &motionBlurMax, 1, 0, 64);
		
	}
	ImGui::End();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::CreatePostProcessResources()
{
	for (int i = 0; i < RENDER_TARGETS_COUNT; i++)
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> rtvText;
		D3D11_TEXTURE2D_DESC texDesc = {};
		if (i != VELOCITY && i != NEIGHBORHOOD_MAX)
		{
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else
		{
			texDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		}
		texDesc.Width = windowWidth;
		texDesc.Height = windowHeight;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = 1;
		device->CreateTexture2D(&texDesc, 0, rtvText.GetAddressOf());

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Format = texDesc.Format;
		device->CreateRenderTargetView(rtvText.Get(), &rtvDesc, renderTargetsRTV[i].GetAddressOf());

		device->CreateShaderResourceView(rtvText.Get(), 0, renderTargetsSRV[i].GetAddressOf());
	}

}

void Renderer::DrawPointLights(std::shared_ptr<Camera> camera)
{
	Assets* instance = &Assets::GetInstance();
	shared_ptr<SimpleVertexShader> lightVS = instance->GetVertexShader("VertexShader");
	shared_ptr<SimplePixelShader> lightPS = instance->GetPixelShader("SolidColorPS");
	shared_ptr<Mesh> lightMesh = instance->GetMesh("Sphere");
	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", camera->GetView());
	lightPS->SetMatrix4x4("projection", camera->GetProjection());

	for (int i = 0; i < lights.size(); i++)
	{
		std::shared_ptr<Light> light = lights[i];

		// Only drawing points, so skip others
		if (light->GetType() != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		float scale = light->info.Range / 10.0f;

		// Set up the world matrix for this light
		Transform* lightTransform = light->GetTransform();
		lightTransform->SetScale(scale, scale, scale);
		lightVS->SetMatrix4x4("world", lightTransform->GetWorldMatrix());
		lightVS->SetMatrix4x4("worldInverseTranspose", lightTransform->GetWorldInverseTransposeMatrix());

		// Set up the pixel shader data
		XMFLOAT3 finalColor = light->info.Color;
		finalColor.x *= light->info.Intensity;
		finalColor.y *= light->info.Intensity;
		finalColor.z *= light->info.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
		lightTransform = nullptr;
	}
}
