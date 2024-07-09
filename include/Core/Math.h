#pragma once

#include <cstdint>

namespace Cobra {

	template <typename T>
	struct Vec2Base
	{
		union {
			struct { T x, y; };
			struct { T r, g; };
			T data[2];
		};

		Vec2Base() = default;
		Vec2Base(T x, T y) : x(x), y(y) { }

		Vec2Base<T> operator-(const Vec2Base<T>& other) { return { x - other.x, y - other.y }; }
		Vec2Base<T> operator*(const T& other) { return { x * other, y * other }; }
		
		template <typename U>
		explicit operator Vec2Base<U>() const { return Vec2Base<U>((U)x, (U)y); }
	};

	template <typename T>
	bool operator==(const Vec2Base<T>& left, const Vec2Base<T>& right) { return left.x == right.x && left.y == right.y; }

	template <typename T>
	struct Vec3Base
	{
		union {
			struct { T x, y, z; };
			struct { T r, g, b; };
			T data[3];
		};

		Vec3Base() = default;
		Vec3Base(T x, T y, T z) : x(x), y(y), z(z) { }
		Vec3Base(const Vec2Base<T>& v, T z) : x(v.x), y(v.y), z(z) { }
	};

	template <typename T>
	struct Vec4Base
	{
		union {
			struct { T x, y, z, w; };
			struct { T r, g, b, a; };
			T data[4];
		};

		Vec4Base(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
		Vec4Base(const Vec2Base<T>& v, T z, T w) : x(v.x), y(v.y), z(z), w(w) { }
		Vec4Base(const Vec3Base<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) { }
	};

	using uVec2 = Vec2Base<uint32_t>;
	using iVec2 = Vec2Base<int32_t>;
	using Vec2 = Vec2Base<float>;

	using uVec3 = Vec3Base<uint32_t>;
	using iVec3 = Vec3Base<int32_t>;
	using Vec3 = Vec3Base<float>;

	using uVec4 = Vec4Base<uint32_t>;
	using iVec4 = Vec4Base<int32_t>;
	using Vec4 = Vec4Base<float>;

}