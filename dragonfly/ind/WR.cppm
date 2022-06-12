// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:WR;

export import :tech;

export namespace dragonfly::ind {

struct WR {
	int n;
	std::vector<float> pos; // 0.0-1.0
	std::vector<float> ll;
	std::vector<float> hh;
	WR(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs, int n) {
		this->n = n;
		init(hs, ls, cs);
	}
	double GetPosByPrice(int dataI, double price) const {
		return (price - ll[dataI]) / (hh[dataI] - ll[dataI]);
	}
	float GetPriceByPos(int dataI, double pos) const {
		return pos * (hh[dataI] - ll[dataI]) + ll[dataI];
	}
	void init(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs) {
		if (cs.empty())
			return;
		pos.resize(cs.size());
		ll = LowValues(ls, n);
		hh = HighValues(hs, n);
		for (size_t i = 1; i < cs.size(); i++) {
			int beginI = i - n + 1;
			beginI = std::max(0, beginI);
			if (equal(ll[i], hh[i]) || equal(hh[i] - ll[i], float(0.0))) {
				continue;
			}
			pos[i] = (cs[i] - ll[i]) / (hh[i] - ll[i]);
		}
	}
};

}
