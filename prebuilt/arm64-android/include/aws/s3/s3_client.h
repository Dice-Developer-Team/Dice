#ifndef AWS_S3_CLIENT_H
#define AWS_S3_CLIENT_H

/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <aws/io/retry_strategy.h>
#include <aws/s3/s3.h>

#include <aws/auth/signing_config.h>

struct aws_allocator;

struct aws_http_stream;
struct aws_http_message;
struct aws_http_headers;
struct aws_tls_connection_options;
struct aws_input_stream;

struct aws_s3_client;
struct aws_s3_request;
struct aws_s3_meta_request;
struct aws_s3_meta_request_result;

enum aws_s3_meta_request_type {
    AWS_S3_META_REQUEST_TYPE_DEFAULT,
    AWS_S3_META_REQUEST_TYPE_GET_OBJECT,
    AWS_S3_META_REQUEST_TYPE_PUT_OBJECT,

    AWS_S3_META_REQUEST_TYPE_MAX,
};

typedef int(aws_s3_meta_request_headers_callback_fn)(
    struct aws_s3_meta_request *meta_request,
    const struct aws_http_headers *headers,
    int response_status,
    void *user_data);

typedef int(aws_s3_meta_request_receive_body_callback_fn)(
    /* The meta request that the callback is being issued for. */
    struct aws_s3_meta_request *meta_request,

    /* The body data for this chunk of the object. */
    const struct aws_byte_cursor *body,

    /* The byte index of the object that this refers to. For example, for an HTTP message that has a range header, the
       first chunk received will have a range_start that matches the range header's range-start.*/
    uint64_t range_start,

    /* User data specified by aws_s3_meta_request_options.*/
    void *user_data);

typedef void(aws_s3_meta_request_finish_fn)(
    struct aws_s3_meta_request *meta_request,
    const struct aws_s3_meta_request_result *meta_request_result,
    void *user_data);

typedef void(aws_s3_meta_request_shutdown_fn)(void *user_data);

typedef void(aws_s3_client_shutdown_complete_callback_fn)(void *user_data);

enum aws_s3_meta_request_tls_mode {
    AWS_MR_TLS_ENABLED,
    AWS_MR_TLS_DISABLED,
};

enum aws_s3_meta_request_compute_content_md5 {
    AWS_MR_CONTENT_MD5_DISABLED,
    AWS_MR_CONTENT_MD5_ENABLED,
};

/* Options for a new client. */
struct aws_s3_client_config {

    /* When set, this will cap the number of active connections. When 0, the client will determine this value based on
     * throughput_target_gbps. (Recommended) */
    uint32_t max_active_connections_override;

    /* Region that the S3 bucket lives in. */
    struct aws_byte_cursor region;

    /* Client bootstrap used for common staples such as event loop group, host resolver, etc.. s*/
    struct aws_client_bootstrap *client_bootstrap;

    /* How tls should be used while performing the request
     * If this is ENABLED:
     *     If tls_connection_options is not-null, then those tls options will be used
     *     If tls_connection_options is NULL, then default tls options will be used
     * If this is DISABLED:
     *     No tls options will be used, regardless of tls_connection_options value.
     */
    enum aws_s3_meta_request_tls_mode tls_mode;

    /* TLS Options to be used for each connection, if tls_mode is ENABLED. When compiling with BYO_CRYPTO, and tls_mode
     * is ENABLED, this is required. Otherwise, this is optional. */
    struct aws_tls_connection_options *tls_connection_options;

    /* Signing options to be used for each request. Specify NULL to not sign requests. */
    struct aws_signing_config_aws *signing_config;

    /* Size of parts the files will be downloaded or uploaded in. */
    size_t part_size;

    /* If the part size needs to be adjusted for service limits, this is the maximum size it will be adjusted to.. */
    size_t max_part_size;

    /* Throughput target in Gbps that we are trying to reach. */
    double throughput_target_gbps;

    /* Retry strategy to use. If NULL, a default retry strategy will be used. */
    struct aws_retry_strategy *retry_strategy;

    /**
     * For multi-part upload, content-md5 will be calculated if the AWS_MR_CONTENT_MD5_ENABLED is specified
     *     or initial request has content-md5 header.
     * For single-part upload, keep the content-md5 in the initial request unchanged. */
    enum aws_s3_meta_request_compute_content_md5 compute_content_md5;

    /* Callback and associated user data for when the client has completed its shutdown process. */
    aws_s3_client_shutdown_complete_callback_fn *shutdown_callback;
    void *shutdown_callback_user_data;
};

/* Options for a new meta request, ie, file transfer that will be handled by the high performance client. */
struct aws_s3_meta_request_options {

    /* The type of meta request we will be trying to accelerate. */
    enum aws_s3_meta_request_type type;

    /* Signing options to be used for each request created for this meta request.  If NULL, options in the client will
     * be used. If not NULL, these options will override the client options. */
    struct aws_signing_config_aws *signing_config;

    /* Initial HTTP message that defines what operation we are doing. */
    struct aws_http_message *message;

    /* User data for all callbacks. */
    void *user_data;

    /* Callback for receiving incoming headers. */
    aws_s3_meta_request_headers_callback_fn *headers_callback;

    /* Callback for incoming body data. */
    aws_s3_meta_request_receive_body_callback_fn *body_callback;

    /* Callback for when the meta request is completely finished. */
    aws_s3_meta_request_finish_fn *finish_callback;

    /* Callback for when the meta request has completely cleaned up. */
    aws_s3_meta_request_shutdown_fn *shutdown_callback;
};

/* Result details of a meta request.
 *
 * If error_code is AWS_ERROR_SUCCESS, then response_status will match the response_status passed earlier by the header
 * callback and error_response_headers and error_response_body will be NULL.
 *
 * If error_code is equal to AWS_ERROR_S3_INVALID_RESPONSE_STATUS, then error_response_headers, error_response_body, and
 * response_status will be populated by the failed request.
 *
 * For all other error codes, response_status will be 0, and the error_response variables will be NULL.
 */
struct aws_s3_meta_request_result {

    /* HTTP Headers for the failed request that triggered finish of the meta request.  NULL if no request failed. */
    struct aws_http_headers *error_response_headers;

    /* Response body for the failed request that triggered finishing of the meta request.  NUll if no request failed.*/
    struct aws_byte_buf *error_response_body;

    /* Response status of the failed request or of the entire meta request. */
    int response_status;

    /* Final error code of the meta request. */
    int error_code;
};

AWS_EXTERN_C_BEGIN

AWS_S3_API
struct aws_s3_client *aws_s3_client_new(
    struct aws_allocator *allocator,
    const struct aws_s3_client_config *client_config);

AWS_S3_API
void aws_s3_client_acquire(struct aws_s3_client *client);

AWS_S3_API
void aws_s3_client_release(struct aws_s3_client *client);

AWS_S3_API
struct aws_s3_meta_request *aws_s3_client_make_meta_request(
    struct aws_s3_client *client,
    const struct aws_s3_meta_request_options *options);

AWS_S3_API
void aws_s3_meta_request_cancel(struct aws_s3_meta_request *meta_request);

AWS_S3_API
void aws_s3_meta_request_acquire(struct aws_s3_meta_request *meta_request);

AWS_S3_API
void aws_s3_meta_request_release(struct aws_s3_meta_request *meta_request);

AWS_S3_API
void aws_s3_init_default_signing_config(
    struct aws_signing_config_aws *signing_config,
    const struct aws_byte_cursor region,
    struct aws_credentials_provider *credentials_provider);

AWS_EXTERN_C_END

#endif /* AWS_S3_CLIENT_H */
