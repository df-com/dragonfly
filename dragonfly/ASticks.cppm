// License: MIT License   See LICENSE.txt for the full license.
//#define DF_MYSQL_USING

#include <iostream>
#include <map>
#include <cassert>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <httplib.h>
#include <spdlog/spdlog.h>
#ifdef DF_MYSQL_USING
#include <mysqlx/xdevapi.h>
#endif
//#include "candlesticks_generated.h"
#include "candlestick.pb.h"
#include "pbsqlite_copied.h"

import arc.Candlestick;

export module dragonfly:ASticks;

import :Config;
import :Candlestick;
import :Time;
import :base;

export namespace dragonfly {

enum class InstrumentType {
	None, ChineseFutures, ChineseStock, Futures, Forex, Others
};

inline int GetInsNumber(const std::string& id) {
	std::string upperId = boost::to_upper_copy<std::string>(id);
	if (id.rfind("r_", 0) == 0 || id.rfind("R_", 0) == 0) {
		upperId = boost::to_upper_copy<std::string>(id.substr(2, id.length() - 2));
	}
	std::vector<std::string> ids = { "AU", "AP", "AG", "NI", "AL", "ZN", "PG", "SS", "SA", "EB", "EG", "SP", "V", "FU", "I", "RB", "RU", "BU", "JM", "J", "JD", "A", "C", "CF", "CS", "M", "MA", "ZC", "TA", "SR", "SM", "RM", "Y", "OI", "P", "PP", "HC", "L", "FG",
									 "EURUSD", "GBPUSD", "USDJPY", "USDCAD", "USDCHF", "USDCNH", "AUDUSD", "NZDUSD", "USDOLLAR"
									 "NAS100", "CHN50", "USOIL", "COPPER", "XAUUSD", "XAGUSD", "SOYF"
	};
	return (int)std::distance(ids.begin(), std::find(ids.begin(), ids.end(), upperId));
}

inline InstrumentType GetInstrumentTypeById(const std::string& id) {
	std::string upperId = boost::to_upper_copy<std::string>(id);
	if (id.rfind("r_", 0) == 0 || id.rfind("R_", 0) == 0) {
		upperId = boost::to_upper_copy<std::string>(id.substr(2, id.length() - 2));
	}
	std::vector<std::string> cnFuturesIds = { "AU", "AP","CU", "AG", "NI", "AL", "ZN", "PG", "SS", "SA", "EB", "EG", "SP", "V", "FU", "I", "RB", "RU", "BU", "JM", "J", "JD", "A", "C", "CF", "CS", "M", "MA", "ZC", "TA", "SR", "SM", "RM", "Y", "OI", "P", "PP", "HC", "L", "FG","PB"
	};
	std::vector<std::string> forexIds = { "EUR/USD", "GBP/USD", "USD/JPY", "USD/CAD", "USD/CHF", "USD/CNH", "AUD/USD", "NZD/USD",
										  "EURUSD", "GBPUSD", "USDJPY", "USDCAD", "USDCHF", "USDCNH", "AUDUSD", "NZDUSD", "USDOLLAR"
	};
	std::vector<std::string> futuresIds = { "NAS100", "CHN50", "USOIL", "COPPER", "XAU/USD", "XAUUSD", "XAG/USD", "XAGUSD", "SOYF", "WHEATF", "CORNF"
	};
	if (std::find(cnFuturesIds.begin(), cnFuturesIds.end(), upperId) != cnFuturesIds.end()) {
		return InstrumentType::ChineseFutures;
	}
	if (std::find(forexIds.begin(), forexIds.end(), upperId) != forexIds.end()) {
		return InstrumentType::Forex;
	}
	if (std::find(futuresIds.begin(), futuresIds.end(), upperId) != futuresIds.end()) {
		return InstrumentType::Futures;
	}
	if (upperId.length() == 6 && (upperId[0] == '0' || upperId[0] == '3' || upperId[0] == '6')) {
		return InstrumentType::ChineseStock;
	}
	std::cout << "id out of range:" << id << "\n";
	assert(false);
	return InstrumentType::Others;
}

inline std::string InstrumentTypeToString(InstrumentType t) {
	switch (t) {
	case InstrumentType::None:
		return "None";
	case InstrumentType::ChineseFutures:
		return "ChineseFutures";
	case InstrumentType::ChineseStock:
		return "ChineseStock";
	case InstrumentType::Futures:
		return "Futures";
	case InstrumentType::Forex:
		return "Forex";
	case InstrumentType::Others:
		return "Others";
	default:
		return "None";
	}
}

inline bool WillAddOneHour(Time t) {
	if (t.ToTM().tm_mon + 1 >= 4 && t.ToTM().tm_mon + 1 < 11) {
		return false;
	}
	if (t.ToTM().tm_mon + 1 <= 2 || t.ToTM().tm_mon + 1 >= 12) {
		return true;
	}
	int wd = t.ToTM().tm_wday;
	if (wd == 0) {
		wd = 7;
	}
	if (t.ToTM().tm_mon + 1 == 3) {
		if (t.ToTM().tm_mday - wd >= 8) {
			return false;
		}
		else {
			return true;
		}
	}
	if (t.ToTM().tm_mon + 1 == 11) {
		if (t.ToTM().tm_mday - wd >= 1) {
			return true;
		}
		else {
			return false;
		}
	}
	assert(false);
	return false;
}

inline TimeOnly GetMarketCloseTime(Time t) {
	TimeOnly ret;
	ret = TimeOnly(5, 0);
	if (WillAddOneHour(t)) {
		ret.AddOneHour();
	}
	return ret;
}

inline std::vector<Candlestick> csv_to_sticks(const std::vector<std::vector<std::string>>& rows) {
	std::vector<Candlestick> sticks;
	assert(rows.size() >= 2);
	std::vector<std::string> names;
	for (auto& name : rows[0]) {
		names.push_back(name);
	}
	int datetimeI = get_index(names, std::vector<std::string>({ "datetime", "date" }));
	int openI = get_index(names, "open");
	int highI = get_index(names, "high");
	int lowI = get_index(names, "low");
	int closeI = get_index(names, "close");
	int volumeI = get_index(names, "volume");
	for (int i = 1; i < rows.size(); i++) {
		auto& cells = rows[i];
		if (cells.size() >= 6) {
			Time date(cells[datetimeI], "%Y-%m-%d %H:%M");
			float open = (float)std::stod(cells[openI]);
			float high = (float)std::stod(cells[highI]);
			float low = (float)std::stod(cells[lowI]);
			float close = (float)std::stod(cells[closeI]);
			auto volume = std::stod(cells[volumeI]);
			Candlestick stick(date.epoch(), open, high, low, close, volume);
			sticks.push_back(stick);
		}
	}
	return sticks;
}

inline std::vector<Candlestick> csv_to_sticks(std::istream& ss, char delim) {
	std::vector<Candlestick> sticks;
	auto rows = get_cvs_rows(ss, delim);
	return csv_to_sticks(rows);
}

inline std::vector<Candlestick> csv_to_sticks(const std::string& str, char delim) {
	std::stringstream ss;
	ss << str;
	return csv_to_sticks(ss, delim);
}
#ifdef DF_MYSQL_USING
inline std::vector<Candlestick> FetchSticksFromDatabase(mysqlx::Session& sess, const std::string& period, const std::string& insId, Time beginTime, Time endTime) {
	std::vector<Candlestick> sticks;
	std::string dbName = "hdins";
	InstrumentType insType = GetInstrumentTypeById(id_without_slash(insId));
	if (insType == InstrumentType::ChineseStock) {
		dbName = "cnstock";
	}
	auto db = sess.getSchema(dbName);
	auto table = db.getTable(period + "_" + id_without_slash(insId));

	std::vector<std::string> columnNames = { "UNIX_TIMESTAMP(datetime)", "open", "high", "low", "close", "volume" };
	std::string condition = fmt::format("'{0}'<=datetime AND datetime<='{1}'", beginTime.ToString(), endTime.ToString());

	auto res = table.select(columnNames).where(condition).orderBy("datetime").execute();
	std::list<mysqlx::Row> rows = res.fetchAll();

	for (auto& row : rows) {
		Candlestick stick(row[0].get<std::int64_t>(), row[1].get<double>(), row[2].get<double>(), row[3].get<double>(), row[4].get<double>(), row[5].get<double>());
		sticks.push_back(stick);
	}
	return sticks;
}
#endif

inline std::vector<Candlestick> FetchSticksFromCSVContent(std::string csv, char delim) {
	std::vector<Candlestick> sticks;
	std::stringstream ss;
	ss << csv;
	sticks = csv_to_sticks(ss, delim);
	return sticks;
}

std::vector<char> get_data(httplib::Client& cli, std::string url) {
	std::vector<char> ret;
	ret.reserve(1024 * 1024);
	auto res = cli.Get(url.c_str(), [&](const char* data, size_t data_length) {
		for (int i = 0; i < data_length; i++) {
			ret.push_back(data[i]);
		}
	return true;
		});
	if (res != nullptr && res->status != 200) {
		std::string data(ret.begin(), ret.end());
		spdlog::error(std::to_string(res->status) + " " + res->reason + " " + data);
	}
	return ret;
}

class ASticks {
protected:
	std::vector<Candlestick> sticks_;
public:
	const std::vector<Candlestick>& sticks() const { return sticks_; }
	std::vector<Candlestick>& mutable_sticks() { return sticks_; }
public:
	int GetIndex(const Time& time) const {
		Candlestick stick;
		stick.mutate_datetime(time.epoch());
		auto it = std::lower_bound(sticks().begin(), sticks().end(), stick,
			[](const Candlestick& lhs, const Candlestick& rhs) -> bool { return lhs.datetime() < rhs.datetime(); });
		if (it == sticks().end())
			return -1;
		if (it->datetime() == time.epoch())
			return (int)(it - sticks().begin());
		else
			return -1;
	}
	int GetNearestIndex(const Time& time) const {
		Candlestick stick;
		stick.mutate_datetime(time.epoch());
		auto it = std::lower_bound(sticks().begin(), sticks().end(), stick,
			[](const Candlestick& lhs, const Candlestick& rhs) -> bool { return lhs.datetime() < rhs.datetime(); });
		if (it == sticks().end())
			return (int)(sticks().size() - 1);
		return (int)(it - sticks().begin());
	}
	int GetNearestIndex(const std::string& timeStr) const {
		return GetNearestIndex(Time(timeStr));
	}
public:
	void FetchFromTraderScaleSever(const std::string& period, const std::string& insId, Time beginTime, Time endTime) {
		while (true) {
			try {
				httplib::Client cli(Config::instance().data_provider_addr(), 15000);
				std::string url = fmt::format("/sticks/{0}/{1}?a={2}&b={3}", period, insId, beginTime.epoch(), endTime.epoch());
				if (GetInstrumentTypeById(insId) == InstrumentType::ChineseFutures)
					url = "/cnf" + url;
				if (GetInstrumentTypeById(insId) == InstrumentType::Forex || GetInstrumentTypeById(insId) == InstrumentType::Futures)
					url = "/glx" + url;
				url += "&token=" + Config::instance().token;

				std::vector<char> buffer = get_data(cli, url);

				if (!buffer.empty()) {
					auto arcsticks = arc::GetCandlesticks(buffer.data());
					this->sticks_.clear();
					this->sticks_.reserve(arcsticks->sticks()->size());
					for (const auto& s : *arcsticks->sticks()) {
						this->sticks_.push_back(*s);
					}
				}
				else {
					spdlog::warn("FetchFromArcSever empty will retry");
					Sleep(3000);
					continue;
				}
				return;
			}
			catch (const std::exception& e) {
				spdlog::warn(e.what());
				spdlog::warn("FetchFromArcSever retry");
			}
			Sleep(3000);
		}
	}
#ifdef DF_MYSQL_USING
	void FetchFromMySQLDatabase(const std::string& period, const std::string& insId, Time beginTime, Time endTime) {
		mysqlx::Session sess(Config::instance().mysql_ip(), 33060, "root", Config::instance().mysql_root_password());
		sticks_ = FetchSticksFromDatabase(sess, period, insId, beginTime, endTime);
	}
#endif
	void FetchFromSQLiteDatabase(const std::string& fullfilename, Time beginTime, Time endTime) {
		pbsqlite::Database db(fullfilename, SQLite::OPEN_READONLY);
		std::string condition = fmt::format("WHERE datetime BETWEEN {0} AND {1} ORDER BY datetime", beginTime.epoch(), endTime.epoch());
		std::vector<arc::pb::Candlestick> rs = db.Select<arc::pb::Candlestick>(condition);
		sticks_ = ToFBCandlesticks(rs);
	}
	void FetchFromCSVContent(const std::string& csv, char delim) {
		sticks_ = FetchSticksFromCSVContent(csv, delim);
	}
	void ReplaceInto(pbsqlite::Database& db) {
		SQLite::Transaction tran(db);
		for (auto& s : sticks()) {
			db.exec(pbsqlite::ToInsertString(ToPBCandlestick(s), "", "REPLACE INTO"));
		}
		tran.commit();
	}
	static void CreateTableIfNotExists(const std::string& fullfilename) {
		pbsqlite::Database db(fullfilename, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
		db.CreateTableIfNotExists(arc::pb::Candlestick(), "datetime");
	}
	friend class ASticksEx;
};


class ASticksEx : public ASticks {
public:
	void Sort() {
		std::sort(sticks_.begin(), sticks_.end(), [](const Candlestick& a, const Candlestick& b) {
			return a.datetime() < b.datetime();
			});
	}
	void SortByVolume() {
		std::sort(sticks_.begin(), sticks_.end(), [](const Candlestick& a, const Candlestick& b) {
			return a.volume() < b.volume();
			});
	}
	void ReplaceIntoStrict(const std::vector<Candlestick>& data) {
		// data must be sorted and all time after sticks.back
		int index = GetIndex(Time(data.front().datetime()));
		sticks_.erase(sticks_.begin() + index, sticks_.end());
		sticks_.reserve(sticks().size() + data.size());
		sticks_.insert(sticks_.end(), data.begin(), data.end());
	}
	void Pop(int n) {
		for (int i = 0; i < n; i++)
			sticks_.pop_back();
	}
	void ReplaceInsert(const std::vector<Candlestick>& sticks) {
		for (auto& stick : sticks) {
			int index = GetIndex(Time(stick.datetime()));
			if (index != -1) {
				sticks_[index] = stick;
			}
			else {
				sticks_.insert(std::upper_bound(sticks_.begin(), sticks_.end(), stick, [](const Candlestick& lhs, const Candlestick& rhs) -> bool { return lhs.datetime() < rhs.datetime(); }), stick);
			}
		}
	}
	ASticks Sub(int a, int b) const {
		ASticks ret;
		int beginI = std::min((int)sticks().size() - 1, std::max(0, a));
		int endI = std::min((int)sticks().size() - 1, std::max(0, b));
		ret.sticks_ = std::vector<Candlestick>(sticks().begin() + beginI, sticks().begin() + endI + 1);
		return ret;
	}
	ASticks Sub(Time a, Time b) const {
		int ib = GetNearestIndex(b);
		if (ib == -1) {
			ib = sticks().size() - 1;
		}
		return Sub(GetNearestIndex(a), ib);
	}
};

}
