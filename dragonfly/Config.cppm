// License: MIT License   See LICENSE.txt for the full license.
#include <iostream>
#include <mutex>

export module dragonfly:Config;

export namespace dragonfly {

enum class DataProvider {
	/*GoogleFinace, Yahoo, TencentFinace, THS, TLD, Wind, Localhost, LAN,*/ Sina, Hexun, ArcData, TraderScale, Jin10, WenHua, Cffex, JoinQuant, None
};

enum class LocalStoreType {
	Oracle, PostgreSQL, SQLServer, MySQL, MariaDB, SQLite, Reids, MicrosoftAccess, MongDB, None
};

class Config {
public:
	static Config& instance() {
		static std::mutex smu;
		smu.lock();
		static Config instance_;
		smu.unlock();
		return instance_;
	}
public:
	DataProvider data_provider;
	LocalStoreType local_store_type;
	std::vector<std::string> data_provider_addrs;
	std::string token;
	std::string local_store_path() const { return "D:\\DFData"; }
	std::string data_provider_addr() const {
		if (!data_provider_addrs.empty()) {
			srand(time(nullptr));
			int rd = rand() % data_provider_addrs.size();
			return data_provider_addrs[rd];
		}
		return "";
	}
public:
	void Reset() {
		data_provider_addrs.clear();
		local_store_type = LocalStoreType::SQLite;
	}
private:
	Config() {
		Reset();
	};

	Config(Config const&);
	void operator=(Config const&);
};

}
