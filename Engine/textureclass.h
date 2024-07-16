// TextureClass는 텍스쳐 자원을 불러오고, 해제하고, 접근하는 작업을 캡슐화함
// 모든 텍스쳐에 대해 각각 이 클래스가 만들어져 있어야 함

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
	// Targa(tga파일, 래스터 그래픽 파일 포맷)을 읽는 함수
	// 다른 형식도 읽고 싶으면 여기 추가
	bool LoadTarga32Bit(char*);

private:
	unsigned char* m_targaData; // Targa 데이터 보유
	ID3D11Texture2D* m_texture; // DirectX가 렌더링에 사용할 구조화된 텍스쳐 데이터 보유
	ID3D11ShaderResourceView* m_textureView; // 셰이더가 그릴 때 텍스쳐 데이터에 엑세스하는 데 사용되는 리소스 뷰
	int m_width, m_height; // 텍스쳐 크기
};

#endif