struct VertexOutput
{
   float4 position: SV_Position;
   float3 color: COLOR0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexID: SV_VertexID)
{
   float2 positions[3] = {
      { 0.0, 0.5 },
      { 0.5, -0.5 },
      { -0.5, -0.5 }
   };

   float3 colors[3] = {
      { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 }
   };

   VertexOutput output;
   output.position = float4(positions[vertexID], 0, 1);
   output.color = colors[vertexID];
   return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput vertexOutput): SV_Target
{
   return float4(vertexOutput.color, 1);
}