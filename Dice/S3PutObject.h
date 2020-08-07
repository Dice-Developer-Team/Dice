#pragma once
#ifndef TRPGLOGGER_S3PUTOBJECT
#define TRPGLOGGER_S3PUTOBJECT
#include <string>
#include <aws/core/Aws.h>

extern Aws::SDKOptions options;
// 上传文件至S3, 采用S3-accelerate
// 成功时返回"SUCCESS", 否则返回错误信息
std::string put_s3_object(const Aws::String& s3_bucket_name,
	const Aws::String& s3_object_name,
	const std::string& file_name,
	const Aws::String& region = "");

#endif /*TRPGLOGGER_S3PUTOBJECT*/