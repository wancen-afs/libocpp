// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#ifndef DEVICE_MODEL_STORAGE_SQLITE_HPP
#define DEVICE_MODEL_STORAGE_SQLITE_HPP

#include <filesystem>
#include <sqlite3.h>

#include <everest/logging.hpp>
#include <ocpp/common/database/database_connection.hpp>
#include <ocpp/v201/device_model_storage.hpp>

namespace ocpp {
namespace v201 {

class DeviceModelStorageSqlite : public DeviceModelStorage {

private:
    std::unique_ptr<ocpp::common::DatabaseConnectionInterface> db;

    int get_component_id(const Component& component_id);

    int get_variable_id(const Component& component_id, const Variable& variable_id);

public:
    /// \brief Opens SQLite connection at given \p db_path
    /// \param db_path  path to database
    explicit DeviceModelStorageSqlite(const fs::path& db_path);
    ~DeviceModelStorageSqlite() = default;

    std::map<Component, std::map<Variable, VariableMetaData>> get_device_model() final;

    std::optional<VariableAttribute> get_variable_attribute(const Component& component_id, const Variable& variable_id,
                                                            const AttributeEnum& attribute_enum) final;

    std::vector<VariableAttribute> get_variable_attributes(const Component& component_id, const Variable& variable_id,
                                                           const std::optional<AttributeEnum>& attribute_enum) final;

    bool set_variable_attribute_value(const Component& component_id, const Variable& variable_id,
                                      const AttributeEnum& attribute_enum, const std::string& value,
                                      const std::string& source) final;

    std::optional<VariableMonitoringMeta> set_monitoring_data(const SetMonitoringData& data,
                                                              const VariableMonitorType type) final;

    std::vector<VariableMonitoringMeta> get_monitoring_data(const std::vector<MonitoringCriterionEnum>& criteria,
                                                            const Component& component_id,
                                                            const Variable& variable_id) final;

    ClearMonitoringStatusEnum clear_variable_monitor(int monitor_id, bool allow_protected) final;

    int32_t clear_custom_variable_monitors() final;

    void check_integrity() final;
};

} // namespace v201
} // namespace ocpp

#endif // DEVICE_MODEL_STORAGE_SQLITE_HPP
