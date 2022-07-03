// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <numeric>
#include <limits>
#include <string>
#include <sstream>

export module dragonfly:base;

export namespace dragonfly {

inline bool AreNear(float a, float b, float abs_error) { return fabs(a - b) < abs_error; }
inline bool AreNear(double a, double b, double abs_error) { return fabs(a - b) < abs_error; }
inline bool nearly_equal(float a, float b, float abs_error) { return fabs(a - b) < abs_error; }
inline bool nearly_equal(double a, double b, double abs_error) { return fabs(a - b) < abs_error; }

template<typename T>
bool equal(T a, T b) {
	if constexpr (std::is_floating_point<T>::value) { return std::fabs(a - b) <= std::numeric_limits<T>::epsilon(); }
	else return a == b;
}

template<typename T> bool equal(T a, T b, T c) { return equal(a, b) && equal(b, c); }

template<int N>
inline bool equal(const std::array<float, N>& a, std::array<float, N>& b) {
	for (int i = 0; i < a.size(); i++) {
		if (!equal(a[i], b[i]))
			return false;
	}
	return true;
}

template<int N>
inline bool equal(const std::vector<std::array<float, N>>& a, const std::vector<std::array<float, N>>& b) {
	for (int i = 0; i < a.size(); i++) {
		if (!equal(a[i], b[i]))
			return false;
	}
	return true;
}

template<typename T, std::size_t... Ns>
auto concatenate(const std::array<T, Ns>&... arrays) {
	std::array<T, (Ns + ...)> result;
	std::size_t index = 0;
	((std::copy_n(arrays.begin(), Ns, result.begin() + index), index += Ns), ...);
	return result;
}

template<int PRECISION>
inline std::string round_to_string(float value) {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(PRECISION) << value;
	return ss.str();
}

template<int PRECISION, int N1>
inline std::string round_to_string(const std::array<float, N1>& values, const std::string& sep) {
	std::string ret;
	for (int i = 0; i < values.size() - 1; i++) {
		ret += round_to_string<PRECISION>(values[i]);
		ret += sep;
	}
	ret += round_to_string<PRECISION>(values[values.size() - 1]);
	return ret;
}

inline std::vector<std::vector<std::string>> get_cvs_rows(std::istream& ss, char delim) {
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

}
