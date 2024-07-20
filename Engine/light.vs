cbuffer MatrixBuffer
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

//////////////
// TYPEDEFS //
//////////////
// 법선 벡터는 법선의 방향과 빛의 방향 사이의 각도를 사용하여 빛의 양을 계산하는 데 사용
struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType LightVertexShader(VertexInputType input)
{
    PixelInputType output;
    

    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;

    // 이 꼭지점의 법선 벡터는 월드 좌표계에서 계산되고 픽셀 셰이더의 입력으로 들어가기 전에 정규화됨
    // Calculate the normal vector against the world matrix only.
    output.normal = mul(input.normal, (float3x3) worldMatrix);
	
    // Normalize the normal vector.
    output.normal = normalize(output.normal);

    return output;
}