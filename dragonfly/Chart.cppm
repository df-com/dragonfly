// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <map>
#include <cassert>
#include <vector>
#include <magic_enum.hpp>
#include <boost/filesystem/operations.hpp>
#include <SQLiteCpp/Database.h>

import dragonfly.ind;

export module dragonfly:Chart;

import :Config;
import :Candlestick;
import :ASticks;

using namespace dragonfly::ind;

using time_point = std::chrono::system_clock::time_point;

export namespace dragonfly {

enum class Period {
	M1, M5, M15, D1, Nil
};

class Chart : public ASticks {
public:
	static std::string PeriodToString(const Period& period) {
		return magic_enum::enum_name(period).data();
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

	Period period_;

	std::vector<float> hs_;
	std::vector<float> ls_;
	std::vector<float> cs_;

	std::vector<std::unique_ptr<CBL>> cbls_;
	std::vector<std::unique_ptr<DMI_I>> dmis_;
	std::vector<std::unique_ptr<BOLL>> bolls_;
	std::vector<std::unique_ptr<KDJ>> kdjs_;
	std::vector<std::unique_ptr<MACD>> macds_;
	std::vector<std::unique_ptr<RSI>> rsis_;
	std::vector<std::unique_ptr<ind::MIKE>> mikes_;
	std::vector<std::unique_ptr<WR>> wrs_;
	std::vector<std::unique_ptr<SAR>> sars_;
public:
	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const InstrumentType instrument_type() const { return instrument_type_; }
	Period period() const { return period_; }
public:
	const std::vector<float>& hs() const { return hs_; }
	const std::vector<float>& ls() const { return ls_; }
	const std::vector<float>& cs() const { return cs_; }
	const std::vector<std::unique_ptr<BOLL>>& bolls() const { return bolls_; }
	const std::vector<std::unique_ptr<KDJ>>& kdjs() const { return kdjs_; }
	const std::vector<std::unique_ptr<WR>>& wrs() const { return wrs_; }
	const std::vector<std::unique_ptr<DMI_I>>& dmis() const { return dmis_; }
	const std::vector<std::unique_ptr<SAR>>& sars() const { return sars_; }
	Chart() {}
	Chart(const std::string& id, Period period, time_point beginTime, time_point endTime) {
		this->id_ = id;
		this->period_ = period;
		this->instrument_type_ = GetInstrumentTypeById(this->id());
		loadData(Time(beginTime), Time(endTime));
		hs_ = Hs(sticks());
		ls_ = Ls(sticks());
		cs_ = Cs(sticks());
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
			if (!boost::filesystem::is_directory(folder) || !boost::filesystem::exists(folder)) {
				boost::filesystem::create_directories(folder);
			}
			auto dbpath = folder + "\\" + id() + ".db";
			ASticks::CreateTableIfNotExists(dbpath);
			arc::pb::Candlestick max_date_stick;
			max_date_stick.set_datetime(Time().AddDays(-365 * 12).epoch());
			pbsqlite::Database db(dbpath, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
			auto dbsticks = db.Select<arc::pb::Candlestick>("ORDER BY datetime DESC limit 1");
			if (!dbsticks.empty()) {
				max_date_stick = dbsticks.front();
			}
			ASticks downloader;
			downloader.FetchFromTraderScaleSever(periodString, id(), Time(max_date_stick.datetime()), endTime.AddDays(1));
			downloader.ReplaceInto(db);
			FetchFromSQLiteDatabase(dbpath, beginTime, endTime);
		}
		return;
	}
	const KNode k(size_t i) const { return KNode(sticks(), i); }
	const KNode k(const Time& t) const { return KNode(sticks(), GetIndex(t)); }
	const KNode nearest_k(const Time& t) const { return KNode(sticks(), GetNearestIndex(t)); }
	const BOLL& boll(int n, int p) {
		std::vector<std::unique_ptr<BOLL>>::iterator it = std::find_if(bolls_.begin(), bolls_.end(), [&](const std::unique_ptr<BOLL>& v) {
			return (v->n == n, v->p == p);
			});
		if (it == bolls_.end()) {
			bolls_.push_back(std::unique_ptr<BOLL>(new BOLL(this->cs(), n, p)));
			return *bolls_.back();
		}
		else
			return **it;
	}
	const CBL* cbl(int n) {
		std::vector<std::unique_ptr<CBL>>::iterator it = std::find_if(cbls_.begin(), cbls_.end(), [&](const std::unique_ptr<CBL>& v) {
			return (v->CombineN == n);
			});
		if (it == cbls_.end()) {
			cbls_.push_back(std::make_unique<CBL>(cs(), hs(), ls(), n));
			return cbls_.back().get();
		}
		else
			return (*it).get();
	}
	const DMI_I* dmi(int n1, int n2) {
		std::vector<std::unique_ptr<DMI_I>>::iterator it = std::find_if(dmis_.begin(), dmis_.end(), [&](const std::unique_ptr<DMI_I>& v) {
			return (v->n1 == n1 && v->n2 == n2);
			});
		if (it == dmis_.end()) {
			dmis_.push_back(std::unique_ptr<DMI_I>(new DMI_I(cs(), hs(), ls(), n1, n2)));
			return dmis_.back().get();
		}
		else
			return (*it).get();
	}
	const WR* wrs_find(int n) const {
		std::vector<std::unique_ptr<WR>>::const_iterator it = std::find_if(wrs_.begin(), wrs_.end(), [&](const std::unique_ptr<WR>& v) {
			return (v->n == n);
			});
		if (it == wrs_.end()) {
			return nullptr;
		}
		else
			return (*it).get();
	}
	const WR& wr(int n) {
		std::vector<std::unique_ptr<WR>>::iterator it = std::find_if(wrs_.begin(), wrs_.end(), [&](const std::unique_ptr<WR>& v) {
			return (v->n == n);
			});
		if (it == wrs_.end()) {
			wrs_.push_back(std::move(std::unique_ptr<WR>(new WR(hs(), ls(), cs(), n))));
			return *wrs_.back();
		}
		else
			return **it;
	}
	const KDJ& kdj(int n, int sn1, int sn2) {
		std::vector<std::unique_ptr<KDJ>>::iterator it = std::find_if(kdjs_.begin(), kdjs_.end(), [&](const std::unique_ptr<KDJ>& v) {
			return (v->n() == n && v->sn1() == sn1 && v->sn2() == sn2);
			});
		if (it == kdjs_.end()) {
			kdjs_.push_back(std::unique_ptr<KDJ>(new KDJ(cs(), hs(), ls(), n, sn1, sn2)));
			return *kdjs_.back();
		}
		else
			return **it;
	}
	const MACD& macd(int shortN, int longN, int deaN) {
		std::vector<std::unique_ptr<MACD>>::iterator it = std::find_if(macds_.begin(), macds_.end(), [&](const std::unique_ptr<MACD>& v) {
			return (v->short_n == shortN && v->long_n == longN && v->dea_n == deaN);
			});
		if (it == macds_.end()) {
			macds_.push_back(std::make_unique<MACD>(Cs(sticks()), shortN, longN, deaN));
			return *macds_.back();
		}
		else
			return **it;
	}
	const RSI& rsi(int n) {
		auto it = std::find_if(rsis_.begin(), rsis_.end(), [&](const std::unique_ptr<RSI>& v) {
			return (v->n == n);
			});
		if (it == rsis_.end()) {
			rsis_.push_back(std::make_unique<RSI>(Cs(sticks()), n));
			return *rsis_.back();
		}
		else
			return **it;
	}
	const ind::MIKE& mike(int n) {
		std::vector<std::unique_ptr<ind::MIKE>>::iterator it = std::find_if(mikes_.begin(), mikes_.end(), [&](const std::unique_ptr<ind::MIKE>& v) {
			return (v->n == n);
			});
		if (it == mikes_.end()) {
			mikes_.push_back(std::unique_ptr<ind::MIKE>(new ind::MIKE(cs(), hs(), ls(), n)));
			return *mikes_.back();
		}
		else
			return **it;
	}
	const SAR* sar(double af_init, double af_delta, double af_max, int n) {
		std::vector<std::unique_ptr<SAR>>::iterator it = std::find_if(sars_.begin(), sars_.end(), [&](const std::unique_ptr<SAR>& v) {
			return v->IsParametersEqual(af_init, af_delta, af_max, n);
			});
		if (it == sars_.end()) {
			sars_.push_back(std::make_unique<SAR>(cs(), hs(), ls(), af_init, af_delta, af_max, n));
			return sars_.back().get();
		}
		else
			return (*it).get();
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
