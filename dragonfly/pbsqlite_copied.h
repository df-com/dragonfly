#pragma once

#include <fmt/format.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <google/protobuf/message.h>

namespace pbsqlite {

inline std::vector<std::string> GetColumnNames(const google::protobuf::Message& m) {
	std::vector<std::string> ret;
	using namespace google::protobuf;
	const Descriptor* desc = m.GetDescriptor();
	const Reflection* refl = m.GetReflection();
	int fieldCount = desc->field_count();

	for (int i = 0; i < fieldCount; i++) {
		const FieldDescriptor* field = desc->field(i);
		ret.push_back(field->name());
	}
	return ret;
}

inline std::vector<std::string> GetColumnTypeNames(const google::protobuf::Message& m) {
	std::vector<std::string> ret;
	using namespace google::protobuf;
	const Descriptor* desc = m.GetDescriptor();
	const Reflection* refl = m.GetReflection();
	int fieldCount = desc->field_count();

	for (int i = 0; i < fieldCount; i++) {
		const FieldDescriptor* field = desc->field(i);
		ret.push_back(field->type_name());
	}
	return ret;
}

inline std::string GetSQLiteTypeName(const std::string& messageTypeName) {
	if (messageTypeName == "string")
		return "TEXT";
	if (messageTypeName == "int32")
		return "INTEGER";
	if (messageTypeName == "int64")
		return "INTEGER";
	if (messageTypeName == "uint32")
		return "INTEGER";
	if (messageTypeName == "uint64")
		return "INTEGER";
	if (messageTypeName == "float")
		return "REAL";
	if (messageTypeName == "double")
		return "REAL";
	assert(false);
	return "TEXT";
}

template<typename T>
inline std::string ToCreateTableQuery(const T& m, const std::string& tableName, const std::string& primaryKey) {
	using namespace google::protobuf;
	const Descriptor* desc = m.GetDescriptor();
	const Reflection* refl = m.GetReflection();
	std::string query = fmt::format("CREATE TABLE if not exists {0} (", tableName);
	int fieldCount = desc->field_count();

	for (int i = 0; i < fieldCount; i++) {
		const FieldDescriptor* field = desc->field(i);
		query += fmt::format("{0} {1},", field->name(), GetSQLiteTypeName(field->type_name()));
	}
	query += "PRIMARY KEY (" + primaryKey + "));";
	return query;
}

template<typename T>
inline std::string ToCreateTableQuery(const T& m, const std::string& primaryKey) {
	using namespace google::protobuf;
	const Descriptor* desc = m.GetDescriptor();
	std::string tableName = desc->name();
	return ToCreateTableQuery(m, tableName, primaryKey);
}

inline std::string desc_name(const google::protobuf::Message& m) {
	const google::protobuf::Descriptor* desc = m.GetDescriptor();
	return desc->name();
}

template<typename T>
inline std::string ToInsertString(const T& m, const std::string& tableName, const std::string& insertType) {
	using namespace google::protobuf;
	const Descriptor* desc = m.GetDescriptor();
	const Reflection* refl = m.GetReflection();
	std::string finalTableName = tableName;
	if (finalTableName.empty())
		finalTableName = desc->name();
	std::string query = fmt::format("{0} {1} VALUES(", insertType, finalTableName);
	int fieldCount = desc->field_count();

	if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
		for (int i = 0; i < fieldCount; i++) {
			const FieldDescriptor* field = desc->field(i);
			switch (field->cpp_type()) {
			case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
				query += fmt::format("'{0}',", refl->GetString(m, field));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				query += std::to_string(refl->GetInt32(m, field)) + ",";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				query += std::to_string(refl->GetInt64(m, field)) + ",";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				query += std::to_string(refl->GetFloat(m, field)) + ",";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				query += std::to_string(refl->GetDouble(m, field)) + ",";
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	else {
		for (int i = 0; i < fieldCount; i++) {
			const FieldDescriptor* field = desc->field(i);
			switch (field->cpp_type()) {
			case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
				query += fmt::format("'{0}',", refl->GetString(m.buf, field));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				query += std::to_string(refl->GetInt32(m.buf, field)) + ",";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				query += std::to_string(refl->GetInt64(m.buf, field)) + ",";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				query += std::to_string(refl->GetFloat(m.buf, field)) + ",";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				query += std::to_string(refl->GetDouble(m.buf, field)) + ",";
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	query.pop_back();
	query += ");";
	return query;
}

class Database : public SQLite::Database {
public:
	Database(const std::string& aFilename,
		const int          aFlags = SQLite::OPEN_READONLY,
		const int          aBusyTimeoutMs = 0,
		const std::string& aVfs = "") : SQLite::Database(aFilename, aFlags, aBusyTimeoutMs, aVfs) {
		if ((aFlags & SQLite::OPEN_CREATE) && (aFlags & SQLite::OPEN_READWRITE)) {
			createStoreTable();
		}
	}

	template<typename T>
	std::vector<T> Select(const std::string& condition = "") {
		std::vector<T> ret;
		T m;
		using namespace google::protobuf;
		const Descriptor* desc = m.GetDescriptor();
		const Reflection* refl = m.GetReflection();
		int fieldCount = desc->field_count();
		std::string tableName = desc->name();
		std::string query = fmt::format("SELECT * FROM {0} {1}", tableName, condition);
		SQLite::Statement stmt(*this, query);
		if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
			while (stmt.executeStep()) {
				for (int i = 0; i < fieldCount; i++) {
					const FieldDescriptor* field = desc->field(i);
					switch (field->cpp_type()) {
					case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
						refl->SetString(&m, field, stmt.getColumn(i).getString());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
						refl->SetInt32(&m, field, stmt.getColumn(i).getInt());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
						refl->SetInt64(&m, field, stmt.getColumn(i).getInt64());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
						refl->SetFloat(&m, field, stmt.getColumn(i).getDouble());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
						refl->SetDouble(&m, field, stmt.getColumn(i).getDouble());
						break;
					default:
						assert(false);
						break;
					}
				}
				ret.push_back(m);
			}
		}
		else {
			while (stmt.executeStep()) {
				for (int i = 0; i < fieldCount; i++) {
					const FieldDescriptor* field = desc->field(i);
					switch (field->cpp_type()) {
					case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
						refl->SetString(&m.buf, field, stmt.getColumn(i).getString());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
						refl->SetInt32(&m.buf, field, stmt.getColumn(i).getInt());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
						refl->SetInt64(&m.buf, field, stmt.getColumn(i).getInt64());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
						refl->SetFloat(&m.buf, field, (float)stmt.getColumn(i).getDouble());
						break;
					case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
						refl->SetDouble(&m.buf, field, stmt.getColumn(i).getDouble());
						break;
					default:
						assert(false);
						break;
					}
				}
				ret.push_back(m);
			}
		}
		return ret;
	}
	bool TableExists(const google::protobuf::Message& m) {
		auto desc = m.GetDescriptor();
		return tableExists(desc->name());
	}
	template<typename T>
	void CreateTableIfNotExists(const T& m, const std::string& primaryKey) {
		exec(ToCreateTableQuery(m, primaryKey));
	}
	template<typename T>
	void CreateTableIfNotExists(const T& m, const std::string& tableName, const std::string& primaryKey) {
		exec(ToCreateTableQuery(m, tableName, primaryKey));
	}
	template<typename T>
	void InsertInto(const T& m) {
		exec(ToInsertString(m, "", "INSERT INTO"));
	}
	template<typename T>
	void ReplaceInto(const T& m) {
		exec(ToInsertString(m, "", "REPLACE INTO"));
	}
	int table_count() {
		int value;
		std::string query = fmt::format("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name != 'sqlite_sequence';");
		SQLite::Statement stmt(*this, query);
		while (stmt.executeStep()) {
			value = stmt.getColumn(0).getInt();
		}
		return value;
	}
public:
	void Save(const std::string& id, const std::string& value) {
		SQLite::Statement stmt(*this, fmt::format("REPLACE INTO pbstore VALUES('{0}',?);", id));
		stmt.bind(1, value);
		stmt.exec();
	}
	std::string Get(const std::string& id) {
		std::string value;
		std::string query = fmt::format("SELECT value from pbstore WHERE id = '{0}'", id);
		SQLite::Statement stmt(*this, query);
		while (stmt.executeStep()) {
			value = stmt.getColumn(0).getString();
		}
		return value;
	}
private:
	void createStoreTable() {
		std::string query = fmt::format("CREATE TABLE if not exists {0} (id TEXT,value TEXT,PRIMARY KEY (id));", "pbstore");
		exec(query);
	}
};

class Table {
	Database& db_;
	std::string table_name_;
public:
	const std::string& table_name() const { return table_name_; }
public:
	Table(Database& db, const std::string& tableName) :db_(db) {
		//this->db_ = db;
		this->table_name_ = tableName;
	}
	void InsertInto(const google::protobuf::Message& m) {
		db_.exec(ToInsertString(m, table_name(), "INSERT INTO"));
	}
	void ReplaceInto(const google::protobuf::Message& m) {
		db_.exec(ToInsertString(m, table_name(), "REPLACE INTO"));
	}
	template<typename T>
	std::vector<T> Select(const std::string& condition = "") {
		std::vector<T> ret;
		T m;
		using namespace google::protobuf;
		const Descriptor* desc = m.GetDescriptor();
		const Reflection* refl = m.GetReflection();
		int fieldCount = desc->field_count();
		std::string query = fmt::format("SELECT * FROM {0} {1}", table_name(), condition);
		SQLite::Statement stmt(this->db_, query);
		while (stmt.executeStep()) {
			for (int i = 0; i < fieldCount; i++) {
				const FieldDescriptor* field = desc->field(i);
				switch (field->cpp_type()) {
				case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
					refl->SetString(&m, field, stmt.getColumn(i).getString());
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
					refl->SetInt32(&m, field, stmt.getColumn(i).getInt());
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
					refl->SetInt64(&m, field, stmt.getColumn(i).getInt64());
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
					refl->SetFloat(&m, field, (float)stmt.getColumn(i).getDouble());
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
					refl->SetDouble(&m, field, stmt.getColumn(i).getDouble());
					break;
				default:
					assert(false);
					break;
				}
			}
			ret.push_back(m);
		}
		return ret;
	}
};

}
