#pragma once
#include <d3d11.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <WICTextureLoader.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <SpriteFont.h>

#include "Mesh.h"
#include "SimpleShader.h"


class Assets
{
#pragma region Singleton
public:
	static Assets& GetInstance() {
		if (!instance) {
			instance = new Assets();
		}

		return *instance;
	}

	Assets(Assets const&) = delete;
	void operator=(Assets const&) = delete;

private:
	static Assets* instance;
	Assets() :
		allowOnDemandLoading(true),
		printLoadingProgress(false) {};
#pragma endregion
public:
	~Assets();

	void Initialize(
		std::string rootAssetPath,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		bool allowOnDemandLoading = false,
		bool printLoadingProgress = true);

	void LoadAllAssets();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateSolidColorTexture(std::string textureName, int width, int height, DirectX::XMFLOAT4 color);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateTexture(std::string textureName, int width, int height, DirectX::XMFLOAT4* pixels);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateFloatTexture(std::string textureName, int width, int height, DirectX::XMFLOAT4* pixels);

	std::shared_ptr<Mesh> GetMesh(std::string name);
	std::shared_ptr<DirectX::SpriteFont> GetSpriteFont(const std::string name);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture(std::string name);
	std::shared_ptr<SimplePixelShader> GetPixelShader(std::string name);
	std::shared_ptr<SimpleVertexShader> GetVertexShader(std::string name);

	void AddMesh(std::string name, std::shared_ptr<Mesh> mesh);
	void AddSpriteFont(std::string name, std::shared_ptr<DirectX::SpriteFont> sprite);
	void AddTexture(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture);
	void AddPixelShader(std::string name, std::shared_ptr<SimplePixelShader> PSShader);
	void AddVertexShader(std::string name, std::shared_ptr<SimpleVertexShader> VSShader);

	unsigned int GetMeshCount();
	unsigned int GetSpriteFontCount();
	unsigned int GetTextureCount();
	unsigned int GetPixelShaderCount();
	unsigned int GetVertexShaderCount();

private:
	std::shared_ptr<Mesh> LoadMesh(std::string path, std::string filename);
	std::shared_ptr<DirectX::SpriteFont> LoadSpriteFont(std::string path, std::string filename);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadTexture(std::string path, std::string filename);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadDDSTexture(std::string path, std::string filename);
	void LoadUnknownShader(std::string path, std::string filename);
	std::shared_ptr<SimplePixelShader> LoadPixelShader(std::string file);
	std::shared_ptr<SimpleVertexShader> LoadVertexShader(std::string file);

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	std::string rootAssetPath;
	bool printLoadingProgress;

	bool allowOnDemandLoading;

	std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
	std::unordered_map < std::string, std::shared_ptr < DirectX::SpriteFont>> spriteFonts;
	std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>> pixelShaders;
	std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>> vertexShaders;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;

	std::string GetExePath();
	std::wstring GetExePath_Wide();

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);

	bool EndsWith(std::string str, std::string ending);
	std::wstring ToWideString(std::string str);
	std::string RemoveFileExtension(std::string str);
};

