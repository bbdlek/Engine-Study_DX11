#include "applicationclass.h"
#include <fstream>


ApplicationClass::ApplicationClass()
{
	m_Direct3D = 0;
	m_Camera = 0;
	m_Model = 0;
	m_LightShader = 0;
	m_Light = 0;
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
	m_Camera->SetPosition(0.0f, 0.0f, -10.0f);

	// Set the file name of the model
	strcpy_s(modelFilename, "../Engine/data/cube.txt");

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

	// Create and initialize the light object
	m_Light = new LightClass;

	m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light->SetDirection(0.0f, 0.0f, 1.0f);

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
	// Release the light object.
	if (m_Light)
	{
		delete m_Light;
		m_Light = 0;
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
	static float rotation = 0.0f;
	bool result;

	// Update the rotation variable each frame.
	rotation -= 0.0174532925f * 0.1f;
	if (rotation < 0.0f)
	{
		rotation += 360.0f;
	}

	// Render the graphics scene.
	result = Render(rotation);
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
bool ApplicationClass::Render(float rotation)
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, rotateMatrix, translateMatrix, scaleMatrix, srMatrix;
	bool result;


	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	// Generate the view matrix based on the camera's position.
	m_Camera->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetProjectionMatrix(projectionMatrix);

	// rotation 변수를 사용하여 Y축을 중심으로 회전하기 위한 회전 행렬을 만듦
	// 그 다음 큐브를 왼쪽으로 2단위 이동시키는 변환 행렬을 만듦
	// 두 행렬을 만들고 난뒤 올바른 순서(SRT)로 곱하여 회전을 먼저 하고 변환을 마지막으로 하여 결합된 변환을 갖는 최종 월드 행렬을 만듦
	// 그런 다음 일반 뷰 및 투영 행렬과 함께 월드 행렬만 셰이더로 보내 라이트 셰이더에서 렌더링할 때 큐브 모델을 회전하고 변환
	rotateMatrix = XMMatrixRotationY(rotation); // Build the rotation matrix
	translateMatrix = XMMatrixTranslation(-2.0f, 0.0f, 0.0f);  // Build the translation matrix.

	// Multiply them together to create the final world transformation matrix.
	worldMatrix = XMMatrixMultiply(rotateMatrix, translateMatrix);

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_Model->Render(m_Direct3D->GetDeviceContext());

	// Render the model using the light shader.
	result = m_LightShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture(),
		m_Light->GetDirection(), m_Light->GetDiffuseColor());
	if (!result)
	{
		return false;
	}

	// 두 번째 변환을 위해 스케일링 추가
	// 큐브를 균일하게 절반으로 줄인 다음 회전과 변환도 적용
	// 따라서 세 축 모두에 0.5를 사용하여 스케일 행렬을 만드는 것으로 시작
	// 그런 다음 Y 회전 변수를 사용하여 위와 동일한 회전 행렬을 만듦
	// 그런 다음 오른쪽으로 2 단위 이동하여 이 큐브를 다른 방향으로 이동하는 변환 행렬을 만듦
	// 세 행렬을 모두 구하면 두 번의 곱셈을 수행하고 모든 변환을 최종 월드 행렬에 저장하여 올바른 SRT(Scale, Rotate, Transform)순서로 결합
	// 그 다음 라이트 셰이더에서 새 월드 행렬을 설정하고 큐브 모델을 다시 렌더링하여 원하는 변환이 적용된 장면에서 두 번째 큐브 확인
	scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);  // Build the scaling matrix.
	rotateMatrix = XMMatrixRotationY(rotation);  // Build the rotation matrix.
	translateMatrix = XMMatrixTranslation(2.0f, 0.0f, 0.0f);  // Build the translation matrix.

	// Multiply the scale, rotation, and translation matrices together to create the final world transformation matrix.
	srMatrix = XMMatrixMultiply(scaleMatrix, rotateMatrix);
	worldMatrix = XMMatrixMultiply(srMatrix, translateMatrix);

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_Model->Render(m_Direct3D->GetDeviceContext());

	// Render the model using the light shader.
	result = m_LightShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture(),
		m_Light->GetDirection(), m_Light->GetDiffuseColor());
	if (!result)
	{
		return false;
	}

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

	return true;
}