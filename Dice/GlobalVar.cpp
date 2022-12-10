/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2022 String.Empty
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
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include "GlobalVar.h"
#include "MsgFormat.h"
#include "DiceExtensionManager.h"
#include "DiceMod.h"

bool Enabled = false;

std::string Dice_Full_Ver_On;

std::unique_ptr<ExtensionManager> ExtensionManagerInstance;

#ifdef _WIN32
HMODULE hDllModule = nullptr;
#endif

bool msgSendThreadRunning = false;

const dict_ci<string> PlainMsg
{
	{"strParaEmpty","参数不能为空×"},			//偷懒用万能回复
	{"strParaIllegal","参数非法×"},			//偷懒用万能回复
	{"stranger","陌生人"},			//{nick}无法获取非空昵称时的称呼
	{"strCallUser", "用户"},
	{"strSummonWord", ""},
	{"strSummonEmpty", "召唤{self}，{nick}有何事么？"},
	{"strModList", "{self}的模块加载列表:{li}"},
	{"strModOn", "已令{self}人格激活记忆体「{mod}」√"},
	{"strModOnAlready", "{self}的记忆体「{mod}」已激活！"},
	{"strModOff", "已将记忆体「{mod}」弹出{self}人格√"},
	{"strModOffAlready", "{self}的记忆体「{mod}」已停用！"},
	{"strModReload", "已为{self}人格重铸记忆体「{mod}」√"},
	{"strModDelete", "已将记忆体「{mod}」从{self}的人格排空√"},
	{"strModNameEmpty", "请{nick}输入模块名！"},
	{"strModDescLocal", "{self}已装载模块:{mod_desc}"},
	{"strModDescCloud", "{self}已获取源模块信息:{mod_desc}"},
	{"strModDetail", "{self}所载模块详细信息:{mod_detail}"},
	{"strModInstalled", "{self}成功注入记忆体「{mod}」{mod_ver}√"},
	{"strModInstallErr", "{self}注入记忆体「{mod}」失败×{err}"},
	{"strModUpdated", "更新{self}记忆体「{mod}」{ex_ver}->{mod_ver}√"},
	{"strModUpdateErr", "更新{self}记忆体「{mod}」失败×{err}"},
	{"strModReinstalled", "成功重写{self}记忆体「{mod}」{ex_ver}->{mod_ver}√"},
	{"strModLoadErr", "{self}读取模块「{mod}」失败×{err}"},
	{"strModNotFound", "{self}未找到指定模块「{mod}」!"},
	{"strAkForkNew","{self}已创建分歧\n#{fork}"},
	{"strAkAdd","{self}已加入新选项√\n当前分歧:{fork}{li}"},
	{"strAkAddEmpty","给完{nick}的选项，就像{self}没看到这句话之前一样×"},
	{"strAkDel","{self}已删除指定选项√\n当前分歧:{fork}{li}"},	
	{"strAkOptEmptyErr","备选项为空×\n{nick}想让{self}编出一个选项吗？"},
	{"strAkNumErr","请{nick}选择合法的选项序号×"},
	{"strAkGet","#{fork}{li}\n\n{self}的选择：{get}"},
	{"strAkShow","{self}的当前分歧:{fork} {li}"},
	{"strAkClr","{self}已清除本轮分歧{fork}√"},
	{"strLogNew","{self}已新开记录日志{log_name}√\n请适时用.log off暂停或.log end完成记录"},
	{"strLogOn","{self}开始日志记录{log_name}√\n可使用.log off暂停记录"},
	{"strLogOnAlready","{self}正在记录中！"},
	{"strLogOff","{self}已暂停日志记录√\n可使用.log on恢复记录"},
	{"strLogOffAlready","{self}已经暂停记录！"},
	{"strLogEnd","{self}已完成日志记录√\n正在上传日志文件{log_file}"},
	{"strLogEndEmpty","{self}已结束记录√\n本次无日志产生"},
	{"strLogNullErr","{self}无日志记录或已结束！"},
	{"strLogUpSuccess","{self}已完成日志上传√\n请访问 {log_url} 以查看记录"},
	{"strLogUpFailure","{self}上传日志文件失败，正在第{retry}次重传{log_file}…{ret}"},
	{"strLogUpFailureEnd","很遗憾，{self}无法成功上传日志文件×\n{ret}\n如需获取可联系Master:{print:master}\n文件名:{log_file}"},
	{"strGMTableShow","{self}记录的{table_name}列表: {res}"},
	{"strGMTableClr","{self}已清除{table_name}表√"},
	{"strGMTableItemDel","{self}已移除{table_name}表的项目{table_item}√"},
	{"strGMTableNotExist","{self}没有保存{table_name}表×"},
	{"strGMTableItemNotFound","{self}没有找到{table_name}表的项目{table_item}×"},
	{"strGMTableItemEmpty","请告知{self}待移除的{table_name}列表项目×"},
	{"strUserTrustShow","{user}在{self}处的信任级别为{trust}"},
	{"strUserTrusted","已将{self}对{user}的信任级别调整为{trust}"},
	{"strUserTrustDenied","{nick}在{self}处无权访问对方的权限×"},
	{"strUserTrustIllegal","将目标权限修改为{trust}是非法的×"},
	{"strUserNotFound","{self}无{user}的用户记录"},
	{"strGroupAuthorized","A roll to the table turns to a dice fumble!\nDice Roller {strSelfName}√\n本群已授权许可，请尽情使用本骰娘√\n请遵守协议使用，服务结束后使用.dismiss送出!" },
	{"strGroupLicenseDeny","本群未获{self}许可使用，自动在群内静默。\n请先.help协议 阅读并同意协议后向运营方申请许可使用，\n否则请管理员使用!dismiss送出{self}\n可按以下格式填写并发送申请:\n!authorize 申请用途:[ **请写入理由** ] 我已了解Dice!基本用法，仔细阅读并保证遵守{strSelfName}的用户协议，如需停用指令使用[ **请写入指令** ]，用后使用[ **请写入指令** ]送出群" },
	{"strGroupLicenseApply","此群未通过自助授权×\n许可申请已发送√" },
	{"strGroupSetOn","现已开启{self}在此群的“{option}”选项√"},			//群内开关和遥控开关通用此文本
	{"strGroupSetOnAlready","{self}已在此群设置了{option}！"},			
	{"strGroupSetOff","现已关闭{self}在此群的“{option}”选项√"},			
	{"strGroupSetOffAlready","{self}未在此群设置{option}！"},
	{"strGroupMultiSet","{self}已将此群的选项修改为:{opt_list}"},
	{"strGroupSetAll","{self}已修改记录中{cnt}个群的“{option}”选项√"},
	{"strGroupDenied","{nick}在{self}处无权访问此群的设置×"},
	{"strGroupSetDenied","{nick}在{self}处设置{option}的权限不足×"},
	{"strGroupSetInvalid","{nick}尝试设置无效的群词条{option}×"},
	{"strGroupSetNotExist","{self}无{option}此选项×"},
	{"strGroupWholeUnban","{self}已关闭全局禁言√"},
	{"strGroupWholeBan","{self}已开启全局禁言√"},
	{"strGroupWholeBanErr","{self}开启全局禁言失败×"},
	{"strGroupUnban","{self}裁定:{member}解除禁言√"},
	{"strGroupBan","{self}裁定:{member}禁言{res}分钟√"},
	{"strGroupNotFound","{self}无群{group_id}记录×"},
	{"strGroupNot","{group}不是群！"},
	{"strGroupNotIn","{self}当前不在{group}内×"},
	{"strGroupExit","{self}已退出该群√"},
	{"strGroupCardSet","{self}已将{target}的群名片修改为「{card}」√"},
	{"strGroupTitleSet","{self}已将{target}的头衔修改为「{title}」√"},
	{"strPcNewEmptyCard","已为{nick}新建{type}空白卡「{char}」√"},
	{"strPcNewCardShow","已为{nick}新建{type}卡「{char}」：{show}"},//由于预生成选项而存在属性
	{"strPcCardSet","已将{nick}当前角色卡绑定为「{char}」√"},//{nick}-用户昵称 {pc}-原角色卡名 {char}-新角色卡名
	{"strPcCardReset","已解绑{nick}当前的默认卡√"},//{nick}-用户昵称 {pc}-原角色卡名
	{"strPcCardRename","已将{old_name}重命名为「{new_name}」√"},
	{"strPcCardDel","已将角色卡「{char}」删除√"},
	{"strPcCardCpy","已将「{char2}」的属性复制到「{char1}」√"},
	{"strPcClr","已清空{nick}的角色卡记录√"},
	{"strPcCardList","{nick}的角色列表：{show}"},
	{"strPcCardBuild","{nick}的「{char}」生成：{show}"},
	{"strPcCardShow","{nick}的<{type}>{char}：{show}"},	//{nick}-用户昵称 {type}-角色卡类型 {char}-角色卡名
	{"strPcCardRedo","{nick}的「{char}」重新生成：{show}"},
	{"strPcGroupList","{nick}的各群角色列表：{show}"},
	{"strPcStatShow","{pc}在{self}处的骰点统计:{stat}"},
	{"strPcStatEmpty","{pc}在{self}处还没有检定被记录的样子×"},
	{"strPcNotExistErr","{self}无{nick}的角色卡记录，无法删除×"},
	{"strPcCardFull","{nick}在{self}处的角色卡已达上限，请先清理多余角色卡×"},
	{"strPcTempChange","{self}已将{pc}的模板切换为{new_tpye}×"},
	{"strPcTempInvalid","{self}无法识别的角色卡模板×"},
	{"strPcNameEmpty","名称不能为空×"},
	{"strPcNameExist","{nick}已存在同名卡×"},
	{"strPcNameNotExist","{nick}无该名称角色卡×"},
	{"strPcNameInvalid","非法的角色卡名（存在冒号）×"},
	{"strPcInitDelErr","{nick}的初始卡不可删除×"},
	{"strPcNoteTooLong","备注长度不能超过255×"},
	{"strPcTextTooLong","文本长度不能超过255×"},
	{"strSetDefaultDice","{self}已将{pc}的默认骰设置为D{default}√"},
	{"strCOCBuild","{pc}的调查员作成:{res}"},
	{"strDNDBuild","{pc}的英雄作成:{res}"},
	{"strCensorCaution","提醒：{nick}的指令包含敏感词，{self}已上报"},
	{"strCensorWarning","警告：{nick}的指令包含敏感词，{self}已记录并上报！"},
	{"strCensorDanger","警告：{nick}的指令包含敏感词，{self}拒绝指令并已上报！"},
	//{"strCensorCritical","警告：{nick}的指令包含敏感词，{self}已记录并上报！"},
	{"strSpamFirstWarning","你短时间内对{self}指令次数过多！请善用多轮掷骰和复数生成指令（刷屏初次警告）"},
	{"strSpamFinalWarning","请暂停你的一切指令，避免因高频指令被{self}拉黑！（刷屏最终警告）"},
	{"strRegexInvalid","正则表达式{key}无效: {err}"},
	{"strReplyOn","{self}现在本群启用关键词回复√"},
	{"strReplyOff","{self}现在本群禁用关键词回复√"},
	{"strReplySet","{self}已设置关键词回复条目{key}√"},
	{"strReplyShow","{self}的关键词条目{key}为:{show}"},
	{"strReplyList","{self}的回复触发词共有:{res}"},
	{"strReplyDel","{self}已移除回复关键词条目{key}√"},
	{"strReplyKeyEmpty","{nick}请输入回复关键词×"},
	{"strReplyKeyNotFound","{self}未找到回复关键词{key}×"},
	{"strReplyLuaErr","似乎出了点问题，请{nick}耐心等待（Lua脚本运行出错）" },
	{"strStModify","{self}对已记录{pc}的属性变化:\n{change}"},		//存在技能值变化情况时，优先使用此文本
	{"strStDetail","{self}对已设置{pc}的属性："},		//存在掷骰时，使用此文本(暂时无用)
	{"strStValEmpty","{self}未记录{attr}原值×"},		
	{"strBlackQQAddNotice","{nick}，你已被{self}加入黑名单，详情请联系Master:{print:master}"},				
	{"strBlackQQAddNoticeReason","{nick}，由于{reason}，你已被{self}加入黑名单，申诉解封请联系管理员。Master:{print:master}"},
	{"strBlackQQDelNotice","{nick}，你已被{self}移出黑名单，现在可以继续使用了"},
	{"strWhiteQQAddNotice","{user_nick}，您已获得{self}的信任，请尽情使用{self}√"},
	{"strWhiteQQDenied","你不是{self}的信任用户×"},
	{"strDeckNew","{self}已为{nick}自定义新牌堆<{deck_name}>√"},
	{"strDeckSet","{nick}已用<{deck_name}>创建{self}的牌堆实例√"},
	{"strDeckSetRename","{nick}已用<{deck_cited}>创建{self}的牌堆实例{deck_name}√"},
	{"strDeckRestEmpty","牌堆<{deck_name}>已抽空，请使用.deck reset {deck_name}手动重置牌堆"},		
	{"strDeckOversize","{nick}定义的牌太多，{self}装不下啦×"},
	{"strDeckRestShow","当前牌堆<{deck_name}>剩余卡牌:{deck_rest}"},
	{"strDeckRestReset","{self}已重置牌堆实例<{deck_name}>√"},
	{"strDeckDelete","{self}已移除牌堆实例<{deck_name}>√"},
	{"strDeckListShow","在{self}处创建的牌堆实例有:{res}"},
	{"strDeckListClr","{nick}已清空{self}处牌堆实例√"},
	{"strDeckListEmpty","{self}处牌堆实例列表为空！"},
	{"strDeckNewEmpty","{self}无法为{nick}新建虚空牌堆×"},
	{"strDeckListFull","{self}处牌堆实例已达上限，请先清理无用实例×"},
	{"strDeckNotFound","{self}找不到牌堆{deck_name}×"},
	{"strDeckCiteNotFound","{self}找不到公共牌堆{deck_cited}×" },
	{"strDeckNameEmpty","{nick}未指定牌堆名×"},
	{"strRangeEmpty","{self}没法对着空气数数×" },
	{"strOutRange","{nick}定义的数列超出{self}允许范围×" },
	{"strRollDice","{pc}掷骰: {res}"},
	{"strRollDiceReason","{pc}掷骰 {reason}: {res}"},
	{"strRollHidden","{pc}进行了一次暗骰"},
	{"strRollTurn","{pc}的掷骰轮数: {turn}轮"},
	{"strRollMultiDice","{pc}掷骰{turn}次: {dice_exp}={res}"},
	{"strRollMultiDiceReason","{pc}掷骰{turn}次{reason}: {dice_exp}={res}"},
	{"strRollInit","{char}的先攻骰点：{res}" },
	{"strRollSkill","{pc}进行{attr}检定："},
	{"strRollSkillReason","由于{reason} {pc}进行{attr}检定："},
	{"strRollSkillHidden","{pc}进行了一次暗中{attr}检定√" },
	{"strEnRoll","{pc}的{attr}增强或成长检定：\n{res}"},//{attr}在用户省略技能名后替换为{strEnDefaultName}
	{"strEnRollNotChange","{strEnRoll}\n{pc}的{attr}值没有变化"},
	{"strEnRollFailure","{strEnRoll}\n{pc}的{attr}变化{change}点，当前为{final}点"},
	{"strEnRollSuccess","{strEnRoll}\n{pc}的{attr}增加{change}点，当前为{final}点"},
	{"strEnDefaultName","属性或技能"},//默认文本
	{"strEnValEmpty", "未对{self}设定待成长属性值，请先.st {attr} 属性值 或查看.help en×"},
	{"strEnValInvalid", "{attr}值输入不正确,请输入1-99范围内的整数!"},
	{"strSendMsg","{self}已将消息送出√"},//Master定向发送的回执
	{"strSendMasterMsg","消息{self}已发送给Master√"},//向Master发送的回执
	{"strSendMsgEmpty","发送消息内容为空×"},
	{"strSendMsgInvalid","{self}没有可以发送的对象×"},//没有Master
	{"strDefaultCOCShow","{self}记录默认房规为{rule}" },
	{"strDefaultCOCShowDefault","{self}未记录默认房规，使用默认房规{rule}" },
	{"strDefaultCOCClr","默认检定房规已清除√"},
	{"strDefaultCOCNotFound","默认检定房规不存在×"},
	{"strDefaultCOCSet","默认检定房规已设置:"},
	{"strLinked","{self}已为对象{target}建立链接√"},
	{"strLinkClose","{self}已断开与对象的链接√" },
	{"strLinkBusy","{nick}的目标已经有对象啦×\n{self}不支持多边关系" },
	{"strLinkState","{self}当前链接状态:{link_info}" },
	{"strLinkList","{self}当前链接列表:{link_list}" },
	{"strLinkedAlready","{self}正在被其他对象链接×\n请{nick}先断绝当前关系" },
	{"strLinkingAlready","{self}已经开启链接啦!" },
	{"strLinkCloseAlready","{self}断开链接失败：{nick}当前本就没有对象！" },
	{"strLinkNotFound","{self}找不到{nick}的对象×"},
	{"strNotMaster","你不是{self}的master！你想做什么？"},
	{"strNotAdmin","你不是{self}的管理员×"},
	{"strAdminDismiss","{strDismiss}"},					//管理员指令退群的回执
	{"strDismiss",""},						//.dismiss退群前的回执
	{"strHlpSet","已为{key}设置词条√"},
	{"strHlpReset","已清除{key}的词条√"},
	{"strHlpNameEmpty","Master想要自定义什么词条呀？"},
	{"strHelpNotFound","{self}未找到「{help_word}」相关的词条×"},
	{"strHelpSuggestion","{self}猜{nick}想要查找的是:\n{res}"},
	{"strHelpRedirect","{self}仅找到相近词条「{redirect_key}」:\n{redirect_res}" },
	{"strClockToWork","{self}已按时启用√"},
	{"strClockOffWork","{self}已按时关闭√"},
	{"strNameGenerator","{pc}的随机名称：{res}"},
	{"strDrawCard", "来看看{pc}抽到了什么：{res}"},
	{"strDrawHidden", "{pc}抽了{cnt}张手牌√" },
	{"strMeOn", "成功在这里启用{self}的.me命令√"},
	{"strMeOff", "成功在这里禁用{self}的.me命令√"},
	{"strMeOnAlready", "在这里{self}的.me命令没有被禁用!"},
	{"strMeOffAlready", "在这里{self}的.me命令已经被禁用!"},
	{"strObOn", "成功在这里启用{self}的旁观模式√"},
	{"strObOff", "成功在这里禁用{self}的旁观模式√"},
	{"strObOnAlready", "在这里{self}的旁观模式没有被禁用!"},
	{"strObOffAlready", "在这里{self}的旁观模式已经被禁用!"},
	{"strObList", "当前{self}的旁观者有:"},
	{"strObListEmpty", "当前{self}暂无旁观者"},
	{"strObListClr", "{self}成功删除所有旁观者√"},
	{"strObEnter", "{nick}成功加入{self}的旁观√"},
	{"strObExit", "{nick}成功退出{self}的旁观√"},
	{"strObEnterAlready", "{nick}已经处于{self}的旁观模式!"},
	{"strObExitAlready", "{nick}没有加入{self}的旁观模式!"},
	{"strUIDEmpty", "请{nick}写出账号×"},
	{"strGroupIDEmpty", "请{nick}写出群号×"},
	{"strBlackGroup", "该群在黑名单中，如有疑问请联系{print:master}"},
	{"strBotOn", "成功开启{self}√"},
	{"strBotOff", "成功关闭{self}√"},
	{"strBotOnAlready", "{self}已经处于开启状态!"},
	{"strBotOffAlready", "{self}已经处于关闭状态!"},
	{"strBotChannelOn", "已在频道内开启{self}√" },
	{"strBotChannelOff", "已在频道内关闭{self}√" },
	{"strRollCriticalSuccess", "大成功！"}, //一般检定用
	{"strRollExtremeSuccess", "极难成功"},
	{"strRollHardSuccess", "困难成功"},
	{"strRollRegularSuccess", "成功"},
	{"strRollFailure", "失败"},
	{"strRollFumble", "大失败！"},
	{"strFumble", "大失败!"}, //多轮检定用，请控制长度
	{"strFailure", "失败"},
	{"strSuccess", "成功"},
	{"strHardSuccess", "困难成功"},
	{"strExtremeSuccess", "极难成功"},
	{"strCriticalSuccess", "大成功!"},
	{"strNumCannotBeZero", "无意义的数目！莫要消遣于我!"},
	{"strDeckNotFound", "是说{deck_name}？{self}没听说过的牌堆名呢……"},
	{"strDeckEmpty", "{self}已经一张也不剩了！"},
	{"strNameNumTooBig", "生成数量过多!请输入1-10之间的数字!"},
	{"strNameNumCannotBeZero", "生成数量不能为零!请输入1-10之间的数字!"},
	//{"strSetInvalid", "无效的默认骰!请输入1-9999之间的数字!"},
	{"strSetTooBig", "这面数……让我丢个球啊!请输入1-9999之间的数字!"},
	{"strSetCannotBeZero", "默认骰不能为零!请输入1-9999之间的数字!"},
	{"strCharacterCannotBeZero", "人物作成次数不能为零!请输入1-10之间的数字!"},
	{"strCharacterTooBig", "人物作成次数过多!请输入1-10之间的数字!"},
	{"strCharacterInvalid", "人物作成次数无效!请输入1-10之间的数字!"},
	{"strSanRoll", "{pc}的San Check：\n{res}"},
	{"strSanRollRes", "{strSanRoll}\n{pc}的San值减少{change}点,当前剩余{final}点"},
	{"strSanCostInvalid", "{pc}输入SC表达式不正确,格式为成功扣San/失败扣San,如1/1d6!"},
	{"strSanInvalid", "San值输入不正确,请{pc}输入1-99范围内的整数!"},
	{"strSanEmpty", "未设定San值，请{pc}先.st san 或查看.help sc×"},
	{"strTempInsane", "{pc}的疯狂发作-临时症状:\n{res}" },
	{"strLongInsane", "{pc}的疯狂发作-总结症状:\n{res}" },
	{"strSuccessRateErr", "这成功率还需要检定吗？"},
	{"strGroupIDInvalid", "无效的群号!"},
	{"strSendErr", "消息发送失败!"},
	{"strSendSuccess", "命令执行成功√"},
	{"strActionEmpty", "动作不能为空×"},
	{"strMEDisabledErr", "管理员已在此群中禁用.me命令!"},
	{"strDisabledMeGlobal", "{self}恕不提供.me服务×"},
	{"strDisabledJrrpGlobal", "{self}恕不提供.jrrp服务×"},
	{"strDisabledDeckGlobal", "{self}恕不提供.deck服务×"},
	{"strDisabledDrawGlobal", "{self}恕不提供.draw服务×"},
	{"strDisabledSendGlobal", "{self}恕不提供.send服务×"},
	{"strHELPDisabledErr", "管理员已在此群中禁用.help命令!"},
	{"strNameDelEmpty", "{nick}未设置名称,无法删除!"},
	{"strValueErr", "掷骰表达式输入错误!"},
	{"strInputErr", "命令或掷骰表达式输入错误!"},
	{"strUnknownErr", "发生了未知错误!"},
	{"strUnableToGetErrorMsg", "无法获取错误信息!"},
	{"strDiceTooBigErr", "{self}被你扔出的骰子淹没了×（骰数过多）"},
	{"strRequestRetCodeErr", "{self}访问服务器时出现错误! HTTP状态码: {error}"},
	{"strRequestNoResponse", "服务器未返回任何信息×"},
	{"strTypeTooBigErr", "哇!让{self}数数骰子有多少面先~1...2..."},
	{"strZeroTypeErr", "这是...!!时空裂*{self}被骰子产生的时空裂缝卷走了*（骰面为0）"},
	{"strAddDiceValErr", "你这样要让{self}扔骰子扔到什么时候嘛~(请输入正确的加骰参数:5-10之内的整数)"},
	{"strZeroDiceErr", "咦?{self}的骰子呢?（骰数为0）"},
	{"strRollTimeExceeded", "{nick}的掷骰轮数超过了最大轮数限制×"},
	{"strRollTimeErr", "{nick}的掷骰轮数异常×"},
	{"strDismissPrivate", "你在私聊窗口想{self}走哪呢？"},
	{"strWelcomePrivate", "你在私聊窗口想{self}欢迎谁呢？"},
	{"strWelcomeMsgClearNotice", "{self}已清除本群的入群欢迎词√"},
	{"strWelcomeMsgClearErr", "没有设置{self}入群欢迎词，清除失败×"},
	{"strWelcomeMsgUpdateNotice", "{self}已更新本群的入群欢迎词√"},
	{"strWelcomeMsgEmpty", "{self}当前未记录入群欢迎词" },
	{"strPermissionDeniedErr", "请让群内管理对{self}发送该指令×"},
	{"strSelfPermissionErr", "{self}权限不够无能为力呢×"},
	{"strNameTooLongErr", "称呼过长×(最多50字节)"},
	{"strNameSet", "{self}已将{old_nick}改称为{new_nick}√"},
	{"strNameDel", "{self}已删除{old_nick}在当前窗口的称呼√" },
	{"strNameClr", "{self}已清空{old_nick}的所有称呼√" },
	{"strUnknownPropErr", "未设定{attr}成功率，请先.st {attr} 技能值 或查看.help rc×"},
	{"strPropErr", "请{pc}认真输入属性哦~"},
	{"strSetPropSuccess", "已为{pc}录入{cnt}条属性√"},
	{"strPropCleared", "已清空{char}的所有属性√"},
	{"strRuleReset", "已重置默认规则√"},
	{"strRuleSet", "已设置默认规则√"},
	{"strRuleErr", "规则数据获取失败,具体信息:\n"},
	{"strRulesFailedErr", "请求失败,{self}无法连接数据库×"},
	{"strPropDeleted", "已删除{pc}的{attr}√"},
	{"strPropNotFound", "属性{attr}不存在×"},
	{"strRuleNotFound", "{self}未找到对应的规则信息×"},
	{"strProp", "{pc}的{attr}为{val}"},
	{"strPropList", "{nick}的{char}属性列表为：{show}"},
	{"strStErr", "格式错误:请参考.help st获取.st命令的使用方法"},
	{"strRulesFormatErr", "格式错误:正确格式为.rules[规则名称:]规则条目 如.rules COC7:力量"},
	{"strLeaveDiscuss", "{self}现不支持讨论组服务，即将退出"},
	{"strLeaveNoPower", "{self}未获得群管理，即将退群"},
	{"strLeaveUnused", "{self}已经在这里被放置{day}天啦，马上就会离开这里了"},
	{"strGlobalOff", "{self}休假中，暂停服务×"},
	{"strPreserve", "{self}私有私用，勿扰勿怪\n如需申请许可请发送!authorize +[群号] 申请用途:[ **请写入理由** ] 我已了解Dice!基本用法，仔细阅读并保证遵守{strSelfName}的用户协议，如需停用指令使用[ **请写入指令** ]，用后使用[ **请写入指令** ]送出群"},
	{"strJrrp", "{nick}今天的人品值是: {res}"},
	{"strJrrpErr", "JRRP获取失败! 错误信息: \n{res}"},
	{ "strFriendDenyNotUser", "很遗憾，你没有对{self}使用指令的记录" },
	{ "strFriendDenyNoTrust", "很遗憾，你不是{self}信任的用户，如需使用可联系{print:master}" },
	{"strAddFriendWhiteQQ", "{strAddFriend}"}, //白名单用户添加好友时回复此句
	{
		"strAddFriend",
		R"(欢迎选择{strSelfName}的免费掷骰服务！
.help协议 确认服务协议
.help指令 查看指令列表
.help设定 确认骰娘设定
.help链接 查看源码文档
使用服务默认已经同意服务协议)"
	}, //同意添加好友时额外发送的语句
	{
		"strAddGroup",
		R"(欢迎选择{strSelfName}的免费掷骰服务！
请使用.dismiss QQ号（或后四位） 使{self}退群退讨论组
.bot on/off QQ号（或后四位） //开启或关闭指令
.reply on/off 启用/禁用回复 //禁用或启用回复
.help协议 确认服务协议
.help指令 查看指令列表
.help设定 确认骰娘设定
.help链接 查看源码文档
邀请入群默认视为同意服务协议，知晓禁言或移出的后果)"
	}, 
	{ "strNewMaster","试问，你就是{strSelfName}的Master√\n请认真阅读当前版本Master手册以及用户手册。请注意版本号对应: https://v2docs.kokona.tech\f{strSelfName}默认开启对群移出、禁言、刷屏事件的监听，如要关闭请手动调整；\n请注意云黑系统默认开启，如无需此功能请关闭CloudBlackShare；" },
	{ "strNewMasterPublic",R"({strSelfName}以公骰模式认主：
自动开启BelieveDiceList响应来自骰娘列表的warning；
已开启自动通过非黑名单好友申请；
已开启黑名单自动清理，拉黑时及每日定时会自动清理与黑名单用户的共同群聊，黑名单用户群权限不低于自己时自动退群；
已开启拉黑群时连带邀请人；
已启用send功能接收用户发送的消息；)" },
	{ "strNewMasterPrivate",R"({strSelfName}以私骰模式认主：
默认拒绝陌生人的群邀请，只同意来自管理员、受信任用户的邀请；
默认拒绝陌生人的好友邀请，如要同意请开启AllowStranger；
已开启黑名单自动清理，拉黑时及每日定时会自动清理与黑名单用户的共同群聊，黑名单用户群权限高于自己时自动退群；
.me功能默认不可用，需要手动开启；
切换公用请使用.admin public，但不会初始化相应设置；
可在.master delete后使用.master public来重新初始化；)" },
	{"strSelfCall", "&strSelfName"},
	{"strSelfName", "" },
	{"strSelfNick", "&strSelfName" },
	{"self", "&strSelfCall"},
	{"strBotHeader", "试验型 " },
	{"strBotMsg", "\n使用.help 查看{self}帮助文档"},
	{"strHlpMsg", R"(请使用.dismiss ID（或后四位） 使{self}退群退讨论组
.bot on/off ID（或后四位） //开启或关闭指令
.help协议 确认服务协议
.help指令 查看指令列表
.help群管 查看群管指令
.help设定 确认骰娘设定
.help链接 查看源码文档
官方论坛: https://forum.kokona.tech/
Dice!众筹计划: https://afdian.net/@suhuiw4123)"
	}
};
dict_ci<string> GlobalMsg{ PlainMsg };

fifo_dict_ci<string> EditedMsg;
fifo_dict_ci<string> CustomHelp;
const dict_ci<string> GlobalComment{
	{"self", "自称，引用自strSelfCall"},
	//{"strActionEmpty", "当前无用"},
	{"strAddDiceValErr", "ww指令加骰值非法（过小）"},
	{"strAddFriend", "通过申请后发送给好友的迎新词"},
	{"strAddFriendWhiteQQ", "通过受信任用户好友后的迎新词"},
	{"strAddGroup", "自身入群时群内发送的入场词"},
	{"strAdminDismiss", "dismiss指令由管理使用时的退场词，默认一致"},
	{"strAdminOptionEmpty", "admin指令参数为空"},
	{"stranger", "昵称空白或无法获取时的代称"},
	{"strBlackGroup", "识别到黑名单群的退场词"},
	{"strBlackQQAddNotice", "新拉黑对象有使用记录时发送通知"},
	{"strBlackQQAddNoticeReason", "带理由的拉黑通知"},
	{"strBlackQQDelNotice", "解黑通知"},
	{"strBotHeader", "bot回执开头附加文本"},
	{"strBotMsg", "bot回执附于Dice信息后的文本"},
	{"strBotOff", "bot指令群内关闭回执"},
	{"strBotOffAlready", "bot指令群内已经关闭"},
	{"strBotOn", "bot指令群内开启回执"},
	{"strBotOnAlready", "bot指令群内已经开启"},
	//
	{"strCallUser", "对用户的称呼"},
	//
	{"strCOCBuild", "coc指令回执"},
	{"strCriticalSuccess", "多轮检定大成功"},
	//
	{"strDeckNew", "deck指令定义牌堆实例"},
	{"strDeckSet", "deck指令设置公共牌堆为实例"},
	//
	{"strDismiss", "dismiss指令退场词"},
	{"strDismissPrivate", "dismiss指令私聊且对象不明的回执"},
	{"strDNDBuild", "dnd指令回执"},
	{"strDrawCard", "draw指令回执"},
	{"strDrawHidden", "draw暗抽时的公屏回执"},
	//
	{"strEnRoll", "en指令回执"},
	//
	{"strExtremeSuccess", "多轮检定极难成功"},
	{"strFailure", "多轮检定与SC失败"},
	{"strFriendDenyNoTrust", "AllowStranger=0时非信任用户的拒绝回执"},
	{"strFriendDenyNotUser", "AllowStranger=1时无使用记录的拒绝回执"},
	{"strFumble", "多轮检定大失败"},
	{"strGlobalOff", "全局静默时对私聊指令或开启指令的回执"},
	//
	{"strGroupAuthorized", "!authorize+群号授权成功后在目标群的回执"},
	{"strGroupBan", "group ban禁言群员的回执"},
	{"strGroupCardSet", "group card修改群名片的回执"},
	//
	{"strGroupUnBan", "group ban解除群员禁言的回执"},
	//
	{"strHardSuccess", "多轮检定困难成功"},
	{"strHardSuccess", "多轮检定困难成功"},
	{"strHelpDisabledErr", "group+禁用help后群内help回执"},
	{"strHelpNotFound", "help未找到近似词条"},
	{"strHelpRedirect", "help找到唯一近似词条"},
	{"strHelpSuggestion", "help找到多条近似词条"},
	{"strHlpMsg", "help指令裸参数回执"},
	//
	{"strRollCriticalSuccess", "检定大成功"}, 
	{"strRollExtremeSuccess", "检定极难成功"},
	{"strRollHardSuccess", "检定困难成功"},
	{"strRollRegularSuccess", "检定成功"},
	{"strRollFailure", "检定失败"},
	{"strRollFumble", "检定大失败"},
	//
	{"strSanRollRes", ".sc指令回执"},
	{"strSelfCall", "自称，多用于回执"},
	{"strSelfName", "名称，用于自我展示场合"},
	{"strSelfNick", "用户对自己的昵称，用于扩展指令"},
	//
	{"strSuccess", "多轮检定与SC成功"},
	//
	{"strSummonWord", "自定义召唤词，等效于at指名"},
	//
	{"strWhiteQQDenied","权限要求群管理或者信任1"},
};
const dict_ci<> HelpDoc = {
{"更新",R"(
635:webui可自定义化
634:转义优化，支持变量赋值
633:format新增wait，花括号嵌套优化
632:format新增ran
631:WebUI新增mod页
630:修复初始认主
629:恢复远程更新
628:支持mod更新
627:更新认主口令
626:前缀匹配记录后缀
625:支持welcome转义
624:支持mod远程安装/详细信息
623:优化mod热插拔
622:支持手动时差
621:扩展代理事件及ex接口
620:留档每日数据，新增每日清算事件
619:统一reply与order格式
618:支持reply(Order形式)覆盖指令
617:更新grade分档转义
616:转义&case支持取用户/群配置
615:支持自定义数据读写
614:更新reply:limit:lock
613:更新lua交互机制
612:新增mod代理事件
611:更新WebUI自定义回执
610:新增mod定时事件
609:新增mod循环事件
608:新增.mod指令
607:修改.nn语法
593:reply新增触发限制
589:ak安科安价指令
585:WebUI
581:角色掷骰统计
569:.rc/.draw暗骰暗抽
567:敏感词检测
566:.help查询建议
565:.log日志记录)"},
{"协议","0.本协议是Dice!默认服务协议。如果你看到了这句话，意味着Master应用默认协议，请注意。\n1.邀请骰娘、使用掷骰服务和在群内阅读此协议视为同意并承诺遵守此协议，否则请使用.dismiss移出骰娘。\n2.不允许禁言、移出骰娘或刷屏掷骰等对骰娘的不友善行为，这些行为将会提高骰娘被制裁的风险。开关骰娘响应请使用.bot on/off。\n3.骰娘默认邀请行为已事先得到群内同意，因而会自动同意群邀请。因擅自邀请而使骰娘遭遇不友善行为时，邀请者因未履行预见义务而将承担连带责任。\n4.禁止将骰娘用于赌博及其他违法犯罪行为。\n5.对于设置敏感昵称等无法预见但有可能招致言论审查的行为，骰娘可能会出于自我保护而拒绝提供服务\n6.由于技术以及资金原因，我们无法保证机器人100%的时间稳定运行，可能不定时停机维护或遭遇冻结，但是相应情况会及时通过各种渠道进行通知，敬请谅解。临时停机的骰娘不会有任何响应，故而不会影响群内活动，此状态下仍然禁止不友善行为。\n7.对于违反协议的行为，骰娘将视情况终止对用户和所在群提供服务，并将不良记录共享给其他服务提供方。黑名单相关事宜可以与服务提供方协商，但最终裁定权在服务提供方。\n8.本协议内容随时有可能改动。请注意帮助信息、签名、空间、官方群等处的骰娘动态。\n9.骰娘提供掷骰服务是完全免费的，欢迎投食。\n10.本服务最终解释权归服务提供方所有。"},
{"链接","Dice!论坛: https://kokona.tech\nDice!手册: https://v2docs.kokona.tech\n支持Shiki: https://afdian.net/@dice_shiki"},
{"设定",R"(Master：{print:master}
群内使用：{case:self.Private?else=白名单制，需预申请&0={case:self.CheckGroupLicense?2=审核制，需申请后使用&1=审核制，入新群需申请&else=黑名单制，自由使用}}
好友申请：{case:self.AllowStanger?2=允许任何人&1=需要使用记录&else=仅白名单}
移出反制：{case:self.ListenGroupKicked?0=无&else=拉黑{case:self.KickedBanInviter?1=连带邀请人}}
禁言反制：{case:self.ListenGroupBanned?0=无&else=拉黑{case:self.BannedBanInviter?1=连带邀请人}}
刷屏反制：{case:self.ListenSpam?0=关闭&else=开启}
窥屏可能：{窥屏可能}
其他插件：{其他插件}{姐妹骰}
骰娘用户群:{骰娘用户群}
私骰分享群：192499947
开发交流群：1029435374)"},
{"骰娘用户群","【未设置】"},
{"窥屏可能","无"},
{"其他插件","【未设置】"},
{"姐妹骰","{list_dice_sister}"},
{"作者","Copyright (C) 2018-2021 w4123溯洄\nCopyright (C) 2019-2022 String.Empty\nGithub@Dice-Developer-Team"},
{"指令",R"(指令前接at可以指定骰娘响应，如
{at:self}.bot on
请.help对应指令 获取详细信息，如.help r
控制指令:
.dismiss 退群
.bot 版本信息
.bot on/off 启用/停用指令
.reply on/off 启用/禁用回复
.group 群管
.authorize 授权许可
.send 向后台发送消息
.mod 模块操作)"
"\f"
R"([第二页]跑团指令
.rules 规则速查
.r 掷骰
.log 日志记录
.ob 旁观模式
.set 设置默认骰
.coc COC人物作成
.dnd DND人物作成
.st 属性记录
.pc 角色卡记录
.rc 检定
.setcoc 设置rc房规
.sc 理智检定
.en 成长/增强检定
.ri 先攻
.init 先攻列表
.ww 骰池)"
"\f"
R"([第三页]其他指令
.nn 设置称呼
.draw 抽牌
.deck 牌堆实例
.name 随机姓名
.ak 安科/安价
.jrrp 今日人品
.welcome 入群欢迎
.me 第三人称动作
为了避免未预料到的指令误判，请尽可能在参数之间使用空格)"
"\f"
R"({help:扩展指令})"},
{"master",R"(当前Master:{print:master}
Master拥有最高权限，且可以调整任意信任)"},
{"mod",R"(模块指令.mod
本指令限信任4使用
`.mod list` 查看已加载mod列表
`.mod on 模块名` 启用指定模块
`.mod off 模块名` 停用指定模块
`.mod info 模块名` 指定模块简介信息
`.mod detail 模块名` 指定模块详细信息
`.mod delete 模块名` 卸载指定模块
mod按序读取，且从后向前覆盖)"},
{"ak",R"(安科+安价指令.ak
.ak#[标题]或.ak new [标题] 新建分歧并设置标题（可为空）
.ak+[选项]或.ak add [选项] 为本轮分歧添加新选项，用|分隔可一次添加多个选项
.ak-[选项序号]或.ak del [选项序号] 移除指定序号的选项
.ak= 或.ak get 均等随机抽取一个选项并结束本轮分歧
.ak show 查看分歧选项
.ak clr 清除本轮分歧)"},
{"log",R"(跑团日志记录.log
`.log new 日志名` 另开日志并开始记录
`.log on` 继续记录
`.log off` 暂停记录
`.log end` 完成记录并发送日志文件
日志名须作为文件名合法，省略则使用创建时间戳。上传有失败风险，届时请.send {self}后台索取)"},
{"deck",R"(牌堆实例.deck
`.deck set (牌堆名=)公共牌堆名` //从公共牌堆创建实例
`.deck set (牌堆名=)member` //从群成员列表创建实例
`.deck set (牌堆名=)range 下限 上限` //创建等差数列作为实例
`.deck show` //列出所有牌堆实例
`.deck show 牌堆名` //查看牌堆剩余卡牌
`.deck reset 牌堆名` //重置剩余卡牌
`.deck clr` //清空所有实例
`.deck new 牌堆名=[卡面1](...|[卡面n])` //自定义牌堆
例:
.deck new 俄罗斯轮盘=有弹|::5::无弹 //::张数::卡面
*实例可以操作抽牌洗牌*
*每个群至多保存10个实例*
*使用.draw时，牌堆实例优先级高于同名公共对象*
*从实例抽牌不会放回直到抽空*
*除show外其他群内操作需要用户信任或管理权限*)"},
{"退群","&dismiss"},
{"退群指令","&dismiss"},
{"dismiss","该指令需要群管理员权限，使用后即退出群聊\n!dismiss [目标QQ(完整或末四位)]指名退群\n!dismiss无视内置黑名单和静默状态，只要插件开启总是有效"},
{"授权许可","&authoize"},
{"authorize","授权许可(非信任用户使用时转为向管理申请许可)\n!authorize (+[群号]) ([申请理由])\n群内原地发送可省略群号，无法自动授权时会连同理由发给管理\n默认格式为:!authorize 申请用途:[ **请写入理由** ] 我已了解Dice!基本用法，仔细阅读并保证遵守{strSelfName}的用户协议，如需停用指令使用[ **请写入指令** ]，用后使用[ **请写入指令** ]送出群"},
{"开关","&bot"},
{"bot",".bot on/off开启/静默骰子（限群管理）\n.bot无视静默状态，只要插件开启且不在黑名单总是有效"},
{"规则速查","&rules"},
{"规则","&rules"},
{"rules","规则速查：.rules ([规则]):[待查词条] 或.rules set [规则]\n.rules 跳跃 //复数规则有相同词条时，择一返回\n.rules COC：大失败 //coc默认搜寻coc7的词条,dnd默认搜寻3r\n.rules dnd：语言\n.rules set dnd //设置后优先查询dnd同名词条，无参数则清除设置"},
{"掷骰","&r"},
{"rd","&r"},
{"r","掷骰：.r [掷骰表达式] ([掷骰原因]) [掷骰表达式]：([掷骰轮数]#)[骰子个数]d骰子面数(p[惩罚骰个数])(k[取点数最大的骰子数])不带参数时视为掷一个默认骰\n合法参数要求掷骰轮数1-10，奖惩骰个数1-9，个数范围1-100，面数范围1-1000\n.r3#d\t//3轮掷骰\n.rh心理学 暗骰\n.rs1D10+1D6+3 沙鹰伤害\t//rs省略单个骰子的点数，直接给结果"},
{"暗骰","群聊限定，掷骰指令后接h视为暗骰，结果将私发本人和群内ob的用户\n为了保证发送成功，请加骰娘好友"},
{"reply",R"(自定义回复：.reply
.reply on/off 开启/关闭群内回复
回复触发顺序：指令->完全Match->前缀Prefix->模糊Search->正则Regex
//以下操作回复指令仅admin可用
.reply set
Type=[回复性质](Reply/Order)
[触发模式](Match/Prefix/Search/Regex)=[触发词]
(Limit=[触发条件])
[回复模式](Deck/Text/Lua)=[回复词]
*Type一行可省略，默认为Reply
.reply show [触发词] 查看指定回复
.reply list 查看全部回复
.reply del [触发词] 清除指定回复
)"},
{"回复触发限制",R"(
每项关键词回复可以有零或多项触发条件，只要一项不满足就不触发
用户名单`user_id:账号列表` 指定账号触发，账号以|分隔
- 反向名单：冒号后加!表示指定账号不触发
群聊名单`grp_id:群号列表` 指定群聊触发，grp_id:0表示私聊可以触发
概率`prob:百分比` 依概率触发
冷却计时`cd:秒数` 非冷却状态触发，触发后计算冷却
当日计数`today:上限` 未达上限触发，触发后计数+1
阈值`user_var``grp_var``self_var`变量满足指定条件触发
骰娘识别`dicemaid:only` 仅Dice!骰娘触发
`dicemaid:off` Dice!骰娘不触发
)" },
{"回复列表","{strSelfName}的回复触发词列表:{list_reply_deck}"},
{"旁观","&ob"},
{"旁观模式","&ob"},
{"ob",R"(旁观模式：.ob (join/exit/list/clr/on/off)
.ob join //加入旁观，可以看到房内暗骰结果
.ob exit //退出旁观模式
.ob list //查看群内旁观者
.ob clr //清除所有旁观者
.ob on //全群允许旁观模式
.ob off //禁用旁观模式
暗骰与旁观私聊无效)"},
{"默认骰","&set"},
{"set","当表达式中‘D’之后没有接面数时，视为投掷默认骰\n.set20 将默认骰设置为20\n.set 不带参数视为将默认骰重置为默认的100\n若所用规则判定掷骰形如2D6，推荐使用.st &=2D6"},
{"个位骰","个位骰有十面，为0~9十个数字，百面骰的结果为十位骰与个位骰之和（但00+0时视为100）"},
{"十位骰","十位骰有十面，为00~90十个数字，百面骰的结果为十位骰与个位骰之和（但00+0时视为100）"},
{"奖励骰","&奖励/惩罚骰"},
{"惩罚骰","&奖励/惩罚骰"},
{"奖惩骰","&奖励/惩罚骰"},
{"奖励/惩罚骰","COC中奖励/惩罚骰是额外投掷的十位骰，最终结果选用点数更低/更高的结果（不出现大失败的情况下等价于更小/更大的十位骰）\n.rb2 2个奖励骰\nrcp 射击 1个惩罚骰"},
{"随机姓名","&name"},
{"name","随机姓名：.name (cn/jp/en)([生成数量])\n.name 10\t//默认4类名称随机生成\n.name en\t//后接cn/jp/en/enzh则限定生成中文/日文/英文/英文中译名\n名称生成个数范围1-10"},
{"设置昵称","&nn"},
{"昵称","&nn"},
{"nn","设置群内称呼：.nn [昵称] / .nn / .nnn(cn/jp/en) \n.nn kp\t//昵称前的./！等符号会被自动忽略\n.nn del\t//删除当前窗口称呼\n.nn clr\t//删除所有记录的称呼\n.nnn\t//从随机姓名牌堆设置随机称呼\n.nnn jp\t/从指定子牌堆随机昵称\n私聊.nn视为操作全局称呼\n该称呼用于\\{nick\\}的显示，优先级：群内称呼>全局称呼>群名片>QQ昵称\n无角色卡或未命名时也用于显示\\{pc\\}"},
{"人物作成","该版本人物作成支持COC7(.coc、.draw调查员背景/英雄天赋)、COC6(.coc6、.draw煤气灯)、DND(.dnd)、AMGC(.draw AMGC)"},
{"coc","克苏鲁的呼唤(COC)人物作成：.coc([7/6])(d)([生成数量])\n.coc 10\t//默认生成7版人物\n.coc6d\t//接d为详细作成，一次只能作成一个\n仅用作骰点法人物作成，可应用变体规则，参考.rules创建调查员的其他选项"},
{"dnd","龙与地下城(DND)人物作成：.dnd([生成数量])\n.dnd 5\t//仅作参考，可自行应用变体规则"},
{"属性记录","&st"},
{"st","属性记录：.st (del/clr/show) ([属性名]:[属性值])\n用户默认所有群使用同一张卡，pl如需多开请使用.pc指令切卡\n.st力量:50 体质:55 体型:65 敏捷:45 外貌:70 智力:75 意志:35 教育:65 幸运:75\n.st hp-1 后接+/-时视为从原值上变化\n.st san+1d6 修改属性时可使用掷骰表达式\n.st del kp裁决\t//删除已保存的属性\n.st clr\t//清空当前卡\n.st show 灵感\t//查看指定属性\n.st show\t//无参数时查看所有属性，请使用只st加点过技能的半自动人物卡！\n部分COC属性会被视为同义词，如智力/灵感、理智/san、侦查/侦察"},
{"角色卡","&pc"},
{"pc",R"(角色卡：.pc 
.pc new ([模板]:([生成参数]:))([卡名]) 
完全省略参数将生成一张COC7模板的随机姓名卡
.pc tag ([卡名]) //为当前群绑定指定卡，为空则解绑使用默认卡
所有群默认使用私聊绑定卡，未绑定则使用0号卡
.pc show ([卡名]) //展示指定卡所有记录的属性，为空则展示当前卡
.pc nn [新卡名] //重命名当前卡，不允许重名
.pc type [模板] //将切换当前卡模板
.pc cpy [卡名1]=[卡名2] //将后者属性复制给前者
.pc del [卡名] //删除指定卡
.pc list //列出全部角色卡
.pc grp //列出各群绑定卡
.pc build ([生成参数]:)(卡名) //根据模板填充生成属性（COC7为9项主属性）
.pc stat //查看当前角色卡骰点统计
.pc redo ([生成参数]:)(卡名) //清空原有属性后重新生成
.pc clr //销毁全部角色卡记录
//掷骰统计以角色卡为单位，每名用户最多可同时保存16张角色卡
)"
	},
	{"rc", "&rc/ra"},
	{"ra", "&rc/ra"},
	{"检定", "&rc/ra"},
	{
		"rc/ra",
		"检定指令：.rc/ra (_)([检定次数]#)([难度])[属性名]( [成功率])\n角色卡设置了属性时，可省略成功率\n.rc体质*5\t//允许使用+-*/，但顺序要求为乘法>加减>除法\n.rc 困难幸运\t//技能名开头的困难和极难会被视为关键词\n.rc _心理学50\t//暗骰结果仅本人及旁观者可见\n.rc 敏捷-10\t//修正后成功率必须在1-1000内\n.rcp 手枪\t//奖惩骰至多9个\n默认以规则书判定，大成功大失败的房规由.setcoc设置"
	},
	{
		"房规",
		"Dice!目前只为COC7检定提供房规，指令为.setcoc:{setcoc}"
	},
	{
		"setcoc",
		R"(为当前群或讨论组设置COC房规，如.setcoc 1,当前参数0-6
`.setcoc show`查看当前房规
`.setcoc clr`清除当前房规
0 规则书
出1大成功
不满50出96 - 100大失败，满50出100大失败
1
不满50出1大成功，满50出1 - 5大成功
不满50出96 - 100大失败，满50出100大失败
2
出1 - 5且 <= 成功率大成功
出100或出96 - 99且 > 成功率大失败
3
出1 - 5大成功
出96 - 100大失败
4
出1 - 5且 <= 十分之一大成功
不满50出 >= 96 + 十分之一大失败，满50出100大失败
5
出1 - 2且 < 五分之一大成功
不满50出96 - 100大失败，满50出99 - 100大失败
6 绿色三角洲
出1或出个位十位相同且<=成功率大成功
出100或出个位十位相同且>成功率大失败

如需其他房规可向开发者反馈
无论如何，群内检定只会调用群内设置，以免团内成员不对等
未设置房规将使用配置项DefaultCOCRoomRule（初始为0）)"
	},
	{"san check", "&sc"},
	{"理智检定", "&sc"},
	{
		"sc",
		"San Check指令：.sc[成功损失]/[失败损失] ([当前san值])\n已经.st了理智/san时，可省略最后的参数\n.sc0/1 70\n.sc1d10/1d100 直面外神\n大失败自动失去最大值\n当调用角色卡san时，san会自动更新为sc后的剩余值\n程序上可以损失负数的san，也就是可以用.sc-1d6/-1d6来回复san，但请避免这种奇怪操作"
	},
	{"ti", "&ti/li"},
	{"li", "&ti/li"},
	{"疯狂症状", "&ti/li"},
	{"ti/li", "疯狂症状：\n.ti 临时疯狂症状\n.li 总结疯狂症状\n适用coc7版规则，6版请自行用百面骰配合查表\n具体适用哪项参见.rules 疯狂发作"},
	{"成长检定", "&en"},
	{"增强检定", "&en"},
	{
		"en",
		"成长检定：.en [技能名称]([技能值])(([失败成长值]/)[成功成长值])\n已经.st时，可省略最后的参数\n.en 教育 60 +1D10 教育增强\t//后接成功时成长值\n.en 幸运 +1D3/1D10幸运成长\t//调用人物卡属性时，成长后的值会自动更新\n可变成长值必须以加减号开头，不限制加减"
	},
	{"抽牌", "&draw"},
	{
		"draw",
		R"(抽牌：.draw [牌堆名称] ([抽牌数量])	
.draw _[牌堆名称] ([抽牌数量])	//暗抽，结果私聊发送
.drawh [牌堆名称] ([抽牌数量])	//暗抽，参数h后必须留空格
.draw _狼人杀	//抽牌结果通过私聊获取，可ob
*牌堆名称优先调用牌堆实例，如未设置则从同名公共牌堆生成临时实例
*抽到的牌不放回，牌堆抽空后无法继续
*查看{self}已安装牌堆，可.help 全牌堆列表或.help 扩展牌堆)"
	},
	{ "扩展牌堆","{list_extern_deck}" },
	{ "全牌堆列表","{list_all_deck}" },
	{ "扩展指令","{list_extern_order}" },
	{"先攻", "&ri"},
	{"ri", "先攻（群聊限定）：.ri([加值])([昵称])\n.ri -1 某pc\t//自动记入先攻列表\n.ri +5 boss"},
	{"先攻列表", "&init"},
	{"init", "先攻列表：\n.init list\t//查看先攻列表\n.init clr\t//清除先攻列表\n.init del [项目名]\t//从先攻列表移除项目"},
	{"骰池", "&ww"},
	{"ww", R"(骰池：.w(w) [骰子个数]a[加骰参数]
.w会直接给出结果而.ww会给出每个骰子的点数
固定10面骰，每有一个骰子点数达到加骰参数，则加骰一次，最后计算点数达到8的骰子数
其中5≤加骰参数≤10
具体用法请参考相关游戏规则)"},
	{"第三人称", "&me"},
	{"第三人称动作", "&me"},
	{
		"me",
		"第三人称动作：.me([群号])[动作].me 笑出了声\t//仅限群聊使用\n.me 941980833 抱群主\t//仅限私聊使用，此命令可伪装成骰子在群里说话\n.me off\t//群内禁用.me\n.me on\t//群内启用.me"
	},
	{"今日人品", "&jrrp"},
	{
		"jrrp",
		"今日人品：.jrrp\n.jrrp off\t//群内禁用jrrp\n.jrrp on\t//群内启用jrrp\n一天一个人品值\n2.3.5版本后随机值为均匀分布\n骰娘只负责从溯洄的服务器搬运结果，请勿无能狂怒\n如何发配所有人的命运，只有孕育万千骰娘生机之母，萌妹吃鱼之神，正五棱双角锥体对的监护人，一切诡秘的窥见者，时空舞台外的逆流者，永转的命运之轮——溯洄本体掌握"
	},
	{"send", "发送消息：.send 想对Master说的话\n如果用来调戏Master请做好心理准备"},
	{"入群欢迎", "&welcome"},
	{"入群欢迎词", "&welcome"},
	{
		"welcome",
		R"(入群欢迎词：.welcome
.welcome \{at}欢迎\{nick}入群~	//设置欢迎词
//\{at}视为at入群者，\{nick}会替换为新人的昵称，更多转义见手册
.welcome clr //清空欢迎词
.welcome show //查看欢迎词
无论指令是否停用，只要有欢迎词时有人入群，都会响应)"
	},
	{"group", "&群管"},
	{
		"群管",
		R"(群管指令.group(群管理员限定)
.group state //查看在群内对骰娘的设置
.group pause/restart //群全体禁言/全体解除禁言
.group card [at/用户QQ] [名片] //设置群员名片
.group title [at/用户QQ] [头衔] //设置群员头衔
.group diver //查看潜水成员
.group +/-[群管词条] //为群加减设置，需要对应权限
例:.group +禁用回复 //关闭本群自定义回复
群管词条:停用指令/禁用回复/禁用jrrp/禁用draw/禁用me/禁用help/禁用ob/拦截消息/许可使用/免清/免黑)"
	},
	{ "groups_list", "&取群列表" },
	{ "取群列表", R"(取群列表.groups list(管理限定)
.groups list idle //按闲置天数降序列出群
.groups list size //按群规模降序列出群
.groups list [群管词条] //列出带有词条的群
群管词条:停用指令/禁用回复/禁用jrrp/禁用draw/禁用me/禁用help/禁用ob/拦截消息/许可使用/免清/免黑)" },
	{"消息链接","&link"},
	{"link",R"(消息链接.link
.link [转发方向] [对象窗口] 建立本窗口与对象窗口的转发
.link start 开启上次关闭的链接
.link close 关闭链接
[转发方向]:to=转发本窗口消息到对象窗口;from=转发对象窗口消息到本窗口;with=双向转发
[对象窗口]:群/讨论组=[群号];私聊窗口=q[uid号]
例:.link with q1605271653 //建立双向私聊链接
.link from 928626681 //接收目标群的消息转发
.link state 查看当前链接状态
.link list 列出全部窗口的链接
每个窗口为起点只能链接一个对象)"},
	{ "敏感词检测","&censor" },
	{"censor",R"(敏感词检测.admin censor
.admin censor +([触发等级])=[敏感词0](|[敏感词1]...) //添加敏感词
.admin censor -[敏感词0](|[敏感词1]...) //移除敏感词
例:.admin censor +=nmsl //将“nmsl”设置为Warning级
.admin censor +Danger=nn老公|nn主人 //将“nn老公”、“nn主人”设置为Danger级
.admin censor -手枪 //移除敏感词“手枪”
# 匹配机制
骰娘会模糊匹配指令标识符(.)开头的消息，并返回所含敏感词的最高触发等级
匹配过程自动跳过文本中的特殊符号和空格，且大小写不敏感
受信任用户会相应降低触发等级，信任4以上用户将不触发检测
# 触发等级
Ignore //无视
Notice //仅在0级窗口通知
Caution //提醒用户，并在1级窗口提醒
Warning //【默认等级】警告用户，并在1级窗口提醒
Danger //警告用户且拒绝指令，并在3级窗口警告
*请避免为纯字母/数字的敏感词设置较高触发等级，这些字符存在误匹配图片码的可能性
# 词库批量加载方式见手册)" },
	{"溯洄", "孕育万千骰娘生机之母，萌妹吃鱼之神，正五棱双角锥体对的监护人，一切诡秘的窥见者，拟人者主宰，时空舞台外的逆流者，永转的命运之轮"},
	{"投喂","&投食"},
	{"投食", "投食Shiki，请选择https://afdian.net/@dice_shiki\n投食溯洄，可选择https://afdian.net/@suhuiw4123\n投食{self}，可选择……充超会？"},
	{"愚者正位", "憧憬自然的地方、毫无目的地前行、喜欢尝试挑战新鲜事物、四处流浪。美好的梦想。"},
	{
		"愚者逆位",
		"冒险的行动，追求可能性，重视梦想，无视物质的损失，离开家园，过于信赖别人，为出外旅行而烦恼。心情空虚、轻率的恋情、无法长久持续的融洽感、不安的爱情的旅程、对婚姻感到束缚、彼此忽冷忽热、不顾众人反对坠入爱河、为恋人的负心所伤、感情不专一。"
	},
	{"魔术师正位", "事情的开始，行动的改变，熟练的技术及技巧，贯彻我的意志，运用自然的力量来达到野心。"},
	{"魔术师逆位", "意志力薄弱，起头难，走入错误的方向，知识不足，被骗和失败。"},
	{"女祭司正位", "开发出内在的神秘潜力，前途将有所变化的预言，深刻地思考，敏锐的洞察力，准确的直觉。"},
	{"女祭司逆位", "过于洁癖，无知，贪心，目光短浅，自尊心过高，偏差的判断，有勇无谋，自命不凡。"},
	{"女皇正位", "幸福，成功，收获，无忧无虑，圆满的家庭生活，良好的环境，美貌，艺术，与大自然接触，愉快的旅行，休闲。"},
	{"女皇逆位", "不活泼，缺乏上进心，散漫的生活习惯，无法解决的事情，不能看到成果，担于享乐，环境险恶，与家人发生纠纷。"},
	{"皇帝正位", "光荣，权力，胜利，握有领导权，坚强的意志，达成目标，父亲的责任，精神上的孤单。"},
	{"皇帝逆位", "幼稚，无力，独裁，撒娇任性，平凡，没有自信，行动力不足，意志薄弱，被支配。"},
	{"教皇正位", "援助，同情，宽宏大量，可信任的人给予的劝告，良好的商量对象，得到精神上的满足，遵守规则，志愿者。"},
	{"教皇逆位", "错误的讯息，恶意的规劝，上当，援助被中断，愿望无法达成，被人利用，被放弃。"},
	{"恋人正位", "撮合，爱情，流行，兴趣，充满希望的未来，魅力，增加朋友。"},
	{"恋人逆位", "禁不起诱惑，纵欲过度，反覆无常，友情变淡，厌倦，争吵，华丽的打扮，优柔寡断。"},
	{"战车正位", "努力而获得成功，胜利，克服障碍，行动力，自立，尝试，自我主张，年轻男子，交通工具，旅行运大吉。"},
	{"战车逆位", "争论失败，发生纠纷，阻滞，违返规则，诉诸暴力，顽固的男子，突然的失败，不良少年，挫折和自私自利。"},
	{"力量正位", "大胆的行动，有勇气的决断，新发展，大转机，异动，以意志力战胜困难，健壮的女人。"},
	{"力量逆位", "胆小，输给强者，经不起诱惑，屈服在权威与常识之下，没有实践便告放弃，虚荣，懦弱，没有耐性。"},
	{"隐者正位", "隐藏的事实，个别的行动，倾听他人的意见，享受孤独，有益的警戒，年长者，避开危险，祖父，乡间生活。"},
	{"隐者逆位", "憎恨孤独，自卑，担心，幼稚思想，过于慎重导致失败，偏差，不宜旅行。"},
	{"命运之轮正位", "关键性的事件，有新的机会，因的潮流，环境的变化，幸运的开端，状况好转，问题解决，幸运之神降临。"},
	{"命运之轮逆位", "挫折，计划泡汤，障碍，无法修正方向，往坏处发展，恶性循环，中断。"},
	{"正义正位", "公正、中立、诚实、心胸坦荡、表里如一、身兼二职、追求合理化、协调者、与法律有关、光明正大的交往、感情和睦。"},
	{"正义逆位", "失衡、偏见、纷扰、诉讼、独断专行、问心有愧、无法两全、表里不一、男女性格不合、情感波折、无视社会道德的恋情。"},
	{"倒吊人正位", "接受考验、行动受限、牺牲、不畏艰辛、不受利诱、有失必有得、吸取经验教训、浴火重生、广泛学习、奉献的爱。"},
	{"倒吊人逆位", "无谓的牺牲、骨折、厄运、不够努力、处于劣势、任性、利己主义者、缺乏耐心、受惩罚、逃避爱情、没有结果的恋情。"},
	{"死神正位", "失败、接近毁灭、生病、失业、维持停滞状态、持续的损害、交易停止、枯燥的生活、别离、重新开始、双方有很深的鸿沟、恋情终止。"},
	{"死神逆位", "抱有一线希望、起死回生、回心转意、摆脱低迷状态、挽回名誉、身体康复、突然改变计划、逃避现实、斩断情丝、与旧情人相逢。"},
	{"节制正位", "单纯、调整、平顺、互惠互利、好感转为爱意、纯爱、深爱。"},
	{"节制逆位", "消耗、下降、疲劳、损失、不安、不融洽、爱情的配合度不佳。"},
	{"恶魔正位", "被束缚、堕落、生病、恶意、屈服、欲望的俘虏、不可抗拒的诱惑、颓废的生活、举债度日、不可告人的秘密、私密恋情。"},
	{"恶魔逆位", "逃离拘束、解除困扰、治愈病痛、告别过去、暂停、别离、拒绝诱惑、舍弃私欲、别离时刻、爱恨交加的恋情。"},
	{"塔正位", "破产、逆境、被开除、急病、致命的打击、巨大的变动、受牵连、信念崩溃、玩火自焚、纷扰不断、突然分离，破灭的爱。"},
	{"塔逆位", "困境、内讧、紧迫的状态、状况不佳、趋于稳定、骄傲自大将付出代价、背水一战、分离的预感、爱情危机。"},
	{"星星正位", "前途光明、充满希望、想象力、创造力、幻想、满足愿望、水准提高、理想的对象、美好的恋情。"},
	{"星星逆位", "挫折、失望、好高骛远、异想天开、仓皇失措、事与愿违、工作不顺心、情况悲观、秘密恋情、缺少爱的生活。"},
	{"月亮正位", "不安、迷惑、动摇、谎言、欺骗、鬼迷心窍、动荡的爱、三角关系。"},
	{"月亮逆位", "逃脱骗局、解除误会、状况好转、预知危险、等待、正视爱情的裂缝。"},
	{"太阳正位", "活跃、丰富的生命力、充满生机、精力充沛、工作顺利、贵人相助、幸福的婚姻、健康的交际。"},
	{"太阳逆位", "消沉、体力不佳、缺乏连续性、意气消沉、生活不安、人际关系不好、感情波动、离婚。"},
	{"审判正位", "复活的喜悦、康复、坦白、好消息、好运气、初露锋芒、复苏的爱、重逢、爱的奇迹。"},
	{"审判逆位", "一蹶不振、幻灭、隐瞒、坏消息、无法决定、缺少目标、没有进展、消除、恋恋不舍。"},
	{"世界正位", "完成、成功、完美无缺、连续不断、精神亢奋、拥有毕生奋斗的目标、完成使命、幸运降临、快乐的结束、模范情侣。"},
	{"世界逆位", "未完成、失败、准备不足、盲目接受、一时不顺利、半途而废、精神颓废、饱和状态、合谋、态度不够融洽、感情受挫。"},
};

const std::string getMsg(const std::string& key, AttrObject maptmp){
	return fmt->format(fmt->msg_get(key), maptmp);
}
const std::string getComment(const std::string& key) {
	if (auto it{ GlobalComment.find(key) };it!= GlobalComment.end())return it->second;
	return {};
}
