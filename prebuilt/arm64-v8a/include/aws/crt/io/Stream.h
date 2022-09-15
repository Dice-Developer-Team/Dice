#pragma once
/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <aws/crt/Exports.h>
#include <aws/crt/Types.h>
#include <aws/io/stream.h>

namespace Aws
{
    namespace Crt
    {
        namespace Io
        {
            using StreamStatus = aws_stream_status;

            /**
             * @deprecated Use int64_t instead for offsets in public APIs.
             */
            using OffsetType = aws_off_t;

            enum class StreamSeekBasis
            {
                Begin = AWS_SSB_BEGIN,
                End = AWS_SSB_END,
            };

            /***
             * Interface for building an Object oriented stream that will be honored by the CRT's low-level
             * aws_input_stream interface. To use, create a subclass of InputStream and define the abstract
             * functions.
             */
            class AWS_CRT_CPP_API InputStream
            {
              public:
                virtual ~InputStream();

                InputStream(const InputStream &) = delete;
                InputStream &operator=(const InputStream &) = delete;
                InputStream(InputStream &&) = delete;
                InputStream &operator=(InputStream &&) = delete;

                explicit operator bool() const noexcept { return IsValid(); }
                virtual bool IsValid() const noexcept = 0;

                aws_input_stream *GetUnderlyingStream() noexcept { return &m_underlying_stream; }

                bool Read(ByteBuf &dest) { return aws_input_stream_read(&m_underlying_stream, &dest) == 0; }
                bool Seek(int64_t offset, StreamSeekBasis seekBasis)
                {
                    return aws_input_stream_seek(&m_underlying_stream, offset, (aws_stream_seek_basis)seekBasis) == 0;
                }
                bool GetStatus(StreamStatus &status)
                {
                    return aws_input_stream_get_status(&m_underlying_stream, &status) == 0;
                }
                bool GetLength(int64_t &length)
                {
                    return aws_input_stream_get_length(&m_underlying_stream, &length) == 0;
                }

              protected:
                Allocator *m_allocator;
                aws_input_stream m_underlying_stream;

                InputStream(Aws::Crt::Allocator *allocator = g_allocator);

                /***
                 * Read up-to buffer::capacity - buffer::len into buffer::buffer
                 * Increment buffer::len by the amount you read in.
                 *
                 * @return true on success, false otherwise. Return false, when there is nothing left to read.
                 * You SHOULD raise an error via aws_raise_error()
                 * if an actual failure condition occurs.
                 */
                virtual bool ReadImpl(ByteBuf &buffer) noexcept = 0;

                /**
                 * @return the current status of the stream.
                 */
                virtual StreamStatus GetStatusImpl() const noexcept = 0;

                /**
                 * @return the total length of the available data for the stream.
                 * @return -1 if not available.
                 */
                virtual int64_t GetLengthImpl() const noexcept = 0;

                /**
                 * Seek's the stream to seekBasis based offset bytes.
                 *
                 * It is expected, that if seeking to the beginning of a stream,
                 * all error's are cleared if possible.
                 *
                 * @return true on success, false otherwise. You SHOULD raise an error via aws_raise_error()
                 * if a failure occurs.
                 */
                virtual bool SeekImpl(int64_t offset, StreamSeekBasis seekBasis) noexcept = 0;

              private:
                static int s_Seek(aws_input_stream *stream, int64_t offset, enum aws_stream_seek_basis basis);
                static int s_Read(aws_input_stream *stream, aws_byte_buf *dest);
                static int s_GetStatus(aws_input_stream *stream, aws_stream_status *status);
                static int s_GetLength(struct aws_input_stream *stream, int64_t *out_length);
                static void s_Destroy(struct aws_input_stream *stream);

                static aws_input_stream_vtable s_vtable;
            };

            /***
             * Implementation of Aws::Crt::Io::InputStream that wraps a std::input_stream.
             */
            class AWS_CRT_CPP_API StdIOStreamInputStream : public InputStream
            {
              public:
                StdIOStreamInputStream(
                    std::shared_ptr<Aws::Crt::Io::IStream> stream,
                    Aws::Crt::Allocator *allocator = g_allocator) noexcept;

                bool IsValid() const noexcept override;

              protected:
                bool ReadImpl(ByteBuf &buffer) noexcept override;
                StreamStatus GetStatusImpl() const noexcept override;
                int64_t GetLengthImpl() const noexcept override;
                bool SeekImpl(OffsetType offsetType, StreamSeekBasis seekBasis) noexcept override;

              private:
                std::shared_ptr<Aws::Crt::Io::IStream> m_stream;
            };
        } // namespace Io
    }     // namespace Crt
} // namespace Aws
