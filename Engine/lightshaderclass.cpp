#include "lightshaderclass.h"

LightShaderClass::LightShaderClass()
{
	m_vertexShader = 0;
	m_pixelShader = 0;
	m_layout = 0;
	m_sampleState = 0;
	m_matrixBuffer = 0;

    m_lightColorBuffer = 0;
    m_lightPositionBuffer = 0;
}

LightShaderClass::LightShaderClass(const LightShaderClass& other)
{
}

LightShaderClass::~LightShaderClass()
{
}

bool LightShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
	wchar_t vsFilename[128];
	wchar_t psFilename[128];
	int error;
	bool result;

    // Set the filename of the vertex shader.
    error = wcscpy_s(vsFilename, 128, L"../Engine/light.vs");
    if (error != 0)
    {
        return false;
    }

    // Set the filename of the pixel shader.
    error = wcscpy_s(psFilename, 128, L"../Engine/light.ps");
    if (error != 0)
    {
        return false;
    }

    // Initialize the vertex and pixel shaders.
    result = InitializeShader(device, hwnd, vsFilename, psFilename);
    if (!result)
    {
        return false;
    }

    return true;
}

void LightShaderClass::Shutdown()
{
    ShutdownShader();

    return;
}

// Render 함수는 이제 빛의 방향과 조명 색상을 입력으로 받음
// SetShaderParameters 함수를 통해 결국 셰이더에 그 값이 적용됨
bool LightShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture, XMFLOAT4 diffuseColor[], XMFLOAT4 lightPosition[])
{
    bool result;

    // Set the shader parameters that it will use for rendering.
    result = SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projectionMatrix, texture, diffuseColor, lightPosition);
    if (!result)
    {
        return false;
    }

    // Now render the prepared buffers with the shader.
    RenderShader(deviceContext, indexCount);

    return true;
}

bool LightShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename)
{
    HRESULT result;
    ID3D10Blob* errorMessage;
    ID3D10Blob* vertexShaderBuffer;
    ID3D10Blob* pixelShaderBuffer;

    // polygonLayout 변수가 2개가 아닌 3개의 요소를 갖도록 변경됨.
    // 이는 레이아웃에서 일반 벡터를 수용할 수 있도록 하기 위함
    D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
    unsigned int numElements;
    D3D11_SAMPLER_DESC samplerDesc;
    D3D11_BUFFER_DESC matrixBufferDesc;
    D3D11_BUFFER_DESC lightColorBufferDesc;
    D3D11_BUFFER_DESC lightPositionBufferDesc;

    // Initialize the pointers this function will use to null.
    errorMessage = 0;
    vertexShaderBuffer = 0;
    pixelShaderBuffer = 0;

    // 새 조명 정점 셰이더 로드
    // Compile the vertex shader code.
    result = D3DCompileFromFile(vsFilename, NULL, NULL, "LightVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        // If the shader failed to compile it should have writen something to the error message.
        if (errorMessage)
        {
            OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
        }
        // If there was nothing in the error message then it simply could not find the shader file itself.
        else
        {
            MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
        }

        return false;
    }

    // 새 조명 픽셀 셰이더 로드
    // Compile the pixel shader code.
    result = D3DCompileFromFile(psFilename, NULL, NULL, "LightPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage);
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

    // Create the vertex input layout description.
    // This setup needs to match the VertexType stucture in the ModelClass and in the shader.
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "TEXCOORD";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    // 셰이더 초기화의 중요한 변화 사항 중 하나가 바로 이 polygonLayout
    // 여기서 조명 효과를 위하여 법선 벡터를 담을 세번째 요소를 추가함
    // SemanticName은 NORMAL이고, 자료형은 법선 벡터의 x, y, z 부동소수점 변수를 담을 수 있는 DXGI_FORMAT_R32G32B32_FLOAT
    // 이 구조는 HLSL 정점 셰이더가 입력으로 받아들일 입력과 잘 맞아떨어지게 됩니다.

    polygonLayout[2].SemanticName = "NORMAL";
    polygonLayout[2].SemanticIndex = 0;
    polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[2].InputSlot = 0;
    polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[2].InstanceDataStepRate = 0;

    // Get a count of the elements in the layout.
    numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

    // Create the vertex input layout.
    result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
        &m_layout);
    if (FAILED(result))
    {
        return false;
    }

    // Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
    vertexShaderBuffer->Release();
    vertexShaderBuffer = 0;

    pixelShaderBuffer->Release();
    pixelShaderBuffer = 0;

    // Create a texture sampler state description.
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the texture sampler state.
    result = device->CreateSamplerState(&samplerDesc, &m_sampleState);
    if (FAILED(result))
    {
        return false;
    }

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

    // 밝은 색상 배열 버퍼
    // Setup the description of the dynamic constant buffer that is in the pixel shader.
    lightColorBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightColorBufferDesc.ByteWidth = sizeof(LightColorBufferType);
    lightColorBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightColorBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    lightColorBufferDesc.MiscFlags = 0;
    lightColorBufferDesc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the pixel shader constant buffer from within this class.
    result = device->CreateBuffer(&lightColorBufferDesc, NULL, &m_lightColorBuffer);
    if (FAILED(result))
    {
        return false;
    }


    // 라이트 포지션 배열 버퍼
    // Setup the description of the dynamic constant buffer that is in the vertex shader.
    lightPositionBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightPositionBufferDesc.ByteWidth = sizeof(LightPositionBufferType);
    lightPositionBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightPositionBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    lightPositionBufferDesc.MiscFlags = 0;
    lightPositionBufferDesc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    result = device->CreateBuffer(&lightPositionBufferDesc, NULL, &m_lightPositionBuffer);
    if (FAILED(result))
    {
        return false;
    }

    return true;
}

void LightShaderClass::ShutdownShader()
{
    // Release the light constant buffers.
    if (m_lightColorBuffer)
    {
        m_lightColorBuffer->Release();
        m_lightColorBuffer = 0;
    }

    if (m_lightPositionBuffer)
    {
        m_lightPositionBuffer->Release();
        m_lightPositionBuffer = 0;
    }

    // Release the matrix constant buffer.
    if (m_matrixBuffer)
    {
        m_matrixBuffer->Release();
        m_matrixBuffer = 0;
    }

    // Release the sampler state.
    if (m_sampleState)
    {
        m_sampleState->Release();
        m_sampleState = 0;
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

void LightShaderClass::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
    char* compileErrors;
    unsigned __int64 bufferSize, i;
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


// 이제 lightDirection 및  diffuseColor를 입력으로 받음
bool LightShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
    ID3D11ShaderResourceView* texture, XMFLOAT4 diffuseColor[], XMFLOAT4 lightPosition[])
{
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    unsigned int bufferNumber;
    MatrixBufferType* dataPtr;
    LightPositionBufferType* dataPtr2;
    LightColorBufferType* dataPtr3;


    // Transpose the matrices to prepare them for the shader.
    worldMatrix = XMMatrixTranspose(worldMatrix);
    viewMatrix = XMMatrixTranspose(viewMatrix);
    projectionMatrix = XMMatrixTranspose(projectionMatrix);

    // Lock the constant buffer so it can be written to.
    result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return false;
    }

    // Get a pointer to the data in the constant buffer.
    dataPtr = (MatrixBufferType*)mappedResource.pData;

    // Copy the matrices into the constant buffer.
    dataPtr->world = worldMatrix;
    dataPtr->view = viewMatrix;
    dataPtr->projection = projectionMatrix;

    // Unlock the constant buffer.
    deviceContext->Unmap(m_matrixBuffer, 0);

    // Set the position of the constant buffer in the vertex shader.
    bufferNumber = 0;

    // Now set the constant buffer in the vertex shader with the updated values.
    deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

    // 꼭짓점 셰이더에 사용될 조명 위치 배열 버퍼
    // Lock the light position constant buffer so it can be written to.
    result = deviceContext->Map(m_lightPositionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return false;
    }

    // Get a pointer to the data in the constant buffer.
    dataPtr2 = (LightPositionBufferType*)mappedResource.pData;

    // Copy the light position variables into the constant buffer.
    dataPtr2->lightPosition[0] = lightPosition[0];
    dataPtr2->lightPosition[1] = lightPosition[1];
    dataPtr2->lightPosition[2] = lightPosition[2];
    dataPtr2->lightPosition[3] = lightPosition[3];

    // Unlock the constant buffer.
    deviceContext->Unmap(m_lightPositionBuffer, 0);

    // Set the position of the constant buffer in the vertex shader.
    bufferNumber = 1;

    // Finally set the constant buffer in the vertex shader with the updated values.
    deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_lightPositionBuffer);

    // Set shader texture resource in the pixel shader.
    deviceContext->PSSetShaderResources(0, 1, &texture);

    // Lock the light color constant buffer so it can be written to.
    result = deviceContext->Map(m_lightColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return false;
    }

    // Get a pointer to the data in the constant buffer.
    dataPtr3 = (LightColorBufferType*)mappedResource.pData;

    // Copy the light color variables into the constant buffer.
    dataPtr3->diffuseColor[0] = diffuseColor[0];
    dataPtr3->diffuseColor[1] = diffuseColor[1];
    dataPtr3->diffuseColor[2] = diffuseColor[2];
    dataPtr3->diffuseColor[3] = diffuseColor[3];

    // Unlock the constant buffer.
    deviceContext->Unmap(m_lightColorBuffer, 0);

    // Set the position of the constant buffer in the pixel shader.
    bufferNumber = 0;

    // Finally set the constant buffer in the pixel shader with the updated values.
    deviceContext->PSSetConstantBuffers(bufferNumber, 1, &m_lightColorBuffer);

    return true;
}

void LightShaderClass::RenderShader(ID3D11DeviceContext* deviceContext, int indexCount)
{
    // Set the vertex input layout.
    deviceContext->IASetInputLayout(m_layout);

    // Set the vertex and pixel shaders that will be used to render this triangle.
    deviceContext->VSSetShader(m_vertexShader, NULL, 0);
    deviceContext->PSSetShader(m_pixelShader, NULL, 0);

    // Set the sampler state in the pixel shader.
    deviceContext->PSSetSamplers(0, 1, &m_sampleState);

    // Render the triangle.
    deviceContext->DrawIndexed(indexCount, 0, 0);

    return;
}
