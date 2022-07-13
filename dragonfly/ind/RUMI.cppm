// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>

export module dragonfly.ind:RUMI;

import :tech;

export namespace dragonfly::ind {

struct RUMI {
	int short_n;
	int long_n;
	std::vector<float> diff;
	RUMI(const std::vector<float>& cs, int short_n, int long_n) {
		this->short_n = short_n;
		this->long_n = long_n;
		auto shortMA = MA(cs, short_n);
		auto longMA = MA(cs, long_n);
		diff.resize(cs.size());
		for (int i = 0; i < cs.size(); i++) {
			diff[i] = (shortMA[i] - longMA[i]) / longMA[i];
		}
	}
};

}
