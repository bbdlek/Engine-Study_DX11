#include "applicationclass.h"
#include <fstream>


ApplicationClass::ApplicationClass()
{
	m_Direct3D = 0;
	m_Camera = 0;
	m_Model = 0;
	m_LightShader = 0;
	m_Lights = 0;
}


ApplicationClass::ApplicationClass(const ApplicationClass& other)
{
}


ApplicationClass::~ApplicationClass()
{
}


bool ApplicationClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	char modelFilename[128];
	char textureFilename[128];
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
	m_Camera->SetPosition(0.0f, 2.0f, -12.0f);
	m_Camera->Render();

	// Set the file name of the model.
	strcpy_s(modelFilename, "../Engine/data/plane.txt");

	// Create and initialize the model object.
	m_Model = new ModelClass;

	// Set the name of the texture file that we will be loading.
	strcpy_s(textureFilename, "../Engine/data/stone01.tga");

	result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), modelFilename, textureFilename);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light shader object.
	m_LightShader = new LightShaderClass;

	result = m_LightShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
		return false;
	}

	// 4개의 조명 배열(빨강, 초록, 파랑, 흰색)
	// Set the number of lights we will use.
	m_numLights = 4;

	// Create and initialize the light objects array.
	m_Lights = new LightClass[m_numLights];

	// Manually set the color and position of each light.
	m_Lights[0].SetDiffuseColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red
	m_Lights[0].SetPosition(-3.0f, 1.0f, 3.0f);

	m_Lights[1].SetDiffuseColor(0.0f, 1.0f, 0.0f, 1.0f);  // Green
	m_Lights[1].SetPosition(3.0f, 1.0f, 3.0f);

	m_Lights[2].SetDiffuseColor(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
	m_Lights[2].SetPosition(-3.0f, 1.0f, -3.0f);

	m_Lights[3].SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);  // White
	m_Lights[3].SetPosition(3.0f, 1.0f, -3.0f);

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
	// Release the light objects.
	if (m_Lights)
	{
		delete[] m_Lights;
		m_Lights = 0;
	}

	// Release the light shader object.
	if (m_LightShader)
	{
		m_LightShader->Shutdown();
		delete m_LightShader;
		m_LightShader = 0;
	}

	// Release the model object.
	if (m_Model)
	{
		m_Model->Shutdown();
		delete m_Model;
		m_Model = 0;
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
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, rotateMatrix, translateMatrix, scaleMatrix, srMatrix;
	XMFLOAT4 diffuseColor[4], lightPosition[4];
	int i;
	bool result;


	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);


	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetProjectionMatrix(projectionMatrix);

	// 4개의 포인트 라이트에서 두 개의 배열(색상 및 위치)을 설정
	// 하면 8개의 서로 다른 조명 구소 요소 대신 두 개의 배열 변수를 더 쉽게 보낼 수 있음
	// Get the light properties.
	for (i = 0; i < m_numLights; i++)
	{
		// Create the diffuse color array from the four light colors.
		diffuseColor[i] = m_Lights[i].GetDiffuseColor();

		// Create the light position array from the four light positions.
		lightPosition[i] = m_Lights[i].GetPosition();
	}

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_Model->Render(m_Direct3D->GetDeviceContext());

	// 조명 셰이더와 4개의 점 조명을 사용하여 모델을 렌더링
	// Render the model using the light shader.
	result = m_LightShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture(),
		diffuseColor, lightPosition);
	if (!result)
	{
		return false;
	}

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

	return true;
}