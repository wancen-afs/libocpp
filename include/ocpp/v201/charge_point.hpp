// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <future>

#include <ocpp/common/charging_station_base.hpp>

#include <ocpp/v201/device_model_management.hpp>
#include <ocpp/v201/enums.hpp>
#include <ocpp/v201/evse.hpp>
#include <ocpp/v201/ocpp_types.hpp>
#include <ocpp/v201/types.hpp>
#include <ocpp/v201/utils.hpp>

#include <ocpp/v201/messages/Authorize.hpp>
#include <ocpp/v201/messages/BootNotification.hpp>
#include <ocpp/v201/messages/GetBaseReport.hpp>
#include <ocpp/v201/messages/GetReport.hpp>
#include <ocpp/v201/messages/GetVariables.hpp>
#include <ocpp/v201/messages/Heartbeat.hpp>
#include <ocpp/v201/messages/MeterValues.hpp>
#include <ocpp/v201/messages/NotifyReport.hpp>
#include <ocpp/v201/messages/Reset.hpp>
#include <ocpp/v201/messages/SetVariables.hpp>
#include <ocpp/v201/messages/StatusNotification.hpp>
#include <ocpp/v201/messages/TransactionEvent.hpp>
#include <ocpp/v201/messages/TriggerMessage.hpp>
namespace ocpp {
namespace v201 {

struct Callbacks {
    std::function<bool(const ResetEnum& reset_type)> is_reset_allowed_callback;
    std::function<void(const ResetEnum& reset_type)> reset_callback;
};

/// \brief Class implements OCPP2.0.1 Charging Station
class ChargePoint : ocpp::ChargingStationBase {

private:
    // reference to evses
    std::map<int32_t, std::unique_ptr<Evse>> evses;

    // utility
    std::unique_ptr<MessageQueue<v201::MessageType>> message_queue;
    std::shared_ptr<DeviceModelManager> device_model_manager;

    // timers
    Everest::SteadyTimer heartbeat_timer;
    Everest::SteadyTimer boot_notification_timer;
    Everest::SteadyTimer aligned_meter_values_timer;

    // states
    RegistrationStatusEnum registration_status;
    WebsocketConnectionStatusEnum websocket_connection_status;

    // callback struct
    Callbacks callbacks;

    // general message handling
    template <class T> bool send(Call<T> call);
    template <class T> std::future<EnhancedMessage<v201::MessageType>> send_async(Call<T> call);
    template <class T> bool send(CallResult<T> call_result);
    bool send(CallError call_error);

    // internal helper functions
    void init_websocket();
    void handle_message(const json& json_message, const MessageType& message_type);
    void message_callback(const std::string& message);
    void update_aligned_data_interval();

    /* OCPP message requests */

    // Functional Block B: Provisioning
    void boot_notification_req(const BootReasonEnum& reason);
    void notify_report_req(const int request_id, const int seq_no, const std::vector<ReportData>& report_data);

    // Functional Block C: Authorization
    AuthorizeResponse authorize_req(const IdToken id_token, const boost::optional<CiString<5500>>& certificate,
                                    const boost::optional<std::vector<OCSPRequestData>>& ocsp_request_data);

    // Functional Block G: Availability
    void status_notification_req(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum status);
    void heartbeat_req();

    // Functional Block E: Transactions
    void transaction_event_req(const TransactionEventEnum& event_type, const DateTime& timestamp,
                               const ocpp::v201::Transaction& transaction,
                               const ocpp::v201::TriggerReasonEnum& trigger_reason, const int32_t seq_no,
                               const boost::optional<int32_t>& cable_max_current,
                               const boost::optional<ocpp::v201::EVSE>& evse,
                               const boost::optional<ocpp::v201::IdToken>& id_token,
                               const boost::optional<std::vector<ocpp::v201::MeterValue>>& meter_value,
                               const boost::optional<int32_t>& number_of_phases_used,
                               const boost::optional<bool>& offline, const boost::optional<int32_t>& reservation_id);

    // Functional Block J: MeterValues
    void meter_values_req(const int32_t evse_id, const std::vector<MeterValue>& meter_values);

    /* OCPP message handlers */

    // Provisioning
    void handle_boot_notification_response(CallResult<BootNotificationResponse> call_result);
    void handle_set_variables_req(Call<SetVariablesRequest> call);
    void handle_get_variables_req(Call<GetVariablesRequest> call);
    void handle_get_base_report_req(Call<GetBaseReportRequest> call);
    void handle_get_report_req(Call<GetReportRequest> call);
    void handle_reset_req(Call<ResetRequest> call);

public:
    /// \brief Construct a new ChargePoint object
    /// \param config OCPP json config
    /// \param ocpp_main_path Path where utility files for OCPP are read and written to
    /// \param message_log_path Path to where logfiles are written to
    /// \param callbacks Callbacks that will be registered for ChargePoint
    ChargePoint(const json& config, const std::string& ocpp_main_path, const std::string& message_log_path,
                const std::string& certs_path, const Callbacks& callbacks);

    /// \brief Starts the ChargePoint, initializes and connects to the Websocket endpoint
    void start();

    /// \brief Stops the ChargePoint. Disconnects the websocket connection and stops MessageQueue and all timers
    void stop();

    /// \brief Event handler that should be called when a session has started
    /// \param evse_id
    /// \param connector_id
    void on_session_started(const int32_t evse_id, const int32_t connector_id);

    /// \brief Event handler that should be called when a transaction has started
    /// \param evse_id 
    /// \param connector_id 
    /// \param session_id 
    /// \param timestamp 
    /// \param meter_start 
    /// \param id_token 
    /// \param reservation_id 
    void on_transaction_started(const int32_t evse_id, const int32_t connector_id, const std::string& session_id,
                                const DateTime& timestamp, const MeterValue& meter_start, const IdToken& id_token,
                                const boost::optional<int32_t>& reservation_id);

    /// \brief Event handler that should be called when a transaction has finished
    /// \param evse_id 
    /// \param timestamp 
    /// \param meter_stop 
    /// \param reason 
    /// \param id_token 
    /// \param signed_meter_value 
    void on_transaction_finished(const int32_t evse_id, const DateTime& timestamp, const MeterValue &meter_stop,
                                 const ReasonEnum reason, const boost::optional<std::string>& id_token,
                                 const boost::optional<std::string>& signed_meter_value);

    /// \brief Event handler that should be called when a session has finished
    /// \param evse_id
    /// \param connector_id
    void on_session_finished(const int32_t evse_id, const int32_t connector_id);

    /// \brief Event handler that should be called a new meter value is present
    /// \param evse_id
    /// \param meter_value
    void on_meter_value(const int32_t evse_id, const MeterValue& meter_value);

    /// \brief Validates provided \p id_token \p certificate and \p ocsp_request_data using CSMS, AuthCache or AuthList
    /// \param id_token
    /// \param certificate
    /// \param ocsp_request_data
    /// \return AuthorizeResponse containing the result of the validation
    AuthorizeResponse validate_token(const IdToken id_token, const boost::optional<CiString<5500>>& certificate,
                                     const boost::optional<std::vector<OCSPRequestData>>& ocsp_request_data);
};

} // namespace v201
} // namespace ocpp
