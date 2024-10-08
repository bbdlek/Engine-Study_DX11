﻿#include "applicationclass.h"
#include <fstream>


ApplicationClass::ApplicationClass()
{
	m_Direct3D = 0;
	m_Camera = 0;
	m_TextureShader = 0;
	m_Bitmap = 0;
}


ApplicationClass::ApplicationClass(const ApplicationClass& other)
{
}


ApplicationClass::~ApplicationClass()
{
}


bool ApplicationClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	char bitmapFilename[128];
	bool result;

	// Create and initialize the Direct3D object.
	m_Direct3D = new D3DClass;

	result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create the camera object.
	m_Camera = new CameraClass;

	// Set the initial position of the camera.
	m_Camera->SetPosition(0.0f, 0.0f, -10.0f);
	m_Camera->Render();

	// Create and initialize the texture shader object.
	m_TextureShader = new TextureShaderClass;

	result = m_TextureShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
		return false;
	}

	// Set the file name of the bitmap file.
	strcpy_s(bitmapFilename, "../Engine/data/stone01.tga");

	// Create and initialize the bitmap object.
	m_Bitmap = new BitmapClass;

	result = m_Bitmap->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, bitmapFilename, 50, 50);
	if (!result)
	{
		return false;
	}

	// TEST
	// 현재 실행 파일의 경로 가져오기
	char moduleFileName[MAX_PATH];
	GetModuleFileNameA(NULL, moduleFileName, MAX_PATH);

	// 경로에서 디렉터리 부분만 추출
	std::string path(moduleFileName);
	size_t pos = path.find_last_of("\\/");
	if (pos != std::string::npos)
	{
		path = path.substr(0, pos);
	}

	// 파일 경로 생성
	std::string filePath = path + "\\VideoCardInfo.txt";

	char cardName[128];
	int memory;

	m_Direct3D->GetVideoCardInfo(cardName, memory);

	std::ofstream ofs(filePath);
	if (ofs.is_open())
	{
		ofs << "Video Card: " << cardName << std::endl;
		ofs << "Memory: " << memory << " MB" << std::endl;
		ofs.close();
	}
	else
	{
		MessageBox(NULL, L"Could not write video card info to file", L"Error", MB_OK);
	}

	return true;
}


void ApplicationClass::Shutdown()
{
	// Release the bitmap object.
	if (m_Bitmap)
	{
		m_Bitmap->Shutdown();
		delete m_Bitmap;
		m_Bitmap = 0;
	}

	// Release the texture shader object.
	if (m_TextureShader)
	{
		m_TextureShader->Shutdown();
		delete m_TextureShader;
		m_TextureShader = 0;
	}

	// Release the camera object.
	if (m_Camera)
	{
		delete m_Camera;
		m_Camera = 0;
	}

	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = 0;
	}

	return;
}


bool ApplicationClass::Frame()
{
	bool result;

	// Render the graphics scene.
	result = Render();
	if (!result)
	{
		return false;
	}

	return true;
}


// Render 함수 변화점 (Chaper4)
// 검은색이라는 것만 빼면 여전히 화면을 초기화하는 코드로 시작
// 우선 Initialize 함수에서 지정한 카메라의 위치를 토대로 뷰 행렬을 만들기 위해 카메라의 Render 함수를 호출
// 뷰 행렬이 만들어지면 그것의 복사본을 가져올 수 있음
// 또한 D3DClass 객체로부터 월드 행렬과 투영 행렬을 복사해 옴
// 그리고 나서 ModelClass::Render 함수를 호출하여 그래픽 파이프라인에 삼각형 모델을 그리도록 함
// 이미 준비한 정점들로 셰이더를 호출하여 셰이더는 모델 정보와 정점을 배치시키기 위한 세 행렬을 사용하여 정점들을 그려냄
// 이제 삼각형이 백버퍼에 그려집니다. 씬 그리기가 완료되었다면 EndScene을 호출하여 화면에 표시하도록 함
bool ApplicationClass::Render()
{
	XMMATRIX worldMatrix, viewMatrix, orthoMatrix;
	bool result;

	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetOrthoMatrix(orthoMatrix);

	// Turn off the Z buffer to begin all 2D rendering.
	m_Direct3D->TurnZBufferOff();

	// Put the bitmap vertex and index buffers on the graphics pipeline to prepare them for drawing.
	result = m_Bitmap->Render(m_Direct3D->GetDeviceContext());
	if (!result)
	{
		return false;
	}

	// Render the bitmap with the texture shader.
	result = m_TextureShader->Render(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix, m_Bitmap->GetTexture());
	if (!result)
	{
		return false;
	}

	// Turn the Z buffer back on now that all 2D rendering has completed.
	m_Direct3D->TurnZBufferOn();

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

	return true;
}