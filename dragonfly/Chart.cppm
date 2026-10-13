// License: MIT License   See LICENSE.txt for the full license.
module;
#include <iostream>
#include <map>
#include <filesystem>
#include <cassert>
#include <vector>
#include <mutex>
#include <SQLiteCpp/Database.h>
//#include "candlestick.pb.h"

import dragonfly.ind;

export module dragonfly:Chart;

import :Config;
import :Candlestick;
import :ASticks;

using time_point = std::chrono::system_clock::time_point;

export namespace dragonfly {

std::mutex g_mu;

enum class Period {
	M1, M5, M15, D1, Nil
};

class Chart : public StickSeries {
public:
	static std::string PeriodToString(const Period& period) {
		std::vector<std::string> strs = { "M1", "M5", "M15", "D1", "Nil" };
		return strs[(int)period];
	}
	static int PeriodMinutes(const Period& period) {
		if (period == Period::M1) { return 1; }
		if (period == Period::M5) { return 5; }
		if (period == Period::M15) { return 15; }
		assert(false);
		return INT_MIN;
	}
protected:
	std::string id_;
	std::string name_;
	InstrumentType instrument_type_;

	std::mutex mu;

	Period period_;

	std::vector<std::unique_ptr<ind::CBL>> cbls_;
	std::vector<std::unique_ptr<ind::DMI_I>> dmis_;
	std::vector<std::unique_ptr<ind::BOLL>> bolls_;
	std::vector<std::unique_ptr<ind::KDJ>> kdjs_;
	std::vector<std::unique_ptr<ind::MACD>> macds_;
	std::vector<std::unique_ptr<ind::RSI>> rsis_;
	std::vector<std::unique_ptr<ind::MIKE>> mikes_;
	std::vector<std::unique_ptr<ind::WR>> wrs_;
	std::vector<std::unique_ptr<ind::SAR>> sars_;
	std::vector<std::unique_ptr<ind::MagicNine>> magic_nines_;
	mutable int cached_sticks_per_day_ = -1;
public:
	void set_id(const std::string& v) { id_ = v; }
	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const InstrumentType instrument_type() const { return instrument_type_; }
	Period period() const { return period_; }
public:
	const std::vector<std::unique_ptr<ind::BOLL>>& bolls() const { return bolls_; }
	const std::vector<std::unique_ptr<ind::KDJ>>& kdjs() const { return kdjs_; }
	const std::vector<std::unique_ptr<ind::WR>>& wrs() const { return wrs_; }
	const std::vector<std::unique_ptr<ind::DMI_I>>& dmis() const { return dmis_; }
	const std::vector<std::unique_ptr<ind::SAR>>& sars() const { return sars_; }
	const std::vector<std::unique_ptr<ind::MagicNine>>& magic_nines() const { return magic_nines_; }
	Chart() {}
	Chart(const std::string& id, Period period, time_point beginTime, time_point endTime) {
		this->id_ = id;
		this->period_ = period;
		this->instrument_type_ = GetInstrumentTypeById(this->id());
		loadData(Time(beginTime), Time(endTime));
	}
	void loadData(Time beginTime, Time endTime) {
		std::string periodString = PeriodToString(period());
		if (Config::instance().data_provider == DataProvider::TraderScale && Config::instance().local_store_type == LocalStoreType::None) {
			if (id().rfind("r_", 0) == 0 || id().rfind("R_", 0) == 0) {
				FetchFromTraderScaleSever(periodString, id().substr(2), beginTime, endTime);
			}
			else {
				FetchFromTraderScaleSever(periodString, id(), beginTime, endTime);
			}
			if ((instrument_type() == InstrumentType::Futures || instrument_type() == InstrumentType::Forex)) {
				for (size_t i = 0; i < sticks().size(); i++) {
					sticks_[i].mutate_datetime(sticks_[i].datetime() + 60 * 60 * 8);
				}
				if (period() == Period::M5)
					for (size_t i = 0; i < sticks().size(); i++) {
						sticks_[i].mutate_datetime(sticks_[i].datetime() + 60 * 5);
					}
			}
		}
		if (Config::instance().data_provider == DataProvider::TraderScale && Config::instance().local_store_type == LocalStoreType::SQLite && instrument_type() == InstrumentType::ChineseFutures) {
			auto folder = Config::instance().local_store_path() + "\\" + "cnf" + "\\" + periodString;
			if (!std::filesystem::is_directory(folder) || !std::filesystem::exists(folder)) {
				std::filesystem::create_directories(folder);
			}
			auto dbpath = folder + "\\" + id() + ".db";
			StickSeries::CreateTableIfNotExists(dbpath);

			Candlestick max_date_stick;
			max_date_stick.mutate_datetime(Time().AddDays(-365 * 12).epoch());
			SQLite::Database db(dbpath, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);

			std::vector<Candlestick> dbsticks = StickSeries::Select(db, "ORDER BY datetime DESC LIMIT 1");
			if (!dbsticks.empty()) {
				max_date_stick = dbsticks.front();
			}

			StickSeries downloader;
			downloader.FetchFromTraderScaleSever(periodString, id(), Time(max_date_stick.datetime()), endTime.AddDays(1));
			downloader.ReplaceInto(db);
			FetchFromSQLiteDatabase(dbpath, beginTime, endTime);
		}
		if (Config::instance().data_provider == DataProvider::Sohu && Config::instance().local_store_type == LocalStoreType::SQLite && instrument_type() == InstrumentType::ChineseStock) {
			std::string url = std::format("http://q.stock.sohu.com/hisHq?code={}&start={}&end={}", id(), beginTime.ToString("%Y%m%d"), endTime.ToString("%Y%m%d"));
		}
		return;
	}
	const KNode k(size_t i) const { return KNode(sticks(), i); }
	const KNode k(const Time& t) const { return KNode(sticks(), GetIndex(t)); }
	const KNode nearest_k(const Time& t) const { return KNode(sticks(), GetNearestIndex(t)); }
	int sticks_size_per_day() const {
		std::lock_guard<std::mutex> lock(g_mu);
		if (cached_sticks_per_day_ != -1) {
			return cached_sticks_per_day_;
		}
		const auto& s = sticks();
		size_t total = s.size();
		if (total == 0) return 0;

		auto get_day_id = [&](size_t idx) -> int64_t {
			return (sticks()[idx].datetime()) / 86400;
		};

		auto count_full_day = [&](size_t sample_idx) -> int {
			int64_t target_day = get_day_id(sample_idx);

			size_t start_idx = sample_idx;
			while (start_idx > 0 && get_day_id(start_idx - 1) == target_day) {
				--start_idx;
			}

			int count = 0;
			for (size_t i = start_idx; i < total; ++i) {
				if (get_day_id(i) == target_day) {
					count++;
				}
				else {
					break;
				}
			}
			return count;
		};

		int c1 = count_full_day(0);
		int c2 = count_full_day(total / 2);
		int c3 = count_full_day(total > 100 ? total - 100 : 0);

		if (c1 == c2 || c1 == c3) return c1;
		if (c2 == c3) return c2;

		cached_sticks_per_day_ = std::max(std::min(c1, c2), std::min(std::max(c1, c2), c3));

		return cached_sticks_per_day_;
	}
	const ind::WR* wrs_find(int n) const {
		std::vector<std::unique_ptr<ind::WR>>::const_iterator it = std::find_if(wrs_.begin(), wrs_.end(), [&](const std::unique_ptr<ind::WR>& v) {
			return (v->n == n);
			});
		if (it == wrs_.end()) {
			return nullptr;
		}
		else
			return (*it).get();
	}
	const ind::BOLL& boll(int n, int p) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::BOLL>>::iterator it = std::find_if(bolls_.begin(), bolls_.end(), [&](const std::unique_ptr<ind::BOLL>& v) {
			return (v->n == n, v->p == p);
			});
		if (it == bolls_.end()) {
			bolls_.push_back(std::unique_ptr<ind::BOLL>(new ind::BOLL(Cs(sticks()), n, p)));
			g_mu.unlock();
			return *bolls_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::CBL& cbl(int n) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::CBL>>::iterator it = std::find_if(cbls_.begin(), cbls_.end(), [&](const std::unique_ptr<ind::CBL>& v) {
			return (v->CombineN == n);
			});
		if (it == cbls_.end()) {
			cbls_.push_back(std::make_unique<ind::CBL>(Cs(sticks()), Hs(sticks()), Ls(sticks()), n));
			g_mu.unlock();
			return *cbls_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::DMI_I& dmi(int n1, int n2) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::DMI_I>>::iterator it = std::find_if(dmis_.begin(), dmis_.end(), [&](const std::unique_ptr<ind::DMI_I>& v) {
			return (v->n1 == n1 && v->n2 == n2);
			});
		if (it == dmis_.end()) {
			dmis_.push_back(std::unique_ptr<ind::DMI_I>(new ind::DMI_I(Cs(sticks()), Hs(sticks()), Ls(sticks()), n1, n2)));
			g_mu.unlock();
			return *dmis_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::WR& wr(int n) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::WR>>::iterator it = std::find_if(wrs_.begin(), wrs_.end(), [&](const std::unique_ptr<ind::WR>& v) {
			return (v->n == n);
			});
		if (it == wrs_.end()) {
			wrs_.push_back(std::move(std::unique_ptr<ind::WR>(new ind::WR(Hs(sticks()), Ls(sticks()), Cs(sticks()), n))));
			g_mu.unlock();
			return *wrs_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::KDJ& kdj(int n, int sn1, int sn2) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::KDJ>>::iterator it = std::find_if(kdjs_.begin(), kdjs_.end(), [&](const std::unique_ptr<ind::KDJ>& v) {
			return (v->n() == n && v->sn1() == sn1 && v->sn2() == sn2);
			});
		if (it == kdjs_.end()) {
			kdjs_.push_back(std::unique_ptr<ind::KDJ>(new ind::KDJ(Cs(sticks()), Hs(sticks()), Ls(sticks()), n, sn1, sn2)));
			g_mu.unlock();
			return *kdjs_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::MACD& macd(int shortN, int longN, int deaN) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::MACD>>::iterator it = std::find_if(macds_.begin(), macds_.end(), [&](const std::unique_ptr<ind::MACD>& v) {
			return (v->short_n == shortN && v->long_n == longN && v->dea_n == deaN);
			});
		if (it == macds_.end()) {
			macds_.push_back(std::make_unique<ind::MACD>(Cs(sticks()), shortN, longN, deaN));
			g_mu.unlock();
			return *macds_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::RSI& rsi(int n) {
		g_mu.lock();
		auto it = std::find_if(rsis_.begin(), rsis_.end(), [&](const std::unique_ptr<ind::RSI>& v) {
			return (v->n == n);
			});
		if (it == rsis_.end()) {
			rsis_.push_back(std::make_unique<ind::RSI>(Cs(sticks()), n));
			g_mu.unlock();
			return *rsis_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::MIKE& mike(int n) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::MIKE>>::iterator it = std::find_if(mikes_.begin(), mikes_.end(), [&](const std::unique_ptr<ind::MIKE>& v) {
			return (v->n == n);
			});
		if (it == mikes_.end()) {
			mikes_.push_back(std::unique_ptr<ind::MIKE>(new ind::MIKE(Cs(sticks()), Hs(sticks()), Ls(sticks()), n)));
			g_mu.unlock();
			return *mikes_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::SAR& sar(double af_init, double af_delta, double af_max, int n) {
		g_mu.lock();
		std::vector<std::unique_ptr<ind::SAR>>::iterator it = std::find_if(sars_.begin(), sars_.end(), [&](const std::unique_ptr<ind::SAR>& v) {
			return v->IsParametersEqual(af_init, af_delta, af_max, n);
			});
		if (it == sars_.end()) {
			sars_.push_back(std::make_unique<ind::SAR>(Cs(sticks()), Hs(sticks()), Ls(sticks()), af_init, af_delta, af_max, n));
			g_mu.unlock();
			return *sars_.back();
		}
		else {
			g_mu.unlock();
			return **it;
		}
	}
	const ind::MagicNine& magic_nine(int n) {
		mu.lock();
		std::vector<std::unique_ptr<ind::MagicNine>>::iterator it = std::find_if(magic_nines_.begin(), magic_nines_.end(), [&](const std::unique_ptr<ind::MagicNine>& v) {
			return (v->period_multiply_n == n);
			});
		if (it == magic_nines_.end()) {
			magic_nines_.push_back(std::make_unique<ind::MagicNine>(Hs(sticks()), Ls(sticks()), Cs(sticks()), n));
			mu.unlock();
			return *magic_nines_.back();
		}
		else {
			mu.unlock();
			return **it;
		}
	}
};

inline Period predict_period(const std::vector<Candlestick>& sticks) {
	std::map<int, int> vote;
	for (int i = 0; i < sticks.size() - 1; i++) {
		++vote[(int)(sticks[i + 1].datetime() - sticks[i].datetime())];
	}
	auto res = ind::max(vote);
	if (res.first == 24 * 60 * 60) { return Period::D1; }
	if (res.first == 5 * 60) { return Period::M5; }
	return Period::Nil;
}

}
