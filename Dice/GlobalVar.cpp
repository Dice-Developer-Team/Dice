/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123溯洄
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "CQLogger.h"
#include "GlobalVar.h"
#include <map>

bool Enabled = false;

bool msgSendThreadRunning = false;

CQ::logger DiceLogger("Dice!");

/*
 * 版本信息
 * 请勿修改Dice_Build, Dice_Ver_Without_Build，DiceRequestHeader以及Dice_Ver常量
 * 请修改Dice_Short_Ver或Dice_Full_Ver常量以达到版本自定义
 */
const unsigned short Dice_Build = 532;
const std::string Dice_Ver_Without_Build = "2.3.8.2i";
const std::string DiceRequestHeader = "Dice/" + Dice_Ver_Without_Build;
const std::string Dice_Ver = Dice_Ver_Without_Build + "(" + std::to_string(Dice_Build) + ")";
const std::string Dice_Short_Ver = "Dice! by 溯洄 Shiki.Ver " + Dice_Ver;
#ifdef __clang__

#ifdef _MSC_VER
const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + __clang_version__ + " MSVC " + std::to_string(_MSC_FULL_VER) + " " + __DATE__ + " " + __TIME__ + "]";
#elif defined(__GNUC__)
const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + __clang_version__ + " GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__ + "]";
#else
const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + __clang_version__ + " UNKNOWN]"
#endif /*__clang__*/

#else

#ifdef _MSC_VER
const std::string Dice_Full_Ver = Dice_Short_Ver + " [MSVC " + std::to_string(_MSC_FULL_VER) + " " + __DATE__ + " " + __TIME__ + "]";
#elif defined(__GNUC__)
const std::string Dice_Full_Ver = Dice_Short_Ver + " [GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__ + "]";
#else
const std::string Dice_Full_Ver = Dice_Short_Ver + " [UNKNOWN COMPILER]"
#endif /*__clang__*/

#endif /*_MSC_VER*/

std::map<std::string, std::string> GlobalMsg
{
	{"strLinkLoss","时空连接已断开"},
	{"strLinked","已创建时空门"},
	{"strLinkWarning","尝试创建时空门，但不保证能否连通"},
	{"strLinkNotFound","时空门要通向不可名状的地方了×"},
	{"strNotMaster","你不是我的master！你想做什么？"},
	{"strHlpSet","已为{0}设置词条√"},
	{"strHlpReset","已清除{0}的词条，重启应用后重置默认词条√"},
	{"strHlpNameEmpty","Master想要自定义什么词条呀？"},
	{"strClockToWork","已按时启用"},
	{"strClockOffWork","已按时关闭"},
	{"strNameGenerator","{0}的随机名称:"},
	{"strDrawCard", "来看看{0}抽到了什么：{1}"},
	{"strHlpNotFound", "未找到指定的帮助信息×"},
	{"strMeOn","成功在这里启用.me命令√"},
	{"strMeOff","成功在这里禁用.me命令√"},
	{"strMeOnAlready","在这里.me命令没有被禁用!"},
	{"strMeOffAlready","在这里.me命令已经被禁用!"},
	{"strObOn","成功在这里启用旁观模式√"},
	{"strObOff","成功在这里禁用旁观模式√"},
	{"strObOnAlready","在这里旁观模式没有被禁用!"},
	{"strObOffAlready","在这里旁观模式已经被禁用!"},
	{"strObList","当前的旁观者有:"},
	{"strObListEmpty","当前暂无旁观者"},
	{"strObListClr","成功删除所有旁观者!"},
	{"strObEnter","成功加入旁观模式!"},
	{"strObExit","成功退出旁观模式!"},
	{"strObEnterAlready","已经处于旁观模式!"},
	{"strObExitAlready","没有加入旁观模式!"},
	{"strGroupIDEmpty","群号不能为空×"},
	{"strBlackGroup", "该群在黑名单中，如有疑问请联系master"},
	{"strBotOn","成功开启本机器人!"},
	{"strBotOff","成功关闭本机器人!"},
	{"strBotOnAlready","本机器人已经处于开启状态!"},
	{"strBotOffAlready","本机器人已经处于关闭状态!"},
	{"strRollCriticalSuccess","大成功！"},
	{"strRollExtremeSuccess","极难成功"},
	{"strRollHardSuccess","困难成功"},
	{"strRollRegularSuccess","成功"},
	{"strRollFailure","失败"},
	{"strRollFumble","大失败！"},
	{"strNumCannotBeZero", "无意义的数目！莫要消遣于我!"},
	{"strDeckNotFound", "没听说过呢……"},
	{"strDeckEmpty", "疲劳警告！已经什么也不剩了！"},
	{"strNameNumTooBig", "生成数量过多!请输入1-10之间的数字!"},
	{"strNameNumCannotBeZero", "生成数量不能为零!请输入1-10之间的数字!"},
	{"strSetInvalid", "无效的默认骰!请输入1-1000之间的数字!"},
	{"strSetTooBig", "这面数……让我丢个球啊!请输入1-1000之间的数字!"},
	{"strSetCannotBeZero", "默认骰不能为零!请输入1-1000之间的数字!"},
	{"strCharacterCannotBeZero", "人物作成次数不能为零!请输入1-10之间的数字!"},
	{"strSetInvalid", "无效的默认骰!请输入1-1000之间的数字!"},
	{"strSetTooBig", "默认骰过大!请输入1-1000之间的数字!"},
	{"strCharacterTooBig", "人物作成次数过多!请输入1-10之间的数字!"},
	{"strCharacterInvalid", "人物作成次数无效!请输入1-10之间的数字!"},
	{"strSCInvalid", "SC表达式输入不正确,格式为成功扣San/失败扣San,如1/1d6!"},
	{"strSanInvalid", "San值输入不正确,请输入1-99范围内的整数!"},
	{"strEnValInvalid", "技能值或属性输入不正确,请输入1-99范围内的整数!"},
	{"strSuccessRateErr","这成功率还需要检定吗？"},
	{"strGroupIDInvalid", "无效的群号!"},
	{"strSendErr", "消息发送失败!"},
	{"strSendSuccess","命令执行成功√"},
	{"strDisabledErr", "命令无法执行:机器人已在此群中被关闭!"},
	{"strActionEmpty","动作不能为空×"},
	{"strMEDisabledErr", "管理员已在此群中禁用.me命令!"},
	{"strDisabledMeGlobal", "恕不提供.me服务×"},
	{"strDisabledJrrpGlobal", "恕不提供.jrrp服务×"},
	{"strHELPDisabledErr", "管理员已在此群中禁用.help命令!"},
	{"strNameDelErr", "没有设置名称,无法删除!"},
	{"strValueErr", "掷骰表达式输入错误!"},
	{"strInputErr", "命令或掷骰表达式输入错误!"},
	{"strUnknownErr", "发生了未知错误!"},
	{"strUnableToGetErrorMsg", "无法获取错误信息!"},
	{"strDiceTooBigErr", "骰娘被你扔出的骰子淹没了"},
	{"strRequestRetCodeErr", "访问服务器时出现错误! HTTP状态码: {0}"},
	{"strRequestNoResponse", "服务器未返回任何信息"},
	{"strTypeTooBigErr", "哇!让我数数骰子有多少面先~1...2..."},
	{"strZeroTypeErr", "这是...!!时空裂(骰娘被骰子产生的时空裂缝卷走了)"},
	{"strAddDiceValErr", "你这样要让我扔骰子扔到什么时候嘛~(请输入正确的加骰参数:5-10之内的整数)"},
	{"strZeroDiceErr", "咦?我的骰子呢?"},
	{"strRollTimeExceeded", "掷骰轮数超过了最大轮数限制!"},
	{"strRollTimeErr", "异常的掷骰轮数"},
	{"strObPrivate", "你想看什么呀？"},
	{"strDismissPrivate", "滚！"},
	{"strWelcomePrivate", "你在这欢迎谁呢？"},
	{"strWelcomeMsgClearNotice", "已清除本群的入群欢迎词"},
	{"strWelcomeMsgClearErr", "错误:没有设置入群欢迎词，清除失败"},
	{"strWelcomeMsgUpdateNotice", "已更新本群的入群欢迎词"},
	{"strPermissionDeniedErr", "错误:此操作需要群主或管理员权限"},
	{"strSelfPermissionErr","本骰娘权限不够无能为力呢"},
	{"strNameTooLongErr", "错误:名称过长(最多为50英文字符)"},
	{"strNameClr","已将{0}的名称删除"},
	{"strNameSet","已将{0}的名称更改为{1}"},
	{"strUnknownPropErr", "错误:属性不存在"},
	{"strEmptyWWDiceErr", "格式错误:正确格式为.w(w)XaY!其中X≥1, 5≤Y≤10"},
	{"strPropErr", "请认真的输入你的属性哦~"},
	{"strSetPropSuccess", "属性设置成功"},
	{"strPropCleared", "已清除所有属性"},
	{"strRuleReset","已重置默认规则"},
	{"strRuleSet","已设置默认规则"},
	{"strRuleErr", "规则数据获取失败,具体信息:\n"},
	{"strRulesFailedErr", "请求失败,无法连接数据库"},
	{"strPropDeleted", "属性删除成功"},
	{"strPropNotFound", "错误:属性不存在"},
	{"strRuleNotFound", "未找到对应的规则信息"},
	{"strProp", "{0}的{1}属性值为{2}"},
	{"strStErr", "格式错误:请参考帮助文档获取.st命令的使用方法"},
	{"strRulesFormatErr", "格式错误:正确格式为.rules[规则名称:]规则条目 如.rules COC7:力量"},
	{"strNoDiscuss", "本骰娘现不支持讨论组服务，即将退出"},
	{"strGroupClr", "本骰娘清退中，即将退群"},
	{"strOverdue","{0}已经在这里被放置{1}天啦，马上就会离开这里了"},
	{"strGlobalOff","休假中，暂停服务×"},
	{"strPreserve", "本骰娘私有私用，勿扰勿怪"},
	{"strJrrp", "{0}今天的人品值是: {1}"},
	{"strJrrpErr", "JRRP获取失败! 错误信息: \n{0}"},
	{"strAddGroup", R"(欢迎使用本骰娘！
请使用.dismiss 机器人QQ号 使机器人退群退讨论组
.bot on/off 机器人QQ号 //机器人开启或关闭
.help协议 确认服务协议
.help指令 查看指令列表
.help设定 确认骰娘设定
千万别想当然以为本骰娘和别的骰娘一样！)" },
	{"strSelfName", "本骰娘"},
	{"strBotMsg", "\n使用.help更新 查看更新内容"},
	{"strHlpMsg" , Dice_Short_Ver + "\n" +
	R"(请使用.dismiss 机器人QQ号 使机器人退群退讨论组
.bot on/off 机器人QQ号 //机器人开启或关闭
.help协议 确认服务协议
.help指令 查看指令列表
.help设定 确认骰娘设定
查看源码:https://github.com/mystringEmpty/Dice
官方文档:https://www.stringempty.xyz
跑团记录着色器: https://logpainter.kokona.tech
插件交流/bug反馈请加QQ群192499947)"}
};

std::map<std::string, std::string> EditedMsg;
std::map<std::string, std::string> HelpDoc = {
{"更新","532：更新了消息转发功能（当前Master专用）\n531：更新了塔罗牌牌阵\n530：允许自定义帮助文档并修复自定义回复的bug\n529：更新master每日定时开关功能，修复掷骰轮数bug\n528：更新.help帮助功能，允许后接参数\n527：更新功能转向追踪Shiki的更新\n526：允许乘法表达式\n525：修复了.draw抽不完最后一张的bug"},
{"协议","0.本协议是Shiki(Death、Judgement、The World)的服务协议，不代表同类插件服务有一致的协议，请注意。\n1.邀请骰娘、使用掷骰服务和在群内阅读此协议视为同意并承诺遵守此协议，否则请使用.dismiss移出骰娘。\n2.不允许禁言、移出骰娘或刷屏掷骰等对骰娘的不友善行为，这些行为将会提高骰娘被制裁的风险。开关骰娘响应请使用.bot on/off。\n3.骰娘默认邀请行为已事先得到群内同意，因而会自动同意群邀请。因擅自邀请而使骰娘遭遇不友善行为时，邀请者因未履行预见义务而将承担连带责任。\n4.禁止将骰娘用于赌博及其他违法犯罪行为。\n5.对于设置敏感昵称等无法预见但有可能招致言论审查的行为，骰娘可能会出于自我保护而拒绝提供服务\n6.由于技术以及资金原因，我们无法保证机器人100%的时间稳定运行，可能不定时停机维护或遭遇冻结，但是相应情况会及时通过各种渠道进行通知，敬请谅解。临时停机的骰娘不会有任何响应，故而不会影响群内活动，此状态下仍然禁止不友善行为。\n7.对于违反协议的行为，骰娘将视情况终止对用户和所在群提供服务，并将不良记录共享给其他服务提供方。黑名单相关事宜可以与服务提供方协商，但最终裁定权在服务提供方。\n8.本协议内容随时有可能改动。请注意帮助信息、签名、空间、官方群等处的骰娘动态。\n9.骰娘提供掷骰服务是完全免费的，欢迎投食。\n10.本服务最终解释权归服务提供方所有。"},
{"作者","Copyright (C) 2018-2019 w4123溯洄\nCopyright (C) 2019 String.Empty"},
{"指令","掷骰指令包括:\n.dismiss 退群\n.bot 开关\n.welcome 入群欢迎\n.rules 规则速查\n.r 掷骰\n.ob 旁观模式\n.set 设置默认骰\n.name 随机姓名\n.nn 设置昵称\n.coc COC人物作成\n.dnd DND人物作成\n.st 角色卡设置\n.rc/ra 检定\n.sc 理智检定\n.en 成长检定\n.ri 先攻\n.init 先攻列表\n.ww 骰池\n.me 第三人称动作\n.jrrp 今日人品\n.group ban 群员禁言\n.group state 本群现状\n.draw 抽牌\nat骰娘后接指令可以指定骰娘单独响应，如at骰娘.bot off\n请.help对应指令 获取详细信息\n为了避免未预料到的指令误判，请尽可能在参数之间使用空格"},
{"退群","&dismiss"},
{"退群指令","&dismiss"},
{"dismiss","该指令需要群管理员权限，使用后即退出群聊\n!dismiss [目标QQ(完整或末四位)]指名退群\n!dismiss无视内置黑名单和静默状态，只要插件开启总是有效"},
{"开关","&bot"},
{"bot",".bot on/off开启/静默骰子（限群管理）\n.bot无视静默状态，只要插件开启且不在黑名单总是有效"},
{"规则速查","&rules"},
{"规则","&rules"},
{"rules ","规则速查：.rules ([规则]):[待查词条] 或.rules set [规则]\n.rules 跳跃 //复数规则有相同词条时，择一返回\n.rules COC：大失败 //coc默认搜寻coc7的词条,dnd默认搜寻3r\n.rules dnd：语言\n.rules set dnd //设置后优先查询dnd同名词条，无参数则清除设置"},
{"掷骰","&r"},
{"rd","&r"},
{"r","掷骰：.r [掷骰表达式] ([掷骰原因]) [掷骰表达式]：([掷骰轮数]#)[骰子个数]d骰子面数(p[惩罚骰个数])(k[取点数最大的骰子数])不带参数时视为掷一个默认骰\n合法参数要求掷骰轮数1-10，奖惩骰个数1-9，个数范围1-100，面数范围1-1000\n.r3#d\t//3轮掷骰\n.rh心理学 暗骰\n.rs1D10+1D6+3 沙鹰伤害\t//省略单个骰子的点数，直接给结果\n现版本开头的r均可用o或d代替，但群聊中.ob会被识别为旁观指令"},
{"暗骰","群聊限定，掷骰指令后接h视为暗骰，结果将私发本人和群内ob的用户\n为了保证发送成功，请加骰娘好友"},
{"旁观","&ob"},
{"旁观模式","&ob"},
{"ob","旁观模式：.ob (exit/list/clr/on/off)\n.ob\t//加入旁观可以看到他人暗骰结果\n.ob exit\t//退出旁观模式\n.ob list\t//查看群内旁观者\n.ob clr\t//清除所有旁观者\n.ob on\t//全群允许旁观模式\n.ob off\t//禁用旁观模式\n暗骰与旁观仅在群聊中有效"},
{"默认骰","&set"},
{"set","当表达式中‘D’之后没有接面数时，视为投掷默认骰\n.set20 将默认骰设置为20\n.set 不带参数视为将默认骰重置为默认的100"},
{"个位骰","个位骰有十面，为0~9十个数字，百面骰的结果为十位骰与个位骰之和（但00+0时视为100）"},
{"十位骰","十位骰有十面，为00~90十个数字，百面骰的结果为十位骰与个位骰之和（但00+0时视为100）"},
{"奖励骰","&奖励/惩罚骰"},
{"惩罚骰","&奖励/惩罚骰"},
{"奖惩骰","&奖励/惩罚骰"},
{"奖励/惩罚骰","COC中奖励/惩罚骰是额外投掷的十位骰，最终结果选用点数更低/更高的结果（不出现大失败的情况下等价于更小/更大的十位骰）\n.rb2 2个奖励骰\nrcp 射击 1个惩罚骰"},
{"随机姓名","&name"},
{"name","随机姓名：.name (cn/jp/en)([生成数量])\n.name 10\t//默认三类名称随机生成\n.name en\t//后接cn/jp/en则限定生成中文/日文/英文名\n名称生成个数范围1-10，太多容易发现姓名库的寒酸"},
{"设置昵称","&nn"},
{"昵称","&nn"},
{"nn","设置昵称：.nn [昵称] / .nn / .nnn(cn/jp/en) \n.nn kp\t//昵称前的./！等符号会被自动忽略\n.nn\t//视为删除昵称\n.nnn\t//设置为随机昵称\n.nnn jp\t/设置限定随机昵称\n私聊.nn视为操作全局昵称\n优先级：群昵称>全局昵称>群名片>QQ昵称"},
{"人物作成","该版本人物作成支持COC7(.coc、.draw调查员背景/英雄天赋)、COC6(.coc6、.draw煤气灯)、DND(.dnd)、AMGC(.draw AMGC)"},
{"coc","克苏鲁的呼唤(COC)人物作成：.coc([7/6])(d)([生成数量])\n.coc 10\t//默认生成7版人物\n.coc6d\t//接d为详细作成，一次只能作成一个\n仅用作骰点法人物作成，可应用变体规则，参考.rules创建调查员的其他选项"},
{"dnd","龙与地下城(DND)人物作成：.dnd([生成数量])\n.dnd 5\t//仅作参考，可自行应用变体规则"},
{"角色卡设置","&st"},
{"st","角色卡设置：.st (del/clr/show) ([属性名]) ([属性值])\n目前用户的人物卡是所有群互通的，因此无法在pl多开时使用同一个骰子设定属性\n.st力量:50 体质:55 体型:65 敏捷:45 外貌:70 智力:75 意志:35 教育:65 幸运:75\n.st del kp裁决\t//删除已保存的属性\n.st clr\t//清空人物卡\n.st show 灵感\t//查看指定人物属性\n.st show\t//无参数时查看所有属性，请使用只st加点过技能的半自动人物卡！\n部分COC属性会被视为同义词，如智力/灵感、理智/san、侦查/侦察"},
{"rc","&rc/ra"},
{"ra","&rc/ra"},
{"检定","&rc/ra"},
{"rc/ra","检定指令：.rc/ra [属性名]([成功率])\n角色卡设置了属性时，可省略成功率\n.rc体质 重伤检定\n.rc kp裁决 99\n.rc 敏捷-10\t//修正后成功率必须在1-1000内\n.rcp 手枪	\t//奖惩骰至多9个\n默认使用的房规将在之后版本被移除"},
{"san check","&sc"},
{"理智检定","&sc"},
{"sc","San Check指令：.sc[成功损失]/[失败损失] ([当前san值])\n已经.st了理智/san时，可省略最后的参数\n.sc0/1 70\n.sc1d10/1d100 直面外神\n大失败自动失去最大值\n当调用角色卡san时，san会自动更新为sc后的剩余值\n程序上可以损失负数的san，也就是可以用.sc-1d6/-1d6来回复san，但请避免这种奇怪操作"},
{"ti","&ti/li"},
{"li","&ti/li"},
{"疯狂症状","&ti/li"},
{"ti/li","疯狂症状：\n.ti 临时疯狂症状\n.li 总结疯狂症状\n适用coc7版规则，6版请自行用百面骰配合查表"},
{"成长检定","&en"},
{"增强检定","&en"},
{"en","成长检定：.en [技能名称]([技能值])\n已经.st时，可省略最后的参数\n.en 教育 60 教育增强\t//用法见.rules\n.en 幸运 幸运成长\t//调用人物卡属性时，成长后的值会自动更新"},
{"抽牌","&draw"},
{"draw","抽牌：.draw [牌堆名称] ([抽牌数量])\t//抽到的牌不放回，抽牌数量不能超过牌堆数量\n当前可用牌堆名：硬币/东方角色/麻将/扑克花色/扑克\n调查员职业/调查员背景/英雄天赋/煤气灯/个人描述/思想信念/重要之人/重要之人理由/意义非凡之地/宝贵之物/调查员特点/即时症状/总结症状/恐惧症状/狂躁症状/\n阵营/哈罗花色/冒险点子/\n人偶暗示/人偶宝物/人偶记忆碎片/\nAMGC/AMGC身材/AMGC专精/AMGC武器/AMGC套装/AMGC才能/AMGC特技1/AMGC特技2/AMGC特技3\n/塔罗牌/正逆/塔罗牌占卜/单张塔罗牌/圣三角牌阵/四要素牌阵/小十字牌阵/六芒星牌阵/凯尔特十字牌阵\n.help审判正位（牌+方向）可获取塔罗牌解读"},
{"先攻","&ri"},
{"ri","先攻（群聊限定）：.ri([加值])([昵称])\n.ri -1 某pc\t//自动记入先攻列表\n.ri +5 boss"},
{"先攻列表","&init"},
{"init","先攻列表：\n.init\t//查看先攻列表\n.init clr\t//清除先攻列表"},
{"骰池","&ww"},
{"ww","骰池：.w(w) [骰子个数]a[加骰参数]\n.w会直接给出结果而.ww会给出每个骰子的点数\n固定10面骰，每有一个骰子点数达到加骰参数，则加骰一次，最后计算点数达到8的骰子数\n具体用法请参考相关游戏规则"},
{"第三人称","&me"},
{"第三人称动作","&me"},
{"me","第三人称动作：.me([群号])[动作].me 笑出了声\t//仅限群聊使用\n.me 941980833 抱群主\t//仅限私聊使用，此命令可伪装成骰子在群里说话\n.me off\t//群内禁用.me\n.me on\t//群内启用.me"},
{"今日人品","&jrrp"},
{"jrrp","今日人品：.jrrp\n.jrrp off\t//群内禁用jrrp\n.jrrp on\t//群内启用jrrp\n一天一个人品值\n2.3.5版本后随机值为均匀分布\n骰娘只负责从溯洄的服务器搬运结果，请勿无能狂怒\n如何发配所有人的命运，只有孕育万千骰娘生机之母，萌妹吃鱼之神，正五棱双角锥体对的监护人，一切诡秘的窥见者，时空舞台外的逆流者，永转的命运之轮——溯洄本体掌握"},
{"入群欢迎","&welcome"},
{"入群欢迎词","&welcome"},
{"welcome","入群欢迎词：.welcome\n.welcome {@}欢迎{nick}入群！\t//{@}视为at入群者，{nick}会替换为新人的昵称\n.welcome\t//不带参数时清除欢迎词\n无论开关状态，只要有欢迎词时有人入群，都会响应"},
{"群禁言","群禁言：.group ban [禁言对象QQ/at对象] [时长表达式（分钟）]	//时长小于0时视为解除禁言"},
{"本群现状","查看在群内对骰娘的设置"},
{"master","插件管理功能请在主页查看Master手册:www.stringempty.xyz"},
{"溯洄","孕育万千骰娘生机之母，萌妹吃鱼之神，正五棱双角锥体对的监护人，一切诡秘的窥见者，拟人者主宰，时空舞台外的逆流者，永转的命运之轮"},
{"投食","投食Shiki，请选择http://shiki.stringempty.xyz/投食\n没有中间商赚差价，请选择http://docs.kokona.tech/zh/latest/About.html"},
{"愚者正位","憧憬自然的地方、毫无目的地前行、喜欢尝试挑战新鲜事物、四处流浪。美好的梦想。"},
{"愚者逆位","冒险的行动，追求可能性，重视梦想，无视物质的损失，离开家园，过于信赖别人，为出外旅行而烦恼。心情空虚、轻率的恋情、无法长久持续的融洽感、不安的爱情的旅程、对婚姻感到束缚、彼此忽冷忽热、不顾众人反对坠入爱河、为恋人的负心所伤、感情不专一。"},
{"魔术师正位","事情的开始，行动的改变，熟练的技术及技巧，贯彻我的意志，运用自然的力量来达到野心。"},
{"魔术师逆位","意志力薄弱，起头难，走入错误的方向，知识不足，被骗和失败。"},
{"女祭司正位","开发出内在的神秘潜力，前途将有所变化的预言，深刻地思考，敏锐的洞察力，准确的直觉。"},
{"女祭司逆位","过于洁癖，无知，贪心，目光短浅，自尊心过高，偏差的判断，有勇无谋，自命不凡。"},
{"女皇正位","幸福，成功，收获，无忧无虑，圆满的家庭生活，良好的环境，美貌，艺术，与大自然接触，愉快的旅行，休闲。"},
{"女皇逆位","不活泼，缺乏上进心，散漫的生活习惯，无法解决的事情，不能看到成果，担于享乐，环境险恶，与家人发生纠纷。"},
{"皇帝正位","光荣，权力，胜利，握有领导权，坚强的意志，达成目标，父亲的责任，精神上的孤单。"},
{"皇帝逆位","幼稚，无力，独裁，撒娇任性，平凡，没有自信，行动力不足，意志薄弱，被支配。"},
{"教皇正位","援助，同情，宽宏大量，可信任的人给予的劝告，良好的商量对象，得到精神上的满足，遵守规则，志愿者。"},
{"教皇逆位","错误的讯息，恶意的规劝，上当，援助被中断，愿望无法达成，被人利用，被放弃。"},
{"恋人正位","撮合，爱情，流行，兴趣，充满希望的未来，魅力，增加朋友。"},
{"恋人逆位","禁不起诱惑，纵欲过度，反覆无常，友情变淡，厌倦，争吵，华丽的打扮，优柔寡断。"},
{"战车正位","努力而获得成功，胜利，克服障碍，行动力，自立，尝试，自我主张，年轻男子，交通工具，旅行运大吉。"},
{"战车逆位","争论失败，发生纠纷，阻滞，违返规则，诉诸暴力，顽固的男子，突然的失败，不良少年，挫折和自私自利。"},
{"力量正位","大胆的行动，有勇气的决断，新发展，大转机，异动，以意志力战胜困难，健壮的女人。"},
{"力量逆位","胆小，输给强者，经不起诱惑，屈服在权威与常识之下，没有实践便告放弃，虚荣，懦弱，没有耐性。"},
{"隐者正位","隐藏的事实，个别的行动，倾听他人的意见，享受孤独，有益的警戒，年长者，避开危险，祖父，乡间生活。"},
{"隐者逆位","憎恨孤独，自卑，担心，幼稚思想，过于慎重导致失败，偏差，不宜旅行。"},
{"命运之轮正位","关键性的事件，有新的机会，因的潮流，环境的变化，幸运的开端，状况好转，问题解决，幸运之神降临。"},
{"命运之轮逆位","边疆的不行，挫折，计划泡汤，障碍，无法修正方向，往坏处发展，恶性循环，中断。"},
{"正义正位","公正、中立、诚实、心胸坦荡、表里如一、身兼二职、追求合理化、协调者、与法律有关、光明正大的交往、感情和睦。"},
{"正义逆位","失衡、偏见、纷扰、诉讼、独断专行、问心有愧、无法两全、表里不一、男女性格不合、情感波折、无视社会道德的恋情。"},
{"倒吊人正位","接受考验、行动受限、牺牲、不畏艰辛、不受利诱、有失必有得、吸取经验教训、浴火重生、广泛学习、奉献的爱。"},
{"倒吊人逆位","无谓的牺牲、骨折、厄运、不够努力、处于劣势、任性、利己主义者、缺乏耐心、受惩罚、逃避爱情、没有结果的恋情。"},
{"死神正位","失败、接近毁灭、生病、失业、维持停滞状态、持续的损害、交易停止、枯燥的生活、别离、重新开始、双方有很深的鸿沟、恋情终止。"},
{"死神逆位","抱有一线希望、起死回生、回心转意、摆脱低迷状态、挽回名誉、身体康复、突然改变计划、逃避现实、斩断情丝、与旧情人相逢。"},
{"节制正位","单纯、调整、平顺、互惠互利、好感转为爱意、纯爱、深爱。"},
{"节制逆位","消耗、下降、疲劳、损失、不安、不融洽、爱情的配合度不佳。"},
{"恶魔正位","被束缚、堕落、生病、恶意、屈服、欲望的俘虏、不可抗拒的诱惑、颓废的生活、举债度日、不可告人的秘密、私密恋情。"},
{"恶魔逆位","逃离拘束、解除困扰、治愈病痛、告别过去、暂停、别离、拒绝诱惑、舍弃私欲、别离时刻、爱恨交加的恋情。"},
{"塔正位","破产、逆境、被开除、急病、致命的打击、巨大的变动、受牵连、信念崩溃、玩火自焚、纷扰不断、突然分离，破灭的爱。"},
{"塔逆位","困境、内讧、紧迫的状态、状况不佳、趋于稳定、骄傲自大将付出代价、背水一战、分离的预感、爱情危机。"},
{"星星正位","前途光明、充满希望、想象力、创造力、幻想、满足愿望、水准提高、理想的对象、美好的恋情。"},
{"星星逆位","挫折、失望、好高骛远、异想天开、仓皇失措、事与愿违、工作不顺心、情况悲观、秘密恋情、缺少爱的生活。"},
{"月亮正位","不安、迷惑、动摇、谎言、欺骗、鬼迷心窍、动荡的爱、三角关系。"},
{"月亮逆位","逃脱骗局、解除误会、状况好转、预知危险、等待、正视爱情的裂缝。"},
{"太阳正位","活跃、丰富的生命力、充满生机、精力充沛、工作顺利、贵人相助、幸福的婚姻、健康的交际。"},
{"太阳逆位","消沉、体力不佳、缺乏连续性、意气消沉、生活不安、人际关系不好、感情波动、离婚。"},
{"审判正位","复活的喜悦、康复、坦白、好消息、好运气、初露锋芒、复苏的爱、重逢、爱的奇迹。"},
{"审判逆位","一蹶不振、幻灭、隐瞒、坏消息、无法决定、缺少目标、没有进展、消除、恋恋不舍。"},
{"世界正位","完成、成功、完美无缺、连续不断、精神亢奋、拥有毕生奋斗的目标、完成使命、幸运降临、快乐的结束、模范情侣。"},
{"世界逆位","未完成、失败、准备不足、盲目接受、一时不顺利、半途而废、精神颓废、饱和状态、合谋、态度不够融洽、感情受挫。"},
};
std::map<std::string, std::string> EditedHelpDoc;