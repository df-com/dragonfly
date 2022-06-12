// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>

export module dragonfly.ind:KDJ;

import :tech;
import :WR;

export namespace dragonfly::ind {

struct KDJ {
private:
	int n_;
	int sn1_;
	int sn2_;
public:
	std::vector<float> K;
	std::vector<float> D;
	std::vector<float> J;
public:
	int n() const { return n_; }
	int sn1() const { return sn1_; }
	int sn2() const { return sn2_; }
	KDJ(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, int n, int sn1, int sn2) {
		this->n_ = n;
		this->sn1_ = sn1;
		this->sn2_ = sn2;
		WR wr(hs, ls, cs, n);
		std::vector<float> pos = wr.pos;
		for (auto& p : pos) {
			if (std::isnan(p) || std::isinf(p))
				p = 0.5;
			p *= 100;
		}
		K = SMA(pos, sn1, 1);
		D = SMA(K, sn2, 1);
		J.resize(cs.size());
		for (int i = 0; i < cs.size(); i++) {
			J[i] = K[i] * 3 - D[i] * 2;
		}
	}
};

}
