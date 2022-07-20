// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>

export module dragonfly.ind;

export import :tech;
export import :KDJ;
export import :MACD;
export import :RUMI;
export import :WR;

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
};

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

struct RSI {
	int n;
	std::vector<float> rsi;
	RSI(const std::vector<float>& arr, int n) {
		this->n = n;
		std::vector<float> ups;
		std::vector<float> downs;
		ups.resize(arr.size());
		downs.resize(arr.size());
		rsi.resize(arr.size());
		ups[0] = 0.0;
		downs[0] = 0.0;
		rsi[0] = 50.0;
		for (int i = 1; i < arr.size(); i++) {
			float lc = arr[i - 1];
			ups[i] = std::max(arr[i] - lc, float(0.0));
			downs[i] = std::abs(arr[i] - lc);
		}
		auto upsma = SMA(ups, n, 1);
		auto downsma = SMA(downs, n, 1);
		for (int i = 1; i < arr.size(); i++) {
			if (equal(downsma[i], float(0.0))) {
				rsi[i] = 50.0;
			}
			else {
				rsi[i] = upsma[i] / downsma[i] * 100;
			}
		}
	}
	const float at(int i) const { return rsi[i]; }
	const float operator[](int i) const { return at(i); }
};

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
