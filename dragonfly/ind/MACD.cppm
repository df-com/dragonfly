// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>

export module dragonfly.ind:MACD;

import :tech;
import :WR;

export namespace dragonfly::ind {

struct MACD {
	int short_n;
	int long_n;
	int dea_n;
	std::vector<float> diff;
	std::vector<float> dea;
	std::vector<float> bar;
	MACD(const std::vector<float>& cs, int short_n, int long_n, int dea_n) {
		this->short_n = short_n;
		this->long_n = long_n;
		this->dea_n = dea_n;
		auto shortEMA = EMA(cs, short_n);
		auto longEMA = EMA(cs, long_n);
		diff.resize(cs.size());
		bar.resize(cs.size());
		for (int i = 0; i < cs.size(); i++) { diff[i] = shortEMA[i] - longEMA[i]; }
		dea = EMA(diff, dea_n);
		for (int i = 0; i < cs.size(); i++) { bar[i] = (diff[i] - dea[i]) * 2; }
		for (int i = 0; i < cs.size(); i++) {
			diff[i] /= cs[i];
			bar[i] /= cs[i];
			dea[i] /= cs[i];
		}
	}
};

}
