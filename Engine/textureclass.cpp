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

// ���� Targa �����͸� �迭�� �ε�
// �� ���� �ؽ��ĸ� ����� Targa �����͸� �ùٸ� �������� �ε�(Targa �̹����� �⺻������ �����̹Ƿ� �����ؾ� ��)
// �׷� ���� �ؽ��İ� �ε�Ǹ� ���̴��� �׸��⿡ ����� �ؽ����� ���ҽ��� ������
bool TextureClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename)
{
	bool result;
	int height, width;
	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT hResult;
	unsigned int rowPitch;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	// ���� Targa ������ m_targaData �迭�� �ε�
	// Load the targa image data into memory.
	result = LoadTarga32Bit(filename);
	if (!result)
	{
		return false;
	}

	// �� ���� Targa �����͸� �ε��� DirectX �ؽ��Ŀ� ���� description�� �����ؾ� ��
	// Targa �̹��� �������� ���̿� �ʺ� ����ϰ�, ������ 32��Ʈ RGBA �ؽ��ķ� ����
	// SampleDesc�� �⺻������ ����
	// �� ���� Usage�� �� ���� ������ �޸��� D3D11_USAGE_DEFAULT�� ����
	// ���������� MipLevels, BindFlags �� MiscFlags�� �Ӹ� �ؽ��Ŀ� �ʿ��� �������� ����
	// description�� �Ϸ�Ǹ� CreateTexture2D�� �� �ؽ��ĸ� ����
	// ���� Targa �����͸� �� �ؽ��Ŀ� ����
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

	// ���⼭�� ������ Targa ������ �迭�� DirectX �ؽ��ķ� �����ϱ� ���� UpdateSubresource�� ���
	// 
	// ModelClass����ó�� Map �� Unmap�� ����Ͽ� ����� ��� ��� ���ۿ� ������ �� �ֱ� ������,
	// �� �ε� ���(UpdateSubresource, Map & Unmap) ��� Ư�� ������ ���� �����Ƿ� ���ɻ��� ������ �ùٸ��� �����ؾ���
	// Map & Unmap : �ξ� ����, �� �����Ӹ��� �Ǵ� �ſ� ���������� �ٽ� �ε�Ǵ� �����Ϳ� ���� ����ϴ� ���� ����
	// UpdateSubresource : �� �� �ε�ǰų� �ε� ������ �߿� ���� �ε���� �ʴ� �����Ϳ� ���� ����ϴ� ���� ����
	// --------> �� ������ UpdateSubresource�� �����͸� �� �����ϰų� �ٽ� �ε����� ���� ���� �˱� ������ ĳ�� ���� �켱 ������ ���� ��� �޸𸮿� �����͸� �����ϱ� ����
	// 
	// UpdateSubresource�� ����Ͽ� �ε��� �� D3D11_USAGE_DEFAULT�� ����Ͽ� DirectX���� �˸�
	// Map �� Unmap�� DirectX���� �����Ͱ� �� ������� ������ �����ϹǷ� ĳ�õ��� �ʴ� �޸� ��ġ�� �����͸� ������
	// �̰��� �ٷ� �츮�� D3D11_USAGE_DYNAMIC�� ����Ͽ� �̷��� ������ �����Ͱ� �Ͻ������� DirectX�� �˸��� ����
	
	// Copy the targa image data into the texture.
	deviceContext->UpdateSubresource(m_texture, 0, NULL, m_targaData, rowPitch, 0);

	// �ؽ��İ� �ε�� �� ���̴����� �ؽ��ĸ� �����ϱ� ���� �����͸� ���� �� �ִ� ���̴� ���ҽ� �� ����
	// description���� �� ���� �߿��� �Ӹ� ������ �����ߴµ�, �̴� � �Ÿ������� ��ǰ�� �ؽ�ó �������� ���� ��ü �Ӹ� ���� ������ ������
	// ���̴� ���ҽ� �䰡 �����Ǹ� GenerateMips�� ȣ���Ͽ� �Ӹ��� ������
	// ���ϴ� ��� �� ���� ǰ���� ���ϸ� Mipmap ������ �������� �ε� ����
	
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

// GetTexture �Լ��� �ٸ� ��ü�� �� �ؽ��� ���̴� �ڿ��� ������ �ʿ䰡 ���� �� ����
// �� �Լ��� ���ؼ� �ؽ��ĸ� �������� �� �ְ� ��
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

// Targa �̹��� �ε�
// Targa �̹����� �Ųٷ� ����ǹǷ� ����ϱ� ���� ������� �Ѵٴ� ���� �ٽ� �� �� ����
// ���� ���⼭�� ������ ���� �迭�� ���� ���� �ش� �迭 �����͸� ������ �ùٸ� ������ m_targaData �迭�� �ε���
// ���⼭�� �ǵ������� ���� ä���� �ִ� 32��Ʈ Targa ���ϸ� �ٷ�� ������, �� ����� 24��Ʈ�� ����� Targa ������ �ź��ϰ� ����
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
