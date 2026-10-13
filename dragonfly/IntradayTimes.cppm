// License: MIT License   See LICENSE.txt for the full license.
module;
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
#include <cmath>

export module dragonfly:IntradayTimes;

import :Time;
import :types;

export namespace dragonfly {
//
//struct IntradayTimes {
//	std::vector<interval<TimeOnly>> times;
//	IntradayTimes() {}
//	IntradayTimes(const TimeOnly& tonly) {
//		*this = tonly;
//	}
//	IntradayTimes(const interval<TimeOnly>& t) {
//		times.push_back(t);
//	}
//	IntradayTimes(const interval<TimeOnly>& t1, const interval<TimeOnly>& t2) {
//		times.push_back(t1);
//		times.push_back(t2);
//	}
//	IntradayTimes(const interval<TimeOnly>& t1, const interval<TimeOnly>& t2, const interval<TimeOnly>& t3) {
//		times.push_back(t1);
//		times.push_back(t2);
//		times.push_back(t3);
//	}
//	bool empty() const { return times.empty(); }
//	bool contains(const TimeOnly& t) const noexcept {
//		for (const auto& time : times) {
//			if (time.contains(t))
//				return true;
//		}
//		return false;
//	}
//	IntradayTimes& operator=(const IntradayTimes& rhs) {
//		this->times = rhs.times;
//		return *this;
//	}
//	IntradayTimes& operator=(const TimeOnly& rhs) {
//		times.clear();
//		times.push_back(interval<TimeOnly>(rhs, rhs));
//		return *this;
//	}
//	void Merge() {
//		std::vector<interval<TimeOnly>> newtimes = times;
//		times.clear();
//		for (auto& it : newtimes) {
//			Add(it);
//		}
//	}
//	IntradayTimes& Add(const interval<TimeOnly>& t) {
//		for (auto& it : times) {
//			if (it.contains(t.min) && it.contains(t.max)) {
//				return *this;
//			}
//		}
//		for (auto& it : times) {
//			if (t.contains(it.min) && t.contains(it.max)) {
//				it.min = t.min;
//				it.max = t.max;
//				Merge();
//				return *this;
//			}
//		}
//		for (auto& it : times) {
//			if (it.contains(t.min) && t.contains(it.max)) {
//				it.max = t.max;
//				Merge();
//				return *this;
//			}
//			if (it.contains(t.max) && t.contains(it.min)) {
//				it.min = t.min;
//				Merge();
//				return *this;
//			}
//		}
//		times.push_back(t);
//		return *this;
//	}
//	IntradayTimes& Add(const TimeOnly& t) { return Add(interval<TimeOnly>(t, t)); }
//	friend std::ostream& operator<<(std::ostream& os, const IntradayTimes& myclass) {
//		if (myclass.times.size() == 1)
//			if (myclass.times[0].min == myclass.times[0].max) {
//				os << "[" << myclass.times[0].min.hour() << ":" << myclass.times[0].min.minute() << "]";
//				return os;
//			}
//		for (auto& it : myclass.times) {
//			os << "[" << it.min.hour() << ":" << it.min.minute() << "," << it.max.hour() << ":" << it.max.minute() << "]";
//		}
//		return os;
//	}
//	std::string ToString() const {
//		std::ostringstream oss;
//		oss << *this;
//		return oss.str();
//	}
//};

template <typename T>
struct IntradayTimesT {
	std::vector<interval<T>> times;

	IntradayTimesT() {}

	IntradayTimesT(const T& tonly) {
		*this = tonly;
	}

	IntradayTimesT(const interval<T>& t) {
		times.push_back(t);
	}

	IntradayTimesT(const interval<T>& t1, const interval<T>& t2) {
		times.push_back(t1);
		times.push_back(t2);
	}

	IntradayTimesT(const interval<T>& t1, const interval<T>& t2, const interval<T>& t3) {
		times.push_back(t1);
		times.push_back(t2);
		times.push_back(t3);
	}

	bool empty() const { return times.empty(); }

	bool contains(const T& t) const noexcept {
		for (const auto& time : times) {
			if (time.contains(t))
				return true;
		}
		return false;
	}

	IntradayTimesT& operator=(const IntradayTimesT& rhs) {
		this->times = rhs.times;
		return *this;
	}

	IntradayTimesT& operator=(const T& rhs) {
		times.clear();
		times.push_back(interval<T>(rhs, rhs));
		return *this;
	}
	bool operator==(const IntradayTimesT&) const = default;
	void Merge() {
		std::vector<interval<T>> newtimes = times;
		times.clear();
		for (auto& it : newtimes) {
			Add(it);
		}
	}

	IntradayTimesT& Add(const interval<T>& t) {
		for (auto& it : times) {
			if (it.contains(t.min) && it.contains(t.max)) {
				return *this;
			}
		}
		for (auto& it : times) {
			if (t.contains(it.min) && t.contains(it.max)) {
				it.min = t.min;
				it.max = t.max;
				Merge();
				return *this;
			}
		}
		for (auto& it : times) {
			if (it.contains(t.min) && it.contains(t.max)) {
				it.max = t.max;
				Merge();
				return *this;
			}
			if (it.contains(t.max) && it.contains(t.min)) {
				it.min = t.min;
				Merge();
				return *this;
			}
		}
		times.push_back(t);
		return *this;
	}

	IntradayTimesT& Add(const T& t) { return Add(interval<T>(t, t)); }

	template <typename U>
	friend std::ostream& operator<<(std::ostream& os, const IntradayTimesT<U>& myclass) {
		if (myclass.times.size() == 1)
			if (myclass.times[0].min == myclass.times[0].max) {
				os << "[" << myclass.times[0].min.hour() << ":" << myclass.times[0].min.minute() << "]";
				return os;
			}
		for (auto& it : myclass.times) {
			os << "[" << it.min.hour() << ":" << it.min.minute() << "," << it.max.hour() << ":" << it.max.minute() << "]";
		}
		return os;
	}

	std::string ToString() const {
		std::ostringstream oss;
		oss << *this;
		return oss.str();
	}
};

typedef IntradayTimesT<TimeOnly> IntradayTimes;
typedef IntradayTimesT<TimeOnlyMin> IntradayMinTimes;

}
