// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:BOLL;

export import :tech;

export namespace dragonfly::ind {

struct BOLL {
	int n;
	int p;
	std::vector<float> mid;
	std::vector<float> upper;
	std::vector<float> lower;
	std::vector<float> width;
	std::vector<float> pos;
	BOLL(const std::vector<float>& cs, int n, int p) {
		this->n = n;
		this->p = p;

		std::vector<float> sd;
		sd.resize(cs.size());
		upper.resize(cs.size());
		lower.resize(cs.size());
		width.resize(cs.size());
		pos.resize(cs.size());

		mid = MA(cs, n);

		for (int i = 0; i < cs.size(); i++) {
			sd[i] = (float)StandardDeviation(cs, i, n, mid[i]);
		}

		for (int i = 0; i < cs.size(); i++) {
			upper[i] = mid[i] + sd[i] * p;
			lower[i] = mid[i] - sd[i] * p;
			width[i] = upper[i] - lower[i];
			pos[i] = (cs[i] - mid[i]) / (width[i] / 2);
		}
	}
	float width_in_percent(int dataI) const {
		return width[dataI] / mid[dataI];
	}
public:
	static double division(double a, double b, double ifBZeroValue) {
		if (equal(b, 0.0)) {
			return ifBZeroValue;
		}
		return a / b;
	}
};

}
