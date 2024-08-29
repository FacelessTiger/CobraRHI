[shader("vertex")]
float4 vertexMain(): SV_Position
{
    return float4(0, 0, 0, 1);
}

[shader("pixel")]
float4 pixelMain(): SV_Target
{
    return float4(1);
}