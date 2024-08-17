#pragma once

#ifdef __cplusplus
    // TODO: Maybe have a cobra matrix type instead of glm?
    #include <glm/glm.hpp>
    #include <CobraRHI.h>

    using float2 = Cobra::Vec2;
    using float3 = Cobra::Vec3;
    using float4 = Cobra::Vec4;
    using float4x4 = glm::mat4;

    template <typename T>
    using Ptr = uint64_t;

    namespace Cobra {

        template <typename T>
	    struct ImageHandle
        {
            uint32_t handle;

            ImageHandle(const Cobra::Image& image) { handle = image.GetHandle(); }
            ImageHandle(const Cobra::Image& image, const Cobra::Sampler& sampler) { handle = image.GetHandle() | (sampler.GetHandle() << 20); }

            ImageHandle(uint32_t image) { handle = image; }
            ImageHandle(uint32_t image, uint32_t sampler) { handle = image | (sampler << 20); }
        };

    }
#else
    float4 operator*(float4x4 matrix, float4 vector) { return mul(matrix, vector); }
    float4 operator*(float4 vector, float4x4 matrix) { return mul(vector, matrix); }
    float4x4 operator*(float4x4 matrixLeft, float4x4 matrixRight) { return mul(matrixLeft, matrixRight); }

    namespace Cobra {

        [vk::binding(0, 0)] SamplerState g_SamplerHeap[];
        [vk::binding(1, 0)] __DynamicResource g_RWTextureHeap[];
        [vk::binding(2, 0)] __DynamicResource g_TextureHeap[];

        struct ImageHandle<T>
        {
            private uint32_t handle;

            __init(uint32_t image) { handle = image; }
            __init(uint32_t image, uint32_t sampler) { handle = image | (sampler << 20); }

            __subscript(uint32_t idx) -> T
            {
                get { return GetTexture()[idx]; }
                set { GetRWTexture()[idx] = newValue; }
            }

            __subscript(uint2 idx) -> T
            {
                get { return GetTexture()[idx]; }
                set { GetRWTexture()[idx] = newValue; }
            }

            T Sample(float2 uv) { return GetTexture().Sample(GetSampler(), uv); }
            T SampleLod(float2 uv, uint32_t lod) { return GetTexture().SampleLevel(GetSampler(), uv, lod); }

            uint2 GetDimensions()
            {
                uint width, height;
                GetTexture().GetDimensions(width, height);

                return uint2(width, height);
            }

            private SamplerState GetSampler() { return g_SamplerHeap[NonUniformResourceIndex(handle >> 20)]; }
            private RWTexture2D<T> GetRWTexture() { return g_RWTextureHeap[NonUniformResourceIndex(handle & 0xfffff)]; }
            private Texture2D<T> GetTexture() { return g_TextureHeap[NonUniformResourceIndex(handle & 0xfffff)]; }
        }

        float UnpackUnorm1x8(uint32_t value) { return float(value & 0xff) / 255.0; }
        float UnpackSnorm1x8(uint32_t value) { return clamp(float(value & 0xff) / 127.0, -1.0, 1.0); }

        float4 UnpackUnorm4x8(uint32_t value)
        {
            __target_switch
            {
                case glsl: __intrinsic_asm "unpackUnorm4x8";
                case spirv: return spirv_asm {
                    result:$$float4 = OpExtInst glsl450 UnpackUnorm4x8 $value
                };
                default:
                    return float4(
                        UnpackUnorm1x8(value),
                        UnpackUnorm1x8(value >> 8),
                        UnpackUnorm1x8(value >> 16),
                        UnpackUnorm1x8(value >> 24)
                    );
            }
        }

        float4 UnpackSnorm4x8(uint32_t value)
        {
            __target_switch
            {
                case glsl: __intrinsic_asm "unpackSnorm4x8";
                case spirv: return spirv_asm {
                    result:$$float4 = OpExtInst glsl450 UnpackSnorm4x8 $value
                };
                default:
                    return float4(
                        UnpackSnorm1x8(value),
                        UnpackSnorm1x8(value >> 8),
                        UnpackSnorm1x8(value >> 16),
                        UnpackSnorm1x8(value >> 24)
                     );
            }
        }

    }
#endif