// License: MIT License   See LICENSE.txt for the full license.
module;
#include <iostream>
#include <vector>
#include <mutex>

#include <winsock2.h>
#include <ws2tcpip.h>

//#pragma comment(lib, "ws2_32.lib")

export module dragonfly:Config;

export namespace dragonfly {

enum class DataProvider {
	/*GoogleFinace, Yahoo, TencentFinace, THS, TLD, Wind, Localhost, LAN,*/ Sina, Sohu, Hexun, ArcData, TraderScale, Jin10, WenHua, Cffex, JoinQuant, None
};

enum class LocalStoreType {
	Oracle, PostgreSQL, SQLServer, MySQL, MariaDB, SQLite, Reids, MicrosoftAccess, MongDB, None
};

std::string getIPAddressByHostname(const std::string& hostname) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed." << std::endl;
		return "";
	}

	addrinfo hints, * res, * p;
	memset(&hints, 0, sizeof(hints));
	//hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
	hints.ai_family = AF_INET; // Allow IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP socket

	if (getaddrinfo(hostname.c_str(), NULL, &hints, &res) != 0) {
		std::cerr << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return "";
	}

	std::string ipAddress;
	for (p = res; p != NULL; p = p->ai_next) {
		void* addr;
		char ipstr[INET6_ADDRSTRLEN];

		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) { // IPv4
			sockaddr_in* ipv4 = (sockaddr_in*)p->ai_addr;
			addr = &(ipv4->sin_addr);
		}
		else { // IPv6
			sockaddr_in6* ipv6 = (sockaddr_in6*)p->ai_addr;
			addr = &(ipv6->sin6_addr);
		}

		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		ipAddress = ipstr;
		break;
	}

	freeaddrinfo(res);
	WSACleanup();

	if (ipAddress.empty()) {
		std::cerr << "Could not resolve hostname." << std::endl;
	}

	return ipAddress;
}

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
	std::string db_addr;
	std::string local_store_path() const { return "D:\\DFData"; }
	std::string data_provider_addr() const {
		if (!data_provider_addrs.empty()) {
			srand(time(nullptr));
			int rd = rand() % data_provider_addrs.size();
			return data_provider_addrs[rd];
		}
		return "";
	}
	int data_provider_port() const { return 15000; }
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
