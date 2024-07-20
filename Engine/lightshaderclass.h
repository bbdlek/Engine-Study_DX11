#ifndef _LIGHTSHADERCLASS_H_
#define _LIGHTSHADERCLASS_H_

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <fstream>
using namespace DirectX;
using namespace std;

class LightShaderClass
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};
	
	// LightBufferType은 조명 정보를 저장하기 위해 사용됨
	// 이 typedef문은 픽셀 셰이더의 typedef문과 그 내용이 동일함
	// 참고로 구조체의 크기가 16의 배수가 되게 하기 위해 마지막에 추가로 사용되지 않는 float를 선언함
	// 만약 저 추가된 변수가 없이 sizeof(LightBufferType)가 28바이트인 크기로 CreateBuffer를 하면 이 함수에서는 무조건 16 배수 크기를 요구하기 때문에 실패함

	struct LightBufferType
	{
		XMFLOAT4 diffuseColor;
		XMFLOAT3 lightDirection;
		float padding;
	};

public:
	LightShaderClass();
	LightShaderClass(const LightShaderClass&);
	~LightShaderClass();

	bool Initialize(ID3D11Device*, HWND);
	void Shutdown();
	bool Render(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4);

private:
	bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

	bool SetShaderParameters(ID3D11DeviceContext*, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4);
	void RenderShader(ID3D11DeviceContext*, int);

private:
	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleState;
	ID3D11Buffer* m_matrixBuffer;

	ID3D11Buffer* m_lightBuffer;

};

#endif