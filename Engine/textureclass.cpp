#include "textureclass.h"

TextureClass::TextureClass()
{
	m_targaData = 0;
	m_texture = 0;
	m_textureView = 0;
}

TextureClass::TextureClass(const TextureClass& other)
{
}

TextureClass::~TextureClass()
{
}

// 먼저 Targa 데이터를 배열에 로드
// 그 다음 텍스쳐를 만들고 Targa 데이터를 올바른 형식으로 로드(Targa 이미지는 기본적으로 위쪽이므로 반전해야 함)
// 그런 다음 텍스쳐가 로드되면 셰이더가 그리기에 사용할 텍스쳐의 리소스뷰 생성됨
bool TextureClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename)
{
	bool result;
	int height, width;
	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT hResult;
	unsigned int rowPitch;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	// 먼저 Targa 파일을 m_targaData 배열에 로드
	// Load the targa image data into memory.
	result = LoadTarga32Bit(filename);
	if (!result)
	{
		return false;
	}

	// 그 다음 Targa 데이터를 로드할 DirectX 텍스쳐에 대한 description을 구성해야 함
	// Targa 이미지 데이터의 높이와 너비를 사용하고, 형식을 32비트 RGBA 텍스쳐로 설정
	// SampleDesc를 기본값으로 설정
	// 그 다음 Usage를 더 나은 성능의 메모리인 D3D11_USAGE_DEFAULT로 설정
	// 마지막으로 MipLevels, BindFlags 및 MiscFlags를 밉맵 텍스쳐에 필요한 세팅으로 설정
	// description이 완료되면 CreateTexture2D로 빈 텍스쳐를 만듦
	// 이후 Targa 데이터를 빈 텍스쳐에 복사
	// Setup the description of the texture.
	textureDesc.Height = m_height;
	textureDesc.Width = m_width;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	// Create the empty texture.
	hResult = device->CreateTexture2D(&textureDesc, NULL, &m_texture);
	if (FAILED(hResult))
	{
		return false;
	}

	// Set the row pitch of the targa image data.
	rowPitch = (m_width * 4) * sizeof(unsigned char);

	// 여기서는 실제로 Targa 데이터 배열을 DirectX 텍스쳐로 복사하기 위해 UpdateSubresource를 사용
	// 
	// ModelClass에서처럼 Map 및 Unmap을 사용하여 행렬을 행렬 상수 버퍼에 복사할 수 있긴 하지만,
	// 두 로딩 방법(UpdateSubresource, Map & Unmap) 모두 특정 목정을 갖고 있으므로 성능상의 이유로 올바르게 선택해야함
	// Map & Unmap : 훨씬 빠름, 매 프레임마다 또는 매우 정기적으로 다시 로드되는 데이터에 대해 사용하는 것이 좋음
	// UpdateSubresource : 한 번 로드되거나 로드 시퀀스 중에 거의 로드되지 않는 데이터에 대해 사용하는 것이 좋음
	// --------> 그 이유는 UpdateSubresource가 데이터를 곧 제거하거나 다시 로드하지 않을 것을 알기 때문에 캐시 보존 우선 순위를 갖는 고속 메모리에 데이터를 저장하기 때문
	// 
	// UpdateSubresource를 사용하여 로드할 때 D3D11_USAGE_DEFAULT를 사용하여 DirectX에도 알림
	// Map 및 Unmap은 DirectX에서 데이터가 곧 덮어쓰여질 것으로 예상하므로 캐시되지 않는 메모리 위치에 데이터를 저장함
	// 이것이 바로 우리가 D3D11_USAGE_DYNAMIC을 사용하여 이러한 유형의 데이터가 일시적임을 DirectX에 알리는 이유
	
	// Copy the targa image data into the texture.
	deviceContext->UpdateSubresource(m_texture, 0, NULL, m_targaData, rowPitch, 0);

	// 텍스쳐가 로드된 후 셰이더에서 텍스쳐를 설정하기 위한 포인터를 가질 수 있는 셰이더 리소스 뷰 생성
	// description에서 두 가지 중요한 밉맵 변수를 설정했는데, 이는 어떤 거리에서도 고품질 텍스처 렌더링을 위한 전체 밉맵 레벨 범위를 제공함
	// 셰이더 리소스 뷰가 생성되면 GenerateMips를 호출하여 밉맵을 생성함
	// 원하는 경우 더 나은 품질을 원하면 Mipmap 레벨을 수동으로 로드 가능
	
	// Setup the shader resource view description.
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	hResult = device->CreateShaderResourceView(m_texture, &srvDesc, &m_textureView);
	if (FAILED(hResult))
	{
		return false;
	}

	// Generate mipmaps for this texture.
	deviceContext->GenerateMips(m_textureView);

	// Release the targa image data now that the image data has been loaded into the texture.
	delete[] m_targaData;
	m_targaData = 0;

	return true;
}

void TextureClass::Shutdown()
{
	// Release the texture view resource.
	if (m_textureView)
	{
		m_textureView->Release();
		m_textureView = 0;
	}

	// Release the texture.
	if (m_texture)
	{
		m_texture->Release();
		m_texture = 0;
	}

	// Release the targa data.
	if (m_targaData)
	{
		delete[] m_targaData;
		m_targaData = 0;
	}

	return;
}

// GetTexture 함수는 다른 객체가 이 텍스쳐 셰이더 자원에 접근할 필요가 있을 때 사용됨
// 이 함수를 통해서 텍스쳐를 렌더링할 수 있게 됨
ID3D11ShaderResourceView* TextureClass::GetTexture()
{
	return m_textureView;
}

int TextureClass::GetWidth()
{
	return m_width;
}

int TextureClass::GetHeight()
{
	return m_height;
}

// Targa 이미지 로드
// Targa 이미지는 거꾸로 저장되므로 사용하기 전에 뒤집어야 한다는 점을 다시 한 번 참고
// 따라서 여기서는 파일을 열고 배열로 읽은 다음 해당 배열 데이터를 가져와 올바른 순서로 m_targaData 배열에 로드함
// 여기서는 의도적으로 알파 채널이 있는 32비트 Targa 파일만 다루고 있으며, 이 기능은 24비트로 저장된 Targa 파일을 거부하고 있음
bool TextureClass::LoadTarga32Bit(char* filename)
{
	int error, bpp, imageSize, index, i, j, k;
	FILE* filePtr;
	unsigned int count;
	TargaHeader targaFileHeader;
	unsigned char* targaImage;


	// Open the targa file for reading in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the file header.
	count = (unsigned int)fread(&targaFileHeader, sizeof(TargaHeader), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Get the important information from the header.
	m_height = (int)targaFileHeader.height;
	m_width = (int)targaFileHeader.width;
	bpp = (int)targaFileHeader.bpp;

	// Check that it is 32 bit and not 24 bit.
	if (bpp != 32)
	{
		return false;
	}

	// Calculate the size of the 32 bit image data.
	imageSize = m_width * m_height * 4;

	// Allocate memory for the targa image data.
	targaImage = new unsigned char[imageSize];

	// Read in the targa image data.
	count = (unsigned int)fread(targaImage, 1, imageSize, filePtr);
	if (count != imageSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Allocate memory for the targa destination data.
	m_targaData = new unsigned char[imageSize];

	// Initialize the index into the targa destination data array.
	index = 0;

	// Initialize the index into the targa image data.
	k = (m_width * m_height * 4) - (m_width * 4);

	// Now copy the targa image data into the targa destination array in the correct order since the targa format is stored upside down and also is not in RGBA order.
	for (j = 0; j < m_height; j++)
	{
		for (i = 0; i < m_width; i++)
		{
			m_targaData[index + 0] = targaImage[k + 2];  // Red.
			m_targaData[index + 1] = targaImage[k + 1];  // Green.
			m_targaData[index + 2] = targaImage[k + 0];  // Blue
			m_targaData[index + 3] = targaImage[k + 3];  // Alpha

			// Increment the indexes into the targa data.
			k += 4;
			index += 4;
		}

		// Set the targa image data index back to the preceding row at the beginning of the column since its reading it in upside down.
		k -= (m_width * 8);
	}

	// Release the targa image data now that it was copied into the destination array.
	delete[] targaImage;
	targaImage = 0;

	return true;
}
