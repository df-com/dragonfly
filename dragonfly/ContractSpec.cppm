// License: MIT License   See LICENSE.txt for the full license.
#include <string>
#include <vector>
#include <fmt/format.h>
#include <boost/algorithm/string/predicate.hpp>
#include <httplib.h>
#include <SQLiteCpp/SQLiteCpp.h>

export module dragonfly:ContractSpec;

import :Config;
import :base;

export namespace dragonfly {

class ContractSpec {
public:
	struct Spec {
		std::string id;
		double contract_size = 0.0;
	};
	std::vector<Spec> specs;
	ContractSpec() {
		httplib::Client cli(Config::instance().data_provider_addr(), 15000);
		std::string url = fmt::format("/contractspecs");

		std::string content;
		auto res = cli.Get(url.c_str(), [&](const char* data, size_t data_length) {
			content.assign(data, data_length);
			return true;
			});
		std::stringstream ss;
		ss << content;
		auto rows = get_cvs_rows(ss, '\t');
		for (int i = 1; i < rows.size(); i++) {
			auto& row = rows[i];
			Spec spec;
			spec.id = row[0];
			spec.contract_size = std::stod(row[1]);
			specs.push_back(spec);
		}
	}
	Spec GetSpecById(const std::string& id) {
		for (auto& s : specs) {
			if (boost::iequals(s.id, id) || boost::iequals(id_without_slash(s.id), id)) {
				return s;
			}
			if ((id.rfind("r_", 0) == 0 || id.rfind("R_", 0) == 0) && (s.id == id.substr(2) || id_without_slash(s.id) == id.substr(2))) {
				return s;
			}
		}
		Spec ret;
		ret.id = id;
		ret.contract_size = 100.0;
		return ret;
		assert(false);
	}
	static ContractSpec& instance() {
		static std::mutex smu;
		smu.lock();
		static ContractSpec instance_;
		smu.unlock();
		return instance_;
	}
};

class StockSpec {
public:
	struct Spec {
		std::string id;
		std::string name;
	};
	std::vector<Spec> specs;
	StockSpec() {
		httplib::Client cli(Config::instance().data_provider_addr(), 15000);
		std::string url = fmt::format("/stockspecs");

		std::string content;
		auto res = cli.Get(url.c_str(), [&](const char* data, size_t data_length) {
			content.assign(data, data_length);
			return true;
			});
		std::stringstream ss;
		ss << content;
		auto rows = get_cvs_rows(ss, '\t');
		for (int i = 1; i < rows.size(); i++) {
			auto& row = rows[i];
			Spec spec;
			spec.id = row[0];
			specs.push_back(spec);
		}
	}
	Spec GetSpecById(const std::string& id) {
		for (auto& s : specs) {
			if (boost::iequals(s.id, id) || boost::iequals(id_without_slash(s.id), id)) {
				return s;
			}
			if ((id.rfind("r_", 0) == 0 || id.rfind("R_", 0) == 0) && (s.id == id.substr(2) || id_without_slash(s.id) == id.substr(2))) {
				return s;
			}
		}
		Spec ret;
		ret.id = id;
		return ret;
		assert(false);
	}
	static StockSpec& instance() {
		static std::mutex smu;
		smu.lock();
		static StockSpec instance_;
		smu.unlock();
		return instance_;
	}
};

}
