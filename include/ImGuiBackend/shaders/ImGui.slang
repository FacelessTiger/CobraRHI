#include "SharedShaderDefs.inl"

struct VertexOutput
{
    float4 position: SV_Position;
    float4 color;
    float2 uv;
};

[vk::push_constant] ImGui::Slang::Push push;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID: SV_VertexID)
{
    ImGui::Slang::Vertex vertex = push.vertices[vertexID];
    
    VertexOutput output;
    output.position = float4(vertex.position * push.scale + push.translate, 0, 1);
    output.color = Cobra::UnpackUnorm4x8(vertex.color);
    output.uv = vertex.uv;

    // HACK
    output.position.y *= -1;

    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput vertex): SV_Target
{
    return vertex.color * push.image.Sample(vertex.uv);
}