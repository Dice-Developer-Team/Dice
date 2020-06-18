#pragma once
#include <vector>
#include <string>

void show(void* t, int len);

class Unpack final
{
	std::vector<unsigned char> buff;
public:
	Unpack() noexcept;
	explicit Unpack(const char*);
	explicit Unpack(std::vector<unsigned char>) noexcept;
	explicit Unpack(const std::string&);

	Unpack& setData(const char* i, int len);
	Unpack& clear() noexcept;
	[[nodiscard]] int len() const noexcept;

	Unpack& add(int i); //添加一个整数
	int getInt() noexcept; //弹出一个整数

	Unpack& add(long long i); //添加一个长整数
	long long getLong() noexcept; //弹出一个长整数

	Unpack& add(short i); //添加一个短整数
	short getshort() noexcept; //弹出一个短整数

	Unpack& add(const unsigned char* i, short len); //添加一个字节集(请用add(std::string i);)
	std::vector<unsigned char> getchars() noexcept; //弹出一个字节集(请用getstring();)

	Unpack& add(std::string i); //添加一个字符串
	std::string getstring(); //弹出一个字符串

	Unpack& add(Unpack& i); //添加一个Unpack
	Unpack getUnpack() noexcept; //弹出一个Unpack

	std::string getAll() noexcept; //返回本包数据

	void show();
};
