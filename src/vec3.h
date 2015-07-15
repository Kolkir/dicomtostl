#ifndef _VEC3_H_
#define _VEC3_H_

#include <cmath>
#include <limits>

namespace DicomToStl
{

struct Vec3
{
    Vec3() : x(0), y(0), z(0){}
    Vec3(float x, float y, float z) : x(x), y(y), z(z){}
    float x;
    float y;
    float z;
};

inline Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 VecAbs(const Vec3& a)
{
    return Vec3(abs(a.x), abs(a.y), abs(a.z));
}

inline float VecLength(const Vec3& a)
{
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

inline Vec3 VecCross(const Vec3& a, const Vec3& b)
{
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline void VecNormalize(Vec3& v)
{
    float l = VecLength(v);
    if (l > std::numeric_limits<float>::epsilon())
    {
        l = 1 / l;
        v.x *= l;
        v.y *= l;
        v.z *= l;
    }
}

}
#endif
