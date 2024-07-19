#include "SharedShaderDefs.inl"

struct VertexOutput
{
    float4 position: SV_Position;
    float4 color;
    float2 uv;
};

[vk::push_constant] Slang::DrawPush push;

[vk::binding(0, 0)] SamplerState g_SamplerState[];
[vk::binding(2, 0)] Texture2D<float4> g_Texture2DFloat4[];

float4 UnpackUnorm4x8(uint value)
{
    uint4 packed = uint4(value & 0xff, (value >> 8) & 0xff, (value >> 16) & 0xff, value >> 24);
    return float4(packed) / 255.0;
}

[shader("vertex")]
VertexOutput vertexMain(uint vertexID: SV_VertexID)
{
    Slang::Vertex vertex = push.vertices[vertexID];
    
    VertexOutput output;
    output.position = float4(vertex.position * push.scale + push.translate, 0, 1);
    output.color = UnpackUnorm4x8(vertex.color);
    output.uv = vertex.uv;

    // HACK
    output.position.y *= -1;

    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput vertex): SV_Target
{
    return vertex.color * g_Texture2DFloat4[NonUniformResourceIndex(push.image & 0xfffff)].Sample(g_SamplerState[NonUniformResourceIndex(push.image >> 20)], vertex.uv);
}