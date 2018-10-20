#pragma once

// 调试       灰色
#define Log_Debug 0
// 信息       黑色
#define Log_Info 10
// 信息(成功) 紫色
#define Log_InfoSuccess 11
// 信息(接收) 蓝色
#define Log_InfoRecv 12
// 信息(发送) 绿色
#define Log_InfoSend 13
// 警告       橙色
#define Log_Warning 20
// 错误       红色
#define Log_Error 30
// 致命错误   深红
#define Log_Fatal 40

// 拦截此条消息，不再传递给其他应用
//注意：应用优先级设置为最高(10000)时，不得使用本返回值
#define Msg_Blocked 1
// 将此消息继续传递给其他应用
#define Msg_Ignored 0

#define FloatingWindows_Green 1
#define FloatingWindows_Orange 2
#define FloatingWindows_Red 3
#define FloatingWindows_DeepRed 4
#define FloatingWindows_Black 5
#define FloatingWindows_Grey 6

#define RequestAccepted 1
#define RequestRefused 2

#define RequestGroupAdd 1
#define RequestGroupInvite 2
