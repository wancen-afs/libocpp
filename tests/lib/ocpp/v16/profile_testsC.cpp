// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

// execute: ./libocpp_unit_tests --gtest_filter=ProfileTestsC.*

#include <chrono>
#include <cstdint>
#include <optional>

#include <gtest/gtest.h>

#include "database_stub.hpp"
#include "ocpp/common/types.hpp"
#include "ocpp/v16/enums.hpp"
#include "profile_tests_common.hpp"

// ----------------------------------------------------------------------------
// Test anonymous namespace
namespace {
using namespace ocpp;
using namespace ocpp::v16;
using namespace std::chrono;

constexpr std::int32_t default_stack_level{0};
constexpr float default_limit{0.0};

// ----------------------------------------------------------------------------
// Test class

class ProfileTestsC : public stubs::DbTestBase {
protected:
    void configure_transaction(const char* transaction_start_time) {
        ocpp::DateTime transaction_start(transaction_start_time);
        configure_transaction(transaction_start);
    }

    void configure_transaction(const DateTime& transaction_start) {
        add_connectors(2);
        std::int32_t meter_start = 0;
        std::int32_t connector_id = 1;
        connectors[connector_id]->transaction = std::make_shared<Transaction>(
            -1, connector_id, "1234", "4567", meter_start, std::nullopt, transaction_start, nullptr);
    }

    void configure() {
        add_connectors(2);
    }
};

// const auto now = date::utc_clock::now();

// 2024-01-01 is a Monday, daily starting at 08:00 for 10 hours
const DateTime profileRecurring_validFrom("2024-01-01T12:00:00Z");
const DateTime profileRecurring_startSchedule("2024-01-01T08:00:00Z");
const DateTime profileRecurring_validTo("2024-01-05T12:00:00Z");
const ChargingProfile profileRecurring{
    301,                                          // chargingProfileId
    5,                                            // stackLevel
    ChargingProfilePurposeType::TxDefaultProfile, // chargingProfilePurpose
    ChargingProfileKindType::Recurring,           // chargingProfileKind
    {
        // EnhancedChargingSchedule
        ChargingRateUnit::A, // chargingRateUnit
        {
            {
                // EnhancedChargingSchedulePeriod
                0,            // startPeriod
                32.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // EnhancedChargingSchedulePeriod
                36000,        // startPeriod
                6.0,          // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
        },
        std::nullopt,                   // optional - int32_t duration
        profileRecurring_startSchedule, // optional - ocpp::DateTime - startSchedule
        std::nullopt,                   // optional - float - minChargingRate
    },                                  // chargingSchedule
    std::nullopt,                       // transactionId
    RecurrencyKindType::Daily,          // recurrencyKind
    profileRecurring_validFrom,         // validFrom
    profileRecurring_validTo,           // validTo
};

// ----------------------------------------------------------------------------
// Test cases

TEST_F(ProfileTestsC, DailyRecurringNotValidYet) {
    const std::int32_t connector_id{1};
    configure_transaction("2024-01-01T07:00:00Z");
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    DateTime start_time("2024-01-01T07:55:00Z");
    DateTime end_time("2024-01-01T08:05:00Z");

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // enhanced_schedule: {"chargingRateUnit":"A","chargingSchedulePeriod":[],"duration":600}
    // test expects chargingSchedulePeriod to have defaults - could be a test error?

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 1);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, default_stack_level);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit, default_limit);
}

TEST_F(ProfileTestsC, DailyRecurringAlmostValid) {
    const std::int32_t connector_id{1};
    DateTime session_start("2024-01-01T07:00:00Z");
    DateTime start_time("2024-01-01T11:55:00Z");
    DateTime end_time("2024-01-01T12:05:00Z");
    configure_transaction(session_start);
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    // std::cout << "valid_profiles: " << valid_profiles << std::endl;
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // enhanced_schedule:
    // {"chargingRateUnit":"A","chargingSchedulePeriod":[{"limit":32.0,"numberPhases":3,"stackLevel":5,"startPeriod":0}],"duration":600}
    // at 11:55 the profile isn't valid which isn't reflected in the schedule
    // startSchedule is missing (not set)

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 2);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, default_stack_level);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit, default_limit);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].startPeriod, 300);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[0].limit);
}

TEST_F(ProfileTestsC, DailyRecurringDuring) {
    const std::int32_t connector_id{1};
    DateTime session_start("2024-01-01T07:00:00Z");
    DateTime start_time("2024-01-01T15:00:00Z");
    DateTime end_time("2024-01-01T15:10:00Z");
    configure_transaction(session_start);
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    // std::cout << "valid_profiles: " << valid_profiles << std::endl;
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    // std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // startSchedule is missing (not set)

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 1);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[0].limit);
}

TEST_F(ProfileTestsC, DailyRecurringApproachingEnd) {
    const std::int32_t connector_id{1};
    DateTime session_start("2024-01-01T07:00:00Z");
    DateTime start_time("2024-01-01T17:55:00Z");
    DateTime end_time("2024-01-01T18:05:00Z");
    configure_transaction(session_start);
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    // std::cout << "valid_profiles: " << valid_profiles << std::endl;
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // enhanced_schedule:
    // {"chargingRateUnit":"A","chargingSchedulePeriod":[{"limit":32.0,"numberPhases":3,"stackLevel":5,"startPeriod":0}],"duration":300}
    // startSchedule is missing (not set)
    // missing the rate change at 18:00 to 6A

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 2);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[0].limit);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].startPeriod, 300);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[1].limit);
}

TEST_F(ProfileTestsC, DailyRecurringNextDay) {
    const std::int32_t connector_id{1};
    DateTime session_start("2024-01-01T07:00:00Z");
    DateTime start_time("2024-01-01T07:30:00Z");
    DateTime end_time("2024-01-01T07:40:00Z");
    configure_transaction(session_start);
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    // std::cout << "valid_profiles: " << valid_profiles << std::endl;
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // enhanced_schedule: {"chargingRateUnit":"A","chargingSchedulePeriod":[],"duration":600}
    // test expects chargingSchedulePeriod to have defaults - could be a test error?

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 1);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, default_stack_level);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit, default_limit);
}

TEST_F(ProfileTestsC, DailyRecurringNextDayStart) {
    const std::int32_t connector_id{1};
    DateTime session_start("2024-01-05T07:00:00Z");
    DateTime start_time("2024-01-02T07:55:00Z");
    DateTime end_time("2024-01-02T08:05:00Z");
    configure_transaction(session_start);
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // enhanced_schedule:
    // {"chargingRateUnit":"A","chargingSchedulePeriod":[{"limit":6.0,"numberPhases":3,"stackLevel":5,"startPeriod":0}],"duration":600}
    // missing rate change at 08:00

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 2);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[1].limit);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].startPeriod, 300);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[0].limit);
}

TEST_F(ProfileTestsC, DailyRecurringApproachingInvalid) {
    const std::int32_t connector_id{1};
    DateTime session_start("2024-01-05T07:00:00Z");
    DateTime start_time("2024-01-05T11:55:00Z");
    DateTime end_time("2024-01-05T12:05:00Z");
    configure_transaction(session_start);
    SmartChargingHandler handler(connectors, database_handler, true);
    handler.add_tx_default_profile(profileRecurring, connector_id);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, connector_id);
    // std::cout << "valid_profiles: " << valid_profiles << std::endl;
    auto enhanced_schedule =
        handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, end_time, connector_id, std::nullopt);
    std::cout << "enhanced_schedule: " << enhanced_schedule << std::endl;

    // enhanced_schedule:
    // {"chargingRateUnit":"A","chargingSchedulePeriod":[{"limit":32.0,"numberPhases":3,"stackLevel":5,"startPeriod":0}],"duration":600}
    // missing end valid profile at 12:00

    EXPECT_EQ(enhanced_schedule.chargingRateUnit, ChargingRateUnit::A);
    EXPECT_EQ(enhanced_schedule.duration, 600);
    EXPECT_EQ(enhanced_schedule.startSchedule, start_time);
    ASSERT_EQ(enhanced_schedule.chargingSchedulePeriod.size(), 2);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].startPeriod, 0);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].stackLevel, profileRecurring.stackLevel);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[0].limit,
              profileRecurring.chargingSchedule.chargingSchedulePeriod[0].limit);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].startPeriod, 300);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].stackLevel, default_stack_level);
    EXPECT_EQ(enhanced_schedule.chargingSchedulePeriod[1].limit, default_limit);
}

} // namespace
