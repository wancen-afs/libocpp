// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/common/websocket/websocket.hpp>
#include <ocpp/v201/ocpp_types.hpp>

#include <functional>
#include <future>
#include <optional>

namespace ocpp {
namespace v201 {

class DeviceModel;

using WebsocketConnectionCallback =
    std::function<void(const int configuration_slot, const NetworkConnectionProfile& network_connection_profile)>;
using WebsocketConnectionFailedCallback = std::function<void(ConnectionFailedReason reason)>;
using ConfigureNetworkConnectionProfileCallback = std::function<std::future<ConfigNetworkResult>(
    const int32_t configuration_slot, const NetworkConnectionProfile& network_connection_profile)>;

class ConnectivityManager {
private:
    /// \brief Reference to the device model
    DeviceModel& device_model;
    /// \brief Pointer to the evse security class
    std::shared_ptr<EvseSecurity> evse_security;
    /// \brief Pointer to the logger
    std::shared_ptr<MessageLogging> logging;
    /// \brief Pointer to the websocket
    std::unique_ptr<Websocket> websocket;
    /// \brief The message callback
    std::function<void(const std::string& message)> message_callback;
    /// \brief Callback that is called when the websocket is connected successfully
    std::optional<WebsocketConnectionCallback> websocket_connected_callback;
    /// \brief Callback that is called when the websocket connection is disconnected
    std::optional<WebsocketConnectionCallback> websocket_disconnected_callback;
    /// \brief Callback that is called when the websocket could not connect with a specific reason
    std::optional<WebsocketConnectionFailedCallback> websocket_connection_failed_callback;
    /// \brief Callback that is called to configure a network connection profile when none is configured
    std::optional<ConfigureNetworkConnectionProfileCallback> configure_network_connection_profile_callback;

    Everest::SteadyTimer websocket_timer;
    bool disable_automatic_websocket_reconnects;
    int network_configuration_priority;
    WebsocketConnectionOptions current_connection_options{};

public:
    ConnectivityManager(DeviceModel& device_model, std::shared_ptr<EvseSecurity> evse_security,
                        std::shared_ptr<MessageLogging> logging,
                        const std::function<void(const std::string& message)>& message_callback);

    /// \brief Set the websocket \p authorization_key
    ///
    void set_websocket_authorization_key(const std::string& authorization_key);

    /// \brief Set the websocket \p connection_options
    ///
    void set_websocket_connection_options(const WebsocketConnectionOptions& connection_options);

    /// \brief Set the websocket connection options without triggering a reconnect
    ///
    void set_websocket_connection_options_without_reconnect();

    /// \brief Set the \p callback that is called when the websocket is connected.
    ///
    void set_websocket_connected_callback(WebsocketConnectionCallback callback);

    /// \brief Set the \p callback that is called when the websocket is disconnected.
    ///
    void set_websocket_disconnected_callback(WebsocketConnectionCallback callback);

    /// \brief Set the \p callback that is called when the websocket could not connect with a specific reason
    ///
    void set_websocket_connection_failed_callback(WebsocketConnectionFailedCallback callback);

    /// \brief Set the \p callback that is called to configure a network connection profile when none is configured
    ///
    void set_configure_network_connection_profile_callback(ConfigureNetworkConnectionProfileCallback callback);

    /// \brief Gets the configured NetworkConnectionProfile based on the given \p configuration_slot . The
    /// central system uri ofthe connection options will not contain ws:// or wss:// because this method removes it if
    /// present \param network_configuration_priority \return
    std::optional<NetworkConnectionProfile> get_network_connection_profile(const int32_t configuration_slot);

    /// \brief Check if the websocket is connected
    /// \return True is the websocket is connected, else false
    ///
    bool is_websocket_connected();

    /// \brief Start the connectivity manager
    ///
    void start();

    /// \brief Stop the connectivity manager
    ///
    void stop();

    /// \brief Connect to the websocket
    ///
    void connect();

    /// \brief Disconnect the websocket with a specific \p reason
    ///
    void disconnect_websocket(WebsocketCloseReason code = WebsocketCloseReason::Normal);

    /// \brief send a \p message over the websocket
    /// \returns true if the message was sent successfully
    ///
    bool send_to_websocket(const std::string& message);

    ///
    /// \brief Can be called when a network is disconnected, for example when an ethernet cable is removed.
    ///
    /// This is introduced because the websocket can take several minutes to timeout when a network interface becomes
    /// unavailable, whereas the system can detect this sooner.
    ///
    /// \param configuration_slot   The slot of the network connection profile that is disconnected.
    /// \param ocpp_interface       The interface that is disconnected.
    ///
    /// \note At least one of the two params must be provided, otherwise libocpp will not know which interface is down.
    ///
    void on_network_disconnected(const std::optional<int32_t> configuration_slot,
                                 const std::optional<OCPPInterfaceEnum> ocpp_interface);

private:
    /// \brief Init the websocket
    ///
    void init_websocket();

    /// \brief Get the current websocket connection options
    /// \returns the current websocket connection options
    ///
    WebsocketConnectionOptions get_ws_connection_options(const int32_t configuration_slot);

    /// \brief Moves websocket network_configuration_priority to next profile
    ///
    void next_network_configuration_priority();

    /// \brief Function invoked when the web socket connected with the \p security_profile
    ///
    void on_websocket_connected(const int security_profile);

    /// \brief Function invoked when the web socket disconnected
    ///
    void on_websocket_disconnected();

    /// \brief Reconnect with the give websocket \p reason
    ///
    void reconnect(WebsocketCloseReason reason);
};

} // namespace v201
} // namespace ocpp
