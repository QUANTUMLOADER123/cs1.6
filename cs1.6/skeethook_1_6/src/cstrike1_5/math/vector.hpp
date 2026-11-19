#pragma once

#include <cmath>
#include <cfloat>
#include <algorithm>

struct vec2_t
{
    float x, y;

    vec2_t() : x(0.0f), y(0.0f) {}
    vec2_t(float px, float py) : x(px), y(py) {}

    vec2_t operator+(const vec2_t& other) const { return vec2_t(x + other.x, y + other.y); }
    vec2_t operator-(const vec2_t& other) const { return vec2_t(x - other.x, y - other.y); }
    vec2_t operator*(float scale) const { return vec2_t(x * scale, y * scale); }
    vec2_t operator/(float scale) const { return vec2_t(x / scale, y / scale); }

    bool operator==(const vec2_t& other) const { return x == other.x && y == other.y; }

    float length() const { return std::sqrtf(length_sqr()); }
    float length_sqr() const { return x * x + y * y; }

    static vec2_t zero() { return vec2_t(0.0f, 0.0f); }
};

struct vec3_t
{
    float x, y, z;

    vec3_t() : x(0.0f), y(0.0f), z(0.0f) {}
    vec3_t(float px, float py, float pz) : x(px), y(py), z(pz) {}

    vec3_t operator+(const vec3_t& other) const { return vec3_t(x + other.x, y + other.y, z + other.z); }
    vec3_t operator-(const vec3_t& other) const { return vec3_t(x - other.x, y - other.y, z - other.z); }
    vec3_t operator*(float scale) const { return vec3_t(x * scale, y * scale, z * scale); }
    vec3_t operator/(float scale) const { return vec3_t(x / scale, y / scale, z / scale); }

    vec3_t& operator+=(const vec3_t& other) { x += other.x; y += other.y; z += other.z; return *this; }
    vec3_t& operator-=(const vec3_t& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    vec3_t& operator*=(float scale) { x *= scale; y *= scale; z *= scale; return *this; }
    vec3_t& operator/=(float scale) { x /= scale; y /= scale; z /= scale; return *this; }

    bool operator==(const vec3_t& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const vec3_t& other) const { return !(*this == other); }

    float length() const { return std::sqrtf(length_sqr()); }
    float length_sqr() const { return x * x + y * y + z * z; }
    float length_2d() const { return std::sqrtf(length_2d_sqr()); }
    float length_2d_sqr() const { return x * x + y * y; }

    float dot(const vec3_t& other) const { return x * other.x + y * other.y + z * other.z; }

    vec3_t cross(const vec3_t& other) const { return vec3_t(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x); }

    vec3_t& normalize()
    {
        const float length = this->length();
        const float radius = 1.0f / (length + FLT_EPSILON);

        x *= radius;
        y *= radius;
        z *= radius;

        return *this;
    }

    vec3_t& normalize_angle()
    {
        while (x < -180.0f) x += 360.0f;
        while (x > 180.0f) x -= 360.0f;
        while (y < -180.0f) y += 360.0f;
        while (y > 180.0f) y -= 360.0f;

        return *this;
    }

    vec3_t& clamp()
    {
        x = std::clamp(x, -89.0f, 89.0f);
        y = std::clamp(std::remainder(y, 360.0f), -180.0f, 180.0f);
        z = 0.0f;

        return *this;
    }

    bool is_valid() const { return x != 0.0f || y != 0.0f || z != 0.0f; }

    vec3_t normalized() const { return *this / (length() + FLT_EPSILON); }
};

struct vec4_t
{
    float x, y, z, w;

    vec4_t() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    vec4_t(float px, float py, float pz, float pw) : x(px), y(py), z(pz), w(pw) {}

    static vec4_t zero() { return vec4_t(0.0f, 0.0f, 0.0f, 0.0f); }
};
