#include "..\CQSDK\CQAPI.h"
#include "..\CQSDK\CQEVE.h"

int AuthCode;
CQEVENT(int, Initialize, 4)(int AuthCode)
{
	::AuthCode = AuthCode;
	return 0;
}
int getAuthCode()
{
	return AuthCode;
}

int CQ::getAuthCode()
{
	return AuthCode;
}
