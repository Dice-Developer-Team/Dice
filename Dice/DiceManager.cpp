#include "DiceManager.h"

void setPassword(const std::string& password)
{
    WebUIPassword = password;
    if (WebUIPassword.empty()) WebUIPassword = "password";
    ofstream passwordStream(DiceDir / "conf" / "WebUIPassword");
	if (passwordStream)
	{
		passwordStream << WebUIPassword;
	}
	passwordStream.close();
}