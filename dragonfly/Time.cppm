// License: MIT License   See LICENSE.txt for the full license.
module;
#include <ctime>
#include <iostream>
#include <assert.h>
#include <sstream>
#include <istream>
#include <locale>
#include <iomanip>
#include <chrono>

export module dragonfly:Time;

export namespace dragonfly {

struct Time {
public:
	std::time_t epoch_ = 0;
public:
	constexpr std::time_t epoch() const { return epoch_; }
	void serialize(auto& ar, unsigned) { ar& epoch_; }
	constexpr void set_epoch(const std::time_t& epoch) { this->epoch_ = epoch; }
	Time() {
		auto d = std::chrono::system_clock::now().time_since_epoch();
		epoch_ = std::chrono::duration_cast<std::chrono::seconds>(d).count();
	}
	constexpr Time(const std::time_t& epoch) : epoch_(epoch) {}
	Time(const std::chrono::system_clock::time_point& tp) {
		this->operator=(tp);
	}
	Time(const std::string& dateString) {
		std::tm t = {};
		std::istringstream ss(dateString);
		std::string f1 = "2000-01-01 12:34";
		std::string f2 = "2000-01-01 12:34:01";
		if (f2.size() == dateString.size())
			ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
		else
			ss >> std::get_time(&t, "%Y-%m-%d %H:%M");

		this->epoch_ = mktime(&t);
	}
	Time(const std::string& dateString, const std::string& format) {
		std::tm t = {};
		std::istringstream ss(dateString);
		ss >> std::get_time(&t, format.c_str());
		this->epoch_ = mktime(&t);
	}
	// days since Sunday - [0, 6]
	int weekday() const {
		auto lt = ToTM();
		return lt.tm_wday;
	}
	Time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0) {
		tm time_in = { second, minute, hour, day, month - 1, year - 1900 };
		this->epoch_ = std::mktime(&time_in);
	}
	Time& AddDays(int n) {
		epoch_ += n * 24 * 60 * 60;
		return *this;
	}
	Time& AddHours(int n) {
		epoch_ += n * 60 * 60;
		return *this;
	}
	Time& AddMinutes(int n) {
		epoch_ += n * 60;
		return *this;
	}
	Time& AddSeconds(int n) {
		epoch_ += n;
		return *this;
	}
	std::time_t ToStdTime() const {
		return epoch_;
	}
	tm ToTM() const {
		struct tm tm_info;
		localtime_s(&tm_info, &epoch_);
		return tm_info;
	}
	std::string ToString(const char* format) const {
		auto stdtime = ToStdTime();
		char buffer[64];

		struct tm tm_info;
		localtime_s(&tm_info, &epoch_);

		strftime(buffer, 64, format, &tm_info);
		return std::string(buffer);
	}
	std::string ToString() const {
		char buffer[64];
		struct tm tm_info;
		localtime_s(&tm_info, &epoch_);

		// %Y-%m-%d %H:%M:%S
		strftime(buffer, 64, "%F %T", &tm_info);
		return std::string(buffer);
	}
	int to_yymmdd_int() const {
		struct tm tm_result;
		errno_t err = localtime_s(&tm_result, &epoch_);
		if (err != 0) {
			return -1;
		}
		return (tm_result.tm_year % 100) * 10000 + (tm_result.tm_mon + 1) * 100 + tm_result.tm_mday;
	}
	int to_yymmddhh_int() const {
		struct tm tm_result;
		errno_t err = localtime_s(&tm_result, &epoch_);
		if (err != 0) {
			return -1;
		}
		return (tm_result.tm_year % 100) * 1000000 + (tm_result.tm_mon + 1) * 10000 + tm_result.tm_mday * 100 + tm_result.tm_hour;
	}
	int to_yymm_int() const {
		struct tm tm_result;
		errno_t err = localtime_s(&tm_result, &epoch_);
		if (err != 0) {
			return -1;
		}
		return (tm_result.tm_year % 100) * 100 + (tm_result.tm_mon + 1);
	}
	Time& operator=(const Time& rhs) {
		this->epoch_ = rhs.epoch_;
		return *this;
	}
	Time& operator=(const std::chrono::system_clock::time_point& rhs) {
		this->epoch_ = std::chrono::duration_cast<std::chrono::seconds>(rhs.time_since_epoch()).count();
		return *this;
	}
	operator std::chrono::system_clock::time_point() const {
		return std::chrono::system_clock::from_time_t(time_t{ 0 }) + std::chrono::seconds(this->epoch());
	}

	Time operator+(const std::chrono::hours& rhs) {
		Time ret = *this;
		ret.epoch_ += std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
		return ret;
	}
	Time operator-(const std::chrono::hours& rhs) {
		Time ret = *this;
		ret.epoch_ -= std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
		return ret;
	}
	Time operator+(const std::chrono::days& rhs) {
		Time ret = *this;
		ret.epoch_ += std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
		return ret;
	}
	Time operator-(const std::chrono::days& rhs) {
		Time ret = *this;
		ret.epoch_ -= std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
		return ret;
	}
	Time operator+(const std::chrono::minutes& rhs) const {
		Time ret = *this;
		ret.epoch_ += std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
		return ret;
	}
	Time operator-(const std::chrono::minutes& rhs) const {
		Time ret = *this;
		ret.epoch_ -= std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
		return ret;
	}

	std::chrono::system_clock::duration operator-(const Time& rhs) const {
		return (std::chrono::system_clock::time_point)(*this) - (std::chrono::system_clock::time_point)(rhs);
	}

	inline bool operator==(const Time& rhs) const {
		return this->epoch_ == rhs.epoch_;
	}
	inline bool operator!=(const Time& rhs) const {
		return !this->operator==(rhs);
	}
	inline bool operator<(const Time& rhs) const {
		return epoch_ < rhs.epoch_;
	}
	inline bool operator<=(const Time& rhs) const {
		return epoch_ <= rhs.epoch_;
	}
	inline bool operator>=(const Time& rhs) const {
		return epoch_ >= rhs.epoch_;
	}
	inline bool operator>(const Time& rhs) const {
		return epoch_ > rhs.epoch_;
	}
	friend std::ostream& operator<<(std::ostream& out, const Time& rhs) {
		out << rhs.ToString();
		return out;
	}
};

struct TimeOnlyMin {
	int16_t elapsed_minutes_ = 0;
	constexpr TimeOnlyMin() = default;
	TimeOnlyMin(int hour, int minute) {
		elapsed_minutes_ = hour * 60 + minute;
	}
	explicit TimeOnlyMin(const Time& time) {
		if constexpr (true) {
			auto t = time.ToTM();
			elapsed_minutes_ = t.tm_hour * 60 + t.tm_min;
		}
		else {
			using namespace std::chrono;
			static const time_zone* tz = current_zone();
			sys_seconds tp{ seconds(time.epoch()) };
			zoned_time zt{ tz, tp };
			constexpr int32_t MINUTES_PER_DAY = 24 * 60;
			auto total_minutes = duration_cast<minutes>(zt.get_local_time().time_since_epoch()).count();
			elapsed_minutes_ = static_cast<int16_t>(total_minutes % MINUTES_PER_DAY);
		}

	}
	int16_t elapsed_minutes() const { return elapsed_minutes_; }
	TimeOnlyMin& set_elapsed_minutes(int value) { elapsed_minutes_ = value; return *this; }
	int hour() const { return elapsed_minutes_ / 60; }
	int minute() const { return elapsed_minutes_ % 60; }
	bool operator==(const TimeOnlyMin& rhs) const noexcept {
		return elapsed_minutes_ == rhs.elapsed_minutes_;
	}

	bool operator<(const TimeOnlyMin& rhs) const noexcept {
		return elapsed_minutes_ < rhs.elapsed_minutes_;
	}

	bool operator!=(const TimeOnlyMin& rhs) const noexcept {
		return !(*this == rhs);
	}

	bool operator>(const TimeOnlyMin& rhs) const noexcept {
		return rhs < *this;
	}

	bool operator<=(const TimeOnlyMin& rhs) const noexcept {
		return !(rhs < *this);
	}

	bool operator>=(const TimeOnlyMin& rhs) const noexcept {
		return !(*this < rhs);
	}
	TimeOnlyMin& operator=(const TimeOnlyMin&) = default;
	void operator-=(const std::chrono::minutes& rhs) {
		elapsed_minutes_ -= std::chrono::duration_cast<std::chrono::minutes>(rhs).count();
	}
	void operator+=(const std::chrono::minutes& rhs) {
		elapsed_minutes_ += std::chrono::duration_cast<std::chrono::minutes>(rhs).count();
	}
	TimeOnlyMin operator+(const std::chrono::minutes& rhs) const {
		TimeOnlyMin tmp = *this;
		tmp += rhs;
		return tmp;
	}
	TimeOnlyMin operator-(const std::chrono::minutes& rhs) const {
		TimeOnlyMin tmp = *this;
		tmp -= rhs;
		return tmp;
	}
	static TimeOnlyMin min() { return TimeOnlyMin(0, 0); }
	static TimeOnlyMin max() { return TimeOnlyMin(23, 59); }
	friend std::ostream& operator<<(std::ostream& os, const TimeOnlyMin& rhs) {
		os << rhs.hour() << ":" << rhs.minute();
		return os;
	}
};

struct TimeOnly {
	int elapsed_in_seconds_ = 0;
	int elapsed_in_seconds() const { return elapsed_in_seconds_; }
	int elapsed_in_minutes() const { return elapsed_in_seconds_ / 60; }
	TimeOnly& set_elapsed_in_minutes(int value) {
		elapsed_in_seconds_ = value * 60;
		return *this;
	}
	constexpr TimeOnly() = default;
	TimeOnly(std::time_t time) : TimeOnly(Time(time)) {
	}
	explicit TimeOnly(const Time& time) {
		if constexpr (true) {
			auto t = time.ToTM();
			elapsed_in_seconds_ = t.tm_hour * 60 * 60 + t.tm_min * 60 + t.tm_sec;
		}
		else {
			using namespace std::chrono;
			static const time_zone* tz = current_zone();
			sys_seconds tp{ seconds(time.epoch()) };
			zoned_time zt{ tz, tp };
			constexpr int32_t SECONDS_PER_DAY = 86400;
			elapsed_in_seconds_ = static_cast<int32_t>(duration_cast<seconds>(zt.get_local_time().time_since_epoch()).count() % SECONDS_PER_DAY);
		}
	}
	TimeOnly(int hour, int minute) {
		set_elapsed_in_minutes(hour * 60 + minute);
	}
	TimeOnly(int hour, int minute, int second) {
		elapsed_in_seconds_ = hour * 60 * 60 + minute * 60 + second;
	}
	void initialize(int hour, int minute, int second) {
		elapsed_in_seconds_ = hour * 60 * 60 + minute * 60 + second;
	}
	TimeOnly(const std::string& time_string) {
		std::istringstream ss(time_string);
		std::vector<std::string> parts;
		std::string part;

		while (std::getline(ss, part, ':')) {
			parts.push_back(part);
		}

		std::string hour_str = parts[0];
		std::string minute_str = parts.size() >= 2 ? parts[1] : "00";
		std::string second_str = parts.size() >= 3 ? parts[2] : "00";

		int hour = std::stoi(hour_str);
		int minute = std::stoi(minute_str);
		int second = std::stoi(second_str);
		initialize(hour, minute, second);
	}
	TimeOnly(std::pair<int16_t, int16_t> hm) {
		set_elapsed_in_minutes(hm.first * 60 + hm.second);
	}
	TimeOnly& operator =(const TimeOnly& rhs) {
		this->elapsed_in_seconds_ = rhs.elapsed_in_seconds_;
		return *this;
	}
	int hour() const { return elapsed_in_minutes() / 60; }
	int minute() const { return elapsed_in_minutes() % 60; }
	int second() const { return elapsed_in_seconds_ % 60; }
	void AddOneHour() { AddMinutes(60); }
	TimeOnly& AddSeconds(int n) { elapsed_in_seconds_ += n; return *this; }
	TimeOnly& AddMinutes(int n) { elapsed_in_seconds_ += n * 60; return *this; }
	int to_hhmm_int() const { return hour() * 100 + minute(); }
	inline bool operator==(const TimeOnly& rhs) const {
		return this->elapsed_in_seconds_ == rhs.elapsed_in_seconds_;
	}
	inline bool operator!=(const TimeOnly& rhs) const {
		return this->elapsed_in_seconds_ != rhs.elapsed_in_seconds_;
	}
	inline bool operator>(const TimeOnly& rhs) const {
		return elapsed_in_seconds_ > rhs.elapsed_in_seconds_;
	}
	inline bool operator<(const TimeOnly& rhs) const {
		return elapsed_in_seconds_ < rhs.elapsed_in_seconds_;
	}
	inline bool operator<=(const TimeOnly& rhs) const {
		return elapsed_in_seconds_ <= rhs.elapsed_in_seconds_;
	}
	inline bool operator>=(const TimeOnly& rhs) const {
		return elapsed_in_seconds_ >= rhs.elapsed_in_seconds_;
	}
	void operator-=(const std::chrono::minutes& rhs) {
		elapsed_in_seconds_ -= std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
	}
	void operator+=(const std::chrono::minutes& rhs) {
		elapsed_in_seconds_ += std::chrono::duration_cast<std::chrono::seconds>(rhs).count();
	}
	TimeOnly operator+(const std::chrono::minutes& rhs) const {
		TimeOnly tmp = *this;
		tmp += rhs;
		return tmp;
	}
	TimeOnly operator-(const std::chrono::minutes& rhs) const {
		TimeOnly tmp = *this;
		tmp -= rhs;
		return tmp;
	}
	static TimeOnly min() { return TimeOnly(0, 0); }
	static TimeOnly max() { return TimeOnly(23, 59, 59); }
	friend std::ostream& operator<<(std::ostream& os, const TimeOnly& rhs) {
		os << rhs.hour() << ":" << rhs.minute() << ":" << rhs.second();
		return os;
	}
};

inline TimeOnlyMin ToTimeOnlyMin(TimeOnly t) { return TimeOnlyMin().set_elapsed_minutes(t.elapsed_in_seconds() / 60); }
inline TimeOnly ToTimeOnly(TimeOnlyMin t) { return TimeOnly().set_elapsed_in_minutes(t.elapsed_minutes()); }

std::string time_point_to_yyyy_mm_dd_hh_ss_ms(const std::chrono::system_clock::time_point& tp) {
	auto time = std::chrono::system_clock::to_time_t(tp);
	std::tm tm;
	errno_t err = localtime_s(&tm, &time);
	if (err != 0) {
		return "err localtime_s";
	}

	auto since_epoch = tp.time_since_epoch();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch) % 1000;

	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();

	return oss.str();
}

std::chrono::system_clock::time_point yyyy_mm_dd_hh_ss_ms_to_time_point(const std::string& str) {
	std::tm tm = {};
	std::istringstream iss(str);
	char dot;
	long milliseconds = 0;

	iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
	if (iss.fail()) {
		throw std::runtime_error("Failed to parse time");
	}

	if (iss >> dot >> milliseconds) {
		if (dot != '.') {
			throw std::runtime_error("Invalid milliseconds format");
		}
	}

	std::time_t tt = std::mktime(&tm);
	if (tt == -1) {
		throw std::runtime_error("Invalid time");
	}

	auto tp = std::chrono::system_clock::from_time_t(tt) +
		std::chrono::milliseconds(milliseconds);

	return tp;
}

inline Time min(const Time& t1, const Time& t2) {
	return t1 < t2 ? t1 : t2;
}

inline Time max(const Time& t1, const Time& t2) {
	return t1 > t2 ? t1 : t2;
}

}
