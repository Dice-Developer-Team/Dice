/* 
CoolQ SDK for VS2017
Api Version 9.13
Written by MukiPy2001 & Thanks for the help of orzFly and Coxxs
*/
#pragma once

#include "cqdefine.h"
#define CQAPI(NAME,ReturnType) extern "C" __declspec(dllimport) ReturnType __stdcall NAME


namespace CQ{
	// 获取调用api所需的AuthCode
	int getAuthCode();
	//发送好友消息 
	//Auth=106 失败返回负值,成功返回消息ID 
	CQAPI(CQ_sendPrivateMsg, int)(
		int AuthCode,// 
		long long QQID,// 目标QQ 
		const char * msg// 消息内容 
		);
	//发送群消息 
	//Auth=101 失败返回负值,成功返回消息ID
	CQAPI(CQ_sendGroupMsg, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		const char * msg// 消息内容 
		);
	//发送讨论组消息 
	//Auth=103 失败返回负值,成功返回消息ID
	CQAPI(CQ_sendDiscussMsg, int)(
		int AuthCode,// 
		long long 讨论组号,// 目标讨论组 
		const char * msg// 消息内容 
		);
	//发送赞 Auth=110
	CQAPI(CQ_sendLike, int)(
		int AuthCode,// 
		long long QQID// 目标QQ 
		);
	//发送赞V2 Auth=110
	CQAPI(CQ_sendLikeV2, int)(
		int AuthCode,// 
		long long QQID,// 目标QQ 
		int times// 赞的次数，最多10次 
		);
	//取Cookies (慎用，此接口需要严格授权) 
	//Auth=20 慎用,此接口需要严格授权 
	CQAPI(CQ_getCookies, const char *)(
		int AuthCode// 
		);
	//接收语音 
	CQAPI(CQ_getRecord, const char *)(
		int AuthCode,// 
		const char * file,// 收到消息中的语音文件名 (file) 
		const char * outformat// 应用所需的格式  mp3,amr,wma,m4a,spx,ogg,wav,flac
		);
	//取CsrfToken (慎用，此接口需要严格授权) 
	//Auth=20 即QQ网页用到的bkn/g_tk等 慎用,此接口需要严格授权 
	CQAPI(CQ_getCsrfToken, int)(
		int AuthCode// 
		);
	//取应用目录 
	//返回的路径末尾带"\" 
	CQAPI(CQ_getAppDirectory, const char *)(
		int AuthCode// 
		);
	//取登录QQ 
	CQAPI(CQ_getLoginQQ, long long)(
		int AuthCode// 
		);
	//取登录昵称 
	CQAPI(CQ_getLoginNick, const char *)(
		int AuthCode// 
		);
	//置群员移除 Auth=120 
	CQAPI(CQ_setGroupKick, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		long long QQID,// 目标QQ 
		CQBOOL 拒绝再加群// 如果为真，则“不再接收此人加群申请”，请慎用 
		);
	//置群员禁言 Auth=121 
	CQAPI(CQ_setGroupBan, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		long long QQID,// 目标QQ 
		long long 禁言时间// 禁言的时间，单位为秒。如果要解禁，这里填写0 
		);
	//置群管理员 Auth=122 
	CQAPI(CQ_setGroupAdmin, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		long long QQID,// 被设置的QQ 
		CQBOOL 成为管理员// 真/设置管理员 假/取消管理员 
		);
	//置群成员专属头衔 Auth=128 需群主权限 
	CQAPI(CQ_setGroupSpecialTitle, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		long long QQID,// 目标QQ 
		const char * 头衔,// 如果要删除，这里填空 
		long long 过期时间// 专属头衔有效期，单位为秒。如果永久有效，这里填写-1
		);
	//置全群禁言 Auth=123 
	CQAPI(CQ_setGroupWholeBan, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		CQBOOL 开启禁言// 真/开启 假/关闭 
		);
	//置匿名群员禁言 Auth=124 
	CQAPI(CQ_setGroupAnonymousBan, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		const char * 匿名,// 群消息事件收到的“匿名”参数 
		long long 禁言时间// 禁言的时间，单位为秒。不支持解禁 
		);
	//置群匿名设置 Auth=125 
	CQAPI(CQ_setGroupAnonymous, int)(
		int AuthCode,// 
		long long 群号,// 
		CQBOOL 开启匿名// 
		);
	//置群成员名片 Auth=126 
	CQAPI(CQ_setGroupCard, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		long long QQID,// 被设置的QQ 
		const char * 新名片_昵称// 
		);
	//置群退出 Auth=127 慎用,此接口需要严格授权 
	CQAPI(CQ_setGroupLeave, int)(
		int AuthCode,// 
		long long 群号,// 目标群 
		CQBOOL 是否解散// 真/解散本群 (群主) 假/退出本群 (管理、群成员) 
		);
	//置讨论组退出 Auth=140 
	CQAPI(CQ_setDiscussLeave, int)(
		int AuthCode,// 
		long long 讨论组号// 目标讨论组 
		);
	//置好友添加请求 Auth=150 
	CQAPI(CQ_setFriendAddRequest, int)(
		int AuthCode,// 
		const char * 请求反馈标识,// 请求事件收到的“反馈标识”参数 
		int 反馈类型,// #请求_通过 或 #请求_拒绝 
		const char * 备注// 添加后的好友备注 
		);
	//置群添加请求 Auth=151 
	CQAPI(CQ_setGroupAddRequest, int)(
		int AuthCode,// 
		const char * 请求反馈标识,// 请求事件收到的“反馈标识”参数 
		int 请求类型,// 根据请求事件的子类型区分 #请求_群添加 或 #请求_群邀请 
		int 反馈类型// #请求_通过 或 #请求_拒绝 
		);
	//置群添加请求 Auth=151 
	CQAPI(CQ_setGroupAddRequestV2, int)(
		int AuthCode,// 
		const char * 请求反馈标识,// 请求事件收到的“反馈标识”参数 
		int 请求类型,// 根据请求事件的子类型区分 #请求_群添加 或 #请求_群邀请 
		int 反馈类型,// #请求_通过 或 #请求_拒绝 
		const char * 理由// 操作理由，仅 #请求_群添加 且 #请求_拒绝 时可用
		);
	//增加运行日志 
	CQAPI(CQ_addLog, int)(
		int AuthCode,// 
		int 优先级,// #Log_ 开头常量 
		const char * 类型,// 
		const char * 内容// 
		);
	//置致命错误提示 
	CQAPI(CQ_setFatal, int)(
		int AuthCode,// 
		const char * 错误信息// 
		);
	//取群成员信息 (旧版,请用CQ_getGroupMemberInfoV2) Auth=130 
	CQAPI(CQ_getGroupMemberInfo, const char *)(
		int AuthCode,// 
		long long 群号,// 目标QQ所在群 
		long long QQID// 目标QQ 
		);
	//取群成员信息 (支持缓存) Auth=130 
	CQAPI(CQ_getGroupMemberInfoV2, const char *)(
		int AuthCode,// 
		long long 群号,// 目标QQ所在群 
		long long QQID,// 目标QQ 
		CQBOOL 不使用缓存
		);
	//取陌生人信息 (支持缓存) Auth=131 
	CQAPI(CQ_getStrangerInfo, const char *)(
		int AuthCode,// 
		long long QQID,// 目标QQ 
		CQBOOL 不使用缓存
        );
    //取群成员列表 Auth=160  
    CQAPI(CQ_getGroupMemberList, const char *)(
        int AuthCode,// 
        long long 群号// 目标QQ所在群
        );
	//取群列表 Auth=161  
	CQAPI(CQ_getGroupList, const char *)(
		int AuthCode
		);
	//撤回消息 Auth=180
	CQAPI(CQ_deleteMsg, int)(
		int AuthCode,
		long long MsgId
		);
}