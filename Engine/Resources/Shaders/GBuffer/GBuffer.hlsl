#pragma once

struct GFragment
{
    float3 albedo;
    float3 normal;
    float height;
    float ao;
    float roughness;
    float metallic;
    float shaderModel;
    float3 emissive;
    float depth;
};

//Temp, until better packing is added
GFragment UnpackGBuffer(in Texture2D gBufferHeap[], 
    in SamplerState nearestSampler, 
    in float2 texCoords)
{
    GFragment fi;
    
    float4 albedoTex = gBufferHeap[0].Sample(nearestSampler, texCoords);
    float4 normalHeightTex = gBufferHeap[1].Sample(nearestSampler, texCoords);
    float4 PBRTex = gBufferHeap[2].Sample(nearestSampler, texCoords);
    float4 emissiveTex = gBufferHeap[3].Sample(nearestSampler, texCoords);
    float depth = gBufferHeap[4].Sample(nearestSampler, texCoords).r;
    
    fi.albedo = albedoTex.rgb;
    fi.normal = normalize(normalHeightTex.rgb);
    fi.height = normalHeightTex.w;
    fi.ao = PBRTex.r;
    fi.roughness = PBRTex.g;
    fi.metallic = PBRTex.b;
    fi.shaderModel = emissiveTex.w;
    fi.emissive = emissiveTex.rgb;
    fi.depth = depth;
    
    return fi;
}