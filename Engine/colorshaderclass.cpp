#include "colorshaderclass.h"

ColorShaderClass::ColorShaderClass()
{
	m_vertexShader = 0;
	m_pixelShader = 0;
	m_layout = 0;
	m_matrixBuffer = 0;
}

ColorShaderClass::ColorShaderClass(const ColorShaderClass&)
{
}

ColorShaderClass::~ColorShaderClass()
{
}

// Initialize 함수는 셰이더에 대한 초기화 함수를 호출합니다
// HLSL 셰이더 파일의 이름을 전달하며, 이 자습서에서는 color.vs 및 color.ps 로 명명됩니다.
bool ColorShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
	bool result;
	wchar_t vsFilename[128];
	wchar_t psFilename[128];
	int error;

	// Set the filename of the vertex shader
	error = wcscpy_s(vsFilename, 128, L"../Engine/color.vs");
	if (error != 0)
	{
		return false;
	}

	// Set the filename of the pixel shader
	error = wcscpy_s(psFilename, 128, L"../Engine/color.ps");
	if (error != 0)
	{
		return false;
	}

	// Initialize the vertex and pixel shaders
	result = InitializeShader(device, hwnd, vsFilename, psFilename);
	if (!result)
	{
		return false;
	}

	return true;
}

void ColorShaderClass::Shutdown()
{
	// Shutdown the vertex and pixel shaders as well as the related objects
	ShutdownShader();

	return;
}

// Render는 먼저 SetShaderParameters 함수를 사용하여 셰이더 내부의 파라미터를 설정합니다.
// 파라미터가 설정되면 RenderShader를 호출하여 HLSL 셰이더를 사용하여 녹색 삼각형을 그립니다.
bool ColorShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix)
{
	bool result;

	// Set the shader parameters that it will use for rendering
	result = SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projectionMatrix);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader
	RenderShader(deviceContext, indexCount);

	return true;
}

// 이 함수에서 실제로 셰이더 파일을 불러오고 DirectX와 GPU에서 사용 가능하도록 하는 일을 함
// 또한 레이아웃을 세팅하고 어떻게 정점 버퍼의 데이터가 GPU에서 사용되는지 볼 수 있음

bool ColorShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage; // ID3D10Blob 은 임의의 길이의 데이터를 반환하는 인터페이스.
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;

	// Initialize the pointers this function will use to null
	errorMessage = 0;
	vertexShaderBuffer = 0;
	pixelShaderBuffer = 0;

	// 여기서 셰이더 프로그램을 버퍼로 컴파일
	// 셰이더 파일의 이름, 셰이더의 이름, 셰이더의 버전(DirectX 11에서는 5.0), 그리고 셰이더가 컴파일될 버퍼를 인자로 넘겨줌
	// Compile the vertex shader code
	result = D3DCompileFromFile(vsFilename, NULL, NULL, "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Compile the pixel shader code.
	result = D3DCompileFromFile(psFilename, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else
		{
			MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// 정점 셰이더와 픽셀 셰이더가 버퍼로 잘 컴파일되면 이를 이용하여 셰이더 객체를 만들 수 있음
	// 여기서 나온 포인터를 정점 셰이더와 픽셀 셰이더의 인터페이스로서 사용함
	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
	if (FAILED(result))
	{
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
	if (FAILED(result))
	{
		return false;
	}

	// 다음 과정은 셰이더에서 사용할 정점 데이터의 레이아웃을 생성
	// 이 셰이더에서는 위치 벡터와 색상 벡터를 사용하므로 레이아웃에 각각의 벡터의 크기를 포함하는 두 레이아웃을 만듦
	// SemanticName은 이 요소가 레이아웃에서 어떻게 사용되는지 알려주므로 레이아웃에서 가장 먼저 채워져야 할 항목
	// 우선 두 다른 요소들 중에서 POSITION을 먼저, COLOR를 두번째로 처리

	// 그 다음으로 레이아웃에서 중요한 부분은 Format
	// 위치 벡터에 대해서는 DXGI_FORMAT_R32G32B32_FLOAT를 사용하고 색상 벡터에 대해서는 DXGI_FORMAT_R32G32B32A32_FLOAT를 사용함

	// 마지막으로 주의를 기울여야 할 것은 버퍼에 데이터가 어떻게 배열되는지 알려주는 AlignedByteOffset
	// 이 레이아웃에서는 처음 12 byte를 위치 벡터에 사용하고 다음 16 byte를 색상으로 사용할 것임을 알려줘야 하는데, AlignedByteOffset이 각 요소가 어디서 시작하는지 보여줌
	// 여기서 직접 값을 입력하기보다 D3D11_APPEND_ALIGNED_ELEMENT 를 지정하여 자동으로 알아내도록 함

	// 나머지는 기본값
	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	// 레이아웃의 description이 채워지면 이것의 크기를 가지고 D3D 디바이스를 사용하여 입력 레이아웃 생성
	// 레이아웃이 생성되면 정점/픽셀 셰이더 버퍼들은 더 이상 사용되지 않으므로 할당 해제
	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &m_layout);
	if (FAILED(result))
	{
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	// 셰이더를 사용하기 위한 마지막 단계는 상수 버퍼(constant buffer)
	// 정점 셰이더에서 보았던 것처럼 일단 지금은 단 하나의 상수 버퍼를 가지고 있기 때문에 여기서 그것을 세팅하여 셰이더에 대한 인터페이스를 사용할 수 있음
	// 매 프레임마다 상수 버퍼를 업데이트하기 때문에 버퍼의 사용은 동적이 될 필요가 있음
	// 바로 BindFlags로 상수 버퍼를 이 버퍼로 한다는 것을 설정
	// CPUAccessFlags도 용도에 맞추어야 하기 때문에 D3D11_CPU_ACCESS_WRITE로 설정
	// 이 description이 채워지면 상수 버퍼의 인터페이스를 만들고 이와 SetShaderParameters 함수를 이용하여 셰이더의 내부 변수들에 접근할 수 있도록 함
	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

void ColorShaderClass::ShutdownShader()
{
	// Release the matrix constant buffer.
	if (m_matrixBuffer)
	{
		m_matrixBuffer->Release();
		m_matrixBuffer = 0;
	}

	// Release the layout.
	if (m_layout)
	{
		m_layout->Release();
		m_layout = 0;
	}

	// Release the pixel shader.
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = 0;
	}

	// Release the vertex shader.
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = 0;
	}

	return;
}

void ColorShaderClass::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
	char* compileErrors;
	unsigned long long bufferSize, i;
	ofstream fout;


	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	fout.open("shader-error.txt");

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		fout << compileErrors[i];
	}

	// Close the file.
	fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

	return;
}

// SetShaderVariables 함수는 셰이더의 전역 변수를 쉽게 다룰 수 있도록 하기 위해 만들어짐
// 이 함수에 사용된 행렬들은 GraphicsClass에서 만들어진 것
bool ColorShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	unsigned int bufferNumber;

	// 행렬을 transpose하여 셰이더에서 사용할 수 있게 합니다.
	worldMatrix = XMMatrixTranspose(worldMatrix);
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projectionMatrix = XMMatrixTranspose(projectionMatrix);

	// 상수 버퍼의 내용을 쓸 수 있도록 잠급니다.
	result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// 상수 버퍼의 데이터에 대한 포인터를 가져옵니다.
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	// 상수 버퍼에 행렬을 복사합니다.
	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	// 상수 버퍼의 잠금을 풉니다.
	deviceContext->Unmap(m_matrixBuffer, 0);

	// 정점 셰이더에서의 상수 버퍼의 위치를 설정합니다.
	bufferNumber = 0;

	// 마지막으로 정점 셰이더의 상수 버퍼를 바뀐 값으로 바꿉니다.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

	return true;
}

// enderShader 함수는 Render 함수에서 불리는 두번째 함수
// SetShaderParameters 이 함수보다 먼저 호출되어 셰이더의 인자들을 올바로 세팅하게 됨

// 이 함수에서 가장 먼저 하는 것은 입력 레이아웃을 입력 어셈블러에 연결하는 것
// 이로써 GPU 정점 버퍼의 자료구조를 알게 됨

// 두번째 단계는 정점 버퍼를 그리기 위한 정점 셰이더와 픽셀 셰이더를 설정하는 것
// 셰이더가 설정되면 D3D 디바이스 컨텍스트에서 DirectX 11의 DrawIndexed 함수를 사용하여 삼각형을 그려냄
// 이 함수가 호출된 이후에 초록색 삼각형이 그려짐
void ColorShaderClass::RenderShader(ID3D11DeviceContext* deviceContext, int indexCount)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(m_layout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(m_vertexShader, NULL, 0);
	deviceContext->PSSetShader(m_pixelShader, NULL, 0);

	// Render the triangle.
	deviceContext->DrawIndexed(indexCount, 0, 0);

	return;
}
