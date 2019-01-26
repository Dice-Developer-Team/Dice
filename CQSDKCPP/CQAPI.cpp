#include "CQAPI.h"
#include "CQEVE.h"

int AuthCode;
CQEVENT(int, Initialize, 4)(const int AuthCode)
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
