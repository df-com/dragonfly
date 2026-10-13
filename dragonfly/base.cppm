// License: MIT License   See LICENSE.txt for the full license.
module;
#include <iostream>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <numeric>
#include <limits>
#include <span>
#include <cmath>
#include <cstdint>
#include <string>
#include <sstream>
#include <functional>

export module dragonfly:base;

export namespace dragonfly {

inline bool AreNear(float a, float b, float abs_error) { return fabs(a - b) < abs_error; }
inline bool AreNear(double a, double b, double abs_error) { return fabs(a - b) < abs_error; }
inline bool nearly_equal(float a, float b, float abs_error) { return fabs(a - b) < abs_error; }
inline bool nearly_equal(double a, double b, double abs_error) { return fabs(a - b) < abs_error; }

template<typename T>
T accumulate(const std::vector<T>& values) {
	T sum = T{};
	for (const auto& value : values) { sum += value; }
	return sum;
}

template<typename T>
bool equal(T a, T b) {
	if constexpr (std::is_floating_point<T>::value) { return std::fabs(a - b) <= std::numeric_limits<T>::epsilon(); }
	else return a == b;
}

template<typename T> bool equal(T a, T b, T c) { return equal(a, b) && equal(b, c); }

template<size_t N>
bool equal(const std::array<float, N>& a, const std::array<float, N>& b) {
	for (int i = 0; i < a.size(); i++) {
		if (!equal(a[i], b[i]))
			return false;
	}
	return true;
}

template<size_t N>
bool equal(const std::vector<std::array<float, N>>& a, const std::vector<std::array<float, N>>& b) {
	for (int i = 0; i < a.size(); i++) {
		if (!equal(a[i], b[i]))
			return false;
	}
	return true;
}

bool equal(const std::vector<int>& v1, const std::vector<int>& v2) { return (v1.size() == v2.size() && std::equal(v1.begin(), v1.end(), v2.begin())); }

int round_to_nearest(int number, int multiple) {
	int remainder = number % multiple;
	if (remainder >= multiple / 2) {
		return number + multiple - remainder;
	}
	else {
		return number - remainder;
	}
}

template<typename T, std::size_t... Ns>
auto concatenate(const std::array<T, Ns>&... arrays) {
	std::array<T, (Ns + ...)> result;
	std::size_t index = 0;
	((std::copy_n(arrays.begin(), Ns, result.begin() + index), index += Ns), ...);
	return result;
}

template<int PRECISION>
std::string round_to_string(float value) {
	std::stringstream ss;
	ss << std::fixed << std::setprecision(PRECISION) << value;
	return ss.str();
}

template<int PRECISION, int N1>
std::string round_to_string(const std::array<float, N1>& values, const std::string& sep) {
	std::string ret;
	for (int i = 0; i < values.size() - 1; i++) {
		ret += round_to_string<PRECISION>(values[i]);
		ret += sep;
	}
	ret += round_to_string<PRECISION>(values[values.size() - 1]);
	return ret;
}

std::vector<std::vector<std::string>> get_cvs_rows(std::istream& ss, char delim) {
	std::vector<std::vector<std::string>>   rows;
	std::string                line;
	while (std::getline(ss, line)) {
		std::vector<std::string> row;
		std::stringstream          lineStream(line);
		std::string                cell;

		while (std::getline(lineStream, cell, delim)) {
			row.push_back(cell);
		}
		rows.push_back(row);
	}
	return rows;
}

inline int get_index(const std::vector<std::string>& names, const std::string& name) {
	auto it = std::find(names.begin(), names.end(), name);
	auto ret = std::distance(names.begin(), it);
	return (int)ret;
}

inline int get_index(const std::vector<std::string>& names, const std::vector<std::string>& ns) {
	for (auto& n : ns) {
		auto it = std::find(names.begin(), names.end(), n);
		if (it != names.end()) {
			auto ret = std::distance(names.begin(), it);
			return (int)ret;
		}
	}
	return -1;
}

inline std::string id_without_slash(const std::string& id) {
	std::string ret = id;
	ret.erase(std::remove(ret.begin(), ret.end(), '/'), ret.end());
	return ret;
}

template<typename T>
void shuffle(std::vector<T>& arr) {
	std::default_random_engine rng(time(0));
	std::shuffle(std::begin(arr), std::end(arr), rng);
}

template<typename T, typename T2>
void shuffle(std::vector<T>& arr1, std::vector<T2>& arr2) {
	std::srand(std::time(nullptr));
	for (int i = 0; i < arr1.size(); i++) {
		int rd = std::rand() % arr1.size();
		std::swap(arr1[i], arr1[rd]);
		std::swap(arr2[i], arr2[rd]);
	}
}

std::string to_upper_copy(const std::string& str) {
	std::string upper_str;
	for (char c : str) {
		upper_str += toupper(c);
	}
	return upper_str;
}

bool icontains(const std::vector<std::string>& vec, const std::string& value) {
	std::vector<std::string> upvec;
	for (auto& v : vec) {
		upvec.push_back(to_upper_copy(v));
	}
	std::string upvalue = to_upper_copy(value);
	if (std::find(upvec.begin(), upvec.end(), upvalue) != upvec.end()) {
		return true;
	}
	return false;
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline std::size_t fast_hash_value(float v) { return std::hash<int>{}(static_cast<int>(v * 100000.0f + 0.5f)); }
inline std::size_t fast_hash_value(double v) { return std::hash<int>{}(static_cast<int>(v * 100000000.0 + 0.5)); }
inline std::size_t fast_hash_value(int v) {return std::hash<int>{}(v);}
inline std::size_t fast_hash_value(int64_t v) {return std::hash<int64_t>{}(v);}

template<typename T, size_t N>
std::size_t fast_hash(const std::array<T, N>& data) {
	std::size_t ret = 0;
	for (const auto& v : data) {
		hash_combine(ret, fast_hash_value(v));
	}
	return ret;
}
template<typename T>
std::size_t fast_hash(const std::vector<T>& data) {
	std::size_t ret = 0;
	for (const auto& v : data) {
		hash_combine(ret, fast_hash_value(v));
	}
	return ret;
}
template<typename T, size_t N>
std::size_t fast_hash(const std::vector<std::array<T, N>>& data) {
	std::size_t ret = 0;
	for (const auto& v : data) {
		hash_combine(ret, fast_hash(v));
	}
	return ret;
}

}
