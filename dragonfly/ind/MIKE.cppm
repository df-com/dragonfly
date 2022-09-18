// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:MIKE;

export import :tech;

export namespace dragonfly::ind {

struct MIKE {
	int n;
	std::vector<double> anchor;
	std::vector<double> resist1;
	std::vector<double> resist2;
	std::vector<double> resist3;
	std::vector<double> support1;
	std::vector<double> support2;
	std::vector<double> support3;
	std::vector<double> pos;
public:
	MIKE(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, int n) {
		init(cs, hs, ls, n);
	}
	MIKE(const std::vector<double>& cs, const std::vector<double>& hs, const std::vector<double>& ls, int n) {
		init(cs, hs, ls, n);
	}
private:
	template<typename T>
	void init(const std::vector<T>& cs, const std::vector<T>& hs, const std::vector<T>& ls, int n) {
		this->n = n;
		this->anchor.resize(cs.size());
		this->resist1.resize(cs.size());
		this->resist2.resize(cs.size());
		this->resist3.resize(cs.size());
		this->support1.resize(cs.size());
		this->support2.resize(cs.size());
		this->support3.resize(cs.size());
		this->pos.resize(cs.size());

		std::vector<double> HHs;
		std::vector<double> LLs;
		std::vector<double> HLCs;
		HHs.resize(cs.size());
		LLs.resize(cs.size());
		HLCs.resize(cs.size());
		for (size_t i = 0; i < cs.size(); i++) {
			HLCs[i] = (hs[i] + ls[i] + cs[i]) / 3.0;
			int beginI = i - n + 1;
			beginI = std::max(0, beginI);
			LLs[i] = *std::min_element(ls.begin() + beginI, std::min(ls.begin() + i + 1, ls.end()));
			HHs[i] = *std::max_element(hs.begin() + beginI, std::min(hs.begin() + i + 1, hs.end()));
		}
		auto maHLCs = MA(HLCs, n);
		this->anchor = REF(maHLCs, 1);
		auto hv = EMA(HHs, 3);
		auto lv = EMA(LLs, 3);

		for (size_t i = n - 1; i < cs.size(); i++) {
			this->resist1[i] = this->anchor[i] + this->anchor[i] - lv[i];
			this->resist2[i] = this->anchor[i] + hv[i] - lv[i];
			this->resist3[i] = 2.0 * hv[i] - lv[i];
			this->support1[i] = this->anchor[i] - (hv[i] - this->anchor[i]);
			this->support2[i] = this->anchor[i] - (hv[i] - lv[i]);
			this->support3[i] = 2.0 * lv[i] - hv[i];
		}
		this->resist3 = EMA(this->resist3, 3);
		this->resist2 = EMA(this->resist2, 3);
		this->resist1 = EMA(this->resist1, 3);
		this->support1 = EMA(this->support1, 3);
		this->support2 = EMA(this->support2, 3);
		this->support3 = EMA(this->support3, 3);

		this->pos = getMikePosList(this->anchor,
			this->resist1, this->resist2, this->resist3,
			this->support1, this->support2, this->support3, cs, n);
	}
	double getPos(double a, double lower, double upper) {
		if (lower >= upper) {
			assert(false);
		}
		double ret = (a - lower) / (upper - lower);
		return ret;
	}
	double getMikePos(double anchor, double resist1, double resist2, double resist3,
		double support1, double support2, double support3, double values) {
		if (values > resist3) {
			if (equal(resist3, resist2)) {
				return 4.0;
			}
			double ret = 3.0 + (values - resist3) / (resist3 - resist2);
			return ret;
		}
		else if (values > resist2) {
			double ret = 2.0 + getPos(values, resist2, resist3);
			return ret;
		}
		else if (values > resist1) {
			double ret = 1.0 + getPos(values, resist1, resist2);
			return ret;
		}
		else if (values > anchor) {
			double ret = getPos(values, anchor, resist1);
			return ret;
		}
		else if (values > support1) {
			double ret = (getPos(values, support1, anchor) - 1.0);
			return ret;
		}
		else if (values > support2) {
			double ret = -1.0 + (getPos(values, support2, support1) - 1.0);
			return ret;
		}
		else if (values > support3) {
			double ret = -2.0 - (getPos(values, support3, support2) - 1.0);
			return ret;
		}
		else {
			if (equal(support2, support3)) {
				return -4.0;
			}
			double ret = -3.0 + (values - support3) / (support2 - support3);
			return ret;
		}
		assert(false);
		return 0.0;
	}
	template<typename T, typename U>
	std::vector<double> getMikePosList(const std::vector<T>& anchor,
		const std::vector<T>& resist1, const std::vector<T>& resist2, const std::vector<T>& resist3,
		const std::vector<T>& support1, const std::vector<T>& support2, const std::vector<T>& support3,
		const std::vector<U>& values, int n) {
		std::vector<double> ret;
		ret.resize(anchor.size());
		for (size_t i = n - 1; i < anchor.size(); i++) {
			ret[i] = getMikePos(anchor[i], resist1[i], resist2[i], resist3[i], support1[i], support2[i], support3[i], values[i]);
		}
		return std::move(ret);
	}
};

}
