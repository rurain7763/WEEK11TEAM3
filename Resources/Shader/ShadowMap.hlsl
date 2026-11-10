#pragma pack_matrix(row_major)

cbuffer ShadowViewConstants : register(b0)
{
    matrix LightViewProjection;
};

cbuffer PerObjectConstants : register(b2) // FRenderer의 PerObjectCB와 같은 레이아웃
{
    matrix World;
    matrix NormalMatrix;
};

struct FVSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float4 Color : COLOR; // 지금은 쓰지 않지만 나중에 쓰게 될 수 있음
    float2 UV : TEXCOORD;
#if USE_SKINNING
    int4 BoneIndices : BLENDINDICES;
    float4 BoneWeights : BLENDWEIGHT;
    int BoneInfluenceCount : BONECOUNT;
#endif
};

struct FPSInput
{
    float4 Position : SV_POSITION;
};

#if USE_SKINNING
// StructuredBuffer 행렬의 CPU 행 우선 데이터 해석 보정
StructuredBuffer<float4x4> BoneMatrices : register(t0);
#endif

// FVertexPNCT의 첫 요소만 받는다 : 입력 레이아웃 오프셋이 정확히 맞는다
FPSInput mainVS(FVSInput Input)
{
    FPSInput Output;
    
    float3 LocalPosition = Input.Position;
#if USE_SKINNING
    if (Input.BoneInfluenceCount > 0)
    {
        LocalPosition = float3(0, 0, 0);
        for (int i = 0; i < Input.BoneInfluenceCount; i++)
        {
            int BoneIndex = Input.BoneIndices[i];
            float BoneWeight = Input.BoneWeights[i];
            float4x4 BoneMatrix = transpose(BoneMatrices[BoneIndex]);
            LocalPosition += mul(float4(Input.Position, 1.0), BoneMatrix).xyz * BoneWeight;
        }
    }
#endif
    
    Output.Position = mul(mul(float4(LocalPosition, 1.0f), World), LightViewProjection);
    
    return Output;
}

// 로더(LoadOrCompileShader)가 mainPS를 요구하므로 둔다. 실제로는 PS를 null로 바인딩한다.
void mainPS(FPSInput Input)
{
}