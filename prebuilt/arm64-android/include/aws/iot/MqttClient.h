#pragma once
/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/Config.h>
#include <aws/crt/Exports.h>
#include <aws/crt/auth/Sigv4Signing.h>
#include <aws/crt/mqtt/MqttClient.h>

#if !BYO_CRYPTO

namespace Aws
{
    namespace Iot
    {
        class MqttClient;

        /**
         * Represents a unique configuration for connecting to a single endpoint. You can use a single instance of this
         * class PER endpoint you want to connect to. This object must live through the lifetime of your connection.
         */
        class AWS_CRT_CPP_API MqttClientConnectionConfig final
        {
          public:
            static MqttClientConnectionConfig CreateInvalid(int lastError) noexcept;

            /**
             * Creates a client configuration for use with making new AWS Iot specific MQTT Connections with MTLS.
             */
            MqttClientConnectionConfig(
                const Crt::String &endpoint,
                uint16_t port,
                const Crt::Io::SocketOptions &socketOptions,
                Crt::Io::TlsContext &&tlsContext);

            /**
             * Creates a client configuration for use with making new AWS Iot specific MQTT Connections with web
             * sockets. interceptor: a callback invoked during web socket handshake giving you the opportunity to mutate
             * the request for authorization/signing purposes. If not specified, it's assumed you don't need to sign the
             * request. proxyOptions: optional, if you want to use a proxy with websockets, specify the configuration
             * options here.
             *
             * If proxy options are used, the tlsContext is applied to the connection to the remote endpoint, NOT the
             * proxy. To make a tls connection to the proxy itself, you'll want to specify tls options in proxyOptions.
             */
            MqttClientConnectionConfig(
                const Crt::String &endpoint,
                uint16_t port,
                const Crt::Io::SocketOptions &socketOptions,
                Crt::Io::TlsContext &&tlsContext,
                Crt::Mqtt::OnWebSocketHandshakeIntercept &&interceptor,
                const Crt::Optional<Crt::Http::HttpClientConnectionProxyOptions> &proxyOptions);

            /**
             * @return true if the instance is in a valid state, false otherwise.
             */
            explicit operator bool() const noexcept { return m_context ? true : false; }
            /**
             * @return the value of the last aws error encountered by operations on this instance.
             */
            int LastError() const noexcept { return m_lastError; }

          private:
            MqttClientConnectionConfig(int lastError) noexcept;

            MqttClientConnectionConfig(
                const Crt::String &endpoint,
                uint16_t port,
                const Crt::Io::SocketOptions &socketOptions,
                Crt::Io::TlsContext &&tlsContext,
                const Crt::Optional<Crt::Http::HttpClientConnectionProxyOptions> &proxyOptions);

            Crt::String m_endpoint;
            uint16_t m_port;
            Crt::Io::TlsContext m_context;
            Crt::Io::SocketOptions m_socketOptions;
            Crt::Mqtt::OnWebSocketHandshakeIntercept m_webSocketInterceptor;
            Crt::String m_username;
            Crt::String m_password;
            Crt::Optional<Crt::Http::HttpClientConnectionProxyOptions> m_proxyOptions;
            int m_lastError;

            friend class MqttClient;
            friend class MqttClientConnectionConfigBuilder;
        };

        using CreateSigningConfig = std::function<std::shared_ptr<Crt::Auth::ISigningConfig>(void)>;

        struct AWS_CRT_CPP_API WebsocketConfig
        {
            /**
             * Create a websocket configuration for use with the default credentials provider chain. Signing region
             * will be used for Sigv4 signature calculations.
             */
            WebsocketConfig(
                const Crt::String &signingRegion,
                Crt::Io::ClientBootstrap *bootstrap,
                Crt::Allocator *allocator = Crt::g_allocator) noexcept;

            /**
             * Create a websocket configuration for use with a custom credentials provider. Signing region will be use
             * for Sigv4 signature calculations.
             */
            WebsocketConfig(
                const Crt::String &signingRegion,
                const std::shared_ptr<Crt::Auth::ICredentialsProvider> &credentialsProvider,
                Crt::Allocator *allocator = Crt::g_allocator) noexcept;

            /**
             * Create a websocket configuration for use with a custom credentials provider, and a custom signer.
             *
             * You'll need to provide a function for use with creating a signing Config and pass it to
             * createSigningConfig.
             *
             * This is useful for cases use with:
             * https://docs.aws.amazon.com/iot/latest/developerguide/custom-auth.html
             */
            WebsocketConfig(
                const std::shared_ptr<Crt::Auth::ICredentialsProvider> &credentialsProvider,
                const std::shared_ptr<Crt::Auth::IHttpRequestSigner> &signer,
                CreateSigningConfig createSigningConfig) noexcept;

            std::shared_ptr<Crt::Auth::ICredentialsProvider> CredentialsProvider;
            std::shared_ptr<Crt::Auth::IHttpRequestSigner> Signer;
            CreateSigningConfig CreateSigningConfigCb;

            /**
             * @deprecated Specify ProxyOptions to use a proxy with your websocket connection.
             *
             * If MqttClientConnectionConfigBuilder::m_proxyOptions is valid, then that will be used over
             * this value.
             */
            Crt::Optional<Crt::Http::HttpClientConnectionProxyOptions> ProxyOptions;
            Crt::String SigningRegion;
            Crt::String ServiceName;
        };

        /**
         * Represents configuration parameters for building a MqttClientConnectionConfig object. You can use a single
         * instance of this class PER MqttClientConnectionConfig you want to generate. If you want to generate a config
         * for a different endpoint or port etc... you need a new instance of this class.
         */
        class AWS_CRT_CPP_API MqttClientConnectionConfigBuilder final
        {
          public:
            MqttClientConnectionConfigBuilder();

            /**
             * Sets the builder up for MTLS using certPath and pkeyPath. These are files on disk and must be in the PEM
             * format.
             */
            MqttClientConnectionConfigBuilder(
                const char *certPath,
                const char *pkeyPath,
                Crt::Allocator *allocator = Crt::g_allocator) noexcept;

            /**
             * Sets the builder up for MTLS using cert and pkey. These are in-memory buffers and must be in the PEM
             * format.
             */
            MqttClientConnectionConfigBuilder(
                const Crt::ByteCursor &cert,
                const Crt::ByteCursor &pkey,
                Crt::Allocator *allocator = Crt::g_allocator) noexcept;

            /**
             * Sets the builder up for Websocket connection.
             */
            MqttClientConnectionConfigBuilder(
                const WebsocketConfig &config,
                Crt::Allocator *allocator = Crt::g_allocator) noexcept;

            /**
             * Sets endpoint to connect to.
             */
            MqttClientConnectionConfigBuilder &WithEndpoint(const Crt::String &endpoint);

            /**
             * Sets endpoint to connect to.
             */
            MqttClientConnectionConfigBuilder &WithEndpoint(Crt::String &&endpoint);

            /**
             * Overrides the default port. By default, if ALPN is supported, 443 will be used. Otherwise 8883 will be
             * used. If you specify 443 and ALPN is not supported, we will still attempt to connect over 443 without
             * ALPN.
             */
            MqttClientConnectionConfigBuilder &WithPortOverride(uint16_t port) noexcept;

            /**
             * Sets the certificate authority for the endpoint you're connecting to. This is a path to a file on disk
             * and must be in PEM format.
             */
            MqttClientConnectionConfigBuilder &WithCertificateAuthority(const char *caPath) noexcept;

            /**
             * Sets the certificate authority for the endpoint you're connecting to. This is an in-memory buffer and
             * must be in PEM format.
             */
            MqttClientConnectionConfigBuilder &WithCertificateAuthority(const Crt::ByteCursor &cert) noexcept;

            /** TCP option: Enables TCP keep alive. Defaults to off. */
            MqttClientConnectionConfigBuilder &WithTcpKeepAlive() noexcept;

            /** TCP option: Sets the connect timeout. Defaults to 3 seconds. */
            MqttClientConnectionConfigBuilder &WithTcpConnectTimeout(uint32_t connectTimeoutMs) noexcept;

            /** TCP option: Sets time before keep alive probes are sent. Defaults to kernel defaults */
            MqttClientConnectionConfigBuilder &WithTcpKeepAliveTimeout(uint16_t keepAliveTimeoutSecs) noexcept;

            /**
             * TCP option: Sets the frequency of sending keep alive probes in seconds once the keep alive timeout
             * expires. Defaults to kernel defaults.
             */
            MqttClientConnectionConfigBuilder &WithTcpKeepAliveInterval(uint16_t keepAliveIntervalSecs) noexcept;

            /**
             * TCP option: Sets the amount of keep alive probes allowed to fail before the connection is terminated.
             * Defaults to kernel defaults.
             */
            MqttClientConnectionConfigBuilder &WithTcpKeepAliveMaxProbes(uint16_t maxProbes) noexcept;

            MqttClientConnectionConfigBuilder &WithMinimumTlsVersion(aws_tls_versions minimumTlsVersion) noexcept;

            /**
             * Sets http proxy options. In order to use an http proxy with mqtt either
             *   (1) Websockets are used
             *   (2) Mqtt-over-tls is used and the ALPN list of the tls context contains a tag that resolves to mqtt
             */
            MqttClientConnectionConfigBuilder &WithHttpProxyOptions(
                const Crt::Http::HttpClientConnectionProxyOptions &proxyOptions) noexcept;

            /**
             * Whether to send the SDK name and version number in the MQTT CONNECT packet.
             * Default is True.
             */
            MqttClientConnectionConfigBuilder &WithMetricsCollection(bool enabled);

            /**
             * Overrides the default SDK Name to send as a metric in the MQTT CONNECT packet.
             */
            MqttClientConnectionConfigBuilder &WithSdkName(const Crt::String &sdkName);

            /**
             * Overrides the default SDK Version to send as a metric in the MQTT CONNECT packet.
             */
            MqttClientConnectionConfigBuilder &WithSdkVersion(const Crt::String &sdkVersion);

            /**
             * Builds a client configuration object from the set options.
             */
            MqttClientConnectionConfig Build() noexcept;
            /**
             * @return true if the instance is in a valid state, false otherwise.
             */
            explicit operator bool() const noexcept { return m_lastError == 0; }
            /**
             * @return the value of the last aws error encountered by operations on this instance.
             */
            int LastError() const noexcept { return m_lastError ? m_lastError : AWS_ERROR_UNKNOWN; }

          private:
            Crt::Allocator *m_allocator;
            Crt::String m_endpoint;
            uint16_t m_portOverride;
            Crt::Io::SocketOptions m_socketOptions;
            Crt::Io::TlsContextOptions m_contextOptions;
            Crt::Optional<WebsocketConfig> m_websocketConfig;
            Crt::Optional<Crt::Http::HttpClientConnectionProxyOptions> m_proxyOptions;
            bool m_enableMetricsCollection = true;
            Crt::String m_sdkName = "CPPv2";
            Crt::String m_sdkVersion = AWS_CRT_CPP_VERSION;

            int m_lastError;
        };

        /**
         * AWS IOT specific Mqtt Client. Sets defaults for using the AWS IOT service. You'll need an instance of
         * MqttClientConnectionConfig to use. Once NewConnection returns, you use it's return value identically
         * to how you would use Aws::Crt::Mqtt::MqttConnection
         */
        class AWS_CRT_CPP_API MqttClient final
        {
          public:
            MqttClient(Crt::Io::ClientBootstrap &bootstrap, Crt::Allocator *allocator = Crt::g_allocator) noexcept;

            std::shared_ptr<Crt::Mqtt::MqttConnection> NewConnection(const MqttClientConnectionConfig &config) noexcept;
            /**
             * @return the value of the last aws error encountered by operations on this instance.
             */
            int LastError() const noexcept { return m_client.LastError(); }
            /**
             * @return true if the instance is in a valid state, false otherwise.
             */
            explicit operator bool() const noexcept { return m_client ? true : false; }

          private:
            Crt::Mqtt::MqttClient m_client;
            int m_lastError;
        };
    } // namespace Iot
} // namespace Aws

#endif // !BYO_CRYPTO
