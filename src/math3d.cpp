#include "math3d.h"

Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

Vec3 Vec3::operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
Vec3 Vec3::operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
Vec3 Vec3::operator*(float s) const { return {x * s, y * s, z * s}; }
float Vec3::dot(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }
Vec3 Vec3::cross(const Vec3& b) const {
    return {
        y * b.z - z * b.y,
        z * b.x - x * b.z,
        x * b.y - y * b.x
    };
}
float Vec3::length() const { return sqrt(x * x + y * y + z * z); }
Vec3 Vec3::normalize() const {
    float len = length();
    if (len == 0) return {0, 0, 0};
    return {x / len, y / len, z / len};
}

Vec3 rotateX(const Vec3& v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    return {
        v.x,
        v.y * cosA - v.z * sinA,
        v.y * sinA + v.z * cosA
    };
}

Vec3 rotateY(const Vec3& v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    return {
        v.x * cosA + v.z * sinA,
        v.y,
        -v.x * sinA + v.z * cosA
    };
}

Vec3 rotateZ(const Vec3& v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    return {
        v.x * cosA - v.y * sinA,
        v.x * sinA + v.y * cosA,
        v.z
    };
}
