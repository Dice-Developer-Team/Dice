#pragma once
/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/Exports.h>

#include <stddef.h>

namespace Aws
{
    namespace Crt
    {
        size_t AWS_CRT_CPP_API HashString(const char *str) noexcept;
    }
} // namespace Aws
