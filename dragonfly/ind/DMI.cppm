// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind:DMI;

export import :tech;

export namespace dragonfly::ind {

struct DMI_I {
	int n1, n2;
	std::vector<float> di1;
	std::vector<float> di2;
	std::vector<float> adx;
	std::vector<float> adxr;
	DMI_I(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, int n1, int n2) {
		this->n1 = n1;
		this->n2 = n2;
		std::vector<double> HD;
		std::vector<double> LD;
		std::vector<double> TR;
		std::vector<double> DMP;
		std::vector<double> DMM;
		std::vector<double> TRSUM;
		std::vector<double> DX;
		std::vector<double> ADX;
		std::vector<double> ADXR;
		HD.resize(cs.size());
		LD.resize(cs.size());
		TR.resize(cs.size());
		DX.resize(cs.size());
		ADXR.resize(cs.size());
		di1.resize(cs.size());
		di2.resize(cs.size());
		adx.resize(cs.size());
		adxr.resize(cs.size());

		HD[0] = 0.0;
		LD[0] = 0.0;
		TR[0] = 0.0;
		di1[0] = 0.0;
		di2[0] = 0.0;
		DX[0] = 0.0;
		for (size_t i = 1; i < cs.size(); i++) {
			HD[i] = hs[i] - hs[i - 1];
			LD[i] = ls[i - 1] - ls[i];
			HD[i] = std::max(0.0, HD[i]);
			LD[i] = std::max(0.0, LD[i]);
			if (equal(HD[i], LD[i])) {
				HD[i] = 0.0;
				LD[i] = 0.0;
			}
			else if (HD[i] > LD[i])
				LD[i] = 0.0;
			else
				HD[i] = 0.0;

			double A = hs[i] - ls[i];
			double B = std::abs(hs[i] - cs[i - 1]);
			double C = std::abs(ls[i] - cs[i - 1]);
			double max = std::max(A, std::max(B, C));
			TR[i] = max;
		}

		DMP = SUM(HD, n1);
		DMM = SUM(LD, n1);
		TRSUM = SUM(TR, n1);
		for (size_t i = 1; i < cs.size(); i++) {
			di1[i] = (float)(DMP[i] / TRSUM[i] * 100);
			di2[i] = (float)(DMM[i] / TRSUM[i] * 100);
			DX[i] = abs((di1[i] - di2[i]) / (di1[i] + di2[i])) * 100;
		}
		ADX = MA(DX, n2);
		for (int i = 0; i < n2 && i < cs.size(); i++) {
			ADXR[i] = ADX[i];
		}
		for (int i = n2; i < cs.size(); i++) {
			ADXR[i] = (ADX[i] + ADX[i - n2]) / 2;
		}

		for (int i = 0; i < cs.size(); i++) {
			adx[i] = (float)ADX[i];
			adxr[i] = (float)ADXR[i];
		}
	}
};

}
