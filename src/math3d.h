#ifndef MATH3D_H
#define MATH3D_H

#include <cmath>

struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0);
    Vec3 operator+(const Vec3& b) const;
    Vec3 operator-(const Vec3& b) const;
    Vec3 operator*(float s) const;
    float dot(const Vec3& b) const;
    Vec3 cross(const Vec3& b) const;
    float length() const;
    Vec3 normalize() const;
};

// Вращение
Vec3 rotateX(const Vec3& v, float angle);
Vec3 rotateY(const Vec3& v, float angle);
Vec3 rotateZ(const Vec3& v, float angle);

#endif
