// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fmt/format.h>

export module dragonfly:interval;

export namespace dragonfly {

template<typename T>
struct interval {
	T min;
	T max;
	interval() { reset(); }
	interval(T min, T max) {
		this->min = min;
		this->max = max;
	}
	template<typename U>
	interval(U min, U max) {
		this->min = min;
		this->max = max;
	}
	bool contains(const T& value) const {
		if constexpr (std::is_floating_point<T>::value) {
			if (std::isgreater(value, max) || std::isless(value, min)) { return false; }
		}
		else {
			if (value > max || value < min) { return false; }
		}
		return true;
	}
	T delta() const { return max - min; }
	void reset() {
		if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
			this->min = -std::numeric_limits<T>::max();
			this->max = std::numeric_limits<T>::max();
		}
		else {
			this->min = T::min();
			this->max = T::max();
			//throw std::exception("reset No Implement");
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
	std::string to_string(std::string_view f = "[{0},{1}]") const { return fmt::format(f, min, max); }
	friend bool operator==(const interval<T>& lhs, const interval<T>& rhs) {
		return equal(lhs.min, rhs.min) && equal(lhs.max, rhs.max);
	}
};

}