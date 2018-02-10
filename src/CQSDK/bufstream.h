#pragma once
#include <string>
namespace CQ {
	class CQstream {
	protected:
		std::string buf;
	public:
		virtual void clear();

		//字符串
		virtual	CQstream & append(const std::string & s);
		virtual	CQstream & operator <<(const std::string & s);
		//
		virtual	CQstream & append(const char * c);
		virtual	CQstream & operator <<(const char * c);

		//整数
		virtual	CQstream & append(const int & i);
		virtual	CQstream & operator <<(const int & i);
		virtual	CQstream & append(const std::size_t & i);
		virtual	CQstream & operator <<(const std::size_t & i);

		//长整数(Q号什么的)
		virtual	CQstream & append(const long long & l);
		virtual	CQstream & operator <<(const long long & l);


		//特殊控制符
		virtual	CQstream & operator <<(void (*control)(CQstream &));
		virtual	void send() = 0;
		virtual	void flush();

		virtual ~CQstream();
	};
	//发送并清除缓冲区
	void send(CQstream & log);
	//只发送,保留缓冲区,下次发送时将发送重复内容
	void flush(CQstream & log);
	void endl(CQstream & log);
}