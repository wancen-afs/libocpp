// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "profile_tests_common.hpp"
#include "ocpp/common/types.hpp"
#include "ocpp/v16/enums.hpp"

// ----------------------------------------------------------------------------
// helper functions

namespace ocpp::v16 {
using json = nlohmann::json;

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

std::ostream& operator<<(std::ostream& os, const std::vector<ChargingSchedulePeriod>& profiles) {
    if (profiles.size() > 0) {
        std::uint32_t count = 0;
        for (const auto& i : profiles) {
            json j;
            to_json(j, i);
            os << "[" << count++ << "] " << j << std::endl;
        }
    } else {
        os << "<no profiles>";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<EnhancedChargingSchedulePeriod>& profiles) {
    if (profiles.size() > 0) {
        std::uint32_t count = 0;
        for (const auto& i : profiles) {
            json j;
            to_json(j, i);
            os << "[" << count++ << "] " << j << std::endl;
        }
    } else {
        os << "<no profiles>";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const EnhancedChargingSchedule& schedule) {
    json j;
    to_json(j, schedule);
    os << j;
    return os;
}

bool operator==(const ChargingSchedulePeriod& a, const ChargingSchedulePeriod& b) {
    auto diff = std::abs(a.startPeriod - b.startPeriod);
    bool bRes = diff < 10; // allow for a small difference
    bRes = bRes && (a.limit == b.limit);
    bRes = bRes && optional_equal(a.numberPhases, b.numberPhases);
    return bRes;
}

bool operator==(const ChargingSchedule& a, const ChargingSchedule& b) {
    bool bRes = true;
    auto min = std::min(a.chargingSchedulePeriod.size(), b.chargingSchedulePeriod.size());
    EXPECT_GT(min, 0);
    for (std::uint32_t i = 0; bRes && i < min; i++) {
        SCOPED_TRACE(std::string("i=") + std::to_string(i));
        bRes = bRes && a.chargingSchedulePeriod[i] == b.chargingSchedulePeriod[i];
        EXPECT_EQ(a.chargingSchedulePeriod[i], b.chargingSchedulePeriod[i]);
    }
    bRes = bRes && (a.chargingRateUnit == b.chargingRateUnit) && optional_equal(a.minChargingRate, b.minChargingRate);
    EXPECT_EQ(a.chargingRateUnit, b.chargingRateUnit);
    if (a.minChargingRate.has_value() && b.minChargingRate.has_value()) {
        EXPECT_EQ(a.minChargingRate.value(), b.minChargingRate.value());
    }
    bRes = bRes && optional_equal(a.startSchedule, b.startSchedule) && optional_equal(a.duration, b.duration);
    if (a.startSchedule.has_value() && b.startSchedule.has_value()) {
        EXPECT_EQ(a.startSchedule.value(), b.startSchedule.value());
    }
    if (a.duration.has_value() && b.duration.has_value()) {
        EXPECT_EQ(a.duration.value(), b.duration.value());
    }
    return bRes;
}

bool operator==(const ChargingSchedulePeriod& a, const EnhancedChargingSchedulePeriod& b) {
    auto diff = std::abs(a.startPeriod - b.startPeriod);
    bool bRes = diff < 10; // allow for a small difference
    bRes = bRes && (a.limit == b.limit);
    bRes = bRes && optional_equal(a.numberPhases, b.numberPhases);
    // b.stackLevel ignored
    return bRes;
}

bool operator==(const ChargingSchedule& a, const EnhancedChargingSchedule& b) {
    bool bRes = true;
    auto min = std::min(a.chargingSchedulePeriod.size(), b.chargingSchedulePeriod.size());
    EXPECT_GT(min, 0);
    for (std::uint32_t i = 0; bRes && i < min; i++) {
        SCOPED_TRACE(std::string("i=") + std::to_string(i));
        bRes = bRes && a.chargingSchedulePeriod[i] == b.chargingSchedulePeriod[i];
        EXPECT_EQ(a.chargingSchedulePeriod[i], b.chargingSchedulePeriod[i]);
    }
    bRes = bRes && (a.chargingRateUnit == b.chargingRateUnit) && optional_equal(a.minChargingRate, b.minChargingRate);
    EXPECT_EQ(a.chargingRateUnit, b.chargingRateUnit);
    if (a.minChargingRate.has_value() && b.minChargingRate.has_value()) {
        EXPECT_EQ(a.minChargingRate.value(), b.minChargingRate.value());
    }
    bRes = bRes && optional_equal(a.startSchedule, b.startSchedule) && optional_equal(a.duration, b.duration);
    if (a.startSchedule.has_value() && b.startSchedule.has_value()) {
        EXPECT_EQ(a.startSchedule.value(), b.startSchedule.value());
    }
    if (a.duration.has_value() && b.duration.has_value()) {
        EXPECT_EQ(a.duration.value(), b.duration.value());
    }
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

bool nearly_equal(const ocpp::DateTime& a, const ocpp::DateTime& b) {
    const auto difference = std::chrono::duration_cast<std::chrono::seconds>(a.to_time_point() - b.to_time_point());
    // allow +- 1 second to be considered equal
    const bool result = std::abs(difference.count()) <= 1;
    if (!result) {
        std::cerr << "nearly_equal (ocpp::DateTime)\n\tA: " << a << "\n\tB: " << b << std::endl;
    }
    return result;
}

} // namespace ocpp::v16
