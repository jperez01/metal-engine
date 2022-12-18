#pragma once
#include <simd/simd.h>

namespace math
{
    struct myFloat3 {
        float x;
        float y;
        float z;
    };

    struct myFloat4 {
        float x;
        float y;
        float z;
        float w;
    };

    struct myFloat4x4 {
        myFloat4 columns[4];
    };

    simd::float3 add( const simd::float3& a, const simd::float3& b );
    simd_float4x4 makeIdentity();
    simd::float4x4 makePerspective(float fovRadians, float aspect, float znear, float zfar);
    simd::float4x4 makeXRotate( float angleRadians );
    simd::float4x4 makeYRotate( float angleRadians );
    simd::float4x4 makeZRotate( float angleRadians );
    simd::float4x4 makeTranslate( const simd::float3& v );
    simd::float4x4 makeScale( const simd::float3& v );
    simd::float3x3 discardTranslation( const simd::float4x4& m );
}
