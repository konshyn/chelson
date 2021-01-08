struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(float3 position : POSITION, uint id: SV_VertexID)
{
    PSInput result;
    result.position = float4(position - float3(0.3f, 0.3f, 0.3f), 1.0f);
	//float2 texcoord = float2((id << 1) & 2, id & 2);
	//result.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 1.0f, 1.0f);
}