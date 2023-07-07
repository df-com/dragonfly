// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fmt/format.h>

export module dragonfly:IntradayTimes;

import :Time;
import :interval;

export namespace dragonfly {

struct IntradayTimes {
	std::vector<interval<TimeOnly>> times;
	IntradayTimes() {}
	IntradayTimes(const interval<TimeOnly>& t) {
		times.push_back(t);
	}
	IntradayTimes(const interval<TimeOnly>& t1, const interval<TimeOnly>& t2) {
		times.push_back(t1);
		times.push_back(t2);
	}
	IntradayTimes(const interval<TimeOnly>& t1, const interval<TimeOnly>& t2, const interval<TimeOnly>& t3) {
		times.push_back(t1);
		times.push_back(t2);
		times.push_back(t3);
	}
	bool empty() const { return times.empty(); }
	bool contains(const TimeOnly& t) const {
		for (const auto& time : times) {
			if (time.contains(t))
				return true;
		}
		return false;
	}
	IntradayTimes& operator=(const IntradayTimes& rhs) {
		this->times = rhs.times;
		return *this;
	}
	void Merge() {
		std::vector<interval<TimeOnly>> newtimes = times;
		times.clear();
		for (auto& it : newtimes) {
			Add(it);
		}
	}
	void Add(const interval<TimeOnly>& t) {
		for (auto& it : times) {
			if (it.contains(t.min) && it.contains(t.max)) {
				return;
			}
		}
		for (auto& it : times) {
			if (t.contains(it.min) && t.contains(it.max)) {
				it.min = t.min;
				it.max = t.max;
				Merge();
				return;
			}
		}
		for (auto& it : times) {
			if (it.contains(t.min) && t.contains(it.max)) {
				it.max = t.max;
				Merge();
				return;
			}
			if (it.contains(t.max) && t.contains(it.min)) {
				it.min = t.min;
				Merge();
				return;
			}
		}
		times.push_back(t);
	}
	friend std::ostream& operator<<(std::ostream& os, const IntradayTimes& myclass) {
		for (auto& it : myclass.times) {
			os << "[" << it.min.hour() << ":" << it.min.minute() << "," << it.max.hour() << ":" << it.max.minute() << "]";
		}
		return os;
	}
};

}
