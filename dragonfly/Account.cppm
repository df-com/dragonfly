// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <numeric>
#include <limits>
#include <string>
#include <sstream>

export module dragonfly:Account;

import :Chart;

export namespace dragonfly::experimental {

struct Position {
	enum Type {
		Long = 1,
		Short = -1
	};
	enum CloseType {
		StopLoss = -1, TimeOut, TakeProfit, Unknown, Conflict, NotSet
	};
	Type type = Type::Long;
	CloseType close_type = CloseType::NotSet;
	const Chart* chart = nullptr;

	float open_price;
	float close_price;

	int open_at = -1;
	int close_at = -1;

	bool is_closed() const { return (open_at >= 0 && close_at >= 0); }

	void Open(Type type, const Chart* chart, int createI, float createPrice) {
		this->type = type;
		this->chart = chart;
		this->open_at = createI;
		this->open_price = createPrice;
	}
	void Close(int closeI, double closePrice, CloseType closeType) {
		this->close_at = closeI;
		this->close_price = closePrice;
		this->close_type = closeType;
	}
	double profit_percent() const {
		return (close_price - open_price) / open_price * type;
	}
	double profit_percent(double price) const { return (price - open_price) / open_price * type; }
	double max_floating_loss() const {
		if (type == Type::Long) {
			auto low_iter = std::min_element(chart->sticks().begin() + open_at + 1, chart->sticks().begin() + close_at + 1, [](auto const& a, auto const& b) {
				return a.low() < b.low();
				});
			return this->profit_percent(low_iter->low());
		}
		else {
			auto high_iter = std::max_element(chart->sticks().begin() + open_at + 1, chart->sticks().begin() + close_at + 1, [](auto const& a, auto const& b) {
				return a.high() < b.high();
				});
			return this->profit_percent(high_iter->high());
		}
	}
	double max_floating_profit() const { return max_floating_profit(close_at); }
	double max_floating_profit(int tempEndI) const {
		if (type == Type::Long) {
			auto high_iter = std::max_element(chart->sticks().begin() + open_at + 1, chart->sticks().begin() + tempEndI + 1, [](auto const& a, auto const& b) {
				return a.high() < b.high();
				});
			return this->profit_percent(high_iter->high());
		}
		else {
			auto low_iter = std::min_element(chart->sticks().begin() + open_at + 1, chart->sticks().begin() + tempEndI + 1, [](auto const& a, auto const& b) {
				return a.low() < b.low();
				});
			return this->profit_percent(low_iter->low());
		}
	}
	Time open_time() const { return Time(chart->sticks()[open_at].datetime()); }
};

class Account {
public:
	std::vector<Position> history; // closed_positions;
	std::vector<Position> holds;
	void OpenPosition(Position p) {
		holds.push_back(p);
	}
	double win_rate() const {
		if (history.size() == 0)
			return 0.0;
		double n = 0;
		for (auto&& p : history) {
			if (p.profit_percent() > 0) {
				n += 1;
			}
		}
		return n / history.size();
	}
	double profit() const {
		if (history.size() == 0) { return -1.0; }
		int n = 0;
		double profit = 0.0;
		for (auto& p : history) {
			n++;
			profit += p.profit_percent();
		}
		return profit;
	}
	double max_drawdown() const {
		if (history.size() == 0) { return 0.0; }
		if (history.size() == 1) { return history[0].profit_percent(); }
		double md = 0.0;
		std::vector<Position> ps = history;
		std::sort(std::begin(ps), std::end(ps), [](const auto& a, const auto& b) {
			return a.open_time() < b.open_time();
			});
		std::vector<double> profits;
		std::vector<double> grandTotals;
		for (auto& pos : ps) {
			profits.push_back(pos.profit_percent());
		}
		grandTotals.resize(profits.size());
		grandTotals[0] = profits[0];
		for (int i = 1; i < grandTotals.size(); i++) {
			grandTotals[i] = grandTotals[i - 1] + profits[i];
		}
		for (int i = 1; i < grandTotals.size(); i++) {
			auto high = *std::max_element(std::begin(grandTotals), std::begin(grandTotals) + i + 1);
			auto temp = grandTotals[i] - high;
			md = std::min(temp, md);
		}
		return md;
	}
};

}
