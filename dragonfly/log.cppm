// License: MIT License   See LICENSE.txt for the full license.
module;
//#define LOG_USE_SPDLOG
//#define LOG_FMT_COLOR
#include <iostream>
#include <chrono>
#include <syncstream>
#include <format>
#ifdef LOG_USE_SPDLOG
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#endif
#ifdef LOG_FMT_COLOR
#define FMT_UNICODE 0
#include <fmt/color.h>
#endif

export module dragonfly:log;

export namespace dragonfly::log {

#ifdef LOG_FMT_COLOR
void log_print(const std::string& level, const fmt::rgb& color, const std::string content) {
	std::cout << "[";
	fmt::print(fmt::emphasis::bold | fg(color), "{}", level);
	std::cout << "]";
	std::cout << content << std::endl;
}
void warn(const std::string& str) {
	spdlog::warn(str);
	log_print("warn", fmt::color::yellow, str);
}
void info(const std::string& str) {
	spdlog::info(str);
	log_print("info", fmt::rgb(19, 161, 14), str);
}
void error(const std::string& str) {
	spdlog::error(str);
	log_print("error", fmt::color::red, str);
}
void critical(const std::string& str) {
	spdlog::critical(str);
	log_print("critical", fmt::color::red, str);
}
void debug(const std::string& str) {
	spdlog::critical(str);
	log_print("debug", fmt::color::red, str);
}
#endif

#ifdef LOG_USE_SPDLOG
void warn(const std::string& str) {
	spdlog::warn(str);
}
void info(const std::string& str) {
	spdlog::info(str);
}
void error(const std::string& str) {
	spdlog::error(str);
}
void critical(const std::string& str) {
	spdlog::critical(str);
}
void debug(const std::string& str) {
	spdlog::critical(str);
}
#endif

namespace detail {
constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view BOLD = "\033[1m";
constexpr std::string_view YELLOW = "\033[33m";
constexpr std::string_view GREEN = "\033[32m";
constexpr std::string_view RED = "\033[31m";
constexpr std::string_view MAGENTA = "\033[35m";
constexpr std::string_view CYAN = "\033[36m";

inline std::string get_timestamp() {
    const auto now = std::chrono::system_clock::now();
    return std::format("{:%m-%d %H:%M:%S}", std::chrono::floor<std::chrono::milliseconds>(std::chrono::current_zone()->to_local(now)));
}

void log_print(std::string_view level, std::string_view color, std::string_view content) {
    std::osyncstream sync_cout(std::cout);
    sync_cout << "[" << get_timestamp() << "] ["
        << BOLD << color << level << RESET << "] "
        << content << "\n";
}
}

void info(std::string_view str) {
    detail::log_print("info", detail::GREEN, str);
}

void warn(std::string_view str) {
    detail::log_print("warn", detail::YELLOW, str);
}

void error(std::string_view str) {
    detail::log_print("error", detail::RED, str);
}

void critical(std::string_view str) {
    detail::log_print("critical", detail::MAGENTA, str);
}

void debug(std::string_view str) {
    detail::log_print("debug", detail::CYAN, str);
}

}
