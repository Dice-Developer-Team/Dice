#pragma once

#include "CQEVE.h"//不能删除此行...
#include <string>
/*
悬浮窗

请设置 eve.data eve.dataf eve.color 其他函数保持不动即可

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)。
名字如果使用下划线开头需要改成双下划线
*/

#define EVE_Status_EX(Name)					\
	void Name(CQ::EVEStatus & eve);\
	EVE_Status(Name)\
	{\
		CQ::EVEStatus tep;\
		static std::string ret;\
		EVETry Name(tep); EVETryEnd(Name,发生了一个错误)										\
		ret = CQ::statusEVEreturn(tep);\
		return ret.c_str();\
	}\
	void Name(CQ::EVEStatus & eve)

namespace CQ {
	struct EVEStatus
	{
		std::string
			//数据
			data,
			//数据单位
			dataf;
		int
			// 1 : 绿
			// 2 : 橙
			// 3 : 红
			// 4 : 深红
			// 5 : 黑
			// 6 : 灰
			color;
		// 1 : 绿
		void color_green();
		// 2 : 橙
		void color_orange();
		// 3 : 红
		void color_red();
		// 4 : 深红
		void color_crimson();
		// 5 : 黑
		void color_black();
		// 6 : 灰
		void color_gray();
	};
	std::string statusEVEreturn(EVEStatus & eve);
}
