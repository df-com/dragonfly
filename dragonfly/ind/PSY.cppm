// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:PSY;

export import :tech;

export namespace dragonfly::ind {

struct PSY {
	int n;
	std::vector<float> psy;
	PSY(const std::vector<float>& a, const std::vector<float>& b, int n) {
		assert(a.size() == b.size());
		psy.resize(a.size());
		this->n = n;
		for (int i = n + 1; i < a.size(); i++) {
			psy[i] = (float)(static_cast<double>(countUp(a, b, i - n + 1, i)) / n * 100);
		}
	}
	int countUp(const std::vector<float>& a, const std::vector<float>& b, int beginI, int endI) {
		int ret = 0;
		for (int i = beginI; i <= endI; i++) {
			if (a[i] < b[i]) {
			}
			else {
				ret++;
			}
		}
		return ret;
	}
};

}
