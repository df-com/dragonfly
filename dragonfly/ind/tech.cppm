// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:tech;

export namespace dragonfly::ind {

template<typename T>
bool equal(T a, T b) {
	if constexpr (std::is_floating_point<T>::value) { return std::fabs(a - b) <= std::numeric_limits<T>::epsilon(); }
	else return a == b;
}

template<typename T>
inline T Accumulate(const std::vector<T>& data, int begin, int end) {
	T ret = 0.0;
	for (int i = begin; i < end; i++) {
		ret += data[i];
	}
	return ret;
}

template<typename T>
inline T trimmed_mean(const std::vector<T>& data, int trim) {
	std::vector<T> d = data;
	std::sort(d.begin(), d.end());
	return std::accumulate(d.begin() + trim, d.end() - trim, 0.0) / (data.size() - trim * 2);
}

template<typename T>
inline std::vector<T> TrimmedMA(const std::vector<T>& data, int n, int trim) {
	std::vector<T> ret;
	if (data.empty()) { return ret; }
	ret.resize(data.size());
	ret[0] = data[0];
	for (int i = 1; i < n - 1 && i < data.size(); i++) {
		ret[i] = 0.0;
		for (int d = std::max(0, static_cast<int>(i) - static_cast<int>(n) + 1); d < i + 1; d++) {
			ret[i] += data[d];
		}
		ret[i] /= (i + 1);
	}
	for (int i = n - 1; i < data.size(); i++) {
		std::vector<T> sub = { data.begin() + i - n + 1, data.begin() + i + 1 };
		ret[i] = trimmed_mean(sub, trim);
	}
	return std::move(ret);
}

template<typename T, typename Iter = typename std::vector<T>::const_iterator>
T MA(const std::vector<T>& data, Iter pos, unsigned int n) {
	return std::accumulate(pos - n, pos + 1, 0.0) / n;
}

template<typename T>
inline std::vector<T> MA(const std::vector<T>& data, unsigned int n) {
	std::vector<T> ret;
	if (data.empty())
		return ret;
	ret.resize(data.size());
	ret[0] = data[0];
	for (int i = 1; i < (int)n - 1 && i < data.size(); i++) {
		ret[i] = 0.0;
		for (int d = std::max(0, static_cast<int>(i) - static_cast<int>(n) + 1); d < i + 1; d++) {
			ret[i] += data[d];
		}
		ret[i] /= (i + 1);
	}
	for (int i = n - 1; i < data.size(); i++) {
		ret[i] = 0.0;
		for (int d = std::max((int)0, i - (int)n + 1); d < i + 1; d++) {
			ret[i] += data[d];
		}
		ret[i] /= n;
	}
	return std::move(ret);
}

template<typename T>
std::vector<T> SMA(const std::vector<T>& data, int N, double M) {
	assert(N > 0);
	std::vector<T> ret;
	if (data.empty()) { return ret; }
	ret.resize(data.size());
	ret[0] = data[0];
	double k = M / double(N);
	for (size_t i = 1; i < data.size(); i++) {
		ret[i] = (T)(data[i] * k + ret[i - 1] * (1 - k));
	}
	return std::move(ret);
}

template<typename T>
std::vector<T> EMA(const std::vector<T>& data, int N) {
	return std::move(SMA(data, N + 1, 2.0));
}

template<typename T>
inline std::vector<T> REF(const std::vector<T>& data, int N) {
	if (N <= 0) {
		assert(false);
	}
	std::vector<T> ret;
	if (data.empty())
		return ret;
	ret.resize(data.size());
	for (int i = 0; i < N; i++) {
		ret[i] = data[i];
	}
	for (size_t i = N; i < data.size(); i++) {
		ret[i] = data[i - N];
	}
	return std::move(ret);
}

template<typename T>
inline std::vector<T> SUM(const std::vector<T>& src, size_t n) {
	std::vector<T> des;
	if (src.empty())
		return des;
	des.resize(src.size());
	auto len = src.size();
	for (int i = (int)n - 1; i < len; i++) {
		des[i] = 0.0;
		for (auto j = i; j > i - n; j--) {
			des[i] += src[j];
		}
	}
	for (int i = 0; i < (int)n - 1 && i < len; i++) {
		des[i] = 0.0;
		for (auto j = i; j > 0; j--) {
			des[i] += src[j];
		}
	}
	return std::move(des);
}

inline void SUM(double* des, const double* src, size_t n, size_t len) {
	for (size_t i = n - 1; i < len; i++) {
		des[i] = 0.0;
		for (auto j = i; j > i - n; j--) {
			des[i] += src[j];
		}
	}
	for (size_t i = 0; i < n - 1 && i < len; i++) {
		des[i] = 0.0;
		for (auto j = i; j > 0; j--) {
			des[i] += src[j];
		}
	}
}
inline void SUM(float* des, const float* src, size_t n, size_t len) {
	for (size_t i = n - 1; i < len; i++) {
		des[i] = 0.0;
		for (auto j = i; j > i - n; j--) {
			des[i] += src[j];
		}
	}
	for (size_t i = 0; i < n - 1 && i < len; i++) {
		des[i] = 0.0;
		for (auto j = i; j > 0; j--) {
			des[i] += src[j];
		}
	}
}

template<typename T>
inline double StandardDeviation(const std::vector<T>& data, int dataI, int n, double u) {
	auto power_sum = 0.0;
	auto i = 1;
	for (; i <= n; i++) {
		if (dataI - (i - 1) <= 0) {
			break;
		}
		auto value = data[dataI - (i - 1)];
		power_sum += (value - u) * (value - u);
	}
	return sqrt(power_sum / double(i));
}

template<typename T>
inline double MeanAbsoluteDeviation(const std::vector<T>& data, double u) {
	double sum = 0.0;
	for (auto& v : data) {
		sum += abs(v - u);
	}
	return sum / data.size();
}

template<typename T>
inline T average(const std::vector<T>& data) {
	return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

template<typename T>
inline T MeanAbsoluteDeviation(const std::vector<T>& data) {
	T sum = std::accumulate(data.begin(), data.end(), 0.0);
	T avg = sum / data.size();
	return MeanAbsoluteDeviation(data, avg);
}

// [beginI,endI]
template<typename T>
inline T MinBetween(const std::vector<T>& data, int beginI, int endI) {
	T ret = data[beginI];
	for (int i = beginI + 1; i <= endI; i++) {
		ret = std::min(ret, data[i]);
	}
	return ret;
}

// [beginI,endI]
template<typename T>
inline T MaxBetween(const std::vector<T>& data, int beginI, int endI) {
	T ret = data[beginI];
	for (int i = beginI + 1; i <= endI; i++) {
		ret = std::max(ret, data[i]);
	}
	return ret;
}

// [beginI,endI]
template<typename T>
T MaxAndIndex(const std::vector<T>& data, int beginI, int endI, int& index) {
	T ret = data[beginI];
	index = beginI;
	for (int i = beginI + 1; i <= endI; i++) {
		if (data[i] > ret) {
			ret = data[i];
			index = i;
		}
	}
	return ret;
}

// [beginI,endI]
template<typename T>
T MinAndIndex(const std::vector<T>& data, int beginI, int endI, int& index) {
	T ret = data[beginI];
	index = beginI;
	for (int i = beginI + 1; i <= endI; i++) {
		if (data[i] < ret) {
			ret = data[i];
			index = i;
		}
	}
	return ret;
}

template<typename T>
std::vector<T> HighValues_Fast(const std::vector<T>& data, int n) {
	std::vector<T> ret;
	ret.resize(data.size());
	assert(data.size() > 0);
	T high = data[0];
	int index = 0;
	ret[0] = high;
	for (int i = 1; i < data.size(); i++) {
		int beginI = i - n + 1;
		beginI = std::max(0, beginI);
		if (beginI > index) {
			high = MaxAndIndex(data, beginI, i, index);
			ret[i] = high;
			continue;
		}
		if (data[i] > high) {
			high = data[i];
			index = i;
		}
		ret[i] = high;
	}
	return std::move(ret);
}

template<typename T>
std::vector<T> HighValues_Classical(const std::vector<T>& data, int n) {
	std::vector<T> ret;
	ret.resize(data.size());
	for (int i = 0; i < data.size(); i++) {
		int beginI = i - n + 1;
		beginI = std::max(0, beginI);
		ret[i] = MaxBetween(data, beginI, i);
	}
	return std::move(ret);
}

template<typename T>
std::vector<T> HighValues(const std::vector<T>& data, int n) {
	return HighValues_Fast(data, n);
}

template<typename T>
std::vector<T> LowValues_Fast(const std::vector<T>& data, int n) {
	std::vector<T> ret;
	ret.resize(data.size());
	assert(data.size() > 0);
	T low = data[0];
	int index = 0;
	ret[0] = low;
	for (int i = 1; i < data.size(); i++) {
		int beginI = i - n + 1;
		beginI = std::max(0, beginI);
		if (beginI > index) {
			low = MinAndIndex(data, beginI, i, index);
			ret[i] = low;
			continue;
		}
		if (data[i] < low) {
			low = data[i];
			index = i;
		}
		ret[i] = low;
	}
	return std::move(ret);
}

template<typename T>
std::vector<T> LowValues_Classical(const std::vector<T>& data, int n) {
	std::vector<T> ret;
	ret.resize(data.size());
	for (int i = 0; i < data.size(); i++) {
		int beginI = i - n + 1;
		beginI = std::max(0, beginI);
		ret[i] = MinBetween(data, beginI, i);
	}
	return std::move(ret);
}

template<typename T>
std::vector<T> LowValues(const std::vector<T>& data, int n) {
	return LowValues_Fast(data, n);
}

inline bool lessfunction(double i, double j) {
	return (i < j);
}
template<typename T>
inline std::vector<double> Ranking(const std::vector<T>& src) {
	std::vector<double> positive;
	std::vector<double> negative;
	std::vector<double> ret;
	ret.resize(src.size());
	for (auto& n : src) {
		if (n > 0) {
			positive.push_back(n);
		}
		else {
			negative.push_back(n);
		}
	}
	std::sort(positive.begin(), positive.end(), lessfunction);
	std::sort(negative.begin(), negative.end(), lessfunction);
	for (int i = 0; i < src.size(); i++) {
		if (src[i] > 0) {
			std::vector<double>::iterator it = std::lower_bound(positive.begin(), positive.end(), src[i]);
			auto d = std::distance(positive.begin(), it);
			ret[i] = static_cast<double>(d) / positive.size();
		}
		else {
			std::vector<double>::iterator it = std::lower_bound(negative.begin(), negative.end(), src[i]);
			auto d = std::distance(negative.begin(), it);
			ret[i] = static_cast<double>(d) / negative.size() - 1.0;
		}
	}
	return std::move(ret);
}

inline float Hn(const std::vector<float>& arr, int data_i, int n) {
	auto beginI = data_i - n + 1;
	beginI = std::max(0, beginI);
	return *std::max_element(arr.begin() + beginI, std::min(arr.begin() + data_i + 1, arr.end()));
}

inline float Ln(const std::vector<float>& arr, int data_i, int n) {
	auto beginI = data_i - n + 1;
	beginI = std::max(0, beginI);
	return *std::min_element(arr.begin() + beginI, std::min(arr.begin() + data_i + 1, arr.end()));
}

template<typename TKey, typename TValue>
std::pair<TKey, TValue> max(const std::map<TKey, TValue>& m) {
	return *std::max_element(m.begin(), m.end(), [](const std::pair<TKey, TValue>& p1, const std::pair<TKey, TValue>& p2) {
		return p1.second < p2.second;
		});
}

}
