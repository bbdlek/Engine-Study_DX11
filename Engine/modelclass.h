#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_

#include <d3d11.h>
#include <DirectXMath.h>

#include "textureclass.h"

using namespace DirectX;

class ModelClass
{
private:
	// VertexType는 색상과 관련된 요소가 텍스쳐 좌표로 대체
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};
	/*struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};*/

public:
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*);
	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();

	// ModelClass 역시 셰이더에게 자신의 텍스쳐 자원을 전달하고 그리기 위한 GetTexture 함수를 갖고 있음
	ID3D11ShaderResourceView* GetTexture();

private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);

	// ModelClass는 private 함수로 이 모델을 그릴 텍스쳐를 불러오고 반환하는 데 사용할 LoadTexture과 ReleaseTexture 함수를 가지고 있음
	bool LoadTexture(ID3D11Device*, ID3D11DeviceContext*, char*);
	void ReleaseTexture();

private:
	// 꼭짓점 및 인덱스 버퍼와 각 버퍼의 크기를 추적하기 위한 두 개의 정수
	ID3D11Buffer* m_vertexBuffer, * m_indexBuffer;
	int m_vertexCount, m_indexCount;

	// 변수 m_Texture은 이 모델의 텍스쳐 자원을 불러오고, 반환하고, 접근하는 데 사용
	TextureClass* m_Texture;
};

#endif // !_MODELCLASS_H_
