#pragma once
#ifndef TRPGLOGGER_S3PUTOBJECT
#define TRPGLOGGER_S3PUTOBJECT
#include <string>
#include <aws/core/Aws.h>

void aws_init();
void aws_shutdown();
// �ϴ��ļ���S3, ����S3-accelerate
// �ɹ�ʱ����"SUCCESS", ���򷵻ش�����Ϣ
std::string put_s3_object(const Aws::String& s3_bucket_name,
	const Aws::String& s3_object_name,
	const std::string& file_name,
	const Aws::String& region = "");

#endif /*TRPGLOGGER_S3PUTOBJECT*/