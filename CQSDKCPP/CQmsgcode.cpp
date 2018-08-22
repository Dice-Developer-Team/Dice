#include <string>
#include "CQmsgcode.h"
#include "CQTools.h"

using namespace std;
using namespace CQ;


/*
CQat::CQat(long long QQ) :QQ(QQ) {}

std::string CQat::tostring() const
{
return std::string("[CQ:at,qq=") + to_string(QQ) + "]";
}

CQface::CQface(int faceid) : faceid(faceid) {}

CQface::CQface(face face) : faceid(face) {}

std::string CQface::tostring() const
{
return std::string("[CQ:face,id=") + to_string(faceid) + "]";
}

CQrecord::CQrecord(string fileurl, bool magic) : fileurl(fileurl), magic(magic) {}

std::string CQrecord::tostring()const
{
string s = std::string("[CQ:record,file=") + fileurl;
if (magic)s += ",magic=true";
return s += "]";
}

CQimage::CQimage(string file) :file(file) {}

std::string CQimage::tostring()const
{
return std::string("[CQ:image,file=") + file + "]";
}
*/
//...
std::string CQ::code::image(std::string file)
{
	return std::string("[CQ:image,file=") + msg_encode(file, true) + "]";
}

std::string CQ::code::record(std::string fileurl, bool magic)
{
	string s = std::string("[CQ:record,file=") + fileurl;
	if (magic)s += ",magic=true";
	return s += "]";
}

std::string CQ::code::face(int faceid)
{
	return std::string("[CQ:face,id=") + to_string(faceid) + "]";
}

std::string CQ::code::face(CQ::face face)
{
	return code::face((int)face);
}

std::string CQ::code::at(long long QQ)
{
	return std::string("[CQ:at,qq=") + to_string(QQ) + "]";
}


void CQ::CodeMsgs::decod()
{
	bool key=false;
	
	bool notStarted = true;
	for (size_t len = 0; len < txt.size(); len++)
	{
		switch (txt[len])
		{
		default://字符串开始
			if (notStarted) {
				notStarted = false;
				msglist.push_back(CodeMsg(false, len));
			}
			break;
		case '['://code串开始
			txt[len] = 0;//指向[
			len += 3;
			txt[len] = 0;//指向:
			msglist.push_back(CodeMsg(true, len+1));
			notStarted = false;
			break;
		case ',':
			if (msglist.rbegin()->isCode) {//只有在code里的,逗号才有意义
				key = true;
				txt[len] = 0;
				msglist.rbegin()->push_back(OneCodeMsg(len + 1));
			}
			break;
		case '=':
			if (key) {//等于号只会跟在key后面
				key = false;
				txt[len] = 0;
				msglist.rbegin()->rbegin()->value = len + 1;
			}
			break;
		case ']'://结束了
			notStarted = true;
			txt[len] = 0;
			break;
		}
	}
}

CQ::CodeMsgs::CodeMsgs(std::string s)
{
	txt = s;
	decod();
}

char * CQ::CodeMsgs::at(int i)
{
	return &txt[i];
}

CQ::CodeMsgs & CQ::CodeMsgs::operator[](int)
{
	return *this;
}

CQ::CodeMsgs & CQ::CodeMsgs::find(std::string s)
{
	return *this;
}

CQ::CodeMsgs & CQ::CodeMsgs::listfind(std::string)
{
	return *this;
}

bool CQ::CodeMsgs::isCQcode()
{
	return false;
}

bool CQ::CodeMsgs::is(std::string)
{
	return false;
}

std::string CQ::CodeMsgs::get()
{
	return std::string();
}

std::string CQ::CodeMsgs::get(std::string key)
{
	return std::string();
}

CQ::CodeMsg::CodeMsg(bool isCode, size_t key):isCode(isCode), key(key){}

CQ::OneCodeMsg::OneCodeMsg(size_t key): key(key){}
