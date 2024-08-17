#include "d3dclass.h"

D3DClass::D3DClass()
{
	m_swapChain = 0;
	m_device = 0;
	m_deviceContext = 0;
	m_renderTargetView = 0;
	m_depthStencilBuffer = 0;
	m_depthStencilState = 0;
	m_depthStencilView = 0;
	m_rasterState = 0;
	m_depthDisabledStencilState = 0;
}

D3DClass::D3DClass(const D3DClass& other)
{
}

D3DClass::~D3DClass()
{
}

bool D3DClass::Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool fullscreen, float screenDepth, float screenNear)
{
	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	float fieldOfView, screenAspect;

	// 새로운 깊이 스텐실을 설정하기 위한 새로운 깊이 스텐실 description 변수
	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;

	// Store the vsync setting.
	m_vsync_enabled = vsync;

	// Video Card, Monitor에서 새로 고침 빈도 가져오기
	// 컴퓨터마다 약간씩 다를 수 있으므로 해당 정보를 쿼리해야함.
	// 분자 및 분모 값을 쿼리한 다음 설정 중에 DirectX에 전달하면 적절한 새로 고침 빈도가 계산됨.
	// 이 작업을 수행하지 않고 모든 컴퓨터에 존재하지 않을 수 있는 기본값으로 설정하면 DirectX는 Buffer Flip 대신 Blit을 수행하여 응답하여 성능을 저하시키고 디버그 출력에 성가신 오류를 발생시킴
	
	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory  to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found stroe the numerator and denominator of the refresh rate for the monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)screenWidth)
		{
			if (displayModeList[i].Height == (unsigned int)screenHeight)
			{
				numerator = displayModeList[i].RefreshRate.Numerator; // 분자
				denominator = displayModeList[i].RefreshRate.Denominator; // 분모
			}
		}
	}

	// 위에서 새로 고침 빈도에 대한 분자, 분모를 얻음
	// Adapter를 사용하여 마지막으로 검색 할 것은 Video Card의 이름과 Video Memory의 양
	
	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// 새로 고침와 비디오 카드 정보에 대한 분자와 분모를 저장했으므로 구조를 해체할 수 있음
	// 정보를 얻는 데 사용되는 인터페이스

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = 0;

	// Release the adapter.
	adapter->Release();
	adapter = 0;

	// Release the factory.
	factory->Release();
	factory = 0;

	// 이제 시스템의 새로 고침 빈도가 있으므로 DirectX를 초기화 할 수 있음
	// 가장 먼저 할 일은 '스왑 체인'의 Description을 작성하는 것
	// '스왑 체인'은 그래픽으 그려지는 앞 뒤 버퍼
	// 일반적으로 단일 백 버퍼를 사용하고 모든 그리기를 수행한 다음 사용자의 화면에 표시되는 전면 버퍼로 바꿈

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// 스왑 체인 Description 중 새로 고침 빈도 부분
	// 새로 고침 빈도는 초당 백 버퍼를 전면 버퍼로 그리는 함수
	// applicationclass.h 에서 vsync가 true로 설정된 경우 시스템 설정으로 잠김
	// false인 경우 초당 가능한 한 많이 화면을 그리지만 이로 인해 일부 시각적 아티팩트가 발생할 수 있음

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// 스왑 체인 Description을 설정한 후 feature Level이라는 변수를 하나 더 설정해야 함.
	// 이 변수는 DirectX에 사용할 버전을 알려줌.

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// 이제 스왑 체인 Description 및 feature level이 채워졌으므로 스왑 체인, Direct3D 장치 및 Direct3D 장치 컨텍스트를 만들 수 있음.
	// Direct3D 장치 및 Direct3D 장치 컨텍스트는 매우 중요하며 모든 Direct3D 기능에 대한 인터페이스입니다. 이 시점부터 거의 모든 작업에 장치 및 장치 컨텍스트를 사용함.
	
	// 사용자에게 DirectX 11 비디오 카드가 없는 경우 이 함수 호출은 디바이스 및 디바이스 컨텍스트를 만들지 못함
	// 또한 DirectX 11 기능을 직접 테스트하고 DirectX 11 비디오 카드가없는 경우 교체 할 수 있음
	// D3D_DRIVER_TYPE_REFERENCE 및 DirectX를 사용하는 D3D_DRIVER_TYPE_HARDWARE는 비디오 카드 하드웨어 대신 CPU를 사용하여 그림을 그림
	// 이것은 1/1000 속도로 실행되지만 아직 모든 컴퓨터에 DirectX 11 비디오 카드가 없는 사람들에게 좋음

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_deviceContext);
	if (FAILED(result))
	{
		return false;
	}

	// 기본 비디오 카드가 DirectX 11과 호환되지 않는 경우 디바이스를 만들기 위한 이 호출이 실패하는 경우가 있음
	// 일부 컴퓨터에는 기본 카드가 DirectX 10 비디오 카드이고 보조 카드가 DirectX 11 비디오 카드일 수 있습니다. 또한 일부 하이브리드 그래픽 카드는 기본 그래픽 카드와 보조 그래픽 카드와 함께 작동합니다
	// 이 문제를 해결하려면 기본 장치를 사용하지 말고 대신 컴퓨터의 모든 비디오 카드를 열거하고 사용자가 사용할 비디오 카드를 선택한 다음 장치를 만들 때 해당 카드를 지정하도록 합니다.

	// 이제 스왑 체인이 있으므로 백 버퍼에 대한 포인터를 가져온 다음 스왑 체인에 연결해야 합니다.
	// CreateRenderTargetView 함수를 사용하여 백 버퍼를 스왑 체인에 연결함

	// Get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// Depth Buffer Description 설정
	// 이를 사용하여 다각형이 3D 공간에서 제대로 렌더링될 수 있도록 깊이 버퍼를 만듬
	// 동시에 Stencil Buffer를 Depth Buffer에 연결
	// Stencil Buffer는 모션 블러, volumetric shadows와 같은 효과를 얻는 데 사용할 수 있음

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = screenWidth;
	depthBufferDesc.Height = screenHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// 이제 해당 Description을 사용하여 Depth, Stencil Buffer를 만듦
	// CreateTexture2D 함수를 사용하여 Buffer를 만드는 것을 볼 수 있으므로, Buffer는 그냥 2D Texture입니다.
	// 그 이유는 폴리곤이 정렬된 다음 래스터화되면 이 2D 버퍼에서 컬러 픽셀이 되기 때문임.
	// 그런 다음 이 2D Buffer가 화면에 그려짐

	// Create the texture for the depth buffer using the filled out description.
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Stencil Desciption 설정
	// 이를 통해 Direct3D가 각 픽셀에 대해 수행할 깊이 테스트 유형을 제어할 수 있음

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Description이 작성되고 이제 depth stencil state를 만들 수 있음

	// Create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if (FAILED(result))
	{
		return false;
	}

	// 생성된 Depth Stencil State를 사용하여 이제 적용되도록 설정할 수 있음.
	// 장치 컨텍스트를 사용하여 설정

	// Set the depth stencil state.
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	// 다음으로 만들어야 할 것은 Depth Stencil Buffer View에 대한 Description
	// Direct3D가 Depth Buffer를 Depth Stencil Texture로 사용하는 것을 알 수 있도록 이 작업 수행
	// Description을 작성한 후 CreateDepthStencilView 함수를 호출하여 만듦

	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	// 이제 OMSetRenderTargets를 호출할 수 있음
	// 그러면 렌더 대상 뷰와 깊이 스텐실 버퍼가 출력 렌더 파이프라인에 바인딩 됨
	// 이렇게 하면 파이프라인이 렌더링하는 그래픽이 이전에 만든 백 버퍼에 그려짐
	// 후면 버퍼에 그래픽이 기록되면 이를 전면으로 교체하고 사용자 화면에 그래픽을 표시할 수 있음

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	// 이제 렌더 타겟이 설정되었으므로 향후 튜토리얼에서 장면을 더 잘 제어할 수 있는 몇 가지 추가 기능을 계속할 수 있음.
	// 먼저 우리가 만들 것은 래스터라이저 상태입니다.
	// 이를 통해 다각형이 렌더링되는 방식을 제어할 수 있음.
	// 장면을 와이어프레임 모드로 렌더링하거나 DirectX가 다각형의 앞면과 뒷면을 모두 그리도록 하는 등의 작업을 수행할 수 있음.
	// 기본적으로 DirectX에는 이미 래스터라이저 상태가 설정되어 있고 아래 상태와 정확히 동일하게 작동하지만 직접 설정하지 않는 한 이를 변경할 수 없음.

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		return false;
	}

	// Now set the rasterizer state.
	m_deviceContext->RSSetState(m_rasterState);

	// 또한 Direct3D가 클립 공간 좌표를 렌더링 대상 공간에 매핑할 수 있도록 뷰포트를 설정해야 함.
	// 이 값을 창의 전체 크기로 설정

	// Setup the viewport for rendering.
	m_viewport.Width = (float)screenWidth;
	m_viewport.Height = (float)screenHeight;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_deviceContext->RSSetViewports(1, &m_viewport);

	// 이제 투영 매트릭스 생성
	// 투영 매트릭스는 3D 장면을 이전에 만든 2D 뷰포트 공간으로 변환하는 데 사용됨.
	// 이 Matrix의 복사본을 보관하여 장면을 렌더링하는 데 사용할 셰이더에 전달할 수 있도록 해야함

	// Setup the projection matrix.
	fieldOfView = 3.141592654f / 4.0f;
	screenAspect = (float)screenWidth / (float)screenHeight;

	// Create the projection matrix for 3D rendering.
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	// 또한 World Matrix라는 또 다른 Matrix를 만들 것입니다.
	// 이 Matrix는 물체의 정점을 3D 장면의 정점으로 변환하는 데 사용됨
	// 이 행렬은 3D 공간에서 개체를 회전, 변환 및 크기를 조정하는 데에도 사용됨.
	// 처음부터 행렬을 단위 행렬로 초기화하고 이 개체에 복사본을 보관함
	// 렌더링을 위해 셰이더에 복사본을 전달해야 함

	// Initialize the world matrix to the identity matrix.
	m_worldMatrix = XMMatrixIdentity();

	// 여기에서 일반적으로 뷰 행렬을 만듦
	// 뷰 매트릭스는 우리가 장면을 보고 있는 위치를 계산하는 데 사용됨
	// 카메라라고 생각하면 되고, 이 카메라를 통해서만 장면을 볼 수 있음.
	// 그 목적 때문에 논리적으로 더 잘 맞기 때문에 이후 튜토리얼의 카메라 클래스에서 생성할 예정이며, 지금은 건너뛰기만 하면 됨.

	// 그리고 초기화 함수에서 설정할 마지막 것은 직교 투영 행렬임
	// 이 매트릭스는 화면의 사용자 인터페이스와 같은 2D 요소를 렌더링하는 데 사용되므로 3D 렌더링을 건너뛸 수 있음.
	// 2D 그래픽과 글꼴을 화면에 렌더링하는 방법을 살펴볼 때 이후 튜토리얼에서 이 방법이 사용되는 것을 볼 수 있음

	// Create an orthographic projection matrix for 2D rendering.
	m_orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	// 여기서는 깊이 스텐실에 대한 설명을 설정함
	// 이 새로운 뎁스 스텐실과 이전 뎁스 스텐실의 유일한 차이점은 2D 드로잉에 대해 DepthEnable이 false로 설정되어 있다는 것임
	// Clear the second depth stencil state before setting the parameters.
	ZeroMemory(&depthDisabledStencilDesc, sizeof(depthDisabledStencilDesc));

	// Now create a second depth stencil state which turns off the Z buffer for 2D rendering.  The only difference is 
	// that DepthEnable is set to false, all other parameters are the same as the other depth stencil state.
	depthDisabledStencilDesc.DepthEnable = false;
	depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDisabledStencilDesc.StencilEnable = true;
	depthDisabledStencilDesc.StencilReadMask = 0xFF;
	depthDisabledStencilDesc.StencilWriteMask = 0xFF;
	depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the state using the device.
	result = m_device->CreateDepthStencilState(&depthDisabledStencilDesc, &m_depthDisabledStencilState);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

// Shutdown 함수는 initialize 함수에 사용된 모든 포인터를 해제하고 정리함
// 단, 그러기 전에 스왑 체인을 창 모드로 강제 전환하는 호출을 넣음.
// 이 작업이 완료되지 않은 상태에서 전체 화면 모드로 스왑 체인을 해제하려고 하면 몇 가지 예외가 발생함.
// 따라서 Direct3D를 종료하기 전에 항상 창 모드를 강제 실행함.

void D3DClass::Shutdown()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_depthDisabledStencilState)
	{
		m_depthDisabledStencilState->Release();
		m_depthDisabledStencilState = 0;
	}

	if (m_rasterState)
	{
		m_rasterState->Release();
		m_rasterState = 0;
	}

	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = 0;
	}

	if (m_depthStencilState)
	{
		m_depthStencilState->Release();
		m_depthStencilState = 0;
	}

	if (m_depthStencilBuffer)
	{
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = 0;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = 0;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	return;
}

// D3DClass에는 몇 가지 도우미 함수가 있습니다. 처음 두 개는 BeginScene 및 EndScene입니다.
// BeginScene 은 각 프레임의 시작 부분에 새로운 3D 씬을 그릴 때마다 호출됩니다.
// 버퍼를 초기화하여 비어 있고 그릴 준비가 되도록 하는 것뿐입니다.
// 다른 기능은 Endscene으로, 각 프레임의 끝에서 모든 그리기가 완료되면 스왑 체인에 3D 장면을 표시하도록 지시합니다.

void D3DClass::BeginScene(float red, float green, float blue, float alpha)
{
	float color[4];


	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	// Clear the back buffer.
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);

	// Clear the depth buffer.
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	return;
}

void D3DClass::EndScene()
{
	// Present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		m_swapChain->Present(0, 0);
	}

	return;
}

// 단순히 Direct3D 디바이스 및 Direct3D 디바이스 컨텍스트에 대한 포인터를 가져옴

ID3D11Device* D3DClass::GetDevice()
{
	return m_device;
}

ID3D11DeviceContext* D3DClass::GetDeviceContext()
{
	return m_deviceContext;
}

// 프로젝션, 월드 및 직교 행렬의 복사본을 호출 함수에 제공
// 대부분의 셰이더는 렌더링을 위해 이러한 행렬이 필요하므로 외부 오브젝트가 복사본을 쉽게 얻을 수 있는 방법이 필요했습니다.
// 이 튜토리얼에서는 이러한 함수를 호출하지 않지만 코드에 있는 이유만 설명하겠습니다.

void D3DClass::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
	return;
}


void D3DClass::GetWorldMatrix(XMMATRIX& worldMatrix)
{
	worldMatrix = m_worldMatrix;
	return;
}


void D3DClass::GetOrthoMatrix(XMMATRIX& orthoMatrix)
{
	orthoMatrix = m_orthoMatrix;
	return;
}

// 참조로 비디오 카드의 이름과 비디오 메모리 양을 반환함
// 비디오 카드 이름을 알면 다양한 구성에서 디버깅하는 데 도움이 될 수 있음

void D3DClass::GetVideoCardInfo(char* cardName, int& memory)
{
	strcpy_s(cardName, 128, m_videoCardDescription);
	memory = m_videoCardMemory;
	return;
}

// 마지막 두 함수는 나중에 render to texture 자습서에서 사용됨

void D3DClass::SetBackBufferRenderTarget()
{
	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	return;
}


void D3DClass::ResetViewport()
{
	// Set the viewport.
	m_deviceContext->RSSetViewports(1, &m_viewport);

	return;
}

// Z 버퍼를 활성화 및 비활성화하는 새로운 기능
// 일반적으로 이러한 기능을 사용하는 가장 좋은 방법은 먼저 모든 3D 렌더링을 수행한 다음 Z 버퍼를 끄고 2D 렌더링을 수행한 다음 Z 버퍼를 다시 켜는 것
void D3DClass::TurnZBufferOn()
{
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);
	return;
}


void D3DClass::TurnZBufferOff()
{
	m_deviceContext->OMSetDepthStencilState(m_depthDisabledStencilState, 1);
	return;
}
