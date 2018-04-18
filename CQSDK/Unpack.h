#pragma once
#include <vector>

void show(void*t, int len);
class Unpack
{
	std::vector<unsigned char> buff;
public:
	Unpack();
	Unpack(const char*);
    Unpack(std::vector<unsigned char>);
	Unpack(std::string);

	Unpack&setData(const char* i, int len);
	Unpack&clear();
    int len() const;

	Unpack&add(int i);//添加一个整数
	int getInt();//弹出一个整数

	Unpack&add(long long i);//添加一个长整数
	long long getLong();//弹出一个长整数

	Unpack&add(short i);//添加一个短整数
	short getshort();//弹出一个短整数

	Unpack&add(unsigned char* i, short len);//添加一个字节集(请用add(std::string i);)
	std::vector<unsigned char> getchars();//弹出一个字节集(请用getstring();)

	Unpack&add(std::string i);//添加一个字符串
	std::string getstring();//弹出一个字符串

	Unpack&add(Unpack&i);//添加一个Unpack
	Unpack getUnpack();//弹出一个Unpack

	std::string getAll();//返回本包数据

	void show();
};