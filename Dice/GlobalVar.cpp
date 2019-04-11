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
const unsigned short Dice_Build = 509;
const std::string Dice_Ver_Without_Build = "2.3.6";
const std::string DiceRequestHeader = "Dice/" + Dice_Ver_Without_Build;
const std::string Dice_Ver = Dice_Ver_Without_Build + "(" + std::to_string(Dice_Build) + ")";
const std::string Dice_Short_Ver = "Dice! by 溯洄 Version " + Dice_Ver;
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
	{"strDisabledErr", "命令无法执行:机器人已在此群中被关闭!"},
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
	{"strWelcomeMsgClearNotice", "已清除本群的入群欢迎词"},
	{"strWelcomeMsgClearErr", "错误:没有设置入群欢迎词，清除失败"},
	{"strWelcomeMsgUpdateNotice", "已更新本群的入群欢迎词"},
	{"strPermissionDeniedErr", "错误:此操作需要群主或管理员权限"},
	{"strSelfPermissionErr","本骰娘权限不够无能为力呢"},
	{"strNameTooLongErr", "错误:名称过长(最多为50英文字符)"},
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
	{"strGlobalOff","休假中，暂停服务×"},
	{"strPreserve", "本骰娘私有私用，勿扰勿怪"},
	{"strJrrp", "{0}今天的人品值是: {1}"},
	{"strJrrpErr", "JRRP获取失败! 错误信息: \n{0}"},
	{"strHlpMsg" , Dice_Short_Ver + "\n" +
	R"(请使用!dismiss [机器人QQ号]命令让机器人自动退群或讨论组！
跑团记录着色器: https://logpainter.kokona.tech
<通用命令>
.r [掷骰表达式*] [原因]			普通掷骰
.rs	[掷骰表达式*] [原因]			简化输出
.w/ww XaY						骰池
.set [1-99999之间的整数]			设置默认骰
.sc SC表达式** [San值]			自动Sancheck
.en [技能名] [技能值]			增强检定/幕间成长
.coc7/6 [个数]					COC7/6人物作成
.dnd [个数]					DND人物作成
.coc7/6d					详细版COC7/6人物作成
.ti/li					疯狂发作-临时/总结症状
.st [del/clr/show] [属性名] [属性值]		人物卡导入
.rc/ra [技能名] [技能值]		技能检定(规则书/房规)
.jrrp [on/off]				今日人品检定
.name [cn/jp/en] [个数]			生成随机名称
.rules [规则名称:]规则条目		规则查询
.draw [牌堆名称] ([抽牌数])		抽牌
.help						显示帮助
<仅限群/多人聊天>
.ri [加值] [昵称]			DnD先攻掷骰
.init [clr]					DnD先攻查看/清空
.nn [名称]					设置/删除昵称
.nnn [cn/jp/en]				随机设置昵称
.rh [掷骰表达式*] [原因]			暗骰,结果私聊发送
.bot [on/off] [机器人QQ号]		机器人开启或关闭
.ob [exit/list/clr/on/off]			旁观模式
.me on/off/动作				以第三方视角做出动作
.welcome 欢迎消息				群欢迎提示
<仅限私聊>
.me 群号 动作				以第三方视角做出动作
*COC7惩罚骰为P+个数,奖励骰为B+个数
 支持使用K来取较大的几个骰子
 支持使用 个数#表达式 进行多轮掷骰
**SC表达式为 成功扣San/失败扣San,如:1/1d6
**官方骰娘相关**/使用手册/插件交流/bug反馈/查看源代码请加QQ群941980833或624807593(已满))"}
};

std::map<std::string, std::string> EditedMsg;