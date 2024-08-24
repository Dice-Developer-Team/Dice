#pragma once
#ifndef TRPGLOGGER_S3PUTOBJECT
#define TRPGLOGGER_S3PUTOBJECT
#include <string>
#include <aws/core/Aws.h>

void aws_init();
void aws_shutdown();
// upload file to S3 by S3-accelerate
// return "SUCCESS" or error info
std::string put_s3_object(const Aws::String& s3_bucket_name,
	const Aws::String& s3_object_name,
	const std::string& file_name,
	const Aws::String& region = "");

#endif /*TRPGLOGGER_S3PUTOBJECT*/