#pragma once
/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/Exports.h>
#include <aws/crt/StlAllocator.h>
#include <aws/crt/Types.h>
#include <aws/crt/http/HttpConnection.h>
#include <aws/crt/io/SocketOptions.h>
#include <aws/crt/io/TlsOptions.h>

#include <aws/mqtt/client.h>

#include <atomic>
#include <functional>
#include <memory>

namespace Aws
{
    namespace Crt
    {
        namespace Io
        {
            class ClientBootstrap;
        }

        namespace Http
        {
            class HttpRequest;
        }

        namespace Mqtt
        {
            class MqttClient;
            class MqttConnection;

            /**
             * Invoked Upon Connection loss.
             */
            using OnConnectionInterruptedHandler = std::function<void(MqttConnection &connection, int error)>;

            /**
             * Invoked Upon Connection resumed.
             */
            using OnConnectionResumedHandler =
                std::function<void(MqttConnection &connection, ReturnCode connectCode, bool sessionPresent)>;

            /**
             * Invoked when a connack message is received, or an error occurred.
             */
            using OnConnectionCompletedHandler = std::function<
                void(MqttConnection &connection, int errorCode, ReturnCode returnCode, bool sessionPresent)>;

            /**
             * Invoked when a suback message is received.
             */
            using OnSubAckHandler = std::function<
                void(MqttConnection &connection, uint16_t packetId, const String &topic, QOS qos, int errorCode)>;

            /**
             * Invoked when a suback message for multiple topics is received.
             */
            using OnMultiSubAckHandler = std::function<void(
                MqttConnection &connection,
                uint16_t packetId,
                const Vector<String> &topics,
                QOS qos,
                int errorCode)>;

            /**
             * Invoked when a disconnect message has been sent.
             */
            using OnDisconnectHandler = std::function<void(MqttConnection &connection)>;

            /**
             * Invoked upon receipt of a Publish message on a subscribed topic.
             * @param connection    The connection object
             * @param topic         The information channel to which the payload data was published.
             * @param payload       The payload data.
             * @param dup           DUP flag. If true, this might be re-delivery of an earlier
             *                      attempt to send the message.
             * @param qos           Quality of Service used to deliver the message.
             * @param retain        Retain flag. If true, the message was sent as a result of
             *                      a new subscription being made by the client.
             */
            using OnMessageReceivedHandler = std::function<void(
                MqttConnection &connection,
                const String &topic,
                const ByteBuf &payload,
                bool dup,
                QOS qos,
                bool retain)>;

            /**
             * @deprecated Use OnMessageReceivedHandler
             */
            using OnPublishReceivedHandler =
                std::function<void(MqttConnection &connection, const String &topic, const ByteBuf &payload)>;

            using OnOperationCompleteHandler =
                std::function<void(MqttConnection &connection, uint16_t packetId, int errorCode)>;

            /**
             * Callback for users to invoke upon completion of, presumably asynchronous, OnWebSocketHandshakeIntercept
             * callback's initiated process.
             */
            using OnWebSocketHandshakeInterceptComplete =
                std::function<void(const std::shared_ptr<Http::HttpRequest> &, int errorCode)>;

            /**
             * Invoked during websocket handshake to give users opportunity to transform an http request for purposes
             * such as signing/authorization etc... Returning from this function does not continue the websocket
             * handshake since some work flows may be asynchronous. To accommodate that, onComplete must be invoked upon
             * completion of the signing process.
             */
            using OnWebSocketHandshakeIntercept = std::function<
                void(std::shared_ptr<Http::HttpRequest> req, const OnWebSocketHandshakeInterceptComplete &onComplete)>;

            /**
             * Represents a persistent Mqtt Connection. The memory is owned by MqttClient.
             * To get a new instance of this class, see MqttClient::NewConnection. Unless
             * specified all function arguments need only to live through the duration of the
             * function call.
             */
            class AWS_CRT_CPP_API MqttConnection final
            {
                friend class MqttClient;

              public:
                ~MqttConnection();
                MqttConnection(const MqttConnection &) = delete;
                MqttConnection(MqttConnection &&) = delete;
                MqttConnection &operator=(const MqttConnection &) = delete;
                MqttConnection &operator=(MqttConnection &&) = delete;
                /**
                 * @return true if the instance is in a valid state, false otherwise.
                 */
                operator bool() const noexcept;
                /**
                 * @return the value of the last aws error encountered by operations on this instance.
                 */
                int LastError() const noexcept;

                /**
                 * Sets LastWill for the connection. The memory backing payload must outlive the connection.
                 */
                bool SetWill(const char *topic, QOS qos, bool retain, const ByteBuf &payload) noexcept;

                /**
                 * Sets login credentials for the connection. The must get set before the Connect call
                 * if it is to be used.
                 */
                bool SetLogin(const char *userName, const char *password) noexcept;

                /**
                 * @deprecated Sets websocket proxy options. Replaced by SetHttpProxyOptions.
                 */
                bool SetWebsocketProxyOptions(const Http::HttpClientConnectionProxyOptions &proxyOptions) noexcept;

                /**
                 * Sets http proxy options. In order to use an http proxy with mqtt either
                 *   (1) Websockets are used
                 *   (2) Mqtt-over-tls is used and the ALPN list of the tls context contains a tag that resolves to mqtt
                 */
                bool SetHttpProxyOptions(const Http::HttpClientConnectionProxyOptions &proxyOptions) noexcept;

                /**
                 * Customize time to wait between reconnect attempts.
                 * The time will start at min and multiply by 2 until max is reached.
                 * The time resets back to min after a successful connection.
                 * This function should only be called before Connect().
                 */
                bool SetReconnectTimeout(uint64_t min_seconds, uint64_t max_seconds) noexcept;

                /**
                 * Initiates the connection, OnConnectionCompleted will
                 * be invoked in an event-loop thread.
                 */
                bool Connect(
                    const char *clientId,
                    bool cleanSession,
                    uint16_t keepAliveTimeSecs = 0,
                    uint32_t pingTimeoutMs = 0,
                    uint32_t protocolOperationTimeoutMs = 0) noexcept;

                /**
                 * Initiates disconnect, OnDisconnectHandler will be invoked in an event-loop thread.
                 */
                bool Disconnect() noexcept;

                /**
                 * @return the pointer to the underlying mqtt connection
                 */
                aws_mqtt_client_connection *GetUnderlyingConnection() noexcept;

                /**
                 * Subscribes to topicFilter. OnMessageReceivedHandler will be invoked from an event-loop
                 * thread upon an incoming Publish message. OnSubAckHandler will be invoked
                 * upon receipt of a suback message.
                 */
                uint16_t Subscribe(
                    const char *topicFilter,
                    QOS qos,
                    OnMessageReceivedHandler &&onMessage,
                    OnSubAckHandler &&onSubAck) noexcept;

                /**
                 * @deprecated Use alternate Subscribe()
                 */
                uint16_t Subscribe(
                    const char *topicFilter,
                    QOS qos,
                    OnPublishReceivedHandler &&onPublish,
                    OnSubAckHandler &&onSubAck) noexcept;

                /**
                 * Subscribes to multiple topicFilters. OnMessageReceivedHandler will be invoked from an event-loop
                 * thread upon an incoming Publish message. OnMultiSubAckHandler will be invoked
                 * upon receipt of a suback message.
                 */
                uint16_t Subscribe(
                    const Vector<std::pair<const char *, OnMessageReceivedHandler>> &topicFilters,
                    QOS qos,
                    OnMultiSubAckHandler &&onOpComplete) noexcept;

                /**
                 * @deprecated Use alternate Subscribe()
                 */
                uint16_t Subscribe(
                    const Vector<std::pair<const char *, OnPublishReceivedHandler>> &topicFilters,
                    QOS qos,
                    OnMultiSubAckHandler &&onOpComplete) noexcept;

                /**
                 * Installs a handler for all incoming publish messages, regardless of if Subscribe has been
                 * called on the topic.
                 */
                bool SetOnMessageHandler(OnMessageReceivedHandler &&onMessage) noexcept;

                /**
                 * @deprecated Use alternate SetOnMessageHandler()
                 */
                bool SetOnMessageHandler(OnPublishReceivedHandler &&onPublish) noexcept;

                /**
                 * Unsubscribes from topicFilter. OnOperationCompleteHandler will be invoked upon receipt of
                 * an unsuback message.
                 */
                uint16_t Unsubscribe(const char *topicFilter, OnOperationCompleteHandler &&onOpComplete) noexcept;

                /**
                 * Publishes to topic. The backing memory for payload must stay available until the
                 * OnOperationCompleteHandler has been invoked.
                 */
                uint16_t Publish(
                    const char *topic,
                    QOS qos,
                    bool retain,
                    const ByteBuf &payload,
                    OnOperationCompleteHandler &&onOpComplete) noexcept;

                OnConnectionInterruptedHandler OnConnectionInterrupted;
                OnConnectionResumedHandler OnConnectionResumed;
                OnConnectionCompletedHandler OnConnectionCompleted;
                OnDisconnectHandler OnDisconnect;
                OnWebSocketHandshakeIntercept WebsocketInterceptor;

              private:
                aws_mqtt_client *m_owningClient;
                aws_mqtt_client_connection *m_underlyingConnection;
                String m_hostName;
                uint16_t m_port;
                Crt::Io::TlsContext m_tlsContext;
                Io::TlsConnectionOptions m_tlsOptions;
                Io::SocketOptions m_socketOptions;
                Crt::Optional<Http::HttpClientConnectionProxyOptions> m_proxyOptions;
                void *m_onAnyCbData;
                bool m_useTls;
                bool m_useWebsocket;

                MqttConnection(
                    aws_mqtt_client *client,
                    const char *hostName,
                    uint16_t port,
                    const Io::SocketOptions &socketOptions,
                    const Crt::Io::TlsContext &tlsContext,
                    bool useWebsocket) noexcept;

                MqttConnection(
                    aws_mqtt_client *client,
                    const char *hostName,
                    uint16_t port,
                    const Io::SocketOptions &socketOptions,
                    bool useWebsocket) noexcept;

                static void s_onConnectionInterrupted(aws_mqtt_client_connection *, int errorCode, void *userData);
                static void s_onConnectionCompleted(
                    aws_mqtt_client_connection *,
                    int errorCode,
                    enum aws_mqtt_connect_return_code returnCode,
                    bool sessionPresent,
                    void *userData);
                static void s_onConnectionResumed(
                    aws_mqtt_client_connection *,
                    ReturnCode returnCode,
                    bool sessionPresent,
                    void *userData);

                static void s_onDisconnect(aws_mqtt_client_connection *connection, void *userData);
                static void s_onPublish(
                    aws_mqtt_client_connection *connection,
                    const aws_byte_cursor *topic,
                    const aws_byte_cursor *payload,
                    bool dup,
                    enum aws_mqtt_qos qos,
                    bool retain,
                    void *user_data);

                static void s_onSubAck(
                    aws_mqtt_client_connection *connection,
                    uint16_t packetId,
                    const struct aws_byte_cursor *topic,
                    enum aws_mqtt_qos qos,
                    int error_code,
                    void *userdata);
                static void s_onMultiSubAck(
                    aws_mqtt_client_connection *connection,
                    uint16_t packetId,
                    const struct aws_array_list *topic_subacks,
                    int error_code,
                    void *userdata);
                static void s_onOpComplete(
                    aws_mqtt_client_connection *connection,
                    uint16_t packetId,
                    int errorCode,
                    void *userdata);

                static void s_onWebsocketHandshake(
                    struct aws_http_message *request,
                    void *user_data,
                    aws_mqtt_transform_websocket_handshake_complete_fn *complete_fn,
                    void *complete_ctx);

                static void s_connectionInit(
                    MqttConnection *self,
                    const char *hostName,
                    uint16_t port,
                    const Io::SocketOptions &socketOptions);
            };

            /**
             * An MQTT client. This is a move-only type. Unless otherwise specified,
             * all function arguments need only to live through the duration of the
             * function call.
             */
            class AWS_CRT_CPP_API MqttClient final
            {
              public:
                /**
                 * Initialize an MqttClient using bootstrap and allocator
                 */
                MqttClient(Io::ClientBootstrap &bootstrap, Allocator *allocator = g_allocator) noexcept;

                ~MqttClient();
                MqttClient(const MqttClient &) = delete;
                MqttClient(MqttClient &&) noexcept;
                MqttClient &operator=(const MqttClient &) = delete;
                MqttClient &operator=(MqttClient &&) noexcept;
                /**
                 * @return true if the instance is in a valid state, false otherwise.
                 */
                operator bool() const noexcept;
                /**
                 * @return the value of the last aws error encountered by operations on this instance.
                 */
                int LastError() const noexcept;

                /**
                 * Create a new connection object using TLS from the client. The client must outlive
                 * all of its connection instances.
                 */
                std::shared_ptr<MqttConnection> NewConnection(
                    const char *hostName,
                    uint16_t port,
                    const Io::SocketOptions &socketOptions,
                    const Crt::Io::TlsContext &tlsContext,
                    bool useWebsocket = false) noexcept;
                /**
                 * Create a new connection object over plain text from the client. The client must outlive
                 * all of its connection instances.
                 */
                std::shared_ptr<MqttConnection> NewConnection(
                    const char *hostName,
                    uint16_t port,
                    const Io::SocketOptions &socketOptions,
                    bool useWebsocket = false) noexcept;

              private:
                aws_mqtt_client *m_client;
            };
        } // namespace Mqtt
    }     // namespace Crt
} // namespace Aws
