// TextureClass�� �ؽ��� �ڿ��� �ҷ�����, �����ϰ�, �����ϴ� �۾��� ĸ��ȭ��
// ��� �ؽ��Ŀ� ���� ���� �� Ŭ������ ������� �־�� ��

#ifndef _TEXTURECLASS_H_
#define _TEXTURECLASS_H_

#include <d3d11.h>
#include <stdio.h>

class TextureClass
{
private:
	struct TargaHeader
	{
		unsigned char data1[12];
		unsigned short width;
		unsigned short height;
		unsigned char bpp;
		unsigned char data2;
	};

public:
	TextureClass();
	TextureClass(const TextureClass&);
	~TextureClass();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*);
	void Shutdown();

	ID3D11ShaderResourceView* GetTexture();

	int GetWidth();
	int GetHeight();

private:
	// Targa(tga����, ������ �׷��� ���� ����)�� �д� �Լ�
	// �ٸ� ���ĵ� �а� ������ ���� �߰�
	bool LoadTarga32Bit(char*);

private:
	unsigned char* m_targaData; // Targa ������ ����
	ID3D11Texture2D* m_texture; // DirectX�� �������� ����� ����ȭ�� �ؽ��� ������ ����
	ID3D11ShaderResourceView* m_textureView; // ���̴��� �׸� �� �ؽ��� �����Ϳ� �������ϴ� �� ���Ǵ� ���ҽ� ��
	int m_width, m_height; // �ؽ��� ũ��
};

#endif