// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_V201_DATABASE_HANDLER_HPP
#define OCPP_V201_DATABASE_HANDLER_HPP

#include "sqlite3.h"
#include <deque>
#include <fstream>
#include <memory>
#include <ocpp/common/support_older_cpp_versions.hpp>

#include <ocpp/common/database/database_connection.hpp>
#include <ocpp/common/database/database_handler_common.hpp>
#include <ocpp/v201/ocpp_types.hpp>

#include <everest/logging.hpp>

namespace ocpp {
namespace v201 {

class DatabaseHandler : public common::DatabaseHandlerCommon {
private:
    void init_sql() override;

    void inintialize_enum_tables();
    void init_enum_table_inner(const std::string& table_name, const int begin, const int end,
                               std::function<std::string(int)> conversion);
    template <typename T>
    void init_enum_table(const std::string& table_name, T begin, T end, std::function<std::string(T)> conversion);

    // Availability management (internal helpers)
    // Setting evse_id to 0 addresses the whole CS, setting evse_id > 0 and connector_id=0 addresses a whole EVSE
    void insert_availability(int32_t evse_id, int32_t connector_id, OperationalStatusEnum operational_status,
                             bool replace);
    OperationalStatusEnum get_availability(int32_t evse_id, int32_t connector_id);

    // Interrupted transactions
    std::vector<TransactionInterruptedResponse> interrupted_transactions;

    /// \brief Process any interrupted transactions.
    /// \details Retriev values from the database and store them in the local vector.
    void process_interrupted_transactions();

public:
    DatabaseHandler(std::unique_ptr<common::DatabaseConnectionInterface> database,
                    const fs::path& sql_migration_files_path);

    // Authorization cache management

    /// \brief Inserts cache entry
    /// \param id_token_hash
    /// \param id_token_info
    void authorization_cache_insert_entry(const std::string& id_token_hash, const IdTokenInfo& id_token_info);

    /// \brief Updates the last_used field in the entry
    ///
    /// \param id_token_hash
    /// \retval true if entry was updated
    void authorization_cache_update_last_used(const std::string& id_token_hash);

    /// \brief Gets cache entry for given \p id_token_hash if present
    /// \param id_token_hash
    /// \return
    std::optional<IdTokenInfo> authorization_cache_get_entry(const std::string& id_token_hash);

    /// \brief Deletes the cache entry for the given \p id_token_hash
    /// \param id_token_hash
    void authorization_cache_delete_entry(const std::string& id_token_hash);

    /// \brief Removes up to \p nr_to_remove items from the cache starting from the least recently used
    ///
    /// \param nr_to_remove Number of items to remove from the database
    /// \retval True if succeeded
    void authorization_cache_delete_nr_of_oldest_entries(size_t nr_to_remove);

    /// \brief Removes all entries from the cache that have passed their expiry date or auth cache lifetime
    ///
    /// \param auth_cache_lifetime The maximum time tokens can stay in the cache without being used
    /// \retval True if succeeded
    void authorization_cache_delete_expired_entries(std::optional<std::chrono::seconds> auth_cache_lifetime);

    /// \brief Deletes all entries of the AUTH_CACHE table. Returns true if the operation was successful, else false
    void authorization_cache_clear();

    /// \brief Get the binary size of the authorization cache table
    ///
    /// \retval The size of the authorization cache table in bytes
    size_t authorization_cache_get_binary_size();

    // Availability

    /// \brief Persist operational settings for the charging station
    virtual void insert_cs_availability(OperationalStatusEnum operational_status, bool replace);
    /// \brief Retrieve persisted operational settings for the charging station
    virtual OperationalStatusEnum get_cs_availability();

    /// \brief Persist operational settings for an EVSE
    virtual void insert_evse_availability(int32_t evse_id, OperationalStatusEnum operational_status, bool replace);
    /// \brief Retrieve persisted operational settings for an EVSE
    virtual OperationalStatusEnum get_evse_availability(int32_t evse_id);

    /// \brief Persist operational settings for a connector
    virtual void insert_connector_availability(int32_t evse_id, int32_t connector_id,
                                               OperationalStatusEnum operational_status, bool replace);
    /// \brief Retrieve persisted operational settings for a connector
    virtual OperationalStatusEnum get_connector_availability(int32_t evse_id, int32_t connector_id);

    // Local authorization list management

    /// \brief Inserts or updates the given \p version in the AUTH_LIST_VERSION table.
    void insert_or_update_local_authorization_list_version(int32_t version);

    /// \brief Returns the version in the AUTH_LIST_VERSION table.
    int32_t get_local_authorization_list_version();

    /// \brief Inserts or updates a local authorization list entry to the AUTH_LIST table.
    void insert_or_update_local_authorization_list_entry(const IdToken& id_token, const IdTokenInfo& id_token_info);

    /// \brief Inserts or updates a local authorization list entries \p local_authorization_list to the AUTH_LIST table.
    void
    insert_or_update_local_authorization_list(const std::vector<v201::AuthorizationData>& local_authorization_list);

    /// \brief Deletes the authorization list entry with the given \p id_tag
    void delete_local_authorization_list_entry(const IdToken& id_token);

    /// \brief Returns the IdTagInfo of the given \p id_tag if it exists in the AUTH_LIST table, else std::nullopt.
    std::optional<v201::IdTokenInfo> get_local_authorization_list_entry(const IdToken& id_token);

    /// \brief Deletes all entries of the AUTH_LIST table.
    void clear_local_authorization_list();

    /// \brief Get the number of entries currently in the authorization list
    int32_t get_local_authorization_list_number_of_entries();

    // Transaction metervalues

    /// \brief Inserts a \p meter_value to the database linked to transaction with id \p transaction_id
    void transaction_metervalues_insert(const std::string& transaction_id, const MeterValue& meter_value);

    /// \brief Get all metervalues linked to transaction with id \p transaction_id
    std::vector<MeterValue> transaction_metervalues_get_all(const std::string& transaction_id);

    /// \brief Remove all metervalue entries linked to transaction with id \p transaction_id
    void transaction_metervalues_clear(const std::string& transaction_id);

    // transactions

    /// @brief Inserts a transaction with the given parameters to the TRANSACTIONS table
    /// @param seq_no
    /// @param transaction_id
    /// @param event_type
    /// @param id_tag_start
    /// @param evse_id
    /// @param connector_id
    /// @param time_start
    /// @param charging_state
    void insert_transaction(int32_t seq_no, const std::string& transaction_id, const std::string& event_type,
                            const std::string& id_tag_start, int32_t evse_id, int32_t connector_id,
                            const ocpp::DateTime& time_start, std::string charging_state,
                            const std::optional<int32_t> reservation_id);

    /// @brief Clear all the transactions from the TRANSACTIONS table.
    /// @param transaction_id transaction id of the transaction to clear from.
    /// @return true if suceeded
    bool clear_transaction(const std::string& transaction_id);

    /// @brief Get any interrupted transactions that hasn't ended.
    /// @return TransactionInterruptedResponse
    std::vector<TransactionInterruptedResponse> get_ongoing_transactions();

    /// @brief Update the sequence number of the given transaction id in the database to retrieve later.
    /// @param transaction_id 
    /// @param seq_no 
    void update_transaction_seq_no(const std::string& transaction_id, int32_t seq_no);

    /// @brief Update the chargign state of the given transaction id in the database to retrieve later.
    /// @param transaction_id
    /// @param charging_state
    void update_charging_state(const std::string& transaction_id, const ChargingStateEnum charging_state);
};

} // namespace v201
} // namespace ocpp

#endif
