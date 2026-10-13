// License: MIT License   See LICENSE.txt for the full license.
module;
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

export namespace dragonfly::ind {

struct ASI {
	std::vector<float> asi;

	ASI(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& os) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		asi.resize(sz, 0.0f);
		std::vector<float> si(sz, 0.0f);

		for (size_t i = 1; i < sz; ++i) {
			if (hs[i] == ls[i] || cs[i - 1] == 0.0f) {
				si[i] = 0.0f;
				continue;
			}

			const float A = std::abs(hs[i] - cs[i - 1]);
			const float B = std::abs(ls[i] - cs[i - 1]);
			const float C = std::abs(hs[i] - ls[i - 1]);
			const float D = std::abs(cs[i - 1] - os[i - 1]);

			float R;
			if (A > B && A > C)      R = A + B * 0.5f + D * 0.25f;
			else if (B > A && B > C) R = B + A * 0.5f + D * 0.25f;
			else                     R = C + D * 0.25f;

			if (R == 0.0f) continue;

			const float E = cs[i] - cs[i - 1];
			const float F = cs[i] - os[i];
			const float G = cs[i - 1] - os[i - 1];
			const float X = E + F * 0.5f + G;
			const float K = (A > B) ? A : B;

			const float SI = (16.0f * X / R) * K;
			si[i] = SI / cs[i - 1];
		}

		for (size_t i = 1; i < sz; ++i) {
			asi[i] = asi[i - 1] + si[i];
		}
	}

	[[nodiscard]] inline float at(int i) const { return asi[i]; }
	[[nodiscard]] inline float operator[](int i) const { return asi[i]; }
};

struct ATR {
	int n;
	std::vector<float> atr;

	ATR(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs, int n) : n(n) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		atr.resize(sz);
		atr[0] = std::abs(hs[0] - ls[0]);

		for (size_t i = 1; i < sz; ++i) {
			const float prev_c = cs[i - 1];
			if (prev_c == 0.0f) {
				atr[i] = 0.0f;
				continue;
			}
			const float inv_prev_c = 1.0f / prev_c;
			const float a1 = std::abs(hs[i] - ls[i]) * inv_prev_c;
			const float a2 = std::abs(hs[i] - prev_c) * inv_prev_c;
			const float a3 = std::abs(ls[i] - prev_c) * inv_prev_c;
			atr[i] = std::max({ a1, a2, a3 });
		}
		atr = MA(atr, n);
	}

	[[nodiscard]] inline float at(int i) const { return atr[i]; }
	[[nodiscard]] inline float operator[](int i) const { return atr[i]; }
};

struct BOLL {
	int n;
	int p;
	std::vector<float> mid, upper, lower, width, pos;

	BOLL(const std::vector<float>& cs, int n, int p) : n(n), p(p) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		mid = MA(cs, n);

		upper.resize(sz);
		lower.resize(sz);
		width.resize(sz);
		pos.resize(sz);

		for (size_t i = 0; i < sz; ++i) {
			const float sd = static_cast<float>(StandardDeviation(cs, i, n, mid[i]));
			const float offset = sd * static_cast<float>(p);

			upper[i] = mid[i] + offset;
			lower[i] = mid[i] - offset;
			const float w = upper[i] - lower[i];
			width[i] = w;

			pos[i] = (w != 0.0f) ? ((cs[i] - mid[i]) * 2.0f / w) : 0.0f;
		}
	}

	[[nodiscard]] inline float width_in_percent(int dataI) const {
		return (mid[dataI] != 0.0f) ? (width[dataI] / mid[dataI]) : 0.0f;
	}

	static inline double division(double a, double b, double ifBZeroValue) {
		return equal(b, 0.0) ? ifBZeroValue : (a / b);
	}
};

struct CBL {
	int CombineN;
	std::vector<int>	trend, Bar3Index;
	std::vector<double>	pos, Bar1Max, Bar1, Bar2, Bar3;

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
	std::vector<float> di1, di2, adx, adxr;
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

struct WR {
	int n;
	std::vector<float> pos/*0.0-1.0*/, ll, hh;

	WR(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs, int n) : n(n) {
		init(hs, ls, cs);
	}

	[[nodiscard]] inline double GetPosByPrice(int dataI, double price) const {
		const double range = static_cast<double>(hh[dataI] - ll[dataI]);
		return (range != 0.0) ? ((price - ll[dataI]) / range) : 0.0;
	}

	[[nodiscard]] inline float GetPriceByPos(int dataI, double pos_val) const {
		return static_cast<float>(pos_val * (hh[dataI] - ll[dataI]) + ll[dataI]);
	}

private:
	void init(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		pos.assign(sz, 0.0f);
		ll = LowValues(ls, n);
		hh = HighValues(hs, n);

		for (size_t i = 1; i < sz; ++i) {
			const float range = hh[i] - ll[i];
			if (range > 1e-6f) {
				pos[i] = (cs[i] - ll[i]) / range;
			}
		}
	}
};

struct CWR {
	int n;
	std::vector<float> pos, ll, hh, amps;

	CWR(const std::vector<float>& cs, int n) : n(n) {
		init(cs);
	}

	[[nodiscard]] inline double GetPosByPrice(int dataI, double price) const {
		const double range = static_cast<double>(hh[dataI] - ll[dataI]);
		return (range != 0.0) ? ((price - ll[dataI]) / range) : 0.0;
	}

	[[nodiscard]] inline float GetPriceByPos(int dataI, double pos_val) const {
		return static_cast<float>(pos_val * (hh[dataI] - ll[dataI]) + ll[dataI]);
	}

private:
	void init(const std::vector<float>& cs) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		pos.assign(sz, 0.0f);
		amps.assign(sz, 0.0f);
		ll = LowValues(cs, n);
		hh = HighValues(cs, n);

		for (size_t i = 1; i < sz; ++i) {
			const float range = hh[i] - ll[i];
			if (range > 1e-6f && ll[i] != 0.0f) {
				pos[i] = (cs[i] - ll[i]) / range;
				amps[i] = range / ll[i];
			}
		}
	}
};

struct KDJ {
private:
	int n_, sn1_, sn2_;
public:
	std::vector<float> K, D, J;

	[[nodiscard]] inline int n() const { return n_; }
	[[nodiscard]] inline int sn1() const { return sn1_; }
	[[nodiscard]] inline int sn2() const { return sn2_; }

	KDJ(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, int n, int sn1, int sn2)
		: n_(n), sn1_(sn1), sn2_(sn2) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		WR wr(hs, ls, cs, n);
		std::vector<float>& pos = wr.pos;

		for (size_t i = 0; i < sz; ++i) {
			if (std::isnan(pos[i]) || std::isinf(pos[i])) pos[i] = 0.5f;
			pos[i] *= 100.0f;
		}

		K = SMA(pos, sn1, 1);
		D = SMA(K, sn2, 1);

		J.resize(sz);
		for (size_t i = 0; i < sz; ++i) {
			J[i] = K[i] * 3.0f - D[i] * 2.0f;
		}
	}
};

struct MACD {
	int short_n, long_n, dea_n;
	std::vector<float> diff, dea, bar;

	MACD(const std::vector<float>& cs, int short_n, int long_n, int dea_n)
		: short_n(short_n), long_n(long_n), dea_n(dea_n) {
		const size_t sz = cs.size();
		if (sz == 0) return;

		const auto shortEMA = EMA(cs, short_n);
		const auto longEMA = EMA(cs, long_n);

		diff.resize(sz);
		bar.resize(sz);

		for (size_t i = 0; i < sz; ++i) {
			diff[i] = shortEMA[i] - longEMA[i];
		}

		dea = EMA(diff, dea_n);

		for (size_t i = 0; i < sz; ++i) {
			bar[i] = (diff[i] - dea[i]) * 2.0f;
			if (cs[i] != 0.0f) {
				const float inv_c = 1.0f / cs[i];
				diff[i] *= inv_c;
				bar[i] *= inv_c;
				dea[i] *= inv_c;
			}
		}
	}
};

struct MIKE {
	int n;
	std::vector<double> anchor, resist1, resist2, resist3, support1, support2, support3, pos;

	MIKE(const std::vector<float>& cs, const std::vector<float>& hs, const std::vector<float>& ls, int n) {
		init(cs, hs, ls, n);
	}
	MIKE(const std::vector<double>& cs, const std::vector<double>& hs, const std::vector<double>& ls, int n) {
		init(cs, hs, ls, n);
	}

private:
	template<typename T>
	void init(const std::vector<T>& cs, const std::vector<T>& hs, const std::vector<T>& ls, int n_val) {
		this->n = n_val;
		const size_t sz = cs.size();
		if (sz == 0) return;

		anchor.resize(sz);
		resist1.resize(sz); resist2.resize(sz); resist3.resize(sz);
		support1.resize(sz); support2.resize(sz); support3.resize(sz);

		std::vector<double> HHs(sz), LLs(sz), HLCs(sz);

		for (size_t i = 0; i < sz; ++i) {
			HLCs[i] = static_cast<double>(hs[i] + ls[i] + cs[i]) / 3.0;
		}

		for (size_t i = 0; i < sz; ++i) {
			const size_t beginI = (i >= static_cast<size_t>(n)) ? (i - n + 1) : 0;
			T max_h = hs[beginI], min_l = ls[beginI];
			for (size_t j = beginI + 1; j <= i; ++j) {
				if (hs[j] > max_h) max_h = hs[j];
				if (ls[j] < min_l) min_l = ls[j];
			}
			HHs[i] = static_cast<double>(max_h);
			LLs[i] = static_cast<double>(min_l);
		}

		auto maHLCs = MA(HLCs, n);
		this->anchor = REF(maHLCs, 1);
		auto hv = EMA(HHs, 3);
		auto lv = EMA(LLs, 3);

		const size_t start_idx = (n > 0) ? (n - 1) : 0;
		for (size_t i = start_idx; i < sz; ++i) {
			const double anc = this->anchor[i];
			const double h_val = hv[i];
			const double l_val = lv[i];

			this->resist1[i] = anc * 2.0 - l_val;
			this->resist2[i] = anc + h_val - l_val;
			this->resist3[i] = h_val * 2.0 - l_val;
			this->support1[i] = anc * 2.0 - h_val;
			this->support2[i] = anc - (h_val - l_val);
			this->support3[i] = l_val * 2.0 - h_val;
		}

		this->resist3 = EMA(this->resist3, 3);
		this->resist2 = EMA(this->resist2, 3);
		this->resist1 = EMA(this->resist1, 3);
		this->support1 = EMA(this->support1, 3);
		this->support2 = EMA(this->support2, 3);
		this->support3 = EMA(this->support3, 3);

		this->pos = getMikePosList(cs, start_idx);
	}

	[[nodiscard]] inline static double getPos(double a, double lower, double upper) {
		const double range = upper - lower;
		return (range != 0.0) ? ((a - lower) / range) : 0.0;
	}

	template<typename U>
	std::vector<double> getMikePosList(const std::vector<U>& values, size_t start_idx) {
		const size_t sz = anchor.size();
		std::vector<double> res(sz, 0.0);

		for (size_t i = start_idx; i < sz; ++i) {
			const double val = static_cast<double>(values[i]);
			const double anc = anchor[i], r1 = resist1[i], r2 = resist2[i], r3 = resist3[i];
			const double s1 = support1[i], s2 = support2[i], s3 = support3[i];

			if (val > r3) {
				res[i] = equal(r3, r2) ? 4.0 : (3.0 + (val - r3) / (r3 - r2));
			}
			else if (val > r2) {
				res[i] = 2.0 + getPos(val, r2, r3);
			}
			else if (val > r1) {
				res[i] = 1.0 + getPos(val, r1, r2);
			}
			else if (val > anc) {
				res[i] = getPos(val, anc, r1);
			}
			else if (val > s1) {
				res[i] = getPos(val, s1, anc) - 1.0;
			}
			else if (val > s2) {
				res[i] = -1.0 + (getPos(val, s2, s1) - 1.0);
			}
			else if (val > s3) {
				res[i] = -2.0 - (getPos(val, s3, s2) - 1.0);
			}
			else {
				res[i] = equal(s2, s3) ? -4.0 : (-3.0 + (val - s3) / (s2 - s3));
			}
		}
		return res;
	}
};

struct PSY {
	int n;
	std::vector<float> psy;

	PSY(const std::vector<float>& a, const std::vector<float>& b, int n) : n(n) {
		assert(a.size() == b.size());
		const size_t sz = a.size();
		psy.resize(sz, 0.0f);

		const float inv_n = 100.0f / static_cast<float>(n);
		for (size_t i = static_cast<size_t>(n) + 1; i < sz; ++i) {
			int up_count = 0;
			const size_t start = i - static_cast<size_t>(n) + 1;
			for (size_t j = start; j <= i; ++j) {
				if (a[j] >= b[j]) ++up_count;
			}
			psy[i] = static_cast<float>(up_count) * inv_n;
		}
	}
};

struct RSI {
	int n;
	std::vector<float> rsi;

	RSI(const std::vector<float>& arr, int n) : n(n) {
		const size_t sz = arr.size();
		if (sz == 0) return;

		std::vector<float> ups(sz, 0.0f), downs(sz, 0.0f);
		rsi.assign(sz, 50.0f);

		for (size_t i = 1; i < sz; ++i) {
			const float diff = arr[i] - arr[i - 1];
			ups[i] = std::max(diff, 0.0f);
			downs[i] = std::abs(diff);
		}

		const auto upsma = SMA(ups, n, 1);
		const auto downsma = SMA(downs, n, 1);

		for (size_t i = 1; i < sz; ++i) {
			if (!equal(downsma[i], 0.0f)) {
				rsi[i] = (upsma[i] / downsma[i]) * 100.0f;
			}
		}
	}

	[[nodiscard]] inline float at(int i) const { return rsi[i]; }
	[[nodiscard]] inline float operator[](int i) const { return rsi[i]; }
};

struct RUMI {
	int short_n;
	int long_n;
	std::vector<float> diff;
	RUMI(const std::vector<float>& cs, int short_n, int long_n) {
		this->short_n = short_n;
		this->long_n = long_n;
		auto shortMA = MA(cs, short_n);
		auto longMA = MA(cs, long_n);
		diff.resize(cs.size());
		for (int i = 0; i < cs.size(); i++) {
			diff[i] = (shortMA[i] - longMA[i]) / longMA[i];
		}
	}
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

struct MagicNine {
	int period_multiply_n;
	std::vector<int> values;
	struct Counter9 {
		int value;
		int max_count;
		int count;
		Counter9(int max_count) {
			this->max_count = max_count;
		}
		void Reset(int value) {
			this->value = value;
			this->count = 1;
		}
		void MoveOn() {
			if (count >= max_count) {
				if (value > 0)
					value++;
				if (value < 0)
					value--;
				count = 1;
			}
			else {
				count++;
			}
			if (value > 9)
				value = 1;
			if (value < -9)
				value = -1;
		}
	};
	MagicNine(const std::vector<float>& hs, const std::vector<float>& ls, const std::vector<float>& cs, int period_multiply_n) {
		this->period_multiply_n = period_multiply_n;
		Init(cs);
	}
	void Init(const std::vector<float>& cs) {
		values.resize(cs.size());
		Counter9 counter(period_multiply_n);
		if (cs[4 * period_multiply_n] > cs[3 * period_multiply_n])
			values[4 * period_multiply_n] = 1;
		else
			values[4 * period_multiply_n] = -1;
		for (int i = 4 * period_multiply_n + 1; i < cs.size(); i++) {
			if (equal(cs[i], cs[i - 4 * period_multiply_n])) {
				counter.MoveOn();
				values[i] = counter.value;
				continue;
			}
			if (cs[i] > cs[i - 4 * period_multiply_n]) {
				if (values[i - 1] < 0) {
					counter.Reset(1);
				}
				else {
					counter.MoveOn();
				}
				values[i] = counter.value;
				continue;
			}
			if (cs[i] < cs[i - 4 * period_multiply_n]) {
				if (values[i - 1] > 0) {
					counter.Reset(-1);
				}
				else {
					counter.MoveOn();
				}
				values[i] = counter.value;
				continue;
			}
		}
	}
	void set_value_using_prev_combine(int at, int mul, int value) {
		for (int i = at - mul + 1; i < values.size(); i++) {
			values[i] = value;
		}
	}
	void InitUsingJumpCombine(const std::vector<float>& cs) {
		values.resize(cs.size());
		values[4 * period_multiply_n] = 0;
		for (int i = 4 * period_multiply_n + 1; i < cs.size(); i++) {
			if (cs[i] > cs[i - 4 * period_multiply_n]) {
				if (values[i - 1] < 0) {
					set_value_using_prev_combine(i, period_multiply_n, 1);
				}
				else if (values[i - 1] >= 9) {
					set_value_using_prev_combine(i, period_multiply_n, 1);
				}
				else {
					set_value_using_prev_combine(i, period_multiply_n, values[i - 1] + 1);
				}
				i += (period_multiply_n - 1);
				continue;
			}
			if (cs[i] < cs[i - 4 * period_multiply_n]) {
				if (values[i - 1] > 0) {
					set_value_using_prev_combine(i, period_multiply_n, -1);
				}
				else if (values[i - 1] <= -9) {
					set_value_using_prev_combine(i, period_multiply_n, -1);
				}
				else {
					set_value_using_prev_combine(i, period_multiply_n, values[i - 1] - 1);
				}
				i += (period_multiply_n - 1);
				continue;
			}
			if (equal(cs[i], cs[i - 4 * period_multiply_n])) {
				if (values[i - 1] < 0) {
					if (values[i - 1] <= -9)
						set_value_using_prev_combine(i, period_multiply_n, -1);
					else
						set_value_using_prev_combine(i, period_multiply_n, values[i - 1] - 1);
				}
				if (values[i - 1] > 0) {
					if (values[i - 1] >= 9)
						set_value_using_prev_combine(i, period_multiply_n, 1);
					else
						set_value_using_prev_combine(i, period_multiply_n, values[i - 1] + 1);
				}
				i += (period_multiply_n - 1);
				continue;
			}
		}
	}
	const int at(int i) const { return values[i]; }
	const int sum_at(int at, int n) const {
		int sum = 0;
		for (int i = 0; i < n && at - i>0; i++) {
			sum += values[at - i];
		}
		return sum;
	}
	static bool all_same_sign(const std::vector<int>& numbers) {
		if (numbers.empty()) {
			return true;
		}
		int firstSign = (numbers[0] < 0) ? -1 : 1;
		for (size_t i = 1; i < numbers.size(); ++i) {
			int currentSign = (numbers[i] < 0) ? -1 : 1;
			if (currentSign != firstSign) {
				return false;
			}
		}
		return true;
	}
	const int int_id(int at) const {
		std::vector<int> arr;
		int id = 0;
		for (int i = 0; i < period_multiply_n && at - i>0; i++) {
			arr.push_back(values[at - i]);
		}
		if (all_same_sign(arr)) {
			for (int i = 0; i < arr.size(); i++) {
				id += arr[arr.size() - 1 - i] * pow(10, i);
			}
		}
		else {
			for (int i = 0; i < arr.size(); i++)
				id += std::abs(arr[arr.size() - 1 - i]) * pow(10, i);
			int firstSign = (arr[0] < 0) ? -1 : 1;
			id *= firstSign;
		}
		return id;
	}
	const int operator[](int i) const { return at(i); }
};

struct AVChanges {
	int n;
	std::vector<float> achanges;
	std::vector<float> vchanges;
	AVChanges(const std::vector<float>& cs, const std::vector<double>& vs, int n) {
		this->n = n;
		init(cs, vs);
	}
	void init(const std::vector<float>& cs, const std::vector<double>& vs) {
		if (cs.empty())
			return;
		CWR cwr(cs, n);
		auto vma = MA(vs, n);
		achanges.resize(cs.size());
		vchanges.resize(cs.size());
		for (int i = 1; i < cs.size(); i++) {
			achanges[i] = cwr.amps[i] / get_avg(cwr.amps, i);
			vchanges[i] = vma[i] / get_avg(vma, i);
		}
	}
	double get_avg(const std::vector<float>& values, int at) const {
		int n = 0;
		double sum = 0.0;
		for (int i = 1; i < 25 && at - i * n>0; i++) {
			sum += values[at - i * n];
			n++;
		}
		return sum / n;
	}
	double get_avg(const std::vector<double>& values, int at) const {
		int n = 0;
		double sum = 0.0;
		for (int i = 1; i < 25 && at - i * n>0; i++) {
			sum += values[at - i * n];
			n++;
		}
		return sum / n;
	}
};

}
