// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>

export module dragonfly.ind:RSI;

import :tech;
import :WR;

export namespace dragonfly::ind {

struct RSI {
	int n;
	std::vector<float> rsi;
	RSI(const std::vector<float>& arr, int n) {
		this->n = n;
		std::vector<float> ups;
		std::vector<float> downs;
		ups.resize(arr.size());
		downs.resize(arr.size());
		rsi.resize(arr.size());
		ups[0] = 0.0;
		downs[0] = 0.0;
		rsi[0] = 50.0;
		for (int i = 1; i < arr.size(); i++) {
			float lc = arr[i - 1];
			ups[i] = std::max(arr[i] - lc, float(0.0));
			downs[i] = std::abs(arr[i] - lc);
		}
		auto upsma = SMA(ups, n, 1);
		auto downsma = SMA(downs, n, 1);
		for (int i = 1; i < arr.size(); i++) {
			if (equal(downsma[i], float(0.0))) {
				rsi[i] = 50.0;
			}
			else {
				rsi[i] = upsma[i] / downsma[i] * 100;
			}
		}
	}
	const float at(int i) const { return rsi[i]; }
	const float operator[](int i) const { return at(i); }
};

}
