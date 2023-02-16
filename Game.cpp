
#include <stdlib.h>     // For seeding random and rand()
#include <time.h>       // For grabbing time (to seed random)

#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Assets.h"
#include "Renderer.h"

#include "WICTextureLoader.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"


// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// Helper macros for making texture and shader loading code more succinct
#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) std::make_shared<type>(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str())


// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true),			   // Show extra stats (fps) in title bar?
	camera(0),
	sky(0),
	lightCount(0)
{
	// Seed random
	srand((unsigned int)time(0));

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
	
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	delete &Assets::GetInstance();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	Assets::GetInstance().Initialize("../../Assets/", device, context, true, true);
	Assets::GetInstance().LoadAllAssets();
	// Asset loading and entity creation
	LoadAssetsAndCreateEntities();
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set up lights initially
	lightCount = 128;
	GenerateLights();

	// Make our camera
	camera = std::make_shared<Camera>(
		0.0f, 0.0f, -10.0f,	// Position
		3.0f,		// Move speed
		1.0f,		// Mouse look
		this->width / (float)this->height); // Aspect ratio
	DXRenderer = std::make_unique<Renderer>(device, context, swapChain, backBufferRTV, depthStencilView, width, height, sky, entities, emitter, lights, hWnd);
}


// --------------------------------------------------------
// Load all assets and create materials, entities, etc.
// --------------------------------------------------------
void Game::LoadAssetsAndCreateEntities()
{
	Assets& instance = Assets::GetInstance();


	// Describe and create our sampler state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, samplerOptions.GetAddressOf());

	D3D11_SAMPLER_DESC clampDesc = {};
	clampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	clampDesc.MaxAnisotropy = 16;
	clampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, clampSamplerOptions.GetAddressOf());


	// Create the sky using 6 images
	sky = std::make_shared<Sky>(
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Clouds Blue\\right.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Clouds Blue\\left.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Clouds Blue\\up.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Clouds Blue\\down.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Clouds Blue\\front.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Clouds Blue\\back.png").c_str(),
		instance.GetMesh("cube"),
		instance.GetVertexShader("SkyVS"),
		instance.GetPixelShader("SkyPS"),
		samplerOptions,
		device,
		context);

	// Create PBR materials
	std::shared_ptr<Material> cobbleMat2xPBR = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Cobble2x PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	cobbleMat2xPBR->AddSampler("BasicSampler", samplerOptions);
	cobbleMat2xPBR->AddSampler("ClampSampler", clampSamplerOptions);
	cobbleMat2xPBR->AddTextureSRV("Albedo", instance.GetTexture("cobblestone_albedo"));
	cobbleMat2xPBR->AddTextureSRV("NormalMap", instance.GetTexture("cobblestone_normals"));
	cobbleMat2xPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("cobblestone_roughness"));
	cobbleMat2xPBR->AddTextureSRV("MetalMap", instance.GetTexture("cobblestone_metal"));
	materials.push_back(cobbleMat2xPBR);

	std::shared_ptr<Material> cobbleMat4xPBR = std::make_shared<Material>(instance.GetPixelShader("RefractionPS"), instance.GetVertexShader("VertexShader"), true, 0.3f, "Cobble4x PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(4, 4));
	cobbleMat4xPBR->AddSampler("BasicSampler", samplerOptions);
	cobbleMat4xPBR->AddSampler("ClampSampler", clampSamplerOptions);
	cobbleMat4xPBR->AddTextureSRV("Albedo", instance.GetTexture("cobblestone_albedo"));
	cobbleMat4xPBR->AddTextureSRV("NormalMap", instance.GetTexture("cobblestone_normals"));
	cobbleMat4xPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("cobblestone_roughness"));
	cobbleMat4xPBR->AddTextureSRV("MetalMap", instance.GetTexture("cobblestone_metal"));
	materials.push_back(cobbleMat4xPBR);

	std::shared_ptr<Material> floorMatPBR = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Floor PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	floorMatPBR->AddSampler("BasicSampler", samplerOptions);
	floorMatPBR->AddSampler("ClampSampler", clampSamplerOptions);
	floorMatPBR->AddTextureSRV("Albedo", instance.GetTexture("floor_albedo"));
	floorMatPBR->AddTextureSRV("NormalMap", instance.GetTexture("floor_normals"));
	floorMatPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("floor_roughness"));
	floorMatPBR->AddTextureSRV("MetalMap", instance.GetTexture("floor_metal"));
	materials.push_back(floorMatPBR);

	std::shared_ptr<Material> paintMatPBR = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Paint PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	paintMatPBR->AddSampler("BasicSampler", samplerOptions);
	paintMatPBR->AddSampler("ClampSampler", clampSamplerOptions);
	paintMatPBR->AddTextureSRV("Albedo", instance.GetTexture("paint_albedo"));
	paintMatPBR->AddTextureSRV("NormalMap", instance.GetTexture("floor_normals"));
	paintMatPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("floor_roughness"));
	paintMatPBR->AddTextureSRV("MetalMap", instance.GetTexture("floor_metal"));
	materials.push_back(paintMatPBR);

	std::shared_ptr<Material> scratchedMatPBR = std::make_shared<Material>(instance.GetPixelShader("RefractionPS"), instance.GetVertexShader("VertexShader"), true, 1.8f, "Scratched PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	scratchedMatPBR->AddSampler("BasicSampler", samplerOptions);
	scratchedMatPBR->AddSampler("ClampSampler", clampSamplerOptions);
	scratchedMatPBR->AddTextureSRV("Albedo", instance.GetTexture("scratched_albedo"));
	scratchedMatPBR->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	scratchedMatPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("scratched_roughness"));
	scratchedMatPBR->AddTextureSRV("MetalMap", instance.GetTexture("scratched_metal"));
	materials.push_back(scratchedMatPBR);

	std::shared_ptr<Material> bronzeMatPBR = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Bronze PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	bronzeMatPBR->AddSampler("BasicSampler", samplerOptions);
	bronzeMatPBR->AddSampler("ClampSampler", clampSamplerOptions);
	bronzeMatPBR->AddTextureSRV("Albedo", instance.GetTexture("bronze_albedo"));
	bronzeMatPBR->AddTextureSRV("NormalMap", instance.GetTexture("bronze_normals"));
	bronzeMatPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("bronze_roughness"));
	bronzeMatPBR->AddTextureSRV("MetalMap", instance.GetTexture("bronze_metal"));
	materials.push_back(bronzeMatPBR);

	std::shared_ptr<Material> roughMatPBR = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Rough PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	roughMatPBR->AddSampler("BasicSampler", samplerOptions);
	roughMatPBR->AddSampler("ClampSampler", clampSamplerOptions);
	roughMatPBR->AddTextureSRV("Albedo", instance.GetTexture("rough_albedo"));
	roughMatPBR->AddTextureSRV("NormalMap", instance.GetTexture("rough_normals"));
	roughMatPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("rough_roughness"));
	roughMatPBR->AddTextureSRV("MetalMap", instance.GetTexture("rough_metal"));
	materials.push_back(roughMatPBR);

	std::shared_ptr<Material> woodMatPBR = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Wood PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	woodMatPBR->AddSampler("BasicSampler", samplerOptions);
	woodMatPBR->AddSampler("ClampSampler", clampSamplerOptions);
	woodMatPBR->AddTextureSRV("Albedo", instance.GetTexture("wood_albedo"));
	woodMatPBR->AddTextureSRV("NormalMap", instance.GetTexture("wood_normals"));
	woodMatPBR->AddTextureSRV("RoughnessMap", instance.GetTexture("wood_roughness"));
	woodMatPBR->AddTextureSRV("MetalMap", instance.GetTexture("wood_metal"));
	materials.push_back(woodMatPBR);
	
	std::shared_ptr<Material> IBLTestMat1 = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Test PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	IBLTestMat1->AddSampler("BasicSampler", samplerOptions);
	IBLTestMat1->AddSampler("ClampSampler", clampSamplerOptions);
	IBLTestMat1->AddTextureSRV("Albedo", instance.GetTexture("white_albedo"));
	IBLTestMat1->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	IBLTestMat1->AddTextureSRV("RoughnessMap", instance.GetTexture("white_smooth"));
	IBLTestMat1->AddTextureSRV("MetalMap", instance.GetTexture("paint_metal"));
	materials.push_back(IBLTestMat1); 
	std::shared_ptr<Material> IBLTestMat2 = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Test 2 PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	IBLTestMat2->AddSampler("BasicSampler", samplerOptions);
	IBLTestMat2->AddSampler("ClampSampler", clampSamplerOptions);
	IBLTestMat2->AddTextureSRV("Albedo", instance.GetTexture("white_albedo"));
	IBLTestMat2->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	IBLTestMat2->AddTextureSRV("RoughnessMap", instance.GetTexture("white_matte"));
	IBLTestMat2->AddTextureSRV("MetalMap", instance.GetTexture("paint_metal"));
	materials.push_back(IBLTestMat2);
	std::shared_ptr<Material> IBLTestMat3 = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Test 3 PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	IBLTestMat3->AddSampler("BasicSampler", samplerOptions);
	IBLTestMat3->AddSampler("ClampSampler", clampSamplerOptions);
	IBLTestMat3->AddTextureSRV("Albedo", instance.GetTexture("white_albedo"));
	IBLTestMat3->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	IBLTestMat3->AddTextureSRV("RoughnessMap", instance.GetTexture("white_rough"));
	IBLTestMat3->AddTextureSRV("MetalMap", instance.GetTexture("paint_metal"));
	materials.push_back(IBLTestMat3);
	std::shared_ptr<Material> IBLTestMat4 = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Test 4 PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	IBLTestMat4->AddSampler("BasicSampler", samplerOptions);
	IBLTestMat4->AddSampler("ClampSampler", clampSamplerOptions);
	IBLTestMat4->AddTextureSRV("Albedo", instance.GetTexture("white_albedo"));
	IBLTestMat4->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	IBLTestMat4->AddTextureSRV("RoughnessMap", instance.GetTexture("white_smooth"));
	IBLTestMat4->AddTextureSRV("MetalMap", instance.GetTexture("bronze_metal"));
	materials.push_back(IBLTestMat4);
	std::shared_ptr<Material> IBLTestMat5 = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Test 5 PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	IBLTestMat5->AddSampler("BasicSampler", samplerOptions);
	IBLTestMat5->AddSampler("ClampSampler", clampSamplerOptions);
	IBLTestMat5->AddTextureSRV("Albedo", instance.GetTexture("white_albedo"));
	IBLTestMat5->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	IBLTestMat5->AddTextureSRV("RoughnessMap", instance.GetTexture("white_matte"));
	IBLTestMat5->AddTextureSRV("MetalMap", instance.GetTexture("bronze_metal"));
	materials.push_back(IBLTestMat5);
	std::shared_ptr<Material> IBLTestMat6 = std::make_shared<Material>(instance.GetPixelShader("PixelShaderPBR"), instance.GetVertexShader("VertexShader"), "Test 6 PBR", XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	IBLTestMat6->AddSampler("BasicSampler", samplerOptions);
	IBLTestMat6->AddSampler("ClampSampler", clampSamplerOptions);
	IBLTestMat6->AddTextureSRV("Albedo", instance.GetTexture("white_albedo"));
	IBLTestMat6->AddTextureSRV("NormalMap", instance.GetTexture("scratched_normals"));
	IBLTestMat6->AddTextureSRV("RoughnessMap", instance.GetTexture("white_rough"));
	IBLTestMat6->AddTextureSRV("MetalMap", instance.GetTexture("bronze_metal"));
	materials.push_back(IBLTestMat6);



	// === Create the PBR entities =====================================
	std::shared_ptr<GameEntity> cobSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), cobbleMat4xPBR);
	cobSpherePBR->GetTransform()->SetPosition(-6, 2, 0);

	std::shared_ptr<GameEntity> floorSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), floorMatPBR);
	floorSpherePBR->GetTransform()->SetPosition(-4, 2, 0);

	std::shared_ptr<GameEntity> paintSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), paintMatPBR);
	paintSpherePBR->GetTransform()->SetPosition(-2, 2, 0);

	std::shared_ptr<GameEntity> scratchSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), scratchedMatPBR);
	scratchSpherePBR->GetTransform()->SetPosition(0, 2, 0);

	std::shared_ptr<GameEntity> bronzeSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), bronzeMatPBR);
	bronzeSpherePBR->GetTransform()->SetPosition(2, 2, 0);

	std::shared_ptr<GameEntity> roughSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), roughMatPBR);
	roughSpherePBR->GetTransform()->SetPosition(4, 2, 0);

	std::shared_ptr<GameEntity> woodSpherePBR = std::make_shared<GameEntity>(instance.GetMesh("sphere"), woodMatPBR);
	woodSpherePBR->GetTransform()->SetPosition(6, 2, 0);

	entities.push_back(cobSpherePBR);
	entities.push_back(floorSpherePBR);
	entities.push_back(paintSpherePBR);
	entities.push_back(scratchSpherePBR);
	entities.push_back(bronzeSpherePBR);
	entities.push_back(roughSpherePBR);
	entities.push_back(woodSpherePBR);

	std::shared_ptr<GameEntity> iblTestSphere1 = std::make_shared<GameEntity>(instance.GetMesh("sphere"), IBLTestMat1);
	iblTestSphere1->GetTransform()->SetPosition(-2, -.5f, 0);
	iblTestSphere1->GetTransform()->SetScale(.5f, .5f, .5f);

	std::shared_ptr<GameEntity> iblTestSphere2 = std::make_shared<GameEntity>(instance.GetMesh("sphere"), IBLTestMat2);
	iblTestSphere2->GetTransform()->SetPosition(0, -.5f, 0);
	iblTestSphere2->GetTransform()->SetScale(.5f, .5f, .5f);

	std::shared_ptr<GameEntity> iblTestSphere3 = std::make_shared<GameEntity>(instance.GetMesh("sphere"), IBLTestMat3);
	iblTestSphere3->GetTransform()->SetPosition(2, -.5f, 0);
	iblTestSphere3->GetTransform()->SetScale(.5f, .5f, .5f);

	std::shared_ptr<GameEntity> iblTestSphere4 = std::make_shared<GameEntity>(instance.GetMesh("sphere"), IBLTestMat4);
	iblTestSphere4->GetTransform()->SetPosition(-2, .5f, 0);
	iblTestSphere4->GetTransform()->SetScale(.5f, .5f, .5f);

	std::shared_ptr<GameEntity> iblTestSphere5 = std::make_shared<GameEntity>(instance.GetMesh("sphere"), IBLTestMat5);
	iblTestSphere5->GetTransform()->SetPosition(0, .5f, 0);
	iblTestSphere5->GetTransform()->SetScale(.5f, .5f, .5f);

	std::shared_ptr<GameEntity> iblTestSphere6 = std::make_shared<GameEntity>(instance.GetMesh("sphere"), IBLTestMat6);
	iblTestSphere6->GetTransform()->SetPosition(2, .5f, 0);
	iblTestSphere6->GetTransform()->SetScale(.5f, .5f, .5f);

	entities.push_back(iblTestSphere1);
	entities.push_back(iblTestSphere2);
	entities.push_back(iblTestSphere3);
	entities.push_back(iblTestSphere4);
	entities.push_back(iblTestSphere5);
	entities.push_back(iblTestSphere6);

	entities[1]->GetTransform()->MoveAbsolute(0, 2, 0);
	entities[2]->GetTransform()->MoveAbsolute(0, 4, 0);
	entities[4]->GetTransform()->MoveAbsolute(0, 6, 0);
	//Creating Emitters

	std::shared_ptr<Emitter> testEmitter = std::make_shared<Emitter>(50, 2, 2.5f, context, device, instance.GetTexture("circle_01"), instance.GetVertexShader("ParticleVS"), instance.GetPixelShader("ParticlePS"));
	testEmitter->SetColor(XMFLOAT4(0, 0, .5f, 1.0f), XMFLOAT4(0, .5f, 0, 0.0f));
	testEmitter->SetScale(XMFLOAT2(0.1f, 0.1f), XMFLOAT2(1.0f, 1.0f));
	testEmitter->GetTransform()->MoveAbsolute(0, -5, 0);

	std::shared_ptr<Emitter> testEmitter2 = std::make_shared<Emitter>(400, 75, 4, context, device, instance.GetTexture("star_06"), instance.GetVertexShader("ParticleVS"), instance.GetPixelShader("ParticlePS"));
	testEmitter2->SetAcceleration(XMFLOAT3(0.0f, -3.0f, 0.0f));
	testEmitter2->SetStartingVelocity(XMFLOAT3(2.0f, 4.0f, 0.0f));
	testEmitter2->SetColor(XMFLOAT4(1, 1, 1, 1.0f), XMFLOAT4(.5f, .5f, 0, 0.2f));
	testEmitter2->SetScale(XMFLOAT2(0.05f, 0.05f), XMFLOAT2(0.1f, 0.1f));
	testEmitter2->SetVelocityRange(XMFLOAT3(.5f, .5f, 0));
	testEmitter2->GetTransform()->MoveAbsolute(2.5f, -5, 0);

	std::shared_ptr<Emitter> testEmitter3 = std::make_shared<Emitter>(50, 3, 2.5f, context, device, instance.GetTexture("smoke_01"), instance.GetVertexShader("ParticleVS"), instance.GetPixelShader("ParticlePS"));
	testEmitter3->SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT4(0, 0.0f, 0, 1.0f));
	testEmitter3->SetScale(XMFLOAT2(0.1f, 0.1f), XMFLOAT2(3.0f, 3.0f));
	testEmitter3->GetTransform()->MoveAbsolute(-2.5f, -5, 0);

	emitter.push_back(testEmitter);
	emitter.push_back(testEmitter2);
	emitter.push_back(testEmitter3);
}


// --------------------------------------------------------
// Generates the lights in the scene: 3 directional lights
// and many random point lights.
// --------------------------------------------------------
void Game::GenerateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(1, -1, 1);
	dir1.Color = XMFLOAT3(0.8f, 0.8f, 0.8f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	// Create the rest of the lights
	//Create new lights for deferred rendering
	while (lights.size() < lightCount)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10.0f, 10.0f), RandomRange(-5.0f, 5.0f), RandomRange(-10.0f, 10.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);

		// Add to the list
		lights.push_back(point);
	}

}



// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	DXRenderer->PreResize();

	// Handle base-level DX resize stuff
	DXCore::OnResize();

	DXRenderer->PostResize(width, height, backBufferRTV, depthStencilView);
	// Update our projection matrix to match the new aspect ratio
	if (camera)
		camera->UpdateProjectionMatrix(this->width / (float)this->height);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Update the camera
	camera->Update(deltaTime);

	entities[0]->GetTransform()->Rotate(0, deltaTime, 0);
	entities[3]->GetTransform()->Rotate(0, deltaTime, 0);
	if (entities[1]->GetTransform()->GetPosition().x > 5 && entityDirection == 1)
		entityDirection = -1;
	else if (entities[1]->GetTransform()->GetPosition().x < -5 && entityDirection == -1)
		entityDirection = 1;
	entities[1]->GetTransform()->MoveAbsolute(deltaTime * 15 * entityDirection, 0, 0);
	entities[2]->GetTransform()->MoveAbsolute(deltaTime * 10 * entityDirection, 0, 0);
	entities[4]->GetTransform()->MoveAbsolute(deltaTime * 5 * entityDirection, 0, 0);
	
	// Check individual input
	Input& input = Input::GetInstance();
	if (input.KeyDown(VK_ESCAPE)) Quit();
	if (input.KeyPress(VK_TAB)) GenerateLights();


	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->width;
	io.DisplaySize.y = (float)this->height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//Determine new input capture
	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);

	for (auto e : emitter)
	{
		e->Update(deltaTime);
	}
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	DXRenderer->Render(camera, materials, deltaTime);
}
