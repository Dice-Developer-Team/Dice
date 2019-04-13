#include "Smtp.h"
//#include <afx.h>//异常类
static const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char* base64Encode(char const* origSigned, unsigned origLength)
{
	unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set  
	if (orig == NULL) return NULL;

	unsigned const numOrig24BitValues = origLength / 3;
	bool havePadding = origLength > numOrig24BitValues * 3;
	bool havePadding2 = origLength == numOrig24BitValues * 3 + 2;
	unsigned const numResultBytes = 4 * (numOrig24BitValues + havePadding);
	char* result = new char[numResultBytes + 3]; // allow for trailing '/0'  

	// Map each full group of 3 input bytes into 4 output base-64 characters:  
	unsigned i;
	for (i = 0; i < numOrig24BitValues; ++i)
	{
		result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
		result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
		result[4 * i + 2] = base64Char[((orig[3 * i + 1] << 2) | (orig[3 * i + 2] >> 6)) & 0x3F];
		result[4 * i + 3] = base64Char[orig[3 * i + 2] & 0x3F];
	}

	// Now, take padding into account.  (Note: i == numOrig24BitValues)  
	if (havePadding)
	{
		result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
		if (havePadding2)
		{
			result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
			result[4 * i + 2] = base64Char[(orig[3 * i + 1] << 2) & 0x3F];
		}
		else
		{
			result[4 * i + 1] = base64Char[((orig[3 * i] & 0x3) << 4) & 0x3F];
			result[4 * i + 2] = '=';
		}
		result[4 * i + 3] = '=';
	}

	result[numResultBytes] = '\0';
	return result;
}


int Csmtp::SendAttachment(SOCKET &sockClient) /*发送附件*/
{
	for (std::vector<string>::iterator iter = filename.begin(); iter != filename.end(); iter++)
	{
		//cout << "Attachment is sending··· " << endl;

		string path = *iter;
		ifstream ifs(path, ios::in | ios::binary); //'或链接2个属性，以输入、二进制打开'
		if (false == ifs.is_open())
		{
			//cout << "无法打开文件！" << endl;
			return 1;
		}
		char pathName[256];
		const char *p = path.c_str();
		strcpy_s(pathName, p + path.find_last_of("\\") + 1);
		string strPathName = pathName;

		string sendstring;
		sendstring = "--@boundary@\r\nContent-Type: application/octet-stream; name=\"" + strPathName + "\"\r\n";
		sendstring += "Content-Disposition: attachment; filename=\"" + strPathName + "\"\r\n";
		sendstring += "Content-Transfer-Encoding: base64\r\n\r\n";
		send(sockClient, sendstring.c_str(), sendstring.length(), 0);

		//infile.read((char*)buffer,sizeof(数据类型));

		// get length of file:
		ifs.seekg(0, ifs.end);
		int length = ifs.tellg();
		ifs.seekg(0, ifs.beg);
		//cout << "length:" << length << endl;
		// allocate memory:
		char * buffer = new char[length];
		// read data as a block:
		ifs.read(buffer, length);
		ifs.close();
		char *pbase;
		pbase = base64Encode(buffer, length);
		delete[]buffer;
		string str(pbase);
		delete[]pbase;
		str += "\r\n";
		int err = send(sockClient, str.c_str(), strlen(str.c_str()), 0);

		if (err != strlen(str.c_str()))
		{
			//cout << "附件发送出错!" << endl;
			return 1;
		}
	}
	return 0;
}


bool Csmtp::CReateSocket()
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 1);
	//WSAStarup，即WSA(Windows SocKNDs Asynchronous，Windows套接字异步)的启动命令
	int err = WSAStartup(wVersionRequested, &wsaData);
	//cout << "WSAStartup(0:successful):" << err << endl;

	char namebuf[128];    //获得本地计算机名
	string ip_list;
	if (0 == gethostname(namebuf, 128))
	{
		struct hostent* pHost;  //获得本地IP地址
		pHost = gethostbyname(namebuf);  //pHost返回的是指向主机的列表
		for (int i = 0; pHost != NULL && pHost->h_addr_list[i] != NULL; i++)
		{
			string tem = inet_ntoa(*(struct in_addr *)pHost->h_addr_list[i]);
			ip_list += tem;
			ip_list += "\n";
		}
	}
	else
	{
		//cout << "获取主机信息失败..." << endl;
	}
	//////////////////////////////////////////////////////////////////////////
	title = namebuf;// 邮件标题
	content = ip_list; //主机ip

	sockClient = socket(AF_INET, SOCK_STREAM, 0); //建立socket对象  

	pHostent = gethostbyname(domain.c_str()); //得到有关于域名的信息

	if (pHostent == NULL)
	{
		//printf("创建连接失败，也许没联网！\n");
		return false;
	}

	return true;
}


int Csmtp::SendMail()
{
	char *ecode;

	char buff[500];  //recv函数返回的结果
	int err = 0;
	string message; //

	SOCKADDR_IN addrServer;  //服务端地址
	addrServer.sin_addr.S_un.S_addr = *((DWORD *)pHostent->h_addr_list[0]); //得到smtp服务器的网络字节序的ip地址     



	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port); //连接端口25 
	//int connect (SOCKET s , const struct sockaddr FAR *name , int namelen );
	err = connect(sockClient, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));   //向服务器发送请求  
	//cout << "connect:" << err << endl;
	//telnet smtp.126.com 25 连接服务器结束
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "connect:" << buff << endl;

	message = "ehlo 126.com\r\n";
	send(sockClient, message.c_str(), message.length(), 0);

	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "helo:" << buff << endl;

	message = "auth login \r\n";
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "auth login:" << buff << endl;
	//上传邮箱名
	message = user;
	ecode = base64Encode(message.c_str(), strlen(message.c_str()));
	message = ecode;
	message += "\r\n";
	delete[]ecode;
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "usrname:" << buff << endl;
	//上传邮箱密码
	message = pass;
	ecode = base64Encode(message.c_str(), strlen(message.c_str()));
	message = ecode;
	delete[]ecode;
	message += "\r\n";
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "password:" << buff << endl;

	message = "mail from:<" + user + ">\r\nrcpt to:<" + target + ">\r\n";
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "mail from: " << buff << endl;
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "rcpt to: " << buff << endl;

	message = "data\r\n";//data要单独发送一次
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "data: " << buff << endl;
	///-----------------------------------------DATA-------------------------------------
	//要使用Csmtp 发送附件, 需要对Csmtp 头信息进行说明, 改变Content-type 及为每一段正文添加BOUNDARY 名,
	//cout << "-------------------DATA------------------------" << endl;
	//  头
	message = "from:" + user + "\r\nto:" + target + "\r\nsubject:" + title + "\r\n";
	message += "MIME-Version: 1.0\r\n";
	message += "Content-Type: multipart/mixed;boundary=@boundary@\r\n\r\n";
	send(sockClient, message.c_str(), message.length(), 0);

	//  正文
	message = "--@boundary@\r\nContent-Type: text/plain;charset=\"gb2312\"\r\n\r\n" + content + "\r\n\r\n";
	send(sockClient, message.c_str(), message.length(), 0);

	//------------------------------------------------------------------------------------------------
	//  发送附件

	SendAttachment(sockClient);

	/*发送结尾信息*/
	message = "--@boundary@--\r\n.\r\n";
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "end_qwertyuiop:" << buff << endl;

	message = "QUIT\r\n";
	send(sockClient, message.c_str(), message.length(), 0);
	buff[recv(sockClient, buff, 500, 0)] = '\0';
	//cout << "Send mail is finish:" << buff << endl;
	return 0;
}
