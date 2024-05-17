// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

// execute: ./libocpp_unit_tests --gtest_filter=ProfileTests.*

#include "ocpp/v16/ocpp_types.hpp"
#include "ocpp/v16/types.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <ocpp/common/sqlite_statement.hpp>
#include <ocpp/v16/connector.hpp>
#include <ocpp/v16/database_handler.hpp>
#include <ocpp/v16/smart_charging.hpp>
#include <optional>
#include <ostream>

using namespace ocpp::v16;
using namespace ocpp;
namespace fs = std::filesystem;

// ----------------------------------------------------------------------------
// helper functions
namespace ocpp::v16 {

template <typename A> bool optional_equal(const std::optional<A>& a, const std::optional<A>& b) {
    bool bRes = true;
    if (bRes && a.has_value() && b.has_value()) {
        bRes = a.value() == b.value();
    }
    return bRes;
}

std::ostream& operator<<(std::ostream& os, const std::vector<ChargingProfile>& profiles) {
    if (profiles.size() > 0) {
        std::uint32_t count = 0;
        for (const auto& i : profiles) {
            os << "[" << count++ << "] " << i;
        }
    } else {
        os << "<no profiles>";
    }
    return os;
}

bool operator==(const ChargingSchedulePeriod& a, const ChargingSchedulePeriod& b) {
    bool bRes = (a.startPeriod == b.startPeriod) && (a.limit == b.limit);
    bRes = bRes && optional_equal(a.numberPhases, b.numberPhases);
    return bRes;
}

bool operator==(const ChargingSchedule& a, const ChargingSchedule& b) {
    bool bRes = a.chargingSchedulePeriod.size() == b.chargingSchedulePeriod.size();
    for (std::uint32_t i = 0; bRes && i < a.chargingSchedulePeriod.size(); i++) {
        bRes = a.chargingSchedulePeriod[i] == b.chargingSchedulePeriod[i];
    }
    bRes = bRes && (a.chargingRateUnit == b.chargingRateUnit) && optional_equal(a.minChargingRate, b.minChargingRate);
    bRes = bRes && optional_equal(a.startSchedule, b.startSchedule) && optional_equal(a.duration, b.duration);
    return bRes;
}

inline bool operator!=(const ChargingSchedule& a, const ChargingSchedule& b) {
    return !(a == b);
}

bool operator==(const ChargingSchedulePeriod& a, const EnhancedChargingSchedulePeriod& b) {
    bool bRes = (a.startPeriod == b.startPeriod) && (a.limit == b.limit);
    bRes = bRes && optional_equal(a.numberPhases, b.numberPhases);
    // b.stackLevel ignored
    return bRes;
}

bool operator==(const ChargingSchedule& a, const EnhancedChargingSchedule& b) {
    bool bRes = a.chargingSchedulePeriod.size() == b.chargingSchedulePeriod.size();
    for (std::uint32_t i = 0; bRes && i < a.chargingSchedulePeriod.size(); i++) {
        bRes = a.chargingSchedulePeriod[i] == b.chargingSchedulePeriod[i];
    }
    bRes = bRes && (a.chargingRateUnit == b.chargingRateUnit) && optional_equal(a.minChargingRate, b.minChargingRate);
    bRes = bRes && optional_equal(a.startSchedule, b.startSchedule) && optional_equal(a.duration, b.duration);
    return bRes;
}

bool operator==(const ChargingProfile& a, const ChargingProfile& b) {
    bool bRes = (a.chargingProfileId == b.chargingProfileId) && (a.stackLevel == b.stackLevel) &&
                (a.chargingProfilePurpose == b.chargingProfilePurpose) &&
                (a.chargingProfileKind == b.chargingProfileKind) && (a.chargingSchedule == b.chargingSchedule);
    bRes = bRes && optional_equal(a.transactionId, b.transactionId) &&
           optional_equal(a.recurrencyKind, b.recurrencyKind) && optional_equal(a.validFrom, b.validFrom) &&
           optional_equal(a.validTo, b.validTo);
    return bRes;
}

} // namespace ocpp::v16

// ----------------------------------------------------------------------------
// Test anonymous namespace
namespace {

// ----------------------------------------------------------------------------
// Test charging profiles

const ocpp::DateTime profileA_start_time("2024-04-01T11:00:00.000Z");
const ocpp::DateTime profileA_end_time("2025-04-01T11:00:00.000Z");
const ChargingProfile profileA{
    301,                                          // chargingProfileId
    5,                                            // stackLevel
    ChargingProfilePurposeType::TxDefaultProfile, // chargingProfilePurpose
    ChargingProfileKindType::Absolute,            // chargingProfileKind
    {
        // ChargingSchedule
        ChargingRateUnit::A, // chargingRateUnit
        {
            {
                // ChargingSchedulePeriod
                0,            // startPeriod
                32.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // ChargingSchedulePeriod
                6000,         // startPeriod
                24.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // ChargingSchedulePeriod
                12000,        // startPeriod
                21.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
        },
        std::nullopt,        // optional - int32_t duration
        profileA_start_time, // optional - ocpp::DateTime - startSchedule
        std::nullopt,        // optional - float - minChargingRate
    },                       // chargingSchedule
    std::nullopt,            // transactionId
    std::nullopt,            // recurrencyKind
    profileA_start_time,     // validFrom
    profileA_end_time,       // validTo
};

ocpp::DateTime profileB_start_time("2024-04-01T12:00:00.000Z");
ocpp::DateTime profileB_end_time("2025-04-01T10:00:00.000Z");
ChargingProfile profileB{
    302,                                          // chargingProfileId
    5,                                            // stackLevel
    ChargingProfilePurposeType::TxDefaultProfile, // chargingProfilePurpose
    ChargingProfileKindType::Absolute,            // chargingProfileKind
    {
        // ChargingSchedule
        ChargingRateUnit::A, // chargingRateUnit
        {
            {
                // ChargingSchedulePeriod
                0,            // startPeriod
                10.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // ChargingSchedulePeriod
                7000,         // startPeriod
                20.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
        },
        std::nullopt,        // optional - int32_t duration
        profileB_start_time, // optional - ocpp::DateTime - startSchedule
        std::nullopt,        // optional - float - minChargingRate
    },                       // chargingSchedule
    std::nullopt,            // transactionId
    std::nullopt,            // recurrencyKind
    profileB_start_time,     // validFrom
    profileB_end_time,       // validTo
};

// ----------------------------------------------------------------------------
// provide access to the SQLite database handle
struct DatabaseHandlerTest : public DatabaseHandler {
    using DatabaseHandler::DatabaseHandler;
    auto count(const char* query) {
        SQLiteStatement stmt(db, query);
        std::size_t result = 0;
        auto res = stmt.step();
        switch (res) {
        case SQLITE_DONE:
            std::cerr << "SQLite: done (no rows)" << std::endl;
            break;
        case SQLITE_ROW:
            result = stmt.column_int(0);
            break;
        default:
            std::cerr << "SQLite: " << res << std::endl;
            break;
        }
        return result;
    }
};

// ----------------------------------------------------------------------------
// Test class
class ProfileTests : public testing::Test {
protected:
    const std::string chargepoint_id = "12345678";
    const fs::path database_path = "/tmp/";
    const fs::path init_script_path = "init.sql";
    const fs::path db_filename = database_path / (chargepoint_id + ".db");

    std::map<int32_t, std::shared_ptr<Connector>> connectors;
    std::shared_ptr<DatabaseHandlerTest> database_handler;

    void add_connectors(unsigned int n) {
        for (unsigned int i = 1; i <= n; i++) {
            connectors[i] = std::make_shared<Connector>(i);
        }
    }

    void SetUp() override {
        database_handler = std::make_shared<DatabaseHandlerTest>(chargepoint_id, database_path, init_script_path);
    }

    void TearDown() override {
        std::filesystem::remove(db_filename);
        connectors.clear();
    }
};

// ----------------------------------------------------------------------------
// Test cases
TEST_F(ProfileTests, init) {
    add_connectors(2);
    database_handler->open_db_connection(connectors.size());
    ASSERT_TRUE(fs::exists(db_filename));
    // map doesn't include connector 0, database does
    ASSERT_EQ(database_handler->count("select count(*) from CONNECTORS;"), connectors.size() + 1);
    SmartChargingHandler handler(connectors, database_handler, true);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
    ChargingProfile profile{
        101,                                                                 // chargingProfileId
        20,                                                                  // stackLevel
        ChargingProfilePurposeType::TxDefaultProfile,                        // chargingProfilePurpose
        ChargingProfileKindType::Relative,                                   // chargingProfileKind
        {ChargingRateUnit::A, {}, std::nullopt, std::nullopt, std::nullopt}, // chargingSchedule
        std::nullopt,                                                        // transactionId
        std::nullopt,                                                        // recurrencyKind
        std::nullopt,                                                        // validFrom
        std::nullopt,                                                        // validTo
    };
    handler.add_tx_default_profile(profile, 1);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 1);
    handler.clear_all_profiles();
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
}

TEST_F(ProfileTests, validate_profileA) {
    // need to have a transaction for calculate_composite_schedule()
    // and calculate_enhanced_composite_schedule()
    int connector_id = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp("2024-04-01T11:00:00.000Z");
    add_connectors(5);
    connectors[connector_id]->transaction =
        std::make_shared<Transaction>(connector_id, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    database_handler->open_db_connection(connectors.size());
    ASSERT_TRUE(fs::exists(db_filename));
    // map doesn't include connector 0, database does
    ASSERT_EQ(database_handler->count("select count(*) from CONNECTORS;"), connectors.size() + 1);
    SmartChargingHandler handler(connectors, database_handler, true);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
    auto tmp_profile = profileA;
    EXPECT_TRUE(
        handler.validate_profile(tmp_profile, 0, true, 100, 10, 10, {ChargingRateUnit::A, ChargingRateUnit::W}));
    // check profile not updated
    EXPECT_EQ(tmp_profile, profileA);
    handler.add_tx_default_profile(tmp_profile, connector_id);
    auto valid_profiles = handler.get_valid_profiles(profileA_start_time, profileA_end_time, connector_id);
    auto schedule = handler.calculate_composite_schedule(valid_profiles, profileA_start_time, profileA_end_time,
                                                         connector_id, std::nullopt);
    EXPECT_EQ(profileA.chargingSchedule, schedule);
    auto enhanced_schedule = handler.calculate_enhanced_composite_schedule(
        valid_profiles, profileA_start_time, profileA_end_time, connector_id, std::nullopt);
    EXPECT_EQ(profileA.chargingSchedule, enhanced_schedule);
}

TEST_F(ProfileTests, validate_profileB) {
    // need to have a transaction for calculate_composite_schedule()
    // and calculate_enhanced_composite_schedule()
    int connector_id = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp("2024-04-01T11:00:00.000Z");
    add_connectors(5);
    connectors[connector_id]->transaction =
        std::make_shared<Transaction>(connector_id, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    database_handler->open_db_connection(connectors.size());
    ASSERT_TRUE(fs::exists(db_filename));
    // map doesn't include connector 0, database does
    ASSERT_EQ(database_handler->count("select count(*) from CONNECTORS;"), connectors.size() + 1);
    SmartChargingHandler handler(connectors, database_handler, true);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
    auto tmp_profile = profileB;
    EXPECT_TRUE(
        handler.validate_profile(tmp_profile, 0, true, 100, 10, 10, {ChargingRateUnit::A, ChargingRateUnit::W}));
    // check profile not updated
    EXPECT_EQ(tmp_profile, profileB);
    handler.add_tx_default_profile(tmp_profile, connector_id);
    auto valid_profiles = handler.get_valid_profiles(profileB_start_time, profileB_end_time, connector_id);
    auto schedule = handler.calculate_composite_schedule(valid_profiles, profileB_start_time, profileB_end_time,
                                                         connector_id, std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, schedule);
    auto enhanced_schedule = handler.calculate_enhanced_composite_schedule(
        valid_profiles, profileB_start_time, profileB_end_time, connector_id, std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, enhanced_schedule);
}

TEST_F(ProfileTests, tx_default_0) {
    add_connectors(5);
    database_handler->open_db_connection(connectors.size());
    ASSERT_TRUE(fs::exists(db_filename));
    // map doesn't include connector 0, database does
    ASSERT_EQ(database_handler->count("select count(*) from CONNECTORS;"), connectors.size() + 1);
    SmartChargingHandler handler(connectors, database_handler, true);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
    ChargingProfile profile{
        201,                                                                 // chargingProfileId
        22,                                                                  // stackLevel
        ChargingProfilePurposeType::TxDefaultProfile,                        // chargingProfilePurpose
        ChargingProfileKindType::Relative,                                   // chargingProfileKind
        {ChargingRateUnit::A, {}, std::nullopt, std::nullopt, std::nullopt}, // chargingSchedule
        std::nullopt,                                                        // transactionId
        std::nullopt,                                                        // recurrencyKind
        std::nullopt,                                                        // validFrom
        std::nullopt,                                                        // validTo
    };
    handler.add_tx_default_profile(profile, 0);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 1);
    handler.clear_all_profiles();
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
}

TEST_F(ProfileTests, single_profile) {
    std::int32_t connector = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp("2024-04-01T11:00:00.000Z");
    add_connectors(1);

    connectors[1]->transaction =
        std::make_shared<Transaction>(connector, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    database_handler->open_db_connection(connectors.size());
    ASSERT_TRUE(fs::exists(db_filename));
    // map doesn't include connector 0, database does
    ASSERT_EQ(database_handler->count("select count(*) from CONNECTORS;"), connectors.size() + 1);
    SmartChargingHandler handler(connectors, database_handler, true);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);

    handler.add_tx_default_profile(profileA, 1);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 1);

    auto valid_profiles = handler.get_valid_profiles(profileA_start_time, profileA_end_time, 1);
    // std::cout << valid_profiles << std::endl;
    ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(profileA.chargingSchedule, valid_profiles[0].chargingSchedule);

    auto schedule =
        handler.calculate_composite_schedule(valid_profiles, profileA_start_time, profileA_end_time, 1, std::nullopt);
    // std::cout << schedule << std::endl;
    EXPECT_EQ(profileA.chargingSchedule, schedule);
}

TEST_F(ProfileTests, replace_profile) {
    std::int32_t connector = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp("2024-04-01T11:00:00.000Z");
    add_connectors(1);

    connectors[1]->transaction =
        std::make_shared<Transaction>(connector, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    database_handler->open_db_connection(connectors.size());
    ASSERT_TRUE(fs::exists(db_filename));
    // map doesn't include connector 0, database does
    ASSERT_EQ(database_handler->count("select count(*) from CONNECTORS;"), connectors.size() + 1);
    SmartChargingHandler handler(connectors, database_handler, true);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);

    handler.add_tx_default_profile(profileA, 1);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 1);
    auto valid_profiles = handler.get_valid_profiles(profileA_start_time, profileA_end_time, 1);
    // std::cout << valid_profiles << std::endl;
    ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(profileA.chargingSchedule, valid_profiles[0].chargingSchedule);
    auto schedule =
        handler.calculate_composite_schedule(valid_profiles, profileA_start_time, profileA_end_time, 1, std::nullopt);
    // std::cout << schedule << std::endl;
    EXPECT_EQ(profileA.chargingSchedule, schedule);

    // expected to replace profileA (same purpose and stack level)
    handler.add_tx_default_profile(profileB, 1);
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 1);

    // use profileA timespan
    valid_profiles = handler.get_valid_profiles(profileA_start_time, profileA_end_time, 1);
    EXPECT_EQ(valid_profiles.size(), 1);
    ASSERT_GE(valid_profiles.size(), 1);
    EXPECT_EQ(profileB.chargingSchedule, valid_profiles[0].chargingSchedule);
    schedule =
        handler.calculate_composite_schedule(valid_profiles, profileA_start_time, profileA_end_time, 1, std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, schedule);
    if (profileB.chargingSchedule != schedule) {
        std::cout << valid_profiles << std::endl;
        std::cout << schedule << std::endl;
    }

    // use profileB timespan
    valid_profiles = handler.get_valid_profiles(profileB_start_time, profileB_end_time, 1);
    EXPECT_EQ(valid_profiles.size(), 1);
    ASSERT_GE(valid_profiles.size(), 1);
    EXPECT_EQ(profileB.chargingSchedule, valid_profiles[0].chargingSchedule);
    schedule =
        handler.calculate_composite_schedule(valid_profiles, profileB_start_time, profileB_end_time, 1, std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, schedule);
    if (profileB.chargingSchedule != schedule) {
        std::cout << valid_profiles << std::endl;
        std::cout << schedule << std::endl;
    }

    // create a new SmartChargingHandler from the stored information
    SmartChargingHandler handler_restarted(connectors, database_handler, true);
    valid_profiles = handler_restarted.get_valid_profiles(profileB_start_time, profileB_end_time, 1);
    // ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(valid_profiles.size(), 1);
    ASSERT_GE(valid_profiles.size(), 1);
    EXPECT_EQ(profileB.chargingSchedule, valid_profiles[0].chargingSchedule);
    schedule = handler_restarted.calculate_composite_schedule(valid_profiles, profileB_start_time, profileB_end_time, 1,
                                                              std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, schedule);
    if (profileB.chargingSchedule != schedule) {
        std::cout << valid_profiles << std::endl;
        std::cout << schedule << std::endl;
    }

    EXPECT_TRUE(handler_restarted.clear_all_profiles_with_filter(std::nullopt, std::nullopt, 5,
                                                                 ChargingProfilePurposeType::TxDefaultProfile, false));
    EXPECT_EQ(database_handler->count("select count(*) from CHARGING_PROFILES;"), 0);
}

} // namespace
