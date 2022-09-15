#pragma once
/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/Types.h>
#include <aws/crt/crypto/HMAC.h>
#include <aws/crt/crypto/Hash.h>
#include <aws/crt/io/Bootstrap.h>
#include <aws/crt/io/EventLoopGroup.h>
#include <aws/crt/io/TlsOptions.h>
#include <aws/crt/mqtt/MqttClient.h>

#include <aws/common/logging.h>

namespace Aws
{
    namespace Crt
    {
        enum class LogLevel
        {
            None = AWS_LL_NONE,
            Fatal = AWS_LL_FATAL,
            Error = AWS_LL_ERROR,
            Warn = AWS_LL_WARN,
            Info = AWS_LL_INFO,
            Debug = AWS_LL_DEBUG,
            Trace = AWS_LL_TRACE,

            Count
        };

        enum class ApiHandleShutdownBehavior
        {
            Blocking,
            NonBlocking
        };

        class AWS_CRT_CPP_API ApiHandle
        {
          public:
            ApiHandle(Allocator *allocator) noexcept;
            ApiHandle() noexcept;
            ~ApiHandle();
            ApiHandle(const ApiHandle &) = delete;
            ApiHandle(ApiHandle &&) = delete;
            ApiHandle &operator=(const ApiHandle &) = delete;
            ApiHandle &operator=(ApiHandle &&) = delete;
            /**
             * Initialize logging in awscrt.
             * @param level: Display messages of this importance and higher. LogLevel.NoLogs will disable
             * logging.
             * @param filename: Logging destination, a file path from the disk.
             */
            void InitializeLogging(LogLevel level, const char *filename);
            /**
             * Initialize logging in awscrt.
             * @param level: Display messages of this importance and higher. LogLevel.NoLogs will disable
             * logging.
             * @param fp: The FILE object for logging destination.
             */
            void InitializeLogging(LogLevel level, FILE *fp);

            /**
             * Configures the shutdown behavior of the api handle instance
             * @param shutdownBehavior desired shutdown behavior
             */
            void SetShutdownBehavior(ApiHandleShutdownBehavior behavior);

            /**
             * BYO_CRYPTO: set callback for creating MD5 hashes.
             * If using BYO_CRYPTO, you must call this.
             */
            void SetBYOCryptoNewMD5Callback(Crypto::CreateHashCallback &&callback);

            /**
             * BYO_CRYPTO: set callback for creating SHA256 hashes.
             * If using BYO_CRYPTO, you must call this.
             */
            void SetBYOCryptoNewSHA256Callback(Crypto::CreateHashCallback &&callback);

            /**
             * BYO_CRYPTO: set callback for creating Streaming SHA256 HMAC objects.
             * If using BYO_CRYPTO, you must call this.
             */
            void SetBYOCryptoNewSHA256HMACCallback(Crypto::CreateHMACCallback &&callback);

            /**
             * BYO_CRYPTO: set callback for creating a ClientTlsChannelHandler.
             * If using BYO_CRYPTO, you must call this prior to creating any client channels in the
             * application.
             */
            void SetBYOCryptoClientTlsCallback(Io::NewClientTlsHandlerCallback &&callback);

            /**
             * BYO_CRYPTO: set callbacks for the TlsContext.
             * If using BYO_CRYPTO, you need to call this function prior to creating a TlsContext.
             *
             * @param newCallback Create custom implementation object, to be stored inside TlsContext.
             *                    Return nullptr if failure occurs.
             * @param deleteCallback Destroy object that was created by newCallback.
             * @param alpnCallback Return whether ALPN is supported.
             */
            void SetBYOCryptoTlsContextCallbacks(
                Io::NewTlsContextImplCallback &&newCallback,
                Io::DeleteTlsContextImplCallback &&deleteCallback,
                Io::IsTlsAlpnSupportedCallback &&alpnCallback);

            /// @private
            static const Io::NewTlsContextImplCallback &GetBYOCryptoNewTlsContextImplCallback();
            /// @private
            static const Io::DeleteTlsContextImplCallback &GetBYOCryptoDeleteTlsContextImplCallback();
            /// @private
            static const Io::IsTlsAlpnSupportedCallback &GetBYOCryptoIsTlsAlpnSupportedCallback();

          private:
            void InitializeLoggingCommon(struct aws_logger_standard_options &options);

            aws_logger m_logger;

            ApiHandleShutdownBehavior m_shutdownBehavior;
        };

        AWS_CRT_CPP_API const char *ErrorDebugString(int error) noexcept;
        /**
         * @return the value of the last aws error on the current thread. Return 0 if no aws-error raised before.
         */
        AWS_CRT_CPP_API int LastError() noexcept;
        /**
         * @return the value of the last aws error on the current thread. Return AWS_ERROR_UNKNOWN, if no aws-error
         * raised before.
         */
        AWS_CRT_CPP_API int LastErrorOrUnknown() noexcept;
    } // namespace Crt
} // namespace Aws
