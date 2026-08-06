#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using Matrix4 = glm::mat4;
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Matrix2 = glm::mat2;
using Matrix3 = glm::mat3;
using Quaternion = glm::quat;
using Vec2i = glm::ivec2;
using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;
using Vec2u = glm::uvec2;
using Vec3u = glm::uvec3;
using Vec4u = glm::uvec4;
using Vec2d = glm::dvec2;
using Vec3d = glm::dvec3;
using Vec4d = glm::dvec4;
using Mat2d = glm::dmat2;
using Mat3d = glm::dmat3;
using Mat4d = glm::dmat4;
using Quatd = glm::dquat;

namespace Math {
inline Matrix4
perspective(float fov, float aspect, float zNear, float zFar)
{
  return glm::perspective(fov, aspect, zNear, zFar);
}

template<typename T>
T
clamp(T value, T min, T max)
{
  return std::max(min, std::min(value, max));
}

template<typename T>
T
mapRange(T value, T inMin, T inMax, T outMin, T outMax)
{
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

template<typename T>
T
lerp(T a, T b, float t)
{
  return a + (b - a) * t;
}

template<typename T>
T
vectorNormalize(const T& val)
{
  return glm::normalize(val);
}

template<typename T>
T
valueNormalize(const T& a, const T& b)
{
  return (b == 0) ? T(0) : a / b;
}

template<typename T>
T
dot(const T& a, const T& b)
{
  return glm::dot(a, b);
}

template<typename T>
T
cross(const T& a, const T& b)
{
  return glm::cross(a, b);
}

template<typename T>
T
length(const T& val)
{
  return glm::length(val);
}
template<typename T>
T
distance(const T& a, const T& b)
{
  return glm::distance(a, b);
}

template<typename T>
T
abs(const T& val)
{
  return glm::abs(val);
}

template<typename T>
T
sign(const T& val)
{
  return glm::sign(val);
}

template<typename T>
T
mod(const T& a, const T& b)
{
  return glm::mod(a, b);
}

template<typename T>
T
fract(const T& val)
{
  return glm::fract(val);
}

template<typename T>
T
step(const T& a, const T& b)
{
  return (a < b) ? T(1) : T(0);
}

template<typename T>
T
smoothstep(const T& a, const T& b, const T& t)
{
  return glm::smoothstep(a, b, t);
}

template<typename T>
T
inverseLerp(const T& a, const T& b, const T& value)
{
  return (value - a) / (b - a);
}

template<typename T>
T
quatLength(const T& q)
{
  return glm::length(q);
}

template<typename T>
T
quatNormalize(const T& q)
{
  return glm::normalize(q);
}

template<typename T>
T
quatDot(const T& a, const T& b)
{
  return glm::dot(a, b);
}
}