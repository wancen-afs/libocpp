// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <database_testing_utils.hpp>

#include <ocpp/v201/database_handler.hpp>

using namespace ocpp::common;
using namespace ocpp::v201;

class DatabaseHandlerTest : public ::testing::Test {
public:
    static constexpr std::string_view shared_memory_path{"file::memory:?cache=shared"};
    std::unique_ptr<DatabaseHandler> pHandler;
    std::unique_ptr<DatabaseConnectionInterface> pDatabase;

    DatabaseHandlerTest() {
        this->pDatabase = std::make_unique<DatabaseConnection>(this->shared_memory_path);
        EXPECT_TRUE(this->pDatabase->open_connection());

        auto db_conn = std::make_unique<DatabaseConnection>(this->shared_memory_path);
        pHandler = std::make_unique<DatabaseHandler>(std::move(db_conn), MIGRATION_FILES_LOCATION_V201);
        pHandler->open_connection();
    };
};

TEST_F(DatabaseHandlerTest, test_add_auth_cache_entry) {
    IdTokenInfo id_token_info;
    id_token_info.status = AuthorizationStatusEnum::Expired;

    this->pHandler->authorization_cache_insert_entry("token1", id_token_info);

    {
        auto stmt =
            this->pDatabase->new_statement("SELECT ID_TOKEN_INFO FROM AUTH_CACHE WHERE ID_TOKEN_HASH=\"token1\"");
        EXPECT_EQ(stmt->step(), SQLITE_ROW);
        EXPECT_EQ(stmt->column_text(0), json(id_token_info).dump());
        EXPECT_EQ(stmt->step(), SQLITE_DONE);
    }

    id_token_info.status = AuthorizationStatusEnum::Accepted;
    id_token_info.groupIdToken = IdToken{"Test", IdTokenEnum::ISO14443};
    this->pHandler->authorization_cache_insert_entry("token2", id_token_info);

    {
        auto stmt =
            this->pDatabase->new_statement("SELECT ID_TOKEN_INFO FROM AUTH_CACHE WHERE ID_TOKEN_HASH=\"token2\"");
        EXPECT_EQ(stmt->step(), SQLITE_ROW);
        EXPECT_EQ(stmt->column_text(0), json(id_token_info).dump());
        EXPECT_EQ(stmt->step(), SQLITE_DONE);
    }
}

TEST_F(DatabaseHandlerTest, test_removing_expired_tokens) {
    IdTokenInfo id_token_info;
    id_token_info.status = AuthorizationStatusEnum::Accepted;

    this->pHandler->authorization_cache_insert_entry("token1", id_token_info);

    auto result = this->pHandler->authorization_cache_get_entry("token1");
    EXPECT_NE(result, std::nullopt);

    DateTime();

    // result = this->pHandler->authorization_cache_get_entry("token2");
    // EXPECT_NE(result, std::nullopt);
}
