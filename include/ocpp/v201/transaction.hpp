// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_V201_TRANSACTION_HANDLER_HPP
#define OCPP_V201_TRANSACTION_HANDLER_HPP

#include <ocpp/common/aligned_timer.hpp>
#include <ocpp/v201/ocpp_types.hpp>
#include <ocpp/v201/database_handler.hpp>

namespace ocpp {

namespace v201 {

/// \brief Struct that enhances the OCPP Transaction by some meta data and functionality
struct EnhancedTransaction : public Transaction {
    std::optional<IdToken> id_token;
    std::optional<IdToken> group_id_token;
    std::optional<int32_t> reservation_id;
    int32_t connector_id;
    int32_t seq_no = 0;
    std::optional<float> active_energy_import_start_value;
    bool check_max_active_import_energy;
    std::shared_ptr<DatabaseHandler> enhanced_transaction_database_handler;

    ClockAlignedTimer sampled_tx_updated_meter_values_timer;
    ClockAlignedTimer sampled_tx_ended_meter_values_timer;
    ClockAlignedTimer aligned_tx_updated_meter_values_timer;
    ClockAlignedTimer aligned_tx_ended_meter_values_timer;

    /// @brief Get the current sequnce number of the transaction message.
    /// @details This method also increments the sequence number and updates the sequence number of the transaction in the database.
    /// @return int32_t seq number
    int32_t get_seq_no();
    Transaction get_transaction();

    /// @brief Update the charging state of the transaction.
    /// @details ALso update the chargign state in the database
    /// @param charging_state 
    void update_charging_state(const ChargingStateEnum charging_state);
};
} // namespace v201

} // namespace ocpp

#endif // OCPP_V201_TRANSACTION_HANDLER_HPP
