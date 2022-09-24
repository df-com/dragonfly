// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:ASI;

export import :tech;

export namespace dragonfly::ind {

struct ASI {
	std::vector<float> asi;
	ASI(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& os) {
		if (cs.empty())
			return;
		std::vector<double> si;
		si.resize(cs.size());
		asi.resize(cs.size());
		si[0] = 0.0;
		asi[0] = 0.0;
		for (size_t i = 1; i < cs.size(); i++) {
			double A = abs(hs[i] - cs[i - 1]);
			double B = abs(ls[i] - cs[i - 1]);
			double C = abs(hs[i] - ls[i - 1]);
			double D = abs(cs[i - 1] - os[i - 1]);
			double E = cs[i] - cs[i - 1];
			double F = cs[i] - os[i];
			double G = cs[i - 1] - os[i - 1];
			double R;
			if (A > B && A > C) {
				R = A + B / 2 + D / 4;
			}
			else if (B > A && B > C) {
				R = B + A / 2 + D / 4;
			}
			else {
				R = C + D / 4;
			}
			double X = E + F / 2 + G;
			double K = A > B ? A : B;
			double SI = 16 * X / R * K;
			if (hs[i] == ls[i]) {
				SI = 0.0;
			}
			si[i] = SI / cs[i - 1];
		}
		for (size_t i = 1; i < cs.size(); i++) {
			asi[i] = (float)(asi[i - 1] + si[i]);
		}
	}
	const float at(int i) const { return asi[i]; }
	const float operator[](int i) const { return at(i); }
};

}
