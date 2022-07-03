// License: MIT License   See LICENSE.txt for the full license.
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
	std::time_t epoch() const { return epoch_; }
	void serialize(auto& ar, unsigned) { ar& epoch_; }
	void set_epoch(const std::time_t& epoch) {
		this->epoch_ = epoch;
	}
	Time() {
		epoch_ = time(nullptr); // time now
	}
	Time(const std::time_t& epoch) {
		this->epoch_ = epoch;
	}
	Time(const std::chrono::system_clock::time_point& tp) {
		this->operator=(tp);
	}
	Time(const std::string& dateString) {
		std::tm t = {};
		std::istringstream ss(dateString);
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

struct TimeOnly {
	int elapsed_in_minutes;
	TimeOnly() {
	}
	TimeOnly(std::time_t time) : TimeOnly(Time(time)) {
	}
	TimeOnly(Time time) {
		auto t = time.ToTM();
		elapsed_in_minutes = t.tm_hour * 60 + t.tm_min;
	}
	TimeOnly(int hour, int minute) {
		elapsed_in_minutes = hour * 60 + minute;
	}
	TimeOnly(std::pair<int16_t, int16_t> hm) {
		elapsed_in_minutes = hm.first * 60 + hm.second;
	}
	TimeOnly& operator =(const TimeOnly& rhs) {
		this->elapsed_in_minutes = rhs.elapsed_in_minutes;
		return *this;
	}
	int hour() const { return elapsed_in_minutes / 60; }
	int minute() const { return elapsed_in_minutes % 60; }
	void AddOneHour() { elapsed_in_minutes += 60; }
	inline bool operator==(const TimeOnly& rhs) const {
		return this->elapsed_in_minutes == rhs.elapsed_in_minutes;
	}
	inline bool operator!=(const TimeOnly& rhs) const {
		return this->elapsed_in_minutes != rhs.elapsed_in_minutes;
	}
	inline bool operator<(const TimeOnly& rhs) const {
		return elapsed_in_minutes < rhs.elapsed_in_minutes;
	}
	inline bool operator<=(const TimeOnly& rhs) const {
		return elapsed_in_minutes <= rhs.elapsed_in_minutes;
	}
	inline bool operator>=(const TimeOnly& rhs) const {
		return elapsed_in_minutes >= rhs.elapsed_in_minutes;
	}
};

inline Time min(const Time& t1, const Time& t2) {
	return t1 < t2 ? t1 : t2;
}

inline Time max(const Time& t1, const Time& t2) {
	return t1 > t2 ? t1 : t2;
}

}
