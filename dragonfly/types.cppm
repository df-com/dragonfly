// License: MIT License   See LICENSE.txt for the full license.
module;
#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <compare>
#include <format>
#include "hwy/highway.h"

export module dragonfly:types;

namespace hn = hwy::HWY_NAMESPACE;

export namespace dragonfly {

#include <cstdint>
#include <cstring>
#include <compare>

template<float MinVal, float MaxVal>
struct alignas(2) BoundedFloat16 {
	uint16_t raw_bits;

	BoundedFloat16() = default;

	explicit constexpr BoundedFloat16(uint16_t raw) : raw_bits(raw) {}

	static constexpr BoundedFloat16 from_float32(float f) {
		if (f <= MinVal) return BoundedFloat16(static_cast<uint16_t>(0));
		if (f >= MaxVal) return BoundedFloat16(static_cast<uint16_t>(65535));

		constexpr float range = MaxVal - MinVal;
		float normalized = (f - MinVal) / range;
		return BoundedFloat16(static_cast<uint16_t>(normalized * 65535.0f + 0.5f));
	}

	inline constexpr float to_float32() const {
		constexpr float range = MaxVal - MinVal;
		return static_cast<float>(raw_bits) * (range / 65535.0f) + MinVal;
	}

	inline constexpr double to_double() const { return static_cast<double>(to_float32()); }

	explicit inline operator float() const { return to_float32(); }
	explicit inline operator double() const { return to_double(); }

	constexpr BoundedFloat16(float f) : BoundedFloat16(from_float32(f)) {}
	constexpr BoundedFloat16(double d) : BoundedFloat16(from_float32(static_cast<float>(d))) {}

	constexpr BoundedFloat16& operator=(float f) { *this = from_float32(f); return *this; }
	constexpr BoundedFloat16& operator=(double d) { *this = from_float32(static_cast<float>(d)); return *this; }

	constexpr auto operator<=>(const BoundedFloat16& other) const { return raw_bits <=> other.raw_bits; }
	constexpr bool operator==(const BoundedFloat16& other) const = default;

	constexpr auto operator<=>(float f) const { return to_float32() <=> f; }
	constexpr bool operator==(float f) const { return to_float32() == f; }

	constexpr auto operator<=>(double d) const { return to_double() <=> d; }
	constexpr bool operator==(double d) const { return to_double() == d; }

	constexpr BoundedFloat16 operator+(BoundedFloat16 rhs) const { return BoundedFloat16(to_float32() + rhs.to_float32()); }
	constexpr BoundedFloat16 operator-(BoundedFloat16 rhs) const { return BoundedFloat16(to_float32() - rhs.to_float32()); }
	constexpr BoundedFloat16 operator*(BoundedFloat16 rhs) const { return BoundedFloat16(to_float32() * rhs.to_float32()); }
	constexpr BoundedFloat16 operator/(BoundedFloat16 rhs) const { return BoundedFloat16(to_float32() / rhs.to_float32()); }

	constexpr BoundedFloat16& operator+=(BoundedFloat16 rhs) { *this = *this + rhs; return *this; }
	constexpr BoundedFloat16& operator-=(BoundedFloat16 rhs) { *this = *this - rhs; return *this; }
	constexpr BoundedFloat16& operator*=(BoundedFloat16 rhs) { *this = *this * rhs; return *this; }
	constexpr BoundedFloat16& operator/=(BoundedFloat16 rhs) { *this = *this / rhs; return *this; }

	constexpr BoundedFloat16 operator+() const { return *this; }
	constexpr BoundedFloat16 operator-() const { return BoundedFloat16(-to_float32()); }
};

struct alignas(2) BitHackFloat16 {
	uint16_t raw_bits;

	BitHackFloat16() = default;

	explicit constexpr BitHackFloat16(uint16_t raw) : raw_bits(raw) {}

	static constexpr BitHackFloat16 from_float32(float f) {
		if (f <= 0.0f) return BitHackFloat16(static_cast<uint16_t>(0));
		if (f >= 1.0f) return BitHackFloat16(static_cast<uint16_t>(65535));
		return BitHackFloat16(static_cast<uint16_t>(f * 65535.0f + 0.5f));
	}

	inline float to_float32_bithack() const {
		uint32_t bits = 0x3F800000U | (static_cast<uint32_t>(raw_bits) << 7);
		float f;
		std::memcpy(&f, &bits, sizeof(f));
		return f - 1.0f;
	}

	inline constexpr float to_float32_mul() const { return static_cast<float>(raw_bits) * (1.0f / 65535.0f); }
	inline constexpr double to_double() const { return static_cast<double>(to_float32_mul()); }

	explicit inline operator float() const { return to_float32_mul(); }
	explicit inline operator double() const { return to_double(); }

	constexpr BitHackFloat16(float f) : BitHackFloat16(from_float32(f)) {}
	constexpr BitHackFloat16(double d) : BitHackFloat16(from_float32(static_cast<float>(d))) {}

	constexpr auto operator<=>(const BitHackFloat16& other) const { return raw_bits <=> other.raw_bits; }
	constexpr bool operator==(const BitHackFloat16& other) const = default;
	constexpr auto operator<=>(float f) const { return to_float32_mul() <=> f; }
	constexpr bool operator==(float f) const { return to_float32_mul() == f; }
	constexpr auto operator<=>(double d) const { return to_double() <=> d; }
	constexpr bool operator==(double d) const { return to_double() == d; }

	constexpr BitHackFloat16 operator+(BitHackFloat16 rhs) const { return BitHackFloat16(to_float32_mul() + rhs.to_float32_mul()); }
	constexpr BitHackFloat16 operator-(BitHackFloat16 rhs) const { return BitHackFloat16(to_float32_mul() - rhs.to_float32_mul()); }
	constexpr BitHackFloat16& operator+=(BitHackFloat16 rhs) { *this = *this + rhs; return *this; }
	constexpr BitHackFloat16& operator-=(BitHackFloat16 rhs) { *this = *this - rhs; return *this; }
	constexpr BitHackFloat16 operator+() const { return *this; }
	constexpr BitHackFloat16 operator-() const { return BitHackFloat16(-to_float32_mul()); }
};

static_assert(sizeof(BitHackFloat16) == 2, "BitHackFloat16 must be exactly 2 bytes!");

constexpr float DEFAULT_BITHACK_EPSILON = 1.0f / (2.0f * 65535.0f);

template <typename T>
inline bool IsNearEqual(BitHackFloat16 bh, T val, double eps = DEFAULT_BITHACK_EPSILON) {
	static_assert(std::is_floating_point_v<T>, "msut float or double");
	return std::abs(static_cast<double>(bh.to_float32_mul()) - static_cast<double>(val)) <= eps;
}

// UnitBitHack16 -> float32
inline void BatchDecodeRaw(const BitHackFloat16* HWY_RESTRICT in, float* HWY_RESTRICT out, size_t count) {
	const auto* raw_in = reinterpret_cast<const uint16_t*>(in);

	const hn::ScalableTag<float> df;
	const hn::Rebind<uint16_t, decltype(df)> du16;
	const hn::Rebind<uint32_t, decltype(df)> du32;

	const size_t N = hn::Lanes(df);
	const auto inv_scale = hn::Set(df, 1.0f / 65535.0f);

	size_t i = 0;
	for (; i + N <= count; i += N) {
		auto v_u16 = hn::LoadU(du16, raw_in + i);
		auto v_u32 = hn::PromoteTo(du32, v_u16);
		auto v_f32 = hn::ConvertTo(df, v_u32);
		auto v_res = hn::Mul(v_f32, inv_scale);
		hn::StoreU(v_res, df, out + i);
	}

	// Scalar fallback
	for (; i < count; ++i) {
		out[i] = in[i].to_float32_mul();
	}
}

// float32 -> UnitBitHack16
inline void BatchEncodeRaw(const float* HWY_RESTRICT in, BitHackFloat16* HWY_RESTRICT out, size_t count) {
	auto* raw_out = reinterpret_cast<uint16_t*>(out);

	const hn::ScalableTag<float> df;
	const hn::Rebind<uint16_t, decltype(df)> du16;
	const hn::Rebind<uint32_t, decltype(df)> du32;

	const size_t N = hn::Lanes(df);
	const auto scale = hn::Set(df, 65535.0f);
	const auto zero = hn::Set(df, 0.0f);
	const auto one = hn::Set(df, 1.0f);
	const auto half = hn::Set(df, 0.5f);

	size_t i = 0;
	for (; i + N <= count; i += N) {
		auto v_f32 = hn::LoadU(df, in + i);

		// 1. Clamp range [0.0, 1.0]
		v_f32 = hn::Min(hn::Max(v_f32, zero), one);

		// 2. Scale and add 0.5f for proper rounding
		auto v_scaled = hn::Add(hn::Mul(v_f32, scale), half);

		// 3. Convert to uint32 and demote to uint16
		auto v_u32 = hn::ConvertTo(du32, v_scaled);
		auto v_u16 = hn::DemoteTo(du16, v_u32);

		hn::StoreU(v_u16, du16, raw_out + i);
	}

	// Scalar fallback
	for (; i < count; ++i) {
		out[i] = BitHackFloat16::from_float32(in[i]);
	}
}

// std::vector¡¢std::array
template <typename ContainerA, typename ContainerB>
inline bool AreEqual(const ContainerA& a, const ContainerB& b, double eps = DEFAULT_BITHACK_EPSILON) {
	using ElemA = typename ContainerA::value_type;
	using ElemB = typename ContainerB::value_type;

	static_assert(std::is_same_v<ElemA, BitHackFloat16>, "must BitHackFloat16");
	static_assert(std::is_floating_point_v<ElemB>, "must float or double");

	if (a.size() != b.size()) {
		return false;
	}

	for (std::size_t i = 0; i < a.size(); ++i) {
		if (!IsNearEqual(a[i], b[i], eps)) {
			return false;
		}
	}
	return true;
}

inline std::vector<float> ToVector(const std::vector<BitHackFloat16>& input) {
	std::vector<float> result(input.size());
	if (!input.empty()) {
		BatchDecodeRaw(input.data(), result.data(), input.size());
	}
	return result;
}

inline std::vector<BitHackFloat16> ToBitHackFloat16(const std::vector<float>& input) {
	std::vector<BitHackFloat16> result(input.size());
	if (!input.empty()) {
		BatchEncodeRaw(input.data(), result.data(), input.size());
	}
	return result;
}

template <size_t N>
inline std::array<float, N> ToArray(const std::array<BitHackFloat16, N>& input) {
	std::array<float, N> result;
	BatchDecodeRaw(input.data(), result.data(), N);
	return result;
}

template <size_t N>
inline std::array<BitHackFloat16, N> ToBitHackFloat16(const std::array<float, N>& input) {
	std::array<BitHackFloat16, N> result;
	BatchEncodeRaw(input.data(), result.data(), N);
	return result;
}

template <size_t ArraySize>
inline std::vector<std::array<float, ArraySize>> ToArrayVector(const std::vector<std::array<BitHackFloat16, ArraySize>>& input) {
	std::vector<std::array<float, ArraySize>> result(input.size());
	if (input.empty()) return result;

	size_t total_elements = input.size() * ArraySize;

	BatchDecodeRaw(reinterpret_cast<const BitHackFloat16*>(input.data()), reinterpret_cast<float*>(result.data()), total_elements);

	return result;
}

template <size_t ArraySize>
inline std::vector<std::array<BitHackFloat16, ArraySize>> ToBitHackFloat16(const std::vector<std::array<float, ArraySize>>& input) {
	std::vector<std::array<BitHackFloat16, ArraySize>> result(input.size());
	if (input.empty()) return result;

	size_t total_elements = input.size() * ArraySize;

	BatchEncodeRaw(reinterpret_cast<const float*>(input.data()), reinterpret_cast<BitHackFloat16*>(result.data()), total_elements);

	return result;
}

template<typename T>
struct interval {
	T min{};
	T max{};
	interval() {
		reset();
	}
	constexpr interval(T min, T max) noexcept
		: min(min), max(max) {}

	template<typename U>
	constexpr interval(U min, U max) noexcept
		: min(static_cast<T>(min)), max(static_cast<T>(max)) {}

	constexpr bool contains(const T& value) const noexcept {
		if (value > max || value < min) {
			return false;
		}
		return true;
	}
	T delta() const { return max - min; }
	void reset() {
		if constexpr (std::is_arithmetic_v<T>) {
			this->min = std::numeric_limits<T>::lowest();
			this->max = std::numeric_limits<T>::max();
		}
		else {
			this->min = T::min();
			this->max = T::max();
		}
	}
	void reset(T min, T max) {
		this->min = min;
		this->max = max;
	}
	interval<T>& operator =(const T& rhs) {
		this->min = rhs.min;
		this->max = rhs.max;
		return *this;
	}
	interval<T>& operator =(const std::tuple<T, T>& rhs) {
		this->min = std::get<0>(rhs);
		this->max = std::get<1>(rhs);
		return *this;
	}
	interval<T>& operator =(const T&& rhs) {
		this->min = rhs.min;
		this->max = rhs.max;
		return *this;
	}
	std::string to_string(std::string_view f = "[{},{}]") const { return std::format(f, min, max); }
	friend bool operator==(const interval<T>& lhs, const interval<T>& rhs) {
		return equal(lhs.min, rhs.min) && equal(lhs.max, rhs.max);
	}
};

class MixedRadix {
private:
	std::vector<int> bases_;   // high -> low, like {10, 3, 26}
	std::vector<int> digits_;  // high -> low£¬like {1, 2, 25}
	template <typename T>
	static std::vector<int> to_int_vector(const std::vector<T>& src) {
		std::vector<int> res;
		res.reserve(src.size());
		for (const auto& val : src) {
			res.push_back(static_cast<int>(val));
		}
		return res;
	}
public:
	const std::vector<int>& digits() const { return digits_; }

	MixedRadix(std::vector<int> bases, std::vector<int> digits)
		: bases_(std::move(bases)), digits_(std::move(digits)) {
		if (this->digits_.size() != this->bases_.size()) {
			throw std::invalid_argument("digits does not match the number of bases");
		}
	}

	explicit MixedRadix(std::vector<int> bases) : bases_(std::move(bases)) {
		digits_.resize(this->bases_.size(), 0);
	}

	MixedRadix(const std::vector<std::size_t>& bases, const std::vector<std::size_t>& digits)
		: MixedRadix(to_int_vector(bases), to_int_vector(digits)) {}

	explicit MixedRadix(const std::vector<std::size_t>& bases)
		: MixedRadix(to_int_vector(bases)) {}
	long long to_decimal() const {
		long long total = 0;
		for (size_t i = 0; i < bases_.size(); ++i) {
			total += digits_[i];
			if (i + 1 < bases_.size()) {
				total *= bases_[i + 1];
			}
		}
		return total;
	}
	void FromDecimal(long long value) {
		for (int i = static_cast<int>(bases_.size()) - 1; i >= 0; --i) {
			digits_[i] = value % bases_[i];
			value /= bases_[i];
		}
	}
	long long max_decimal() const {
		long long max_val = 1;
		for (int base : bases_) {
			max_val *= base;
		}
		return max_val - 1;
	}
	MixedRadix max_value() const {
		std::vector<int> max_digits(bases_.size());
		for (size_t i = 0; i < bases_.size(); ++i) {
			max_digits[i] = bases_[i] - 1;
		}
		return MixedRadix(bases_, max_digits);
	}
	MixedRadix& operator=(long long value) {
		FromDecimal(value);
		return *this;
	}
	MixedRadix& operator=(const MixedRadix& rhs) {
		if (this != &rhs) {
			if (this->bases_ != rhs.bases_) {
				throw std::invalid_argument("this->bases_ != rhs.bases_)");
			}
			this->digits_ = rhs.digits_;
		}
		return *this;
	}

	friend std::ostream& operator<<(std::ostream& os, const MixedRadix& mr) {
		for (size_t i = 0; i < mr.digits_.size(); ++i) {
			os << mr.digits_[i] << (i == mr.digits_.size() - 1 ? "" : "-");
		}
		return os;
	}
};

}