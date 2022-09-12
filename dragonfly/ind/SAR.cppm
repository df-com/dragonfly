// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:SAR;

export import :tech;

export namespace dragonfly::ind {

struct SAR {
	double af_init = 0.02;
	double af_delta = 0.02;
	double af_max = 0.2;
	int n;
	std::vector<float> sar;
	std::vector<int16_t> trend;
	std::vector<float> ep;
	std::vector<float> af;
	std::vector<float> delta;
	SAR(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, double af_init = 0.02, double af_delta = 0.02, double af_max = 0.2, int n = 10) {
		this->n = n;
		this->af_init = af_init;
		this->af_delta = af_delta;
		this->af_max = af_max;
		sar.resize(cs.size());
		trend.resize(cs.size());
		ep.resize(cs.size());
		af.resize(cs.size());
		delta.resize(cs.size());
		sar[3] = Ln(ls, 3, 3);
		trend[3] = 1;
		ep[3] = Hn(hs, 3, n);
		af[3] = (float)af_init;
		for (int i = 4; i < cs.size(); i++) {
			ep[i] = ep[i - 1];
			af[i] = af[i - 1];
			if (trend[i - 1] > 0) {
				fillRisingSAR(hs, ls, i);
				continue;
			}
			if (trend[i - 1] < 0) {
				fillFallingSAR(hs, ls, i);
				continue;
			}
		}
		initDelta(cs);
	}
	void fillRisingSAR(const std::vector<float>& hs, const std::vector<float>& ls, int i) {
		if (ls[i] > sar[i - 1]) {
			// keep trends
			trend[i] = trend[i - 1] + 1;
			if (hs[i] > ep[i - 1]) {
				ep[i] = hs[i];
				af[i] = (float)(af[i - 1] + af_delta);
			}
			sar[i] = sar[i - 1] + af[i] * (ep[i] - sar[i - 1]);
		}
		else {
			trend[i] = -1;
			sar[i] = Hn(hs, i - 1, n);
			af[i] = (float)af_init;
			ep[i] = ls[i];
		}
	}
	void fillFallingSAR(const std::vector<float>& hs, const std::vector<float>& ls, int i) {
		if (hs[i] < sar[i - 1]) {
			trend[i] = trend[i - 1] - 1;
			if (ls[i] < ep[i]) {
				ep[i] = ls[i];
				af[i] = (float)(af[i - 1] + af_delta);
			}
			sar[i] = sar[i - 1] + af[i] * (ep[i] - sar[i - 1]);
		}
		else {
			trend[i] = 1;
			sar[i] = Ln(ls, i - 1, n);
			af[i] = (float)af_init;
			ep[i] = hs[i];
		}
	}
	void initDelta(const std::vector<float>& cs) {
		for (int i = 4; i < cs.size(); i++) {
			delta[i] = abs(cs[i] - sar[i]) / sar[i];
		}
	}
	bool IsParametersEqual(double af_init, double af_delta, double af_max, int n) {
		if (abs(this->af_init - af_init) > std::numeric_limits<double>::epsilon())
			return false;
		if (abs(this->af_delta - af_delta) > std::numeric_limits<double>::epsilon())
			return false;
		if (abs(this->af_max - af_max) > std::numeric_limits<double>::epsilon())
			return false;
		if (this->n != n)
			return false;
		return true;
	}
};

}
