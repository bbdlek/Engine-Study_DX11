cbuffer MatrixBuffer
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

// ���̻� ������ ������ ������� �ʰ� �ؽ��� ��ǥ�� ����ϰ� �� ��
// �ؽ��Ŀ����� U�� V��ǥ���� ����ϱ� ������ �̸� ǥ���ϱ� ���� float2 �ڷ����� �̿�
// ���� ���̴��� �ȼ� ���̴����� �ؽ��� ��ǥ�� ��Ÿ���� ���� TEXCOORD0�̶�� ���� ���
// ���� ���� �ؽ��� ��ǥ�� �����ϴٸ� �� ���� 0���� �ƹ� ���ڷγ� ������ ���� ����
struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType TextureVertexShader(VertexInputType input)
{
    PixelInputType output;
    

    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    // �ռ� Ʃ�丮���� �÷� ���� ���̴��� �ؽ��� ���� ���̴��� ������ �������� ������ �״�� �����ϴ� ���� �ƴ϶� �ؽ����� ��ǥ���� �����ؼ� �ȼ� ���̴��� �����Ѵٴ� ��
    
    // Store the texture coordinates for the pixel shader
    output.tex = input.tex;
    
    return output;
}