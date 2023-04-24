// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/database_handler.hpp>

namespace fs = std::filesystem;

namespace ocpp {
namespace v201 {

DatabaseHandler::DatabaseHandler(const fs::path& database_path, const fs::path& sql_init_path) :
    sql_init_path(sql_init_path) {
    if (!fs::exists(database_path)) {
        fs::create_directories(database_path);
    }
    this->database_file_path = database_path / "cp.db";
};

DatabaseHandler::~DatabaseHandler() {
    if (sqlite3_close_v2(this->db) == SQLITE_OK) {
        EVLOG_debug << "Successfully closed database file";
    } else {
        EVLOG_error << "Error closing database file: " << sqlite3_errmsg(this->db);
    }
}

void DatabaseHandler::sql_init() {
    EVLOG_debug << "Running SQL initialization script.";
    std::ifstream t(this->sql_init_path.string());
    std::stringstream init_sql;

    init_sql << t.rdbuf();

    char* err = NULL;

    if (sqlite3_exec(this->db, init_sql.str().c_str(), NULL, NULL, &err) != SQLITE_OK) {
        EVLOG_error << "Could not create tables: " << sqlite3_errmsg(this->db);
        throw std::runtime_error("Database access error");
    }
}

void DatabaseHandler::open_connection() {
    if (sqlite3_open(this->database_file_path.c_str(), &this->db) != SQLITE_OK) {
        EVLOG_error << "Error opening database at " << this->database_file_path.c_str() << ": " << sqlite3_errmsg(db);
        throw std::runtime_error("Could not open database at provided path.");
    }
    EVLOG_debug << "Established connection to Database.";
    this->sql_init();
}

void DatabaseHandler::close_connection() {
    if (sqlite3_close(this->db) == SQLITE_OK) {
        EVLOG_debug << "Successfully closed database file";
    } else {
        EVLOG_error << "Error closing database file: " << sqlite3_errmsg(this->db);
    }
}

void DatabaseHandler::insert_auth_cache_entry(const std::string& id_token_hash, const IdTokenInfo& id_token_info) {
    std::string sql = "INSERT OR REPLACE INTO AUTH_CACHE (ID_TOKEN_HASH, ID_TOKEN_INFO) VALUES "
                      "(@id_token_hash, @id_token_info)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
        EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
        return;
    }

    const auto id_token_info_str = json(id_token_info).dump();
    sqlite3_bind_text(stmt, 1, id_token_hash.c_str(), id_token_hash.length(), NULL);
    sqlite3_bind_text(stmt, 2, id_token_info_str.c_str(), id_token_info_str.length(), NULL);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        EVLOG_error << "Could not insert into table: " << sqlite3_errmsg(db);
        throw std::runtime_error("db access error");
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        EVLOG_error << "Error inserting into table: " << sqlite3_errmsg(this->db);
        throw std::runtime_error("db access error");
    }
}

boost::optional<IdTokenInfo> DatabaseHandler::get_auth_cache_entry(const std::string& id_token_hash) {
    try {
        std::string sql = "SELECT ID_TOKEN_INFO FROM AUTH_CACHE WHERE ID_TOKEN_HASH = @id_token_hash";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
            EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
            return boost::none;
        }

        sqlite3_bind_text(stmt, 1, id_token_hash.c_str(), id_token_hash.length(), NULL);

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            return boost::none;
        }
        IdTokenInfo id_token_info =
            json::parse(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
        return id_token_info;
    } catch (const json::exception& e) {
        EVLOG_warning << "Could not parse data of IdTokenInfo: " << e.what();
        return boost::none;
    } catch (const std::exception& e) {
        EVLOG_error << "Unknown Error while parsing IdTokenInfo: " << e.what();
        return boost::none;
    }
}

} // namespace v201
} // namespace ocpp