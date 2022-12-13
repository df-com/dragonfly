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
};

}
