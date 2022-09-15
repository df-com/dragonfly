// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:CBL;

export import :tech;

export namespace dragonfly::ind {

struct CBL {
	int CombineN;
	std::vector<int>	trend;
	std::vector<double>	pos;
	std::vector<double>	Bar1Max;
	std::vector<double>	Bar1;
	std::vector<double>	Bar2;
	std::vector<double>	Bar3;
	std::vector<int>	Bar3Index;

	CBL(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, int combineN) {
		this->CombineN = combineN;
		trend.resize(cs.size());
		pos.resize(cs.size());
		Bar1Max.resize(cs.size());
		Bar1.resize(cs.size());
		Bar2.resize(cs.size());
		Bar3.resize(cs.size());
		Bar3Index.resize(cs.size());
		if (hs[2] > hs[0]) {
			trend[2] = 1;
			Bar1Max[2] = hs[2];
			Bar1[2] = ls[2];
			Bar3[2] = ls[0];
		}
		else {
			trend[2] = -1;
			Bar1Max[2] = ls[2];
			Bar1[2] = hs[2];
			Bar3[2] = hs[0];
		}
		for (int i = 3; i < cs.size(); i++) {
			Bar1Max[i] = Bar1Max[i - 1];
			Bar1[i] = Bar1[i - 1];
			Bar2[i] = Bar2[i - 1];
			Bar3[i] = Bar3[i - 1];
			Bar3Index[i] = Bar3Index[i - 1];
			if (trend[i - 1] > 0) {
				trend[i] = trend[i - 1] + 1;
				if (cs[i] < Bar3[i - 1]) {
					trend[i] = -1;
					Bar1Max[i] = ls[i];
					Bar1[i] = hs[i];
					initBar3(hs, ls, i);
					continue;
				}
				if (hs[i] > Bar1Max[i]) {
					Bar1Max[i] = hs[i];
					Bar1[i] = ls[i];
					initBar3(hs, ls, i);
					continue;
				}
				continue;
			}
			if (trend[i - 1] < 0) {
				trend[i] = trend[i - 1] - 1;
				if (cs[i] > Bar3[i - 1]) {
					trend[i] = 1;
					Bar1Max[i] = hs[i];
					Bar1[i] = ls[i];
					initBar3(hs, ls, i);
					continue;
				}
				if (ls[i] < Bar1Max[i]) {
					Bar1Max[i] = ls[i];
					Bar1[i] = hs[i];
					initBar3(hs, ls, i);
					continue;
				}
				continue;
			}
		}
		initPos(cs);
	}

	double PriceDelta(int dataI) const {
		auto min = std::min(Bar3[dataI], Bar1Max[dataI]);
		auto max = std::max(Bar3[dataI], Bar1Max[dataI]);
		auto delta = max - min;
		return delta;
	}

	double GetPriceByPos(int dataI, double pos) const {
		auto min = std::min(Bar3[dataI], Bar1Max[dataI]);
		auto max = std::max(Bar3[dataI], Bar1Max[dataI]);
		auto delta = max - min;
		auto price = min + delta * pos;
		return price;
	}

	double GetPosByPrice(int dataI, double price) const {
		auto min = std::min(Bar3[dataI], Bar1Max[dataI]);
		auto max = std::max(Bar3[dataI], Bar1Max[dataI]);
		auto delta = max - min;
		double pos = -1.0;
		if (!equal(delta, 0.0))
			pos = (price - min) / delta;
		return pos;
	}
private:
	void initBar3(const std::vector<float>& hs, const std::vector<float>& ls, int tickI) {
		auto bar2BeginI = 0;
		auto bar3BeginI = 0;
		for (int i = 0; i < hs.size() - CombineN && i < CombineN * 16; i++) {
			bar2BeginI = tickI - 1 - CombineN + 1 - i;
			auto	endI = tickI - 1;
			bar2BeginI = std::max(0, bar2BeginI);
			endI = std::max(0, endI);
			auto price = getBarPrice(hs, ls, bar2BeginI, endI, trend[tickI]);
			Bar2[tickI] = price;
			if (isBarOK(Bar1[tickI], Bar2[tickI], trend[tickI])) {
				break;
			}
			if (bar2BeginI == 0) {
				break;
			}
		}
		for (int i = 0; i < hs.size() - CombineN && i < CombineN * 16; i++) {
			bar3BeginI = bar2BeginI - 1 - CombineN + 1 - i;
			auto endI = bar2BeginI - 1;
			bar3BeginI = std::max(0, bar3BeginI);
			endI = std::max(0, endI);
			auto price = getBarPrice(hs, ls, bar3BeginI, endI, trend[tickI]);
			Bar3[tickI] = price;
			Bar3Index[tickI] = bar3BeginI;
			if (isBarOK(Bar2[tickI], Bar3[tickI], trend[tickI])) {
				break;
			}
			if (bar3BeginI == 0) {
				break;
			}
		}
	}
	double getHigh(const std::vector<float>& hs, int beginI, int endI) {
		auto high = hs[beginI];
		for (size_t i = beginI; i <= endI; i++) {
			if (hs[i] > high) {
				high = hs[i];
			}
		}
		return high;
	}

	double getLow(const std::vector<float>& ls, int beginI, int endI) {
		auto low = ls[beginI];
		for (size_t i = beginI; i <= endI; i++) {
			if (ls[i] < low) {
				low = ls[i];
			}
		}
		return low;
	}

	double getBarPrice(const std::vector<float>& hs, const std::vector<float>& ls, int beginI, int endI, int trendNow) {
		if (trendNow > 0) {
			return getLow(ls, beginI, endI);
		}
		else {
			return getHigh(hs, beginI, endI);
		}
	}
	bool isBarOK(double thisBarPrice, double preBarPrice, int trend) {
		if (trend > 0) {
			return preBarPrice < thisBarPrice;
		}
		else {
			return preBarPrice > thisBarPrice;
		}
	}
	void initPos(const std::vector<float>& cs) {
		for (int i = 0; i <= 2 && i < cs.size(); i++) {
			pos[i] = 0.5;
		}
		for (int i = 3; i < cs.size(); i++) {
			pos[i] = GetPosByPrice(i, cs[i]);
		}
	}
};

}
