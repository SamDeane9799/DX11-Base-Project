#include "Assets.h"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

using namespace DirectX;
using namespace std;

Assets* Assets::instance;

Assets::~Assets()
{

}

void Assets::Initialize(std::string rootAssetPath, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, bool allowOnDemandLoading, bool printLoadingProgress)
{
	this->rootAssetPath = rootAssetPath;
	this->device = device;
	this->context = context;
	this->allowOnDemandLoading = allowOnDemandLoading;
	this->printLoadingProgress = printLoadingProgress;

	replace(this->rootAssetPath.begin(), this->rootAssetPath.end(), '\\', '/');

	if (!EndsWith(rootAssetPath, "/"))
		rootAssetPath += "/";
}

void Assets::LoadAllAssets()
{
	if (rootAssetPath.empty())
		return;

	for (auto& item : experimental::filesystem::recursive_directory_iterator(GetFullPathTo(rootAssetPath))) {

		if (item.status().type() == experimental::filesystem::file_type::regular)
		{
			string itemPath = item.path().string();

			replace(itemPath.begin(), itemPath.end(), '\\', '/');

			if (EndsWith(itemPath, ".obj"))
			{
				LoadMesh(itemPath, RemoveFileExtension(item.path().filename().string()));
			}
			if (EndsWith(itemPath, ".jpg") || EndsWith(itemPath, ".png") || EndsWith(itemPath, ".tif"))
			{
				LoadTexture(itemPath, RemoveFileExtension(item.path().filename().string()));
			}
			if (EndsWith(itemPath, ".dds"))
			{
				LoadDDSTexture(itemPath, RemoveFileExtension(item.path().filename().string()));
			}
			if (EndsWith(itemPath, ".spritefont"))
			{
				LoadSpriteFont(itemPath, RemoveFileExtension(item.path().filename().string()));
			}
		}
	}

	for (auto& item : experimental::filesystem::recursive_directory_iterator(GetFullPathTo(".")))
	{
		std::string itemPath = item.path().filename().string();

		if (EndsWith(itemPath, ".cso"))
		{
			LoadUnknownShader(itemPath, item.path().filename().string());
		}
	}
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Assets::CreateSolidColorTexture(std::string textureName, int width, int height, DirectX::XMFLOAT4 color)
{
	return Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Assets::CreateTexture(std::string textureName, int width, int height, DirectX::XMFLOAT4* pixels)
{
	return Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Assets::CreateFloatTexture(std::string textureName, int width, int height, DirectX::XMFLOAT4* pixels)
{
	return Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>();
}

std::shared_ptr<Mesh> Assets::GetMesh(std::string name)
{
	auto it = meshes.find(name);
	if (it != meshes.end())
		return it->second;

	if (allowOnDemandLoading)
	{
		string path = GetFullPathTo(rootAssetPath + name + ".obj");
		if (experimental::filesystem::exists(path))
			return LoadMesh(path, name + ".obj");
		else
		{
			for (const auto& entry : experimental::filesystem::directory_iterator(GetFullPathTo(rootAssetPath)))
			{
				if (experimental::filesystem::is_directory(entry.status()))
				{
					path = entry.path().string() + "\\" + name + ".obj";
					if (experimental::filesystem::exists(path))
					{
						return LoadMesh(path, name + ".obj");
					}
				}
			}
		}
	}

	return 0;
}

std::shared_ptr<DirectX::SpriteFont> Assets::GetSpriteFont(const std::string name)
{
	auto it = spriteFonts.find(name);
	if (it != spriteFonts.end())
		return it->second;

	if (allowOnDemandLoading)
	{
		for (const auto& entry : experimental::filesystem::directory_iterator(GetFullPathTo(rootAssetPath)))
		{
			if (experimental::filesystem::is_directory(entry.status()))
			{
				string path = entry.path().string() + "\\" + name + ".spritefont";
				if(experimental::filesystem::exists(path))
					return LoadSpriteFont(path, name + ".spritefont");
			}
		}
	}

	return 0;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Assets::GetTexture(std::string name)
{
	auto it = textures.find(name);
	if (it != textures.end())
		return it->second;

	if (allowOnDemandLoading)
	{
		for (const auto& entry : experimental::filesystem::directory_iterator(GetFullPathTo(rootAssetPath)))
		{
			if (experimental::filesystem::is_directory(entry.status()))
			{
				string pathJPG = entry.path().string() + "\\" + name + ".jpg";
				if (experimental::filesystem::exists(pathJPG))
					return LoadTexture(pathJPG, name + ".jpg");

				string pathPNG = entry.path().string() + "\\" + name + ".png";
				if (experimental::filesystem::exists(pathPNG))
					return LoadTexture(pathPNG, name + ".png");

				string pathTIF = entry.path().string() + "\\" + name + ".tif";
				if (experimental::filesystem::exists(pathTIF))
					return LoadTexture(pathTIF, name + ".tif");

				string pathDDS = entry.path().string() + "\\" + name + ".dds";
				if (experimental::filesystem::exists(pathDDS))
					return LoadDDSTexture(pathDDS, name + ".dds");
			}
		}

	}

	return 0;
}

std::shared_ptr<SimplePixelShader> Assets::GetPixelShader(std::string name)
{
	auto it = pixelShaders.find(name);
	if (it != pixelShaders.end())
		return it->second;

	if (allowOnDemandLoading)
	{
		string psFileName = name + ".cso";
		if (experimental::filesystem::exists(GetFullPathTo(psFileName)))
		{
			shared_ptr<SimplePixelShader> ps = LoadPixelShader(psFileName);
			if (ps) { return ps; }
		}
	}

	return 0;
}

std::shared_ptr<SimpleVertexShader> Assets::GetVertexShader(std::string name)
{
	auto it = vertexShaders.find(name);
	if (it != vertexShaders.end())
		return it->second;

	if (allowOnDemandLoading)
	{
		string vsFileName = name + ".cso";
		if (experimental::filesystem::exists(GetFullPathTo(vsFileName)))
		{
			shared_ptr<SimpleVertexShader> vs = LoadVertexShader(vsFileName);
			if (vs) { return vs; }
		}
	}

	return 0;
}

void Assets::AddMesh(std::string name, std::shared_ptr<Mesh> mesh)
{
	meshes.insert({ name, mesh });
}

void Assets::AddSpriteFont(std::string name, std::shared_ptr<DirectX::SpriteFont> sprite)
{
	spriteFonts.insert({ name, sprite });
}

void Assets::AddTexture(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture)
{
	textures.insert({ name, texture });
}

void Assets::AddPixelShader(std::string name, std::shared_ptr<SimplePixelShader> PSShader)
{
	pixelShaders.insert({ name, PSShader });
}

void Assets::AddVertexShader(std::string name, std::shared_ptr<SimpleVertexShader> VSShader)
{
	vertexShaders.insert({ name, VSShader });
}

unsigned int Assets::GetMeshCount()
{
	return (unsigned int)meshes.size();
}

unsigned int Assets::GetSpriteFontCount()
{
	return (unsigned int)spriteFonts.size();
}

unsigned int Assets::GetTextureCount()
{
	return (unsigned int)textures.size();
}

unsigned int Assets::GetPixelShaderCount()
{
	return (unsigned int)pixelShaders.size();
}

unsigned int Assets::GetVertexShaderCount()
{
	return (unsigned int)vertexShaders.size();
}

std::shared_ptr<Mesh> Assets::LoadMesh(std::string path, std::string filename)
{
	if (printLoadingProgress) {
		printf("Loading Mesh: ");
		printf(filename.c_str());
		printf("\n");
	}

	std::shared_ptr<Mesh> newMesh = make_shared<Mesh>(path.c_str(), device);

	meshes.insert({ RemoveFileExtension(filename), newMesh });
	return newMesh;
}

std::shared_ptr<DirectX::SpriteFont> Assets::LoadSpriteFont(std::string path, std::string filename)
{
	if (printLoadingProgress) {
		printf("Loading Spritefont: ");
		printf(filename.c_str());
		printf("\n");
	}

	std::shared_ptr<SpriteFont> newSpriteFont = make_shared<SpriteFont>(device.Get(), ToWideString(path).c_str());

	spriteFonts.insert({ RemoveFileExtension(filename), newSpriteFont });

	return newSpriteFont;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Assets::LoadTexture(std::string path, std::string filename)
{

	if (printLoadingProgress)
	{
		printf("Loading Texture: ");
		printf(filename.c_str());
		printf("\n");
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newTexture;
	CreateWICTextureFromFile(device.Get(), context.Get(), ToWideString(path).c_str(), 0, newTexture.GetAddressOf());

	textures.insert({ RemoveFileExtension(filename), newTexture });

	return newTexture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Assets::LoadDDSTexture(std::string path, std::string filename)
{
	if (printLoadingProgress)
	{
		printf("Loading DDSTexture: ");
		printf(filename.c_str());
		printf("\n");
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newDDSTexture;
	CreateDDSTextureFromFile(device.Get(), context.Get(), ToWideString(path).c_str(), 0, newDDSTexture.GetAddressOf());

	textures.insert({ RemoveFileExtension(filename), newDDSTexture });
	return newDDSTexture;
}

void Assets::LoadUnknownShader(std::string path, std::string filename)
{

	ID3DBlob* shaderBlob;
	HRESULT hr = D3DReadFileToBlob(GetFullPathTo_Wide(ToWideString(path)).c_str(), &shaderBlob);
	if (hr != S_OK)
	{
		return;
	}

	ID3D11ShaderReflection* refl;
	D3DReflect(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&refl);

	D3D11_SHADER_DESC shaderDesc;
	refl->GetDesc(&shaderDesc);

	switch (D3D11_SHVER_GET_TYPE(shaderDesc.Version)) {
	case D3D11_SHVER_VERTEX_SHADER: LoadVertexShader(path); break;
	case D3D11_SHVER_PIXEL_SHADER: LoadPixelShader(path); break;
	}

	refl->Release();
	shaderBlob->Release();
}

std::shared_ptr<SimplePixelShader> Assets::LoadPixelShader(std::string file)
{
	if (printLoadingProgress)
	{
		printf("Loading Pixel Shader: ");
		printf(file.c_str());
		printf("\n");
	}

	shared_ptr<SimplePixelShader> newPixelShader;
	newPixelShader = make_shared<SimplePixelShader>(device.Get(), context.Get(), GetFullPathTo_Wide(ToWideString(file)).c_str());
	if (!newPixelShader->IsShaderValid()) { return 0; }

	pixelShaders.insert({ RemoveFileExtension(file), newPixelShader });
	return newPixelShader;
}

std::shared_ptr<SimpleVertexShader> Assets::LoadVertexShader(std::string file)
{
	if (printLoadingProgress)
	{
		printf("Loading Vertex Shader: ");
		printf(file.c_str());
		printf("\n");
	}

	shared_ptr<SimpleVertexShader> newVertexShader;
	newVertexShader = make_shared<SimpleVertexShader>(device.Get(), context.Get(), GetFullPathTo_Wide(ToWideString(file)).c_str());
	if (!newVertexShader->IsShaderValid()) { return 0; }

	vertexShaders.insert({ RemoveFileExtension(file), newVertexShader });
	return newVertexShader;
}

std::string Assets::GetExePath()
{
	string path = ".\\";

	char currentDir[1024] = {};
	GetModuleFileName(0, currentDir, 1024);

	char* lastSlash = strrchr(currentDir, '\\');
	if (lastSlash)
	{
		*lastSlash = 0;
		path = currentDir;
	}

	return path;
}

std::wstring Assets::GetExePath_Wide()
{
	string path = GetExePath();

	wchar_t widePath[1024] = {};
	mbstowcs_s(0, widePath, path.c_str(), 1024);

	return wstring(widePath);
}

std::string Assets::GetFullPathTo(std::string relativeFilePath)
{
	return GetExePath() + '\\' + relativeFilePath;
}

std::wstring Assets::GetFullPathTo_Wide(std::wstring relativeFilePath)
{
	return GetExePath_Wide() + L"\\" + relativeFilePath;
}

bool Assets::EndsWith(std::string str, std::string ending)
{
	return equal(ending.rbegin(), ending.rend(), str.rbegin());
}

std::wstring Assets::ToWideString(std::string str)
{
	wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

std::string Assets::RemoveFileExtension(std::string str)
{
	size_t found = str.find_last_of('.');
	return str.substr(0, found);
}
