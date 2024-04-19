// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/transaction.hpp>

namespace ocpp {

namespace v201 {

Transaction EnhancedTransaction::get_transaction() {
    Transaction transaction = {this->transactionId,     std::nullopt,        this->chargingState,
                               this->timeSpentCharging, this->stoppedReason, this->remoteStartId};
    return transaction;
}

int32_t EnhancedTransaction::get_seq_no() {
    databse_handler->update_transaction_seq_no(this->transactionId,this->seq_no);
    this->seq_no += 1;
    return this->seq_no - 1;
}

void EnhancedTransaction::update_charging_state(const ChargingStateEnum charging_state) {
    this->chargingState = charging_state;
    databse_handler->update_charging_state(this->transactionId, charging_state);
}

} // namespace v201

} // namespace ocpp
