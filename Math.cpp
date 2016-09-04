/*
MIT License

Copyright (c) 2016 Patrick Lafferty

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Math.h"
#include <cmath>

namespace Math
{
    void Vector3::normalize()
    {
        float length = sqrt(lengthSquared(*this));

        if (length > 0.f)
        {
            *this = *this * (1.f / length);
        }
        else
        {
            x = y = z = 0.f;
        }        
    }

    void Vector3::operator-=(const Vector3& v)
    {
        *this = *this - v;
    }

    Vector3 operator+(const Vector3& lhs, const Vector3& rhs)
    {
        Vector3 result;
        result.x = lhs.x + rhs.x;
        result.y = lhs.y + rhs.y;
        result.z = lhs.z + rhs.z;

        return result;
    }

    Vector3 operator-(const Vector3& lhs, const Vector3& rhs)
    {   
        Vector3 result;
        result.x = lhs.x - rhs.x;
        result.y = lhs.y - rhs.y;
        result.z = lhs.z - rhs.z;

        return result;
    }

    Vector3 operator*(const Vector3& v, float s)
    {
        Vector3 result;
        result.x = v.x * s;
        result.y = v.y * s;
        result.z = v.z * s;

        return result;
    }

    float lengthSquared(const Vector3& v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    float distanceSquared(const Vector3& lhs, const Vector3& rhs)
    {
        return lengthSquared(lhs - rhs);
    }

    float dot(const Vector3& lhs, const Vector3& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    float roundToInt(float x)
    {
        return round(x);
    }

    float clamp(float x, float minimum, float maximum)
    {
        return fmax(fmin(x, maximum), minimum);
    }
}