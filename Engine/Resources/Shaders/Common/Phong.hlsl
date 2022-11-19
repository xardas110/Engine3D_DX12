#pragma once

//L - lightdir
//N - normal
float3 CalculateDiffuse(in float3 L, in float3 N, in float3 lightColor)
{
    float factor = max(dot(N, -L), 0.f);
    return factor * lightColor;
}