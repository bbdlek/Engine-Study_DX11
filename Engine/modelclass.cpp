#include "modelclass.h"

ModelClass::ModelClass()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
}

ModelClass::ModelClass(const ModelClass&)
{
}

ModelClass::~ModelClass()
{
}

// 꼭짓점 및 인덱스 버퍼에 대한 초기화 함수를 호출
bool ModelClass::Initialize(ID3D11Device* device)
{
	bool result;


	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	return true;
}

void ModelClass::Shutdown()
{
	// Shutdown the vertex and index buffers.
	ShutdownBuffers();

	return;
}

// Render 함수는 GraphicsClass::Render 함수에서 호출됨
// 이 함수에서는 RenderBuffers 함수를 호출하여 정점 버퍼와 인덱스 버퍼를 그래픽 파이프라인에 넣어 컬러 셰이더가 이들을 그릴 수 있도록 함
void ModelClass::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);

	return;
}

// 해당 모델의 인덱스 수 반환
// 컬러 셰이더에서 모델을 그리기 위해서는 이 정보가 필요
int ModelClass::GetIndexCount()
{
	return m_indexCount;
}

// 정점 버퍼와 인덱스 버퍼를 생성하는 작업을 제어
// 보통 이 부분에서는 데이터 파일로부터 모델의 정보를 읽어 와서 버퍼들을 만드는 일을 함
// 이 튜토리얼에서는 삼각형 하나만을 다루기 때문에 간단히 정점 버퍼와 인덱스 버퍼에 점을 세팅하는 일만을 함
bool ModelClass::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;

	// 우선, 정점과 인덱스 데이터를 담아둘 두 개의 임시 배열을 만들고 나중에 최종 버퍼를 생성할 때 사용하도록 함
	// Set the number of vertices in the vertex array.
	m_vertexCount = 3;

	// Set the number of indices in the index array.
	m_indexCount = 3;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}
	
	// 정점/인덱스 배열에 삼각형의 각 점과 그 순서를 채워넣음
	// 반드시 주의해야 할 점은, 이것을 그리기 위해서는 점들을 시계 방향으로 만들어야 한다는 것
	// 만약 반시계 방향으로 만들게 되면 DirectX에서 이 삼각형은 반대편을 바라본다고 판단하며 backface culling에 의해 그려지지 않게 됨
	// 따라서 GPU에게 도형을 그리도록 할 때 이 순서를 기억하는 것이 중요
	// 여기서 정점의 description을 작성하기 때문에 색상 역시 정해주게 됨. 여기서는 녹색

	// 정점 배열에 값을 넣습니다.
	vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // 왼쪽 아래
	vertices[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // 상단 가운데
	vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // 오른쪽 아래
	vertices[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	// 인덱스 배열에 값을 넣습니다.
	indices[0] = 0;  // 왼쪽 아래
	indices[1] = 1;  // 상단 가운데
	indices[2] = 2;  // 오른쪽 아래

	// 정점 배열과 인덱스 배열이 채워졌으므로 이를 이용하여 정점 버퍼와 인덱스 버퍼를 만듦
	// 두 버퍼를 만드는 일은 비슷한 과정을 거치게 됨
	// 우선 버퍼에 대한 description을 작성
	// 이 description에는 ByteWidth(버퍼의 크기)와 BindFlags(버퍼의 타입)을 정확히 입력해야 함
	// 이를 채워넣은 이후에는 방금 만들었던 정점 배열과 인덱스 배열을 subresource 포인터에 연결
	// 이 descrition과 subresource 포인터, 그리고 D3D 디바이스의 CreateBuffer 함수를 사용하여 새 버퍼의 포인터를 받아옴

	// 정점 버퍼의 description을 작성합니다.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// 정점 데이터를 가리키는 보조 리소스 구조체를 작성합니다.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// 정점 버퍼를 생성합니다.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// 인덱스 버퍼의 description을 작성합니다.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// 인덱스 데이터를 가리키는 보조 리소스 구조체를 작성합니다.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// 인덱스 버퍼를 생성합니다.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// 정점 버퍼와 인덱스 버퍼를 만든 후에는 이미 값이 복사되어 필요가 없어진 정점 배열과 인덱스 배열을 제거
	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

// InitializeBuffers 함수에서 만든 꼭짓점 버퍼 및 인덱스 버퍼를 해제
void ModelClass::ShutdownBuffers()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

// RenderBuffers 함수는 Render 함수에서 호출
// 이 함수의 목적은 바로 정점 버퍼와 인덱스 버퍼를 GPU의 어셈블러의 버퍼로서 활성화시키는 것
// 일단 GPU가 활성화된 정점 버퍼를 가지게 되면 셰이더를 이용하여 버퍼의 내용을 그릴 수 있게 됨
// 또한 이 함수에서는 이 정점을 삼각형이나 선분, 부채꼴 등 어떤 모양으로 그리게 될지 정의
// 이 튜토리얼에서는 어셈블러의 입력에 정점 버퍼와 인덱스 버퍼를 넣고 DirectX의 IASetPrimitiveTopology 함수를 사용하여 GPU에게 이 정점들을 삼각형으로 그리도록 주문
void ModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// 정점 버퍼의 단위와 오프셋을 설정합니다.
	stride = sizeof(VertexType);
	offset = 0;

	// input assembler에 정점 버퍼를 활성화하여 그려질 수 있게 합니다.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// input assembler에 인덱스 버퍼를 활성화하여 그려질 수 있게 합니다.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// 정점 버퍼로 그릴 기본형을 설정합니다. 여기서는 삼각형입니다.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}
