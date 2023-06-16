#ifdef _WIN32
#include <fstream>
#include "filesystem.hpp"

#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/AWSError.h>
#include <aws/s3/S3Request.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>

#include "S3PutObject.h"

#include "GlobalVar.h"

#ifdef _WIN32
#undef GetMessage
#endif

// Aws SDK����
Aws::SDKOptions options;
std::shared_ptr<Aws::Auth::AnonymousAWSCredentialsProvider> awsCredProvider{ std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>() };
std::shared_ptr<Aws::S3::S3EndpointProvider> awsEProvider;
// �ж��ļ��Ƿ����
bool file_exists(const std::string& file_name)
{
	std::error_code ec;
	return std::filesystem::is_regular_file(file_name, ec);
}
void aws_init() {
	Aws::InitAPI(options);
	awsEProvider = Aws::MakeShared<Aws::S3::S3EndpointProvider>(Aws::S3::S3Client::ALLOCATION_TAG);
}
void aws_shutdown() {
	awsEProvider.reset();
	Aws::ShutdownAPI(options); 
}
// �ϴ��ļ���S3, ����S3-accelerate
// �ɹ�ʱ����"SUCCESS", ���򷵻ش�����Ϣ
std::string put_s3_object(const Aws::String& s3_bucket_name,
	const Aws::String& s3_object_name,
	const std::string& file_name,
	const Aws::String& region)
{
	// Verify file_name exists
	if (!file_exists(file_name)) {
		return "ERROR: File Not Found";
	}
	// If region is specified, use it
	Aws::Client::ClientConfiguration clientConfig;
	//if (!region.empty())
		clientConfig.region = region;
	clientConfig.verifySSL = false;
	clientConfig.endpointOverride = "s3-accelerate.amazonaws.com";
	// Set up request
	Aws::S3::S3Client s3_client(awsCredProvider, awsEProvider, clientConfig);
	Aws::S3::Model::PutObjectRequest object_request;
	object_request.SetBucket(s3_bucket_name);
	object_request.SetKey(s3_object_name);
	const std::shared_ptr<Aws::IOStream> input_data =
		Aws::MakeShared<Aws::FStream>(file_name.c_str(),
			file_name.c_str(),
			std::ios_base::in | std::ios_base::binary);
	object_request.SetBody(input_data);
	// Put the object
	auto put_object_outcome = s3_client.PutObject(object_request);
	if (!put_object_outcome.IsSuccess()) {
		const auto& error = put_object_outcome.GetError();
		return std::string("ERROR: ") + error.GetExceptionName().c_str() + ": "
			+ error.GetMessage().c_str();
	}
	return "SUCCESS";
}
#endif //Win32