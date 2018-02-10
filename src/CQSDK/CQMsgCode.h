#pragma once
#include "CQface.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
//#include <shared_mutex>

namespace CQ {
	//cq消息参数
	struct OneCodeMsg { size_t key, keylen = 0, value = 0; OneCodeMsg(size_t key); };
	//一条cq消息或者普通消息
	struct CodeMsg : std::vector<OneCodeMsg> { public: bool isCode; size_t key, keylen = 0; CodeMsg(bool isCode, size_t key); };

	class CodeMsgs;
	struct CodeMsgsFor {
		CodeMsgs&t;
		size_t pos;
		CQ::CodeMsgs&operator*();
		CQ::CodeMsgsFor&operator++();
		bool operator!=(CQ::CodeMsgsFor&);
		CodeMsgsFor(CodeMsgs&t, int pos);
	};
	//消息解析
	class CodeMsgs {
		std::vector<CodeMsg> msglist;
		std::string txt;
		size_t thismsg=0;//指针
		void decod();//解码
		bool find(std::string &s,int);
		bool is(std::string &s, int);
	public:
		CodeMsgs(std::string);

		//char* at(int);

		//定位到指定段
		CQ::CodeMsgs&operator[](size_t);
		CQ::CodeMsgs&operator++(int);
		CQ::CodeMsgs&operator++();
		CQ::CodeMsgs&operator--(int);
		CQ::CodeMsgs&operator--();
		CQ::CodeMsgs&operator-(size_t);
		CQ::CodeMsgs&operator+(size_t);
		//返回指针当前位置
		int pos();
		
		//从当前位置开始搜索指定cq码
		//如果存在,定位到指定段
		//否则返回null,并且不会移动指针
		bool find(std::string s);

		//从当前位置开始反向搜索指定cq码
		//如果存在,定位到指定段
		//否则返回null,并且不会移动指针
		bool lastfind(std::string);


		//判断是否是CQ码
		bool isCQcode();

		//判断是否为指定CQ码
		bool is(std::string);

		//如果是CQ码,返回CQ码类型
		//如果不是,返回消息
		std::string get();

		//如果是CQ码,返回键对应的值
		//如果找不到键,则返回空字符
		//如果不是,返回空字符
		std::string get(std::string key);
		std::vector<std::string> keys();
		
		CQ::CodeMsgsFor begin();
		CQ::CodeMsgsFor end();

		void debug();

	};

	struct code {
		//[CQ:image,file={1}] - 发送自定义图片
		//文件以 酷Q目录\data\image\ 为基础
		static std::string image(std::string fileurl);

		//[CQ:record,file={1},magic={2}] - 发送语音
		//文件以 酷Q目录\data\record\ 为基础
		static std::string record(std::string fileurl, bool magic);

		//[CQ:face,id={1}] - QQ表情
		static std::string face(int faceid);

		//[CQ:face,id={1}] - QQ表情
		static std::string face(CQ::face face);

		//[CQ:at,qq={1}] - @某人
		static std::string at(long long QQ);

		//[CQ:effect,type=art,id=2003,content=小吉] - 魔法字体
		static std::string effect(std::string type, int id, std::string content);

		//[CQ:sign,title=晒幸福,image=http://pub.idqqimg.com/pc/misc/files/20170825/cc9103d0db0b4dcbb7a17554d227f4d7.jpg] - 签到

		//[CQ:hb, title = 恭喜发财] - 红包(只限收,不能发)

		//[CQ:shake, id = 1] - 戳一戳(原窗口抖动，仅支持好友消息使用)

		//[CQ:sface,id={1}] - 小表情

		//[CQ:bface,id={1}] - 原创表情

		//[CQ:emoji,id={1}] - emoji表情

		//[CQ:rps,type={1}] - 发送猜拳魔法表情
		//发送不支持自定义type
		//1 为 石头
		//2 为 剪刀
		//3 为 布

		//[CQ:dice,type={1}] - 发送掷骰子魔法表情
		//发送不支持自定义type
		//type为骰子点数

		//[CQ:anonymous,ignore={1}] - 匿名发消息(仅支持群消息使用)
		//必须在消息最开头
		//ignore为true时,如果发送失败则转为普通消息

		//[CQ:music,type={1},id={2}] - 发送音乐
		//type为音乐平台,支持qq、163、xiami
		//id即为音乐id

		//[CQ:music,type=custom,url={1},audio={2},title={3},content={4},image={5}] - 发送音乐自定义分享
		//url为分享链接,即点击分享后进入的音乐页面（如歌曲介绍页）
		//audio为音频链接（如mp3链接）
		//title为音乐的标题，建议12字以内
		//content为音乐的简介，建议30字以内。该参数可被忽略
		//image为音乐的封面图片链接。若参数为空或被忽略，则显示默认图片
		//!音乐自定义分享只能作为单独的一条消息发送

		//[CQ:share,url={1},title={2},content={3},image={4}] - 发送链接分享
		//url为分享链接。
		//title为分享的标题，建议12字以内。
		//content为分享的简介，建议30字以内。该参数可被忽略。
		//image为分享的图片链接。若参数为空或被忽略，则显示默认图片。
		//!链接分享只能作为单独的一条消息发送

	};
}