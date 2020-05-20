#pragma once
#include <string>

namespace CQ
{
	class CQstream
	{
	protected:
		std::string buf;
	public:
		virtual void clear() noexcept;

		//字符串
		virtual CQstream& append(const std::string& s) noexcept;
		virtual CQstream& operator <<(const std::string& s) noexcept;
		//
		virtual CQstream& append(const char* c) noexcept;
		virtual CQstream& operator <<(const char* c) noexcept;

		//整数
		virtual CQstream& append(const int& i) noexcept;
		virtual CQstream& operator <<(const int& i) noexcept;
		virtual CQstream& append(const std::size_t& i) noexcept;
		virtual CQstream& operator <<(const std::size_t& i) noexcept;

		//长整数(Q号什么的)
		virtual CQstream& append(const long long& l) noexcept;
		virtual CQstream& operator <<(const long long& l) noexcept;


		//特殊控制符
		virtual CQstream& operator <<(void (*control)(CQstream&));
		virtual void send() noexcept = 0;
		virtual void flush() noexcept;

		virtual ~CQstream() noexcept;
	};

	//发送并清除缓冲区
	void send(CQstream& log) noexcept;
	//只发送,保留缓冲区,下次发送时将发送重复内容
	void flush(CQstream& log) noexcept;
	void endl(CQstream& log) noexcept;
}
