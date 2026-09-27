#include <cmath>
#include <vector>
#include <mutex>

namespace Engine
{
	struct Vec2
	{
		float x, y;
		Vec2() : x(0), y(0) {}
		Vec2(float x, float y) : x(x), y(y) {}

		Vec2 operator+(const Vec2& other) const;
		Vec2 operator-(const Vec2& other) const;
		Vec2 operator*(float scalar) const;
		Vec2 operator/(float scalar) const;
		Vec2 operator=(float* other) const;
		float operator*(const Vec2& other) const;
		Vec2& operator*=(const Vec2& other);
		Vec2 operator*=(float scalar);

		float length() const;
		void normalize();
	};

	struct Vec3
	{
		float x, y, z;
		Vec3() : x(0), y(0), z(0) {}
		Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

		Vec3 operator+(const Vec3& other) const;
		Vec3 operator-(const Vec3& other) const;
		Vec3 operator*(float scalar) const;
		Vec3 operator/(float scalar) const;
		float operator*(const Vec3& other) const;
		Vec3 operator^(const Vec3& other) const;
		Vec3 operator=(float* other) const;
		Vec3& operator+=(const float* other);
		Vec3& operator-=(const float* other);
		Vec3& operator+=(const Vec3& other);
		Vec3& operator-=(const Vec3& other);
		Vec3& operator*=(const Vec3& other);
		Vec3 operator*=(const float scalar);
		bool operator==(Vec3 other) const;

		bool isValid();
		float dot(const Vec3& other);
		float length() const;
		void normalize();
		float Distance(Vec3& other);
	};

	struct Vec4
	{
		float x, y, z, w;
		Vec4() : x(0), y(0), z(0), w(0) {}
		Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		Vec4 operator+(const Vec4& other) const;
		Vec4 operator-(const Vec4& other) const;
		Vec4 operator*(float scalar) const;
		Vec4 operator/(float scalar) const;
		float operator*(const Vec4& other) const;
		Vec4 operator=(float* other) const;
		Vec4& operator*=(const Vec4& other);
		float length() const;
		void normalize();
	};

	struct Matrix16
	{
		float m[16];

		Vec4 MatrixMultiply(const Vec3& v) const;
		Vec4 MatrixMultiply(const Vec4& v) const;
	};

	struct Matrix4x4
	{
		float m[4][4];

		Matrix4x4 operator*(const Matrix4x4& mtx) const;
		Vec4 operator*(const Vec4& v) const;

		Vec3 Translate() const; // returns origin
		Vec3 Row0() const;
		Vec3 Row1() const;
		Vec3 Row2() const;
		Vec3 Row3() const;

		Vec4 MatrixMultiply(const Vec3& v) const;
		Vec4 MatrixMultiply(const Vec4& v) const;
		Vec3 TransformPoint3(const Vec3& v) const;
		Vec4 TransformPoint(const Vec3& v) const;
		Vec4 TransformPoint(const Vec4& v) const;
	};

	struct AABB
	{
		Vec3 min, max;

		void GetBoxVerts(Vec3 out[8]);
		void GetRotatedBoxVerts(const Matrix4x4& mtx, Vec3 out[8]);
	};

	static inline Vec3 QuaternionRotate(Vec4 q, Vec3 v)
	{
		// q.xyz = imaginary component
		// q.w   = real component

		Vec3 qv =
		{
			q.x,
			q.y,
			q.z
		};

		Vec3 uv =
		{
			qv.y * v.z - qv.z * v.y,
			qv.z * v.x - qv.x * v.z,
			qv.x * v.y - qv.y * v.x
		};

		Vec3 uuv =
		{
			qv.y * uv.z - qv.z * uv.y,
			qv.z * uv.x - qv.x * uv.z,
			qv.x * uv.y - qv.y * uv.x
		};

		float s = 2.0f * q.w;

		Vec3 out =
		{
			v.x + (uv.x * s) + (uuv.x * 2.0f),
			v.y + (uv.y * s) + (uuv.y * 2.0f),
			v.z + (uv.z * s) + (uuv.z * 2.0f)
		};

		return out;
	}
}