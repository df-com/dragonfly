// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:ATR;

export import :tech;

export namespace dragonfly::ind {

struct ATR {
	int n;
	std::vector<float> atr;
	ATR(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs, int n) {
		this->n = n;
		atr.resize(cs.size());
		atr[0] = std::abs(hs[0] - ls[0]);
		for (int i = 1; i < cs.size(); i++) {
			atr[i] = std::max({ std::abs(hs[i] - ls[i]),std::abs(hs[i] - cs[i - 1]),std::abs(ls[i] - cs[i - 1]) });
		}
	}
	const float at(int i) const { return atr[i]; }
	const float operator[](int i) const { return at(i); }
};

}
