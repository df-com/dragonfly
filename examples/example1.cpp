#include <iostream>
#include <chrono>

import dragonfly;

namespace df = dragonfly;

int main() {
	df::Config::instance().data_provider = df::DataProvider::TraderScale;
	df::Config::instance().data_provider_addrs.push_back("sv01.traderscale.tech");
	df::Config::instance().token = "your token";

	auto now = std::chrono::system_clock::now();
	df::Chart chart("AU", df::Period::D1, now - std::chrono::days(30), now);
	std::cout << chart.id() << "\n";
	for (int i = 1; i < chart.sticks().size(); i++) {
		std::cout << chart.k(i).close_time().ToString() << " ";
		std::cout << chart.macd(12, 26, 9).bar[i] << " " << chart.macd(12, 26, 9).diff[i] << " ";
		std::cout << chart.kdj(9, 3, 3).K[i] << " " << chart.kdj(9, 3, 3).D[i] << " ";
		std::cout << chart.rsi(6).at(i) << " ";
		std::cout << "\n";
	}
	return 0;
}
