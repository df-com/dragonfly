// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <vector>
#include <chrono>
#include <assert.h>

#include "candlesticks_generated.h"
#include "candlestick.pb.h"

export module dragonfly:Candlestick;
import :base;
import :Time;

export namespace dragonfly {

typedef arc::Candlestick Candlestick;

struct KNode {
public:
	const Candlestick* prev = nullptr;
	const Candlestick* self = nullptr;
public:
	//KNode(const KNode& other) { prev = other.prev; self = other.self; }
	KNode(const std::vector<Candlestick>& sticks, size_t i) {
		if (i >= 1) { prev = &sticks[i - 1]; }
		else { prev = nullptr; }
		self = &sticks[i];
	}
	Time close_time() const { return Time(self->datetime()); }
	double open_percent() const {
		if (prev == nullptr) { return 0.0; }
		return (self->open() - prev->close()) / std::abs(prev->close());
	}
	double high_percent() const {
		if (prev == nullptr) { return (self->high() - self->open()) / std::abs(self->open()); }
		return (self->high() - prev->close()) / std::abs(prev->close());
	}
	double low_percent() const {
		if (prev == nullptr) { return (self->low() - self->open()) / std::abs(self->open()); }
		return (self->low() - prev->close()) / std::abs(prev->close());
	}
	double up_percent() const {
		if (prev == nullptr) { return 0.0; }
		return (self->close() - prev->close()) / std::abs(prev->close());
	}
	double amplitude() const {
		if (self->open() == 0.0) { return 0.0; }
		auto ret = std::max({ ((double)self->high() - self->low()) / std::abs(self->open()), std::abs(high_percent()), std::abs(low_percent()) });
		return std::abs(ret);
	}
	double body_amplitude() const { return abs(self->open() - self->close()) / std::abs(self->open()); }
	double body_amplitude_with_direction() const { return (self->close() - self->open()) / std::abs(self->open()); }
	double upper_shadow_amplitude() const { return (self->high() - std::max(self->open(), self->close())) / std::abs(self->open()); }
	double lower_shadow_amplitude() const { return (std::min(self->open(), self->close()) - self->low()) / std::abs(self->open()); }
	double close_prev_open_percent() const { return (self->close() - prev->open()) / std::abs(prev->close()); }

	double roof_price(int percent) const { // percent like 5,10,-5,-10
		int prev_close_x100 = (int)(prev->close() * 100 + 0.5);
		int roof_x100 = (prev_close_x100 * (100 + percent) + 50) / 100;
		return (double)roof_x100 / 100.0;
	}
	bool is_roof(int percent) const {
		if (prev == nullptr)
			return false;
		return (AreNear(roof_price(percent), (double)self->close(), 0.001) && dragonfly::equal(self->close(), self->high()));
	}
	bool is_floor(int percent) const {
		if (prev == nullptr)
			return false;
		return (AreNear(roof_price(percent), (double)self->close(), 0.001) && dragonfly::equal(self->close(), self->low()));
	}
};

inline std::vector<float> Hs(const std::vector<Candlestick>& sticks) {
	std::vector<float> hs;
	hs.resize(sticks.size());
	for (size_t i = 0; i < sticks.size(); i++) {
		hs[i] = sticks[i].high();
	}
	return std::move(hs);
}

inline std::vector<float> Ls(const std::vector<Candlestick>& sticks) {
	std::vector<float> ls;
	ls.resize(sticks.size());
	for (size_t i = 0; i < sticks.size(); i++) {
		ls[i] = sticks[i].low();
	}
	return std::move(ls);
}

inline std::vector<float> Cs(const std::vector<Candlestick>& sticks) {
	std::vector<float> cs;
	cs.resize(sticks.size());
	for (size_t i = 0; i < sticks.size(); i++) {
		cs[i] = sticks[i].close();
	}
	return std::move(cs);
}

inline std::vector<float> Os(const std::vector<Candlestick>& sticks) {
	std::vector<float> cs;
	cs.resize(sticks.size());
	for (size_t i = 0; i < sticks.size(); i++) {
		cs[i] = sticks[i].open();
	}
	return std::move(cs);
}

inline std::vector<double> Vs(const std::vector<Candlestick>& sticks) {
	std::vector<double> vs;
	vs.resize(sticks.size());
	for (size_t i = 0; i < sticks.size(); i++) {
		vs[i] = sticks[i].volume();
	}
	return std::move(vs);
}

inline std::vector<float> Amplitudes(const std::vector<Candlestick>& sticks) {
	std::vector<float> ret;
	ret.resize(sticks.size());
	for (size_t i = 0; i < sticks.size(); i++) {
		KNode k(sticks, i);
		ret[i] = (float)k.amplitude();
	}
	return std::move(ret);
}

inline Candlestick Combine(const std::vector<Candlestick>& sticks, int beginI, int endI) {
	assert(beginI >= 0);
	assert(endI < sticks.size());
	Candlestick ret = sticks[beginI];
	ret.mutate_close(sticks[endI].close());
	ret.mutate_datetime(sticks[endI].datetime());
	for (int i = beginI + 1; i <= endI; i++) {
		if (ret.high() < sticks[i].high()) {
			ret.mutate_high(sticks[i].high());
		}
		if (ret.low() > sticks[i].low()) {
			ret.mutate_low(sticks[i].low());
		}
		ret.mutate_volume(ret.volume() + sticks[i].volume());
	}
	return ret;
}

inline float GetPos(float low, float high, float price) {
	return (price - low) / (high - low);
}

inline Candlestick ToFBCandlestick(const arc::pb::Candlestick& stick) {
	Candlestick ret;
	ret.mutate_datetime(stick.datetime());
	ret.mutate_open(stick.open());
	ret.mutate_high(stick.high());
	ret.mutate_low(stick.low());
	ret.mutate_close(stick.close());
	ret.mutate_volume(stick.volume());
	return ret;
}

inline std::vector<Candlestick> ToFBCandlesticks(const std::vector<arc::pb::Candlestick>& sticks) {
	std::vector<Candlestick> ret;
	ret.reserve(sticks.size());
	for (auto& s : sticks)
		ret.push_back(ToFBCandlestick(s));
	return ret;
}

inline arc::pb::Candlestick ToPBCandlestick(const Candlestick& stick) {
	arc::pb::Candlestick ret;
	ret.set_datetime(stick.datetime());
	ret.set_open(stick.open());
	ret.set_high(stick.high());
	ret.set_low(stick.low());
	ret.set_close(stick.close());
	ret.set_volume(stick.volume());
	return ret;
}

inline std::string to_string(const Candlestick& stick) {
	return std::to_string(stick.datetime()) + std::to_string(stick.open()) + std::to_string(stick.high()) + std::to_string(stick.low()) + std::to_string(stick.close()) + std::to_string(stick.volume());
}

inline bool AreSame(const Candlestick& stick1, const Candlestick& stick2) {
	if (stick1.datetime() != stick2.datetime())
		return false;
	if (!equal(stick1.open(), stick2.open()))
		return false;
	if (!equal(stick1.high(), stick2.high()))
		return false;
	if (!equal(stick1.low(), stick2.low()))
		return false;
	if (!equal(stick1.close(), stick2.close()))
		return false;
	if (!equal(stick1.volume(), stick2.volume()))
		return false;
	return true;
}

inline bool AreSame(const std::vector<Candlestick>& sticks1, const std::vector<Candlestick>& sticks2) {
	if (sticks1.size() != sticks2.size())
		return false;
	for (int i = 0; i < sticks1.size(); i++) {
		if (!AreSame(sticks1[i], sticks2[i]))
			return false;
	}
	return true;
}

inline bool equal(const Candlestick& stick1, const Candlestick& stick2, float price_error_percent, double volume_error_percent) {
	if (stick1.datetime() != stick2.datetime())
		return false;
	if (std::abs(stick1.open() - stick2.open()) > price_error_percent * stick1.open())
		return false;
	if (std::abs(stick1.high() - stick2.high()) > price_error_percent * stick1.open())
		return false;
	if (std::abs(stick1.low() - stick2.low()) > price_error_percent * stick1.open())
		return false;
	if (std::abs(stick1.close() - stick2.close()) > price_error_percent * stick1.open())
		return false;
	if (std::abs(stick1.volume() - stick2.volume()) > volume_error_percent * stick1.volume())
		return false;
	return true;
}

inline bool equal(const std::vector<Candlestick>& sticks1, const std::vector<Candlestick>& sticks2, float price_error_percent, double volume_error_percent) {
	if (sticks1.size() != sticks2.size())
		return false;
	for (int i = 0; i < sticks1.size(); i++) {
		if (!equal(sticks1[i], sticks2[i], price_error_percent, volume_error_percent))
			return false;
	}
	return true;
}

}
