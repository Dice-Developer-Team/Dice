/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123���
 * Copyright (C) 2019-2024 String.Empty
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
#include "GlobalVar.h"

bool Enabled = false;

std::string Dice_Short_Ver = "Dice! by ��� & Shiki Ver " + Dice_Ver;
std::string Dice_Full_Ver_On;
#ifdef __clang__

#ifdef _MSC_VER
std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__) + " with MSVC " + std::to_string(_MSC_VER) + " " + __DATE__ + " " + __TIME__;
#elif defined(__GNUC__)
std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__) + " with GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__;
#else
std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#endif

#else
#ifdef _MSC_VER
std::string Dice_Full_Ver = std::string(Dice_Short_Ver) + "[MSVC " + std::to_string(_MSC_VER) + " " + __DATE__ +
" " + __TIME__ + "]";
#elif defined(__GNUC__)
std::string Dice_Full_Ver = Dice_Short_Ver + " [GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__;
#else
std::string Dice_Full_Ver = Dice_Short_Ver + " [UNKNOWN COMPILER";
#endif

#endif

//std::unique_ptr<ExtensionManager> ExtensionManagerInstance;

#ifdef _WIN32
HMODULE hDllModule = nullptr;
#endif

bool msgSendThreadRunning = false;

const dict_ci<string> PlainMsg
{
	{"strParaEmpty","��������Ϊ�ա�"},			//͵�������ܻظ�
	{"strParaIllegal","�����Ƿ���"},			//͵�������ܻظ�
	{"stranger","İ����"},			//{nick}�޷���ȡ�ǿ��ǳ�ʱ�ĳƺ�
	{"strCallUser", "�û�"},
	{"strSummonWord", ""},
	{"strSummonEmpty", "�ٻ�{self}��{nick}�к��£�"},
	{"strModList", "{self}�ļ������б�:{li}"},
	{"strModOn", "����{self}�˸񼤻�����塸{mod}����"},
	{"strModOnAlready", "{self}�ļ����塸{mod}���Ѽ��"},
	{"strModOff", "�ѽ������塸{mod}������{self}�˸��"},
	{"strModOffAlready", "{self}�ļ����塸{mod}����ͣ�ã�"},
	{"strModReload", "��Ϊ{self}�˸����������塸{mod}����"},
	{"strModDelete", "�ѽ������塸{mod}���ų�{self}���˸��"},
	{"strModNameEmpty", "��{nick}����ģ������"},
	{"strModDescLocal", "{self}��ֲ��ģ��:{mod_desc}"},
	{"strModDescCloud", "{self}�ѻ�ȡԴģ����Ϣ:{mod_desc}"},
	{"strModDetail", "{self}����ģ����ϸ��Ϣ:{mod_detail}"},
	{"strModInstalled", "{self}�ɹ�ֲ������塸{mod}��{mod_ver}��"},
	{"strModInstallErr", "{self}ע������塸{mod}��ʧ�ܡ�{err}"},
	{"strModUpdated", "����{self}�����塸{mod}��{ex_ver}->{mod_ver}��"},
	{"strModUpdateErr", "����{self}�����塸{mod}��ʧ�ܡ�{err}"},
	{"strModReinstalled", "�ɹ���д{self}�����塸{mod}��{ex_ver}->{mod_ver}��"},
	{"strModLoadErr", "{self}��ȡģ�顸{mod}��ʧ�ܡ�{err}"},
	{"strModNotFound", "{self}δ�ҵ�ָ��ģ�顸{mod}��!"},
	{"strAkForkNew","{self}�Ѵ�������\n#{fork}"},
	{"strAkAdd","{self}�Ѽ�����ѡ���\n��ǰ����:{fork}{li}"},
	{"strAkAddEmpty","����{nick}��ѡ�����{self}û������仰֮ǰһ����"},
	{"strAkDel","{self}��ɾ��ָ��ѡ���\n��ǰ����:{fork}{li}"},	
	{"strAkOptEmptyErr","��ѡ��Ϊ�ա�\n{nick}����{self}���һ��ѡ����"},
	{"strAkNumErr","��{nick}ѡ��Ϸ���ѡ����š�"},
	{"strAkGet","#{fork}{li}\n\n{self}��ѡ��{get}"},
	{"strAkShow","{self}�ĵ�ǰ����:{fork} {li}"},
	{"strAkClr","{self}��������ַ���{fork}��"},
	{"strLogNew","{self}���¿���¼��־��{game.log_name}����\n����ʱ��.log off��ͣ��.log end��ɼ�¼"},
	{"strLogOn","{self}��ʼ��־��¼��{game.log_name}����\n��ʹ��.log off��ͣ��¼"},
	{"strLogOnAlready","{self}���ڼ�¼��{game.log_name}���У�"},
	{"strLogOff","{self}����ͣ��־��{game.log_name}����¼��\n��ʹ��.log on�ָ���¼"},
	{"strLogOffAlready","{self}�Ѿ���ͣ��{game.log_name}����¼��"},
	{"strLogEnd","{self}�������־��¼��\n�����ϴ���־�ļ�{log_file}"},
	{"strLogEndEmpty","{self}�ѽ�����{game.log_name}����¼��\n��������־����"},
	{"strLogNullErr","{self}����־��¼���ѽ�����"},
	{"strLogUpSuccess","{self}�������־�ϴ���\n����� {log_url} �Բ鿴��¼"},
	{"strLogUpFailure","{self}�ϴ���־�ļ�ʧ�ܣ����ڵ�{retry}���ش�{log_file}��{ret}"},
	{"strLogUpFailureEnd","���ź���{self}�޷��ɹ��ϴ���־�ļ���\n{ret}\n�����ȡ����ϵMaster:{print:master}\n�ļ���:{log_file}"},
	{"strGameNew","{self}��������Ϸ��{game_id}���ɹ���"},
	{"strGameOver","��Ϸ��{game_id}�����ս��"},
	{"strGameMastered","{self}�ѽ���{nick}Ϊ������Ϸ����Ա��"},
	{"strGameMasterList","������Ϸ����Ա��:\n{items}"},
	{"strGameMasterDenied","��Ǹ��GMȨ�޲��ܽ���{nick}��"},
	{"strGameJoined","{self}�ѽ���{nick}����ұ�����"},
	{"strGamePlayerAlready","{nick}�Ѿ���������"},
	{"strGamePlayerCall","{items}"},
	{"strGamePlayerEmpty","{self}��û�������������Ŷ~"},
	{"strGameKicked","{self}�ѽ�{at:{tid}}��������"},
	{"strGameKickNotPlayer","{nick}Ҫ���ߵ�{tid}�������ϡ�"},
	{"strGameExited","���{nick}���˳���Ϸ��"},
	{"strGameNotJoined","�ţ�{nick}ԭ������������"},
	{"strGameNotMaster","��{nick}��������GM���д˲�����"},
	{"strGameVoidHere","{self}��������û���ϣ�{nick}��Ҫ������ô��(����.game new �½���Ϸ)"},
	{"strGameNotExist","{self}���ǵ���һ����{game_id}����Ϸ�ˡ�"},
	{"strGameItemSet","{self}�ѽ�������Ϸ��{set_item}����Ϊ{set_val}��"},
	{"strGameItemShow","������Ϸ��{set_item}Ϊ{set_val}��"},
	{"strGameItemEmpty","��{nick}��������õ���Ŀ��"},
	{"strGameAreaOpen","{self}���ڴ�չ����{game_id}����Ϸ�����"},
	{"strGameAreaClosed","��Ϸ����رգ�{nick}����.game open {game_id} ����չ����"},
	{"strGameRouletteSet","{self}��Ϊ{face}����������������"},
	{"strGameRouletteTooBig","��ô�󡭡�{nick}��������"},
	{"strGameRouletteHistory","{self}������������¼����:\n{hist}"},
	{"strGameRouletteEmpty","{self}��δ�ڱ����ù���������"},
	{"strGameRouletteClear","{self}�����������������"},
	{"strGameRouletteReset","{self}�Ѹ�ԭ������������"},
	{"strGMTableShow","{self}��¼��{table_name}�б�: {res}"},
	{"strGMTableClr","{self}�����{table_name}���"},
	{"strGMTableItemDel","{self}���Ƴ�{table_name}�����Ŀ{table_item}��"},
	{"strGMTableNotExist","{self}û�б���{table_name}���"},
	{"strGMTableItemNotFound","{self}û���ҵ�{table_name}�����Ŀ{table_item}��"},
	{"strGMTableItemEmpty","���֪{self}���Ƴ���{table_name}�б���Ŀ��"},
	{"strUserTrustShow","{user}��{self}�������μ���Ϊ{trust}"},
	{"strUserTrusted","�ѽ�{self}��{user}�����μ������Ϊ{trust}"},
	{"strUserTrustDenied","{nick}��{self}����Ȩ���ʶԷ���Ȩ�ޡ�"},
	{"strUserTrustIllegal","��Ŀ��Ȩ���޸�Ϊ{trust}�ǷǷ��ġ�"},
	{"strUserNotFound","{self}��{user}���û���¼"},
	{"strGroupAuthorized","A roll to the table turns to a dice fumble!\nDice Roller {strSelfName}��\n��Ⱥ����Ȩ��ɣ��뾡��ʹ�ñ������\n������Э��ʹ�ã����������ʹ��.dismiss�ͳ�!" },
	{"strGroupLicenseDeny","��Ⱥδ��{self}���ʹ�ã��Զ���Ⱥ�ھ�Ĭ��\n����.helpЭ�� �Ķ���ͬ��Э�������Ӫ���������ʹ�ã�\n���������Աʹ��!dismiss�ͳ�{self}\n�ɰ����¸�ʽ��д����������:\n!authorize ������;:[ **��д������** ] �����˽�Dice!�����÷�����ϸ�Ķ�����֤����{strSelfName}���û�Э�飬����ͣ��ָ��ʹ��[ **��д��ָ��** ]���ú�ʹ��[ **��д��ָ��** ]�ͳ�Ⱥ" },
	{"strGroupLicenseApply","��Ⱥδͨ��������Ȩ��\n��������ѷ��͡�" },
	{"strGroupSetOn","���ѿ���{self}�ڴ�Ⱥ�ġ�{option}��ѡ���"},			//Ⱥ�ڿ��غ�ң�ؿ���ͨ�ô��ı�
	{"strGroupSetOnAlready","{self}���ڴ�Ⱥ������{option}��"},			
	{"strGroupSetOff","���ѹر�{self}�ڴ�Ⱥ�ġ�{option}��ѡ���"},			
	{"strGroupSetOffAlready","{self}δ�ڴ�Ⱥ����{option}��"},
	{"strGroupMultiSet","{self}�ѽ���Ⱥ��ѡ���޸�Ϊ:{opt_list}"},
	{"strGroupSetAll","{self}���޸ļ�¼��{cnt}��Ⱥ�ġ�{option}��ѡ���"},
	{"strGroupDenied","{nick}��{self}����Ȩ���ʴ�Ⱥ�����á�"},
	{"strGroupSetDenied","{nick}��{self}������{option}��Ȩ�޲����"},
	{"strGroupSetInvalid","{nick}����������Ч��Ⱥ����{option}��"},
	{"strGroupSetNotExist","{self}��{option}��ѡ���"},
	{"strGroupWholeUnban","{self}�ѹر�ȫ�ֽ��ԡ�"},
	{"strGroupWholeBan","{self}�ѿ���ȫ�ֽ��ԡ�"},
	{"strGroupWholeBanErr","{self}����ȫ�ֽ���ʧ�ܡ�"},
	{"strGroupUnban","{self}�ö�:{member}������ԡ�"},
	{"strGroupBan","{self}�ö�:{member}����{res}���ӡ�"},
	{"strGroupNotFound","{self}��Ⱥ{group_id}��¼��"},
	{"strGroupNot","{group}����Ⱥ��"},
	{"strGroupNotIn","{self}��ǰ����{group}�ڡ�"},
	{"strGroupExit","{self}���˳���Ⱥ��"},
	{"strGroupCardSet","{self}�ѽ�{target}��Ⱥ��Ƭ�޸�Ϊ��{card}����"},
	{"strGroupTitleSet","{self}�ѽ�{target}��ͷ���޸�Ϊ��{title}����"},
	{"strPcNewEmptyCard","��Ϊ{nick}�½�{type}�հ׿���{char}����"},
	{"strPcNewCardShow","��Ϊ{nick}�½�{type}����{char}����{show}"},//����Ԥ����ѡ�����������
	{"strPcCardSet","�ѽ�{nick}��ǰ��ɫ����Ϊ��{char}����"},//{nick}-�û��ǳ� {pc}-ԭ��ɫ���� {char}-�½�ɫ����
	{"strPcCardReset","�ѽ��{nick}��ǰ��Ĭ�Ͽ���"},//{nick}-�û��ǳ� {pc}-ԭ��ɫ����
	{"strPcCardRename","�ѽ�{old_name}������Ϊ��{new_name}����"},
	{"strPcCardDel","�ѽ���ɫ����{char}��ɾ����"},
	{"strPcCardCpy","�ѽ���{char2}�������Ը��Ƶ���{char1}����"},
	{"strPcClr","�����{nick}�Ľ�ɫ����¼��"},
	{"strPcCardList","{nick}�Ľ�ɫ�б�{show}"},
	{"strPcCardBuild","{nick}�ġ�{char}�����ɣ�{show}"},
	{"strPcCardShow","{nick}��<{type}>{char}��{show}"},	//{nick}-�û��ǳ� {type}-��ɫ������ {char}-��ɫ����
	{"strPcCardRedo","{nick}�ġ�{char}���������ɣ�{show}"},
	{"strPcGroupList","{nick}�ĸ�Ⱥ��ɫ�б�{show}"},
	{"strPcStatShow","{pc}��{self}��������ͳ��:{stat}"},
	{"strPcStatEmpty","{pc}��{self}����û�м춨����¼�����ӡ�"},
	{"strPcNotExistErr","{self}��{nick}�Ľ�ɫ����¼���޷�ɾ����"},
	{"strPcCardFull","{nick}��{self}���Ľ�ɫ���Ѵ����ޣ�������������ɫ����"},
	{"strPcTempChange","{self}�ѽ�{pc}��ģ���л�Ϊ{new_type}��"},
	{"strPcTempInvalid","{self}�޷�ʶ��Ľ�ɫ��ģ���"},
	{"strPcNameEmpty","���Ʋ���Ϊ�ա�"},
	{"strPcNameExist","{self}�Ѽ�¼��ͬ����{char}����"},
	{"strPcNameNotExist","{self}δ��¼{nick}��Ϊ��{char}����ɫ����"},
	{"strPcNameInvalid","�Ƿ��Ľ�ɫ����������ð�ţ���"},
	{"strPcInitDelErr","{nick}�ĳ�ʼ������ɾ����"},
	{"strPcTextTooLong","�ı����Ȳ��ܳ���255��"},
	{"strPcLockedKill","{self}������{pc}������ɾ����" },
	{"strPcLockedName","{self}������{pc}�����ɸ�����" },
	{"strPcLockedWrite","{self}������{pc}����������д���" },
	{"strPcLockedRead","{self}������{pc}������鿴���ԡ�" },
	{"strSetDefaultDice","{self}�ѽ�{pc}��Ĭ��������ΪD{default}��"},
	{"strCOCBuild","{pc}�ĵ���Ա����:{res}"},
	{"strDNDBuild","Ϊ{pc}��ð�������������ά������:{res}"},
	{"strCensorCaution","���ѣ�{nick}��ָ��������дʣ�{self}���ϱ�"},
	{"strCensorWarning","���棺{nick}��ָ��������дʣ�{self}�Ѽ�¼���ϱ���"},
	{"strCensorDanger","���棺{nick}��ָ��������дʣ�{self}�ܾ�ָ����ϱ���"},
	//{"strCensorCritical","���棺{nick}��ָ��������дʣ�{self}�Ѽ�¼���ϱ���"},
	{"strSpamFirstWarning","���ʱ���ڶ�{self}ָ��������࣡�����ö��������͸�������ָ�ˢ�����ξ��棩"},
	{"strSpamFinalWarning","����ͣ���һ��ָ��������Ƶָ�{self}���ڣ���ˢ�����վ��棩"},
	{"strRegexInvalid","������ʽ{key}��Ч: {err}"},
	{"strReplyOn","{self}���ڱ�Ⱥ���ùؼ��ʻظ���"},
	{"strReplyOff","{self}���ڱ�Ⱥ���ùؼ��ʻظ���"},
	{"strReplySet","{self}�����ùؼ��ʻظ���Ŀ{key}��"},
	{"strReplyShow","{self}�Ĺؼ�����Ŀ{key}Ϊ:{show}"},
	{"strReplyList","{self}�Ļظ������ʹ���:{res}"},
	{"strReplyDel","{self}���Ƴ��ظ��ؼ�����Ŀ{key}��"},
	{"strReplyKeyEmpty","{nick}������ظ������ʡ�"},
	{"strReplyKeyNotFound","{self}δ�ҵ��ظ��ؼ���{key}��"},
	{"strScriptRunErr","{self}�ƺ����˵����⣬��{nick}���ĵȴ���{lang}�ű����г���" },
	//{"strStModify","{self}�Ѽ�¼{pc}��{cnt}�����Ա仯:\n{change}"},		//���ڼ���ֵ�仯���ʱ������ʹ�ô��ı�
	{"strStDetail","{self}�ѱ༭{pc}��{cnt}������:\n{detail}"},		//��������ʱ��ʹ�ô��ı�
	//{"strStValEmpty","{self}δ��¼{attr}ԭֵ��"},		
	{"strBlackQQAddNotice","{nick}�����ѱ�{self}�������������������ϵMaster:{print:master}"},				
	{"strBlackQQAddNoticeReason","{nick}������{reason}�����ѱ�{self}��������������߽������ϵ����Ա��Master:{print:master}"},
	{"strBlackQQDelNotice","{nick}�����ѱ�{self}�Ƴ������������ڿ��Լ���ʹ����"},
	{"strWhiteQQAddNotice","{user_nick}�����ѻ��{self}�����Σ��뾡��ʹ��{self}��"},
	{"strWhiteQQDenied","�㲻��{self}�������û���"},
	{"strDeckNew","{self}��Ϊ{nick}�Զ������ƶ�<{deck_name}>��"},
	{"strDeckSet","{nick}����<{deck_name}>����{self}���ƶ�ʵ����"},
	{"strDeckSetRename","{nick}����<{deck_cited}>����{self}���ƶ�ʵ��{deck_name}��"},
	{"strDeckRestEmpty","�ƶ�<{deck_name}>�ѳ�գ���ʹ��.deck reset {deck_name}�ֶ������ƶ�"},		
	{"strDeckOversize","{nick}�������̫�࣬{self}װ��������"},
	{"strDeckRestShow","��ǰ�ƶ�<{deck_name}>ʣ�࿨��:{deck_rest}"},
	{"strDeckRestReset","{self}�������ƶ�ʵ��<{deck_name}>��"},
	{"strDeckDelete","{self}���Ƴ��ƶ�ʵ��<{deck_name}>��"},
	{"strDeckListShow","��{self}���������ƶ�ʵ����:{res}"},
	{"strDeckListClr","{nick}�����{self}���ƶ�ʵ����"},
	{"strDeckListEmpty","{self}���ƶ�ʵ���б�Ϊ�գ�"},
	{"strDeckNewEmpty","{self}�޷�Ϊ{nick}�½�����ƶѡ�"},
	{"strDeckListFull","{self}���ƶ�ʵ���Ѵ����ޣ�������������ʵ����"},
	{"strDeckNotFound","{self}�Ҳ����ƶ�{deck_name}��"},
	{"strDeckCiteNotFound","{self}�Ҳ��������ƶ�{deck_cited}��" },
	{"strDeckNameEmpty","{nick}δָ���ƶ�����"},
	{"strRangeEmpty","{self}û�����ſ���������" },
	{"strOutRange","{nick}��������г���{self}����Χ��" },
	{"strRollDice","{pc}����: {res}"},
	{"strRollDiceReason","{pc}���� {reason}: {res}"},
	{"strRollHidden","{pc}������һ�ΰ���"},
	{"strRollTurn","{pc}����������: {turn}��"},
	{"strRollMultiDice","{pc}����{turn}��: {dice_exp}={res}"},
	{"strRollMultiDiceReason","{pc}����{turn}��{reason}: {dice_exp}={res}"},
	{"strRollInit","{char}���ȹ����㣺{res}" },
	{"strRollSkill","{pc}����{attr}�춨��"},
	{"strRollSkillReason","����{reason} {pc}����{attr}�춨��"},
	{"strRollSkillHidden","{pc}������һ�ΰ���{attr}�춨��" },
	{"strEnRoll","{pc}��{attr}��ǿ��ɳ��춨��\n{res}"},//{attr}���û�ʡ�Լ��������滻Ϊ{strEnDefaultName}
	{"strEnRollNotChange","{strEnRoll}\n{pc}��{attr}ֵû�б仯"},
	{"strEnRollFailure","{strEnRoll}\n{pc}��{attr}�仯{change}�㣬��ǰΪ{final}��"},
	{"strEnRollSuccess","{strEnRoll}\n{pc}��{attr}����{change}�㣬��ǰΪ{final}��"},
	{"strEnDefaultName","���Ի���"},//Ĭ���ı�
	{"strEnValEmpty", "δ��{self}�趨���ɳ�����ֵ������.st {attr} ����ֵ ��鿴.help en��"},
	{"strEnValInvalid", "{attr}ֵ���벻��ȷ,������1-99��Χ�ڵ�����!"},
	{"strSendMsg","{self}�ѽ���Ϣ�ͳ���"},//Master�����͵Ļ�ִ
	{"strSendMasterMsg","��Ϣ{self}�ѷ��͸�Master��"},//��Master���͵Ļ�ִ
	{"strSendMsgEmpty","������Ϣ����Ϊ�ա�"},
	{"strSendMsgInvalid","{self}û�п��Է��͵Ķ����"},//û��Master
	{"strDefaultCOCShow","{self}��¼Ĭ�Ϸ���Ϊ{rule}" },
	{"strDefaultCOCShowDefault","{self}δ��¼Ĭ�Ϸ��棬ʹ��Ĭ�Ϸ���{rule}" },
	{"strDefaultCOCClr","Ĭ�ϼ춨�����������"},
	{"strDefaultCOCNotFound","Ĭ�ϼ춨���治���ڡ�"},
	{"strDefaultCOCSet","Ĭ�ϼ춨����������:"},
	{"strLinked","{self}��Ϊ����{target}�������ӡ�"},
	{"strLinkClose","{self}�ѶϿ����������ӡ�" },
	{"strLinkBusy","{nick}��Ŀ���Ѿ��ж�������\n{self}��֧�ֶ�߹�ϵ" },
	{"strLinkState","{self}��ǰ����״̬:{link_info}" },
	{"strLinkList","{self}��ǰ�����б�:{link_list}" },
	{"strLinkedAlready","{self}���ڱ������������ӡ�\n��{nick}�ȶϾ���ǰ��ϵ" },
	{"strLinkingAlready","{self}�Ѿ�����������!" },
	{"strLinkCloseAlready","{self}�Ͽ�����ʧ�ܣ�{nick}��ǰ����û�ж���" },
	{"strLinkNotFound","{self}�Ҳ���{nick}�Ķ����"},
	{"strNotMaster","�㲻��{self}��master��������ʲô��"},
	{"strNotAdmin","�㲻��{self}�Ĺ���Ա��"},
	{"strAdminDismiss","{strDismiss}"},					//����Աָ����Ⱥ�Ļ�ִ
	{"strDismiss",""},						//.dismiss��Ⱥǰ�Ļ�ִ
	{"strHlpSet","��Ϊ{key}���ô�����"},
	{"strHlpReset","�����{key}�Ĵ�����"},
	{"strHlpNameEmpty","Master��Ҫ�Զ���ʲô����ѽ��"},
	{"strHelpNotFound","{self}δ�ҵ���{help_word}����صĴ�����"},
	{"strHelpSuggestion","{self}��{nick}��Ҫ���ҵ���:\n{res}"},
	{"strHelpRedirect","{self}���ҵ����������{redirect_key}��:\n{redirect_res}" },
	{"strClockToWork","{self}�Ѱ�ʱ���á�"},
	{"strClockOffWork","{self}�Ѱ�ʱ�رա�"},
	{"strNameGenerator","{pc}��������ƣ�{res}"},
	{"strDrawCard", "������{pc}�鵽��ʲô��{res}"},
	{"strDrawHidden", "{pc}����{cnt}�����ơ�" },
	{"strMeOn", "�ɹ�����������{self}��.me�����"},
	{"strMeOff", "�ɹ����������{self}��.me�����"},
	{"strMeOnAlready", "������{self}��.me����û�б�����!"},
	{"strMeOffAlready", "������{self}��.me�����Ѿ�������!"},
	{"strObOn", "{self}���ڱ��������Թۡ�"},
	{"strObOff", "{self}���ڱ��������Թۡ�"},
	{"strObOnAlready", "�ڱ���{self}δ�����Թ�!"},
	{"strObOffAlready", "�ڱ���{self}�ѽ����Թ�!"},
	{"strObList", "����{self}���Թ�����:"},
	{"strObListEmpty", "����{self}�����Թ���"},
	{"strObListClr", "{self}������Թ��ߡ�"},
	{"strObEnter", "{nick}�ɹ�����{self}���Թۡ�"},
	{"strObExit", "{nick}�ɹ��˳�{self}���Թۡ�"},
	{"strObEnterAlready", "{nick}�Ѿ�����{self}���Թ�ģʽ!"},
	{"strObExitAlready", "{nick}û�м���{self}���Թ�ģʽ!"},
	{"strUIDEmpty", "��{nick}д���˺š�"},
	{"strGroupIDEmpty", "��{nick}д��Ⱥ�š�"},
	{"strBlackGroup", "��Ⱥ�ں������У�������������ϵ{print:master}"},
	{"strBotOn", "�ɹ�����{self}��"},
	{"strBotOff", "�ɹ��ر�{self}��"},
	{"strBotOnAlready", "{self}�Ѿ����ڿ���״̬!"},
	{"strBotOffAlready", "{self}�Ѿ����ڹر�״̬!"},
	{"strBotChannelOn", "����Ƶ���ڿ���{self}��" },
	{"strBotChannelOff", "����Ƶ���ڹر�{self}��" },
	{"strRollCriticalSuccess", "��ɹ���"}, //һ��춨��
	{"strRollExtremeSuccess", "���ѳɹ�"},
	{"strRollHardSuccess", "���ѳɹ�"},
	{"strRollRegularSuccess", "�ɹ�"},
	{"strRollFailure", "ʧ��"},
	{"strRollFumble", "��ʧ�ܣ�"},
	{"strFumble", "��ʧ�ܣ�"}, //���ּ춨�ã�����Ƴ���
	{"strFailure", "ʧ��"},
	{"strSuccess", "�ɹ�"},
	{"strHardSuccess", "���ѳɹ�"},
	{"strExtremeSuccess", "���ѳɹ�"},
	{"strCriticalSuccess", "��ɹ�!"},
	{"strNumCannotBeZero", "���������Ŀ��ĪҪ��ǲ����!"},
	{"strDeckNotFound", "��˵{deck_name}��{self}û��˵�����ƶ����ء���"},
	{"strDeckEmpty", "{self}�Ѿ�һ��Ҳ��ʣ�ˣ�"},
	{"strNameNumTooBig", "��ȡ���ٸ����ְ�!��{nick}����1-10֮�������!"},
	{"strNameNumCannotBeZero", "��{self}ȡ0�����֣���{nick}����1-10֮�������!"},
	{"strSetTooBig", "�������������Ҷ�����!��{nick}����1-9999֮�������!"},
	{"strSetCannotBeZero", "Ĭ��������Ϊ��!��{nick}����1-9999֮�������!"},
	{"strCharacterCannotBeZero", "�������ɴ�������Ϊ��!������1-10֮�������!"},
	{"strCharacterTooBig", "�������ɴ�������!������1-10֮�������!"},
	{"strCharacterInvalid", "�������ɴ�����Ч!������1-10֮�������!"},
	{"strSanityRoll", "{pc}��San Check��\n{res} {grade:rank?2={strSuccess}&1={strFailure}&0={strFumble}}\n{case:loss?0=��������ʧ��&else=���Ǽ���{change}->ʣ��{final}}" },
	{"strSanCostInvalid", "{pc}����SC���ʽ����ȷ,��ʽΪ�ɹ���San/ʧ�ܿ�San,��1/1d6!"},
	{"strSanInvalid", "Sanֵ���벻��ȷ,��{pc}����1-99��Χ�ڵ�����!"},
	{"strSanEmpty", "δ�趨Sanֵ����{pc}��.st san ��鿴.help sc��"},
	{"strTempInsane", "{pc}�ķ����-��ʱ֢״:\n{res}" },
	{"strLongInsane", "{pc}�ķ����-�ܽ�֢״:\n{res}" },
	{"strSuccessRateErr", "��ɹ��ʻ���Ҫ�춨��"},
	{"strGroupIDInvalid", "��Ч��Ⱥ��!"},
	{"strSendSuccess", "����ִ�гɹ���"},
	{"strActionEmpty", "��������Ϊ�ա�"},
	{"strMEDisabledErr", "����Ա���ڴ�Ⱥ�н���.me����!"},
	{"strDisabledMeGlobal", "ˡ{self}���ṩ.me�����"},
	{"strDisabledJrrpGlobal", "ˡ{self}���ṩ.jrrp�����"},
	{"strDisabledDeckGlobal", "ˡ{self}���ṩ.deck�����"},
	{"strDisabledDrawGlobal", "ˡ{self}���ṩ.draw�����"},
	{"strDisabledSendGlobal", "ˡ{self}���ṩ.send�����"},
	{"strHELPDisabledErr", "����Ա���ڴ�Ⱥ�н���.help����!"},
	{"strNameDelEmpty", "{nick}δ��������,�޷�ɾ��!"},
	{"strValueErr", "�������ʽ�������!"},
	{"strInputErr", "������������ʽ�������:{dice_exp}"},
	{"strUnknownErr", "������δ֪����!"},
	{"strUnableToGetErrorMsg", "�޷���ȡ������Ϣ!"},
	{"strDiceTooBigErr", "{self}�����ӳ���������û�ˡ����������ࣩ"},
	{"strRequestRetCodeErr", "{self}���ʷ�����ʱ���ִ���! HTTP״̬��: {error}"},
	{"strRequestNoResponse", "������δ�����κ���Ϣ��"},
	{"strTypeTooBigErr", "��!��{self}���������ж�������~1...2..."},
	{"strZeroTypeErr", "����...!!ʱ����*{self}�����Ӳ�����ʱ���ѷ������*������Ϊ0��"},
	{"strAddDiceValErr", "������Ҫ��{self}�������ӵ�ʲôʱ����~(��������ȷ�ļ�������:5-10֮�ڵ�����)"},
	{"strZeroDiceErr", "��?{self}��������?������Ϊ0��"},
	{"strRollTimeExceeded", "{nick}��������������������������ơ�"},
	{"strRollTimeErr", "{nick}�����������쳣��"},
	{"strDismissPrivate", "����˽�Ĵ�����{self}�����أ�"},
	{"strWelcomePrivate", "����˽�Ĵ�����{self}��ӭ˭�أ�"},
	{"strWelcomeMsgClearNotice", "{self}�������Ⱥ����Ⱥ��ӭ�ʡ�"},
	{"strWelcomeMsgClearErr", "û������{self}��Ⱥ��ӭ�ʣ����ʧ�ܡ�"},
	{"strWelcomeMsgUpdateNotice", "{self}�Ѹ��±�Ⱥ����Ⱥ��ӭ�ʡ�"},
	{"strWelcomeMsgEmpty", "{self}��ǰδ��¼��Ⱥ��ӭ��" },
	{"strPermissionDeniedErr", "����Ⱥ�ڹ����{self}���͸�ָ���"},
	{"strSelfPermissionErr", "{self}Ȩ�޲�������Ϊ���ء�"},
	{"strNameTooLongErr", "�ƺ�������(���50�ֽ�)"},
	{"strNameSet", "{self}�ѽ�{old_nick}�ĳ�Ϊ{new_nick}��"},
	{"strNameDel", "{self}��ɾ��{old_nick}�ڵ�ǰ���ڵĳƺ���" },
	{"strNameClr", "{self}�����{old_nick}�����гƺ���" },
	{"strUnknownPropErr", "δ�趨{attr}�ɹ��ʣ�����.st {attr} ����ֵ ��鿴.help rc��"},
	{"strPropErr", "{pc}������¼������쳣�������ع淶:\n{err}"},
	{"strSetPropSuccess", "��Ϊ{pc}¼��{cnt}�����ԡ�"},
	{"strPropCleared", "�����{char}���������ԡ�"},
	{"strRuleReset", "������Ĭ�Ϲ����"},
	{"strRuleSet", "������Ĭ�Ϲ����"},
	{"strRuleErr", "�������ݻ�ȡʧ��,������Ϣ:\n"},
	{"strRulesFailedErr", "����ʧ��,{self}�޷��������ݿ��"},
	{"strPropDeleted", "��ɾ��{pc}��{attr}��"},
	{"strPropNotFound", "����{attr}�����ڡ�"},
	{"strRuleNotFound", "{self}δ�ҵ���Ӧ�Ĺ�����Ϣ��"},
	{"strProp", "{pc}��{attr}Ϊ{val}"},
	{"strPropList", "{nick}��{char}�����б�Ϊ��{show}"},
	//{"strStErr", "��ʽ����:��ο�.help st��ȡ.st�����ʹ�÷���"},
	{"strRulesFormatErr", "��ʽ����:��ȷ��ʽΪ.rules[��������:]������Ŀ ��.rules COC7:����"},
	{"strLeaveDiscuss", "{self}�ֲ�֧����������񣬼����˳�"},
	{"strLeaveNoPower", "{self}δ���Ⱥ����������Ⱥ"},
	{"strLeaveUnused", "{self}�Ѿ������ﱻ����{day}���������Ͼͻ��뿪������"},
	{"strGlobalOff", "{self}�ݼ��У���ͣ�����"},
	{"strPreserve", "{self}˽��˽�ã��������\n������������뷢��!authorize +[Ⱥ��] ������;:[ **��д������** ] �����˽�Dice!�����÷�����ϸ�Ķ�����֤����{strSelfName}���û�Э�飬����ͣ��ָ��ʹ��[ **��д��ָ��** ]���ú�ʹ��[ **��д��ָ��** ]�ͳ�Ⱥ"},
	{"strJrrp", "{nick}�������Ʒֵ��: {res}"},
	{ "strFriendDenyNotUser", "���ź�����û�ж�{self}ʹ��ָ��ļ�¼" },
	{ "strFriendDenyNoTrust", "���ź����㲻��{self}���ε��û�������ʹ�ÿ���ϵ{print:master}" },
	{"strAddFriendWhiteQQ", "{strAddFriend}"}, //�������û���Ӻ���ʱ�ظ��˾�
	{
		"strAddFriend",
		R"(��ӭѡ��{strSelfName}�������������
.helpЭ�� ȷ�Ϸ���Э��
.helpָ�� �鿴ָ���б�
.help�趨 ȷ�������趨
.help���� �鿴Դ���ĵ�
ʹ�÷���Ĭ���Ѿ�ͬ�����Э��)"
	}, //ͬ����Ӻ���ʱ���ⷢ�͵����
	{
		"strAddGroup",
		R"(��ӭѡ��{strSelfName}�������������
��ʹ��.dismiss QQ�ţ������λ�� ʹ{self}��Ⱥ��������
.bot on/off QQ�ţ������λ�� //������ر�ָ��
.reply on/off ����/���ûظ� //���û����ûظ�
.helpЭ�� ȷ�Ϸ���Э��
.helpָ�� �鿴ָ���б�
.help�趨 ȷ�������趨
.help���� �鿴Դ���ĵ�
������ȺĬ����Ϊͬ�����Э�飬֪�����Ի��Ƴ��ĺ��)"
	}, 
	{ "strNewMaster","���ʣ������{strSelfName}��Master��\n�������Ķ���ǰ�汾Master�ֲ��Լ��û��ֲᡣ��ע��汾�Ŷ�Ӧ: https://v2docs.kokona.tech\f{strSelfName}Ĭ�Ͽ�����Ⱥ�Ƴ������ԡ�ˢ���¼��ļ�������Ҫ�ر����ֶ�������\n��ע���ƺ�ϵͳĬ�Ͽ�����������˹�����ر�CloudBlackShare��" },
	{ "strNewMasterPublic",R"({strSelfName}�Թ���ģʽ������
�Զ�����BelieveDiceList��Ӧ���������б��warning��
�ѿ����Զ�ͨ���Ǻ������������룻
�ѿ����������Զ���������ʱ��ÿ�ն�ʱ���Զ�������������û��Ĺ�ͬȺ�ģ��������û�ȺȨ�޲������Լ�ʱ�Զ���Ⱥ��
�ѿ�������Ⱥʱ���������ˣ�
������send���ܽ����û����͵���Ϣ��)" },
	{ "strNewMasterPrivate",R"({strSelfName}��˽��ģʽ������
Ĭ�Ͼܾ�İ���˵�Ⱥ���룬ֻͬ�����Թ���Ա���������û������룻
Ĭ�Ͼܾ�İ���˵ĺ������룬��Ҫͬ���뿪��AllowStranger��
�ѿ����������Զ���������ʱ��ÿ�ն�ʱ���Զ�������������û��Ĺ�ͬȺ�ģ��������û�ȺȨ�޸����Լ�ʱ�Զ���Ⱥ��
.me����Ĭ�ϲ����ã���Ҫ�ֶ�������
�л�������ʹ��.admin public���������ʼ����Ӧ���ã�
����.master delete��ʹ��.master public�����³�ʼ����)" },
	{"strSelfCall", "&strSelfName"},
	{"strSelfName", "" },
	{"strSelfNick", "&strSelfName" },
	{"self", "&strSelfCall"},
	{"strBotHeader", "������ " },
	{"strBotMsg", "\nʹ��.help �鿴{self}�����ĵ�"},
	{"strHlpMsg", R"(��ʹ��.dismiss ID�������λ�� ʹ{self}��Ⱥ��������
.bot on/off ID�������λ�� //������ر�ָ��
.helpЭ�� ȷ�Ϸ���Э��
.helpָ�� �鿴ָ���б�
.helpȺ�� �鿴Ⱥ��ָ��
.help�趨 ȷ�������趨
.help���� �鿴Դ���ĵ�
�ٷ���̳: https://forum.kokona.tech/
Dice!�ڳ�ƻ�: https://afdian.net/@suhuiw4123)"
	}
};
dict_ci<string> GlobalMsg{ PlainMsg };

fifo_dict_ci<string> EditedMsg;
fifo_dict_ci<string> CustomHelp;
const dict_ci<string> GlobalComment{
	{"self", "�Գƣ�������strSelfCall"},
	//{"strActionEmpty", "��ǰ����"},
	{"strAddDiceValErr", "wwָ�����ֵ�Ƿ�����С��"},
	{"strAddFriend", "ͨ��������͸����ѵ�ӭ�´�"},
	{"strAddFriendWhiteQQ", "ͨ���������û����Ѻ��ӭ�´�"},
	{"strAddGroup", "������ȺʱȺ�ڷ��͵��볡��"},
	{"strAdminDismiss", "dismissָ���ɹ���ʹ��ʱ���˳��ʣ�Ĭ��һ��"},
	{"strAdminOptionEmpty", "adminָ�����Ϊ��"},
	{"stranger", "�ǳƿհ׻��޷���ȡʱ�Ĵ���"},
	{"strBlackGroup", "ʶ�𵽺�����Ⱥ���˳���"},
	{"strBlackQQAddNotice", "�����ڶ�����ʹ�ü�¼ʱ����֪ͨ"},
	{"strBlackQQAddNoticeReason", "�����ɵ�����֪ͨ"},
	{"strBlackQQDelNotice", "���֪ͨ"},
	{"strBotHeader", "bot��ִ��ͷ�����ı�"},
	{"strBotMsg", "bot��ִ����Dice��Ϣ����ı�"},
	{"strBotOff", "botָ��Ⱥ�ڹرջ�ִ"},
	{"strBotOffAlready", "botָ��Ⱥ���Ѿ��ر�"},
	{"strBotOn", "botָ��Ⱥ�ڿ�����ִ"},
	{"strBotOnAlready", "botָ��Ⱥ���Ѿ�����"},
	//
	{"strCallUser", "���û��ĳƺ�"},
	//
	{"strCOCBuild", "cocָ���ִ"},
	{"strCriticalSuccess", "���ּ춨��ɹ�"},
	//
	{"strDeckNew", "deckָ����ƶ�ʵ��"},
	{"strDeckSet", "deckָ�����ù����ƶ�Ϊʵ��"},
	//
	{"strDismiss", "dismissָ���˳���"},
	{"strDismissPrivate", "dismissָ��˽���Ҷ������Ļ�ִ"},
	{"strDNDBuild", "dndָ���ִ"},
	{"strDrawCard", "drawָ���ִ"},
	{"strDrawHidden", "draw����ʱ�Ĺ�����ִ"},
	//
	{"strEnRoll", "enָ���ִ"},
	//
	{"strExtremeSuccess", "���ּ춨���ѳɹ�"},
	{"strFailure", "���ּ춨��SCʧ��"},
	{"strFriendDenyNoTrust", "AllowStranger=0ʱ�������û��ľܾ���ִ"},
	{"strFriendDenyNotUser", "AllowStranger=1ʱ��ʹ�ü�¼�ľܾ���ִ"},
	{"strFumble", "���ּ춨��ʧ��"},
	{"strGlobalOff", "ȫ�־�Ĭʱ��˽��ָ�����ָ��Ļ�ִ"},
	//
	{"strGroupAuthorized", "!authorize+Ⱥ����Ȩ�ɹ�����Ŀ��Ⱥ�Ļ�ִ"},
	{"strGroupBan", "group ban����ȺԱ�Ļ�ִ"},
	{"strGroupCardSet", "group card�޸�Ⱥ��Ƭ�Ļ�ִ"},
	//
	{"strGroupUnBan", "group ban���ȺԱ���ԵĻ�ִ"},
	//
	{"strHardSuccess", "���ּ춨���ѳɹ�"},
	{"strHardSuccess", "���ּ춨���ѳɹ�"},
	{"strHelpDisabledErr", "group+����help��Ⱥ��help��ִ"},
	{"strHelpNotFound", "helpδ�ҵ����ƴ���"},
	{"strHelpRedirect", "help�ҵ�Ψһ���ƴ���"},
	{"strHelpSuggestion", "help�ҵ��������ƴ���"},
	{"strHlpMsg", "help�޲���ָ���ִ"},
	//
	{"strRollCriticalSuccess", "�춨��ɹ�"}, 
	{"strRollExtremeSuccess", "�춨���ѳɹ�"},
	{"strRollHardSuccess", "�춨���ѳɹ�"},
	{"strRollRegularSuccess", "�춨�ɹ�"},
	{"strRollFailure", "�춨ʧ��"},
	{"strRollFumble", "�춨��ʧ��"},
	//
	{"strSanityRoll", ".scָ���ִ"},
	{"strSelfCall", "�Գƣ������ڻ�ִ"},
	{"strSelfName", "���ƣ���������չʾ����"},
	{"strSelfNick", "�û����Լ����ǳƣ�������չָ��"},
	//
	{"strSuccess", "���ּ춨��SC�ɹ�"},
	//
	{"strSummonWord", "�Զ����ٻ��ʣ���Ч��atָ��"},
	//
	{"strWhiteQQDenied","Ȩ��Ҫ��Ⱥ�����������1"},
};
const dict_ci<> HelpDoc = {
{"����",R"(
662:�ȹ���������
661:�Ż�gameָ�������
660:�Ż�sampleǶ��
659:reply֧����ȴ/�޶�ظ�
658:��д.dndչʾ
657:��ɫ��ģ��֧�ֱ���
656:�����ı�ת��
655:st�Ż������Ӵ���ʱ��
654:������ɫ��ģ��
653:��Ϸ��������
652:game new/over/open/close/state
651:����������
650:����ר��ָ��
649:.game ����
648:��ɫ����������
647:֧���Զ������
646:֧�ּ���Set��д
645:�ض���.sc��ִ
644:����ű��ڽ�ɫ��Actor����
643:rc֧�ֿ��ɫ������
641:SelfData֧��yaml
640:֧�ֵ���JavaScript
639:֧��reply����python
637:ģ����ԴԶ�̷���
636:֧��toml��д
635:webui���Զ��廯
634:ת���Ż���֧�ֱ�����ֵ
633:format����wait��������Ƕ���Ż�
632:format����ran
631:WebUI����modҳ
629:�ָ�Զ�̸���
628:֧��mod����
627:������������
626:ǰ׺ƥ���¼��׺
624:֧��modԶ�̰�װ/��ϸ��Ϣ
622:֧���ֶ�ʱ��
618:֧��reply(Order��ʽ)����ָ��
617:����grade�ֵ�ת��
593:reply������������
589:ak���ư���ָ��
581:��ɫ����ͳ��)"},
{"Э��","0.��Э����Dice!Ĭ�Ϸ���Э�顣����㿴������仰����ζ��MasterӦ��Ĭ��Э�飬��ע�⡣\n1.�������ʹ�������������Ⱥ���Ķ���Э����Ϊͬ�Ⲣ��ŵ���ش�Э�飬������ʹ��.dismiss�Ƴ����\n2.��������ԡ��Ƴ������ˢ�������ȶ�����Ĳ�������Ϊ����Щ��Ϊ����������ﱻ�Ʋõķ��ա�����������Ӧ��ʹ��.bot on/off��\n3.����Ĭ��������Ϊ�ѵõ�Ⱥ������ͬ�Ⲣ�Ͽ�Э�飬������Զ�ͬ��Ⱥ���롣�����������ʹ����������������Ϊʱ����������δ����Ԥ����������е��������Ρ�\n4.��ֹ���������ڶĲ�������Υ��������Ϊ��\n5.�������������ǳƵ��޷�Ԥ�����п�����������������Ϊ��������ܻ�������ұ������ܾ��ṩ����\n6.���ڼ����Լ��ʽ�ԭ�������޷���֤������100%��ʱ���ȶ����У����ܲ���ʱͣ��ά�����������ᣬ������Ӧ����ἰʱͨ��������������֪ͨ�������½⡣��ʱͣ�������ﲻ�����κ���Ӧ���ʶ�����Ӱ��Ⱥ�ڻ����״̬����Ȼ��ֹ��������Ϊ��\n7.����Υ��Э�����Ϊ�����ｫ�������ֹ���û�������Ⱥ�ṩ���񣬲���������¼��������������ṩ����������������˿���������ṩ��Э�̣������ղö�Ȩ�ڷ����ṩ����\n8.��Э��������ʱ�п��ܸĶ�����ע�������Ϣ��ǩ�����ռ䡢�ٷ�Ⱥ�ȴ������ﶯ̬��\n9.�����ṩ������������ȫ��ѵģ���ӭͶʳ��\n10.���������ս���Ȩ������ṩ�����С�"},
{"����","Dice!��̳: https://kokona.tech\nDice!�ֲ�: https://v2docs.kokona.tech\n֧��Shiki: https://afdian.net/@dice_shiki"},
{"�趨",R"(Master��{print:master}
Ⱥ��ʹ�ã�{case:self.Private?else=�������ƣ���Ԥ����&0={case:self.CheckGroupLicense?2=����ƣ��������ʹ��&1=����ƣ�����Ⱥ������&else=�������ƣ�����ʹ��}}
�������룺{case:self.AllowStanger?2=�����κ���&1=��Ҫʹ�ü�¼&else=��������}
�Ƴ����ƣ�{case:self.ListenGroupKicked?0=��&else=����{case:self.KickedBanInviter?1=����������}}
���Է��ƣ�{case:self.ListenGroupBanned?0=��&else=����{case:self.BannedBanInviter?1=����������}}
ˢ�����ƣ�{case:self.ListenSpam?0=�ر�&else=����}
�������ܣ�{��������}
���������{�������}{������}
�����û�Ⱥ:{�����û�Ⱥ}
���ｻ������Ⱥ��928626681)"},
{"�����û�Ⱥ","��δ���á�"},
{"��������","��"},
{"�������","��δ֪��"},
{"������","{list_dice_sister}"},
{"����","Copyright (C) 2018-2021 w4123���\nCopyright (C) 2019-2024 String.Empty\nGithub@Dice-Developer-Team"},
{"ָ��",R"(ָ��ǰ��at����ָ��������Ӧ����
{at:self}.bot on
��.help��Ӧָ�� ��ȡ��ϸ��Ϣ����.help r
����ָ��:
.dismiss ��Ⱥ
.bot �汾��Ϣ
.bot on/off ����/ͣ��ָ��
.reply on/off ����/���ûظ�
.group Ⱥ��
.authorize ��Ȩ���
.send ���̨������Ϣ
.mod ģ�����)"
R"(����ָ��
.game ��Ϸ����
.rules �����ٲ�
.r ����
.log ��־��¼
.ob �Թ�ģʽ
.set ����Ĭ����
.coc COC��������
.dnd DND��������
.st ���Լ�¼
.pc ��ɫ����¼
.rc �춨
.setcoc ����rc����
.sc ���Ǽ춨
.en �ɳ�/��ǿ�춨
.ri �ȹ�
.init �ȹ��б�
.ww ����)"
R"(����ָ��
.nn ���óƺ�
.draw ����
.deck �ƶ�ʵ��
.name �������
.ak ����/����
.jrrp ������Ʒ
.welcome ��Ⱥ��ӭ
Ϊ�˱���δԤ�ϵ���ָ�����У��뾡�����ڲ���֮��ʹ�ÿո�
������Ի�ָ��μ�.help ��չָ��)"},
{"master",R"(��ǰMaster:{print:master}
Masterӵ�����Ȩ�ޣ��ҿ��Ե�����������)"},
{"mod",R"(ģ��ָ��.mod
��ָ��������4ʹ��
`.mod list` �鿴�Ѽ���mod�б�
`.mod on ģ����` ����ָ��ģ��
`.mod off ģ����` ͣ��ָ��ģ��
`.mod del ģ����` ж��ָ��ģ��
`.mod info ģ����` ָ��ģ������Ϣ
`.mod detail ģ����` ָ��ģ����ϸ��Ϣ
mod�����ȡ���ҴӺ���ǰ����)"},
{"ak",R"(����+����ָ��.ak
.ak#[����]��.ak new [����] �½����粢���ñ��⣨��Ϊ�գ�
.ak+[ѡ��]��.ak add [ѡ��] Ϊ���ַ��������ѡ���|�ָ���һ����Ӷ��ѡ��
.ak-[ѡ�����]��.ak del [ѡ�����] �Ƴ�ָ����ŵ�ѡ��
.ak= ��.ak get ���������ȡһ��ѡ��������ַ���
.ak show �鿴����ѡ��
.ak clr ������ַ���)"},
{"log",R"(������־��¼.log
`.log new ��־��` �¿���־����ʼ��¼
`.log on` ������¼
`.log off` ��ͣ��¼
`.log end` ��ɼ�¼��������־�ļ�
�����еǼ�gm��plʱ����.game����¼gm��pl�ķ��ԣ������¼�����˷���
��־������Ϊ�ļ����Ϸ���ʡ����ʹ�ô���ʱ������ϴ���ʧ�ܷ��գ���ʱ��.send {self}��̨��ȡ)"},
{"deck",R"(�ƶ�ʵ��.deck
`.deck set (�ƶ���=)�����ƶ���` //�ӹ����ƶѴ���ʵ��
`.deck set (�ƶ���=)member` //��Ⱥ��Ա�б���ʵ��
`.deck set (�ƶ���=)range ���� ����` //�����Ȳ�������Ϊʵ��
`.deck show` //�г������ƶ�ʵ��
`.deck show �ƶ���` //�鿴�ƶ�ʣ�࿨��
`.deck reset �ƶ���` //����ʣ�࿨��
`.deck clr` //�������ʵ��
`.deck new �ƶ���=[����1](...|[����n])` //�Զ����ƶ�
��:
.deck new ����˹����=�е�|::5::�޵� //::����::����
*ʵ�����Ʋ���Ż�ֱ����գ����Բ�������ϴ��*
*ÿ��Ⱥ���ౣ��10��ʵ��*
*.drawʱ���ƶ�ʵ�����ȼ�����ͬ����������*
*��show������Ⱥ�ڲ�����ҪGMȨ��*)"},
{"��Ⱥ","&dismiss"},
{"��Ⱥָ��","&dismiss"},
{"dismiss","��ָ����ҪȺ����ԱȨ�ޣ�ʹ�ú��˳�Ⱥ��\n!dismiss [Ŀ��QQ(������ĩ��λ)]ָ����Ⱥ\n!dismiss�������ú������;�Ĭ״̬��ֻҪ�������������Ч"},
{"��Ȩ���","&authoize"},
{"authorize","��Ȩ���(�������û�ʹ��ʱתΪ������������)\n!authorize (+[Ⱥ��]) ([��������])\nȺ��ԭ�ط��Ϳ�ʡ��Ⱥ�ţ��޷��Զ���Ȩʱ����ͬ���ɷ�������\nĬ�ϸ�ʽΪ:!authorize ������;:[ **��д������** ] �����˽�Dice!�����÷�����ϸ�Ķ�����֤����{strSelfName}���û�Э�飬����ͣ��ָ��ʹ��[ **��д��ָ��** ]���ú�ʹ��[ **��д��ָ��** ]�ͳ�Ⱥ"},
{"����","&bot"},
{"bot",".bot on/off����/��Ĭ���ӣ���Ⱥ����\n.bot���Ӿ�Ĭ״̬��ֻҪ��������Ҳ��ں�����������Ч"},
{"�����ٲ�","&rules"},
{"����","&rules"},
{"rules","�����ٲ飺.rules ([����]):[�������] ��.rules set [����]\n.rules ��Ծ //������������ͬ����ʱ����һ����\n.rules COC����ʧ�� //cocĬ����Ѱcoc7�Ĵ���,dndĬ����Ѱ3r\n.rules dnd������\n.rules set dnd //���ú����Ȳ�ѯdndͬ���������޲������������"},
{"����","&r"},
{"rd","&r"},
{"r","������.r [�������ʽ] ([����ԭ��]) [�������ʽ]��([��������]#)[���Ӹ���]d��������(p[�ͷ�������])(k[ȡ��������������])��������ʱ��Ϊ��һ��Ĭ����\n�Ϸ�����Ҫ����������1-10������������1-9��������Χ1-100��������Χ1-1000\n.r3#d\t//3������\n.rh����ѧ ����\n.rs1D10+1D6+3 ɳӥ�˺�\t//rsʡ�Ե������ӵĵ�����ֱ�Ӹ����"},
{"����","Ⱥ���޶�������ָ����h��Ϊ�����������˽�����˺�Ⱥ��ob���û�\nΪ�˱�֤���ͳɹ�������������"},
{"reply",R"(�Զ���ظ���.reply
.reply on/off ��/��Ⱥ�ڻظ�
����˳��ָ��->�ظ� ��ȫMatch->ǰ׺Prefix->ģ��Search->����Regex
//���²����ظ�ָ���admin����
.reply set
Type=[�ظ�����](Reply/Order)
[����ģʽ](Match/Prefix/Search/Regex)=[������]
(Limit=[��������])
[�ظ�ģʽ](Deck/Text/Lua)=[�ظ���]
*Typeһ�п�ʡ�ԣ�Ĭ��ΪReply
.reply show [������] �鿴ָ���ظ�
.reply list �鿴ȫ���ظ�
.reply del [������] ���ָ���ظ�
)"},
{"�ظ���������",R"(
ÿ��ؼ��ʻظ��������������������ֻҪһ�����Ͳ�����
�û�����`user_id:�˺��б�` ָ���˺Ŵ������˺���|�ָ�
- ����������ð�ź��!��ʾָ���˺Ų�����
Ⱥ������`grp_id:Ⱥ���б�` ָ��Ⱥ�Ĵ�����grp_id:0��ʾ˽�Ŀ��Դ���
����`prob:�ٷֱ�` �����ʴ���
��ȴ��ʱ`cd:����` ����ȴ״̬�����������������ȴ
���ռ���`today:����` δ�����޴��������������+1
��ֵ`user_var``grp_var``self_var`��������ָ����������
����ʶ��`dicemaid:only` ��Dice!���ﴥ��
`dicemaid:off` Dice!���ﲻ����
)" },
{"�ظ��б�","{strSelfName}�Ļظ��������б�:{list_reply_deck}"},
{"game",R"(��Ϸģʽ��
`.game new ����` ������Ϸ��������ʡ�ԣ�ǰ׺��`����:`����ָ��Ӧ�ù���
��: `.game new DND:`
�������Զ���ΪGM
`.game over` ���ٱ�����Ϸ
`.game state` �鿴����״̬
`.game master` �Ǽ�ΪGM
`.game set ����=ֵ` ��Ϸ����
��: `.game set rule=COC7`
`.game set ����` �鿴����
`.game call` at���
`.game join` ��ҵǼ�
`.game kick ���ID` ������߳���Ϸ
`.game exit` �˳���Ϸ
`.game open ����` ����ǰ���ڼ���ָ����Ϸ
`.game close` �رյ�ǰ��Ϸ
`.game rou 100` ����(����)�����������.help roulette)"},
{"roulette",R"(������roulette
��������������Ϸ�ڽ��ж�Ӧ����������ʱ��ÿ�ν���������������ֱ�����е�����������������
`.game rou 100` ����(����)������
`.game rou 20*5` ����20����������ÿ�������ظ�5�Σ�����������100��
`.game rou hist` �鿴���ּ�¼
`.game rou clr` ���������
`.game rou reset` ��ԭ������
)" },
{"�Թ�","&ob"},
{"�Թ�ģʽ","&ob"},
{"ob",R"(�Թ�ģʽ��.ob (join/exit/list/clr/on/off)
.ob join //�����Թۣ����Կ��������������
.ob exit //�˳��Թ�ģʽ
.ob list //�鿴�����Թ���
.ob clr //��������Թ���
.ob on/off //������/���Թ�ģʽ
�������Թ�˽����Ч)"},
{"Ĭ����","&set"},
{"set","�����ʽ�С�D��֮��û�н�����ʱ����ΪͶ��Ĭ����\n.set20 ��Ĭ��������Ϊ20\n.set ����������Ϊ��Ĭ��������ΪĬ�ϵ�100\n�����ù����ж���������2D6���Ƽ�ʹ��.st &=2D6"},
{"��λ��","��λ����ʮ�棬Ϊ0~9ʮ�����֣��������Ľ��Ϊʮλ�����λ��֮�ͣ���00+0ʱ��Ϊ100��"},
{"ʮλ��","ʮλ����ʮ�棬Ϊ00~90ʮ�����֣��������Ľ��Ϊʮλ�����λ��֮�ͣ���00+0ʱ��Ϊ100��"},
{"������","&����/�ͷ���"},
{"�ͷ���","&����/�ͷ���"},
{"������","&����/�ͷ���"},
{"����/�ͷ���","COC�н���/�ͷ����Ƕ���Ͷ����ʮλ�������ս��ѡ�õ�������/���ߵĽ���������ִ�ʧ�ܵ�����µȼ��ڸ�С/�����ʮλ����\n.rb2 2��������\nrcp ��� 1���ͷ���"},
{"�������","&name"},
{"name","���������.name (cn/jp/en)([��������])\n.name 10\t//Ĭ��4�������������\n.name en\t//���cn/jp/en/enzh���޶���������/����/Ӣ��/Ӣ��������\n�������ɸ�����Χ1-10"},
{"�����ǳ�","&nn"},
{"�ǳ�","&nn"},
{"nn","����Ⱥ�ڳƺ���.nn [�ǳ�] / .nn / .nnn(cn/jp/en) \n.nn kp\t//�ǳ�ǰ��./���ȷ��Żᱻ�Զ�����\n.nn del\t//ɾ����ǰ���ڳƺ�\n.nn clr\t//ɾ�����м�¼�ĳƺ�\n.nnn\t//����������ƶ���������ƺ�\n.nnn jp\t/��ָ�����ƶ�����ǳ�\n˽��.nn��Ϊ����ȫ�ֳƺ�\n�óƺ�����\\{nick\\}����ʾ�����ȼ���Ⱥ�ڳƺ�>ȫ�ֳƺ�>Ⱥ��Ƭ>QQ�ǳ�\n�޽�ɫ����δ����ʱҲ������ʾ\\{pc\\}"},
{"��������","�ð汾��������֧��COC7(.coc��.draw����Ա����/Ӣ���츳)��COC6(.coc6��.drawú����)��DND(.dnd)��AMGC(.draw AMGC)"},
{"coc","����³�ĺ���(COC)�������ɣ�.coc([7/6])(d)([��������])\n.coc 10\t//Ĭ������7������\n.coc6d\t//��dΪ��ϸ���ɣ�һ��ֻ������һ��\n���������㷨�������ɣ���Ӧ�ñ�����򣬲ο�.rules��������Ա������ѡ��"},
{"dnd","������³�(DND)�������ɣ�.dnd (��������)\n��`.dnd 5` //����6ά�������У������ο���������Ӧ�ñ������"},
{"���Լ�¼","&st"},
{"st",R"(���Լ�¼��.st (del/clr/show) ([������]:[����ֵ])
plĬ������Ⱥʹ��ͬһ�ſ�������࿪��ʹ��.pcָ���п�
.st ����:50 ����:55 ����:65 ����:45 ��ò:70 ����:75 ��־:35 ����:65 ����:75
.st hp-1 ���+/-ʱ��Ϊ��ԭֵ�ϱ仯
.st san+1d6 �޸�����ʱ��ʹ���������ʽ
.st del kp�þ� //ɾ���ѱ��������
.st clr //��յ�ǰ��
.st show ��� //�鿴ָ�����ԣ��޲���ʱ�鿴��������
�������Իᱻ��Ϊͬ��ʣ�������/san�����/���
��.st������δ�����仯Ҳ�᷵�ر�����)"},
{"��ɫ��","&pc"},
{"pc",R"(��ɫ����.pc 
.pc new (ģ��:(���ɲ���:))(����) 
��: `.pc new ����` //��ȫʡ�Բ���������һ��COC7ģ������������
.pc tag (����) //Ϊ��ǰȺ��ָ������Ϊ������ʹ��Ĭ�Ͽ�
����ȺĬ��ʹ��˽�İ󶨿���δ����ʹ��0�ſ�
.pc show (����) //չʾָ�������м�¼�����ԣ�Ϊ����չʾ��ǰ��
.pc nn �¿��� //��������ǰ��������������
.pc type ģ�� //���л���ǰ��ģ��
.pc cpy ����1=����2 //���������Ը��Ƹ�ǰ��
.pc del ���� //ɾ��ָ����
.pc list //�г�ȫ����ɫ��
.pc grp //�г���Ⱥ�󶨿�
.pc build (���ɲ���:)(����) //����ģ������������ԣ�COC7Ϊ9�������ԣ�
.pc stat //�鿴��ǰ��ɫ������ͳ��
.pc redo (���ɲ���:)(����) //���ԭ�����Ժ���������
.pc clr //����ȫ����ɫ����¼
//����ͳ���Խ�ɫ��Ϊ��λ��ÿ���û�����ͬʱ����32�Ž�ɫ��
)"
	},
	{"rc", "&rc/ra"},
	{"ra", "&rc/ra"},
	{"�춨", "&rc/ra"},
	{
		"rc/ra",
		"�춨ָ�.rc/ra (_)([�춨����]#)([�Ѷ�])[������]( [�ɹ���])\n��ɫ������������ʱ����ʡ�Գɹ���\n.rc����*5\t//����ʹ��+-*/����˳��Ҫ��Ϊ�˷�>�Ӽ�>����\n.rc ��������\t//��������ͷ�����Ѻͼ��ѻᱻ��Ϊ�ؼ���\n.rc _����ѧ50\t//������������˼��Թ��߿ɼ�\n.rc ����-10\t//������ɹ��ʱ�����1-1000��\n.rcp ��ǹ\t//����������9��\nĬ���Թ������ж�����ɹ���ʧ�ܵķ�����.setcoc����"
	},
	{
		"����",
		"Dice!ĿǰֻΪCOC7�춨�ṩ���棬ָ��Ϊ.setcoc:{setcoc}"
	},
	{
		"setcoc",
		R"(Ϊ��ǰȺ������������COC���棬��.setcoc 1,��ǰ����0-6
`.setcoc show`�鿴��ǰ����
`.setcoc clr`�����ǰ����
0 ������
��1��ɹ�
����50��96 - 100��ʧ�ܣ���50��100��ʧ��
1
����50��1��ɹ�����50��1 - 5��ɹ�
����50��96 - 100��ʧ�ܣ���50��100��ʧ��
2
��1 - 5�� <= �ɹ��ʴ�ɹ�
��100���96 - 99�� > �ɹ��ʴ�ʧ��
3
��1 - 5��ɹ�
��96 - 100��ʧ��
4
��1 - 5�� <= ʮ��֮һ��ɹ�
����50�� >= 96 + ʮ��֮һ��ʧ�ܣ���50��100��ʧ��
5
��1 - 2�� < ���֮һ��ɹ�
����50��96 - 100��ʧ�ܣ���50��99 - 100��ʧ��
6 ��ɫ������
��1�����λʮλ��ͬ��<=�ɹ��ʴ�ɹ�
��100�����λʮλ��ͬ��>�ɹ��ʴ�ʧ��

��������������򿪷��߷���
������Σ�Ⱥ�ڼ춨ֻ�����Ⱥ�����ã��������ڳ�Ա���Ե�
δ���÷��潫ʹ��������DefaultCOCRoomRule����ʼΪ0��)"
	},
	{"san check", "&sc"},
	{"���Ǽ춨", "&sc"},
	{
		"sc",
		"San Checkָ�.sc[�ɹ���ʧ]/[ʧ����ʧ] ([��ǰsanֵ])\n�Ѿ�.st������/sanʱ����ʡ�����Ĳ���\n.sc0/1 70\n.sc1d10/1d100 ֱ������\n��ʧ���Զ�ʧȥ���ֵ\n�����ý�ɫ��sanʱ��san���Զ�����Ϊsc���ʣ��ֵ\n�����Ͽ�����ʧ������san��Ҳ���ǿ�����.sc-1d6/-1d6���ظ�san���������������ֲ���"
	},
	{"ti", "&ti/li"},
	{"li", "&ti/li"},
	{"���֢״", "&ti/li"},
	{"ti/li", "���֢״��\n.ti ��ʱ���֢״\n.li �ܽ���֢״\n����coc7�����6���������ð�������ϲ��\n������������μ�.rules �����"},
	{"�ɳ��춨", "&en"},
	{"��ǿ�춨", "&en"},
	{
		"en",
		"�ɳ��춨��.en [��������]([����ֵ])(([ʧ�ܳɳ�ֵ]/)[�ɹ��ɳ�ֵ])\n�Ѿ�.stʱ����ʡ�����Ĳ���\n.en ���� 60 +1D10 ������ǿ\t//��ӳɹ�ʱ�ɳ�ֵ\n.en ���� +1D3/1D10���˳ɳ�\t//�������￨����ʱ���ɳ����ֵ���Զ�����\n�ɱ�ɳ�ֵ�����ԼӼ��ſ�ͷ�������ƼӼ�"
	},
	{"����", "&draw"},
	{
		"draw",
		R"(���ƣ�.draw [�ƶ�����] ([��������])	
.draw _[�ƶ�����] ([��������])	//���飬���˽�ķ���
.drawh [�ƶ�����] ([��������])	//���飬����h��������ո�
.draw _����ɱ	//���ƽ��ͨ��˽�Ļ�ȡ����ob
*�ƶ��������ȵ����ƶ�ʵ������δ�������ͬ�������ƶ�������ʱʵ��
*�鵽���Ʋ��Żأ��ƶѳ�պ��޷�����
*�鿴{self}�Ѱ�װ�ƶѣ���.help ȫ�ƶ��б��.help ��չ�ƶ�)"
	},
	{ "��չ�ƶ�","{list_extern_deck}" },
	{ "ȫ�ƶ��б�","{list_all_deck}" },
	{ "��չָ��","{list_extern_order}" },
	{"�ȹ�", "&ri"},
	{"ri", "�ȹ���Ⱥ���޶�����.ri(��ֵ)(�ǳ�)\n.ri-1 ĳpc //�Զ������ȹ��б�\n.ri 17 boss //¼�볣����Ϊ�ȹ�ֵ\n.ri+5 3#ľ���� //���ɸ����ȹ�ֵ"},
	{"�ȹ��б�", "&init"},
	{"init", "�ȹ��б�\n.init list\t//�鿴�ȹ��б�\n.init clr\t//����ȹ��б�\n.init del [��Ŀ��]\t//���ȹ��б��Ƴ���Ŀ"},
	{"����", "&ww"},
	{"ww", R"(���أ�.w(w) [���Ӹ���]a[��������]
.w��ֱ�Ӹ��������.ww�����ÿ�����ӵĵ���
�̶�10������ÿ��һ�����ӵ����ﵽ���������������һ�Σ�����������ﵽ8��������
����5�ܼ���������10
�����÷���ο������Ϸ����)"},
	{"�����˳�", "&me"},
	{"�����˳ƶ���", "&me"},
	{
		"me",
		"�����˳ƶ�����.me([Ⱥ��])[����].me Ц������\t//����Ⱥ��ʹ��\n.me 941980833 ��Ⱥ��\t//����˽��ʹ�ã��������αװ��������Ⱥ��˵��\n.me off\t//Ⱥ�ڽ���.me\n.me on\t//Ⱥ������.me"
	},
	{"������Ʒ", "&jrrp"},
	{
		"jrrp",
		"������Ʒ��.jrrp\n.jrrp off\t//Ⱥ�ڽ���jrrp\n.jrrp on\t//Ⱥ������jrrp\nһ��һ����Ʒֵ\n2.3.5�汾�����ֵΪ���ȷֲ�\n����ֻ�������䧵ķ��������˽�����������ܿ�ŭ\n��η��������˵����ˣ�ֻ��������ǧ��������֮ĸ�����ó���֮��������˫��׶��Եļ໤�ˣ�һ�й��صĿ����ߣ�ʱ����̨��������ߣ���ת������֮�֡�����䧱�������"
	},
	{"send", "������Ϣ��.send ���Master˵�Ļ�\n���������ϷMaster����������׼��"},
	{"��Ⱥ��ӭ", "&welcome"},
	{"��Ⱥ��ӭ��", "&welcome"},
	{
		"welcome",
		R"(��Ⱥ��ӭ�ʣ�.welcome
.welcome \{at}��ӭ\{nick}��Ⱥ~	//���û�ӭ��
//\{at}��Ϊat��Ⱥ�ߣ�\{nick}���滻Ϊ���˵��ǳƣ�����ת����ֲ�
.welcome clr //��ջ�ӭ��
.welcome show //�鿴��ӭ��
����ָ���Ƿ�ͣ�ã�ֻҪ�л�ӭ��ʱ������Ⱥ��������Ӧ)"
	},
	{"group", "&Ⱥ��"},
	{
		"Ⱥ��",
		R"(Ⱥ��ָ��.group(Ⱥ����Ա�޶�)
.group state //�鿴��Ⱥ�ڶ����������
.group pause/restart //Ⱥȫ�����/ȫ��������
.group card [at/�û�ID] [��Ƭ] //����ȺԱ��Ƭ
.group title [at/�û�ID] [ͷ��] //����ȺԱͷ��
.group diver //�鿴Ǳˮ��Ա
.group +/-[Ⱥ�ܴ���] //ΪȺ�Ӽ����ã���Ҫ��ӦȨ��
��:.group +���ûظ� //�رձ�Ⱥ�Զ���ظ�
Ⱥ�ܴ���:ͣ��ָ��/���ûظ�/����jrrp/����draw/����me/����help/���ʹ��/����/���)"
	},
	{ "groups_list", "&ȡȺ�б�" },
	{ "ȡȺ�б�", R"(ȡȺ�б�.groups list(�����޶�)
.groups list idle //���������������г�Ⱥ
.groups list size //��Ⱥ��ģ�����г�Ⱥ
.groups list [Ⱥ�ܴ���] //�г����д�����Ⱥ
Ⱥ�ܴ���:ͣ��ָ��/���ûظ�/����jrrp/����draw/����me/����help/���ʹ��/����/���)" },
	{"��Ϣ����","&link"},
	{"link",R"(��Ϣ����.link
.link [ת������] [���󴰿�] ��������������󴰿ڵ�ת��
.link start �����ϴιرյ�����
.link close �ر�����
[ת������]:to=ת����������Ϣ�����󴰿�;from=ת�����󴰿���Ϣ��������;with=˫��ת��
[���󴰿�]:Ⱥ/������=g[Ⱥ��];˽�Ĵ���=u[�˺�]
��:.link with u1605271653 //����˫��˽������
.link from g928626681 //����Ŀ��Ⱥ����Ϣת��
.link state �鿴��ǰ����״̬
.link list �г�ȫ�����ڵ�����
ÿ������Ϊ���ֻ������һ������)"},
	{ "���дʼ��","&censor" },
	{"censor",R"(���дʼ��.admin censor
.admin censor +([�����ȼ�])=[���д�0](|[���д�1]...) //������д�
.admin censor -[���д�0](|[���д�1]...) //�Ƴ����д�
��:.admin censor +=nmsl //����nmsl������ΪWarning��
.admin censor +Danger=nn�Ϲ�|nn���� //����nn�Ϲ�������nn���ˡ�����ΪDanger��
.admin censor -��ǹ //�Ƴ����дʡ���ǹ��
# ƥ�����
�����ģ��ƥ��ָ���ʶ��(.)��ͷ����Ϣ���������������дʵ���ߴ����ȼ�
ƥ������Զ������ı��е�������źͿո��Ҵ�Сд������
�������û�����Ӧ���ʹ����ȼ�������4�����û������������
# �����ȼ�
Ignore //����
Notice //����0������֪ͨ
Caution //�����û�������1����������
Warning //��Ĭ�ϡ������û�������1����������
Danger //�����û��Ҿܾ�ָ�����3�����ھ���
*�����Ϊ����ĸ/���ֵ����д����ýϸߴ����ȼ�����Щ�ַ�������ƥ��ͼƬ��Ŀ�����
# �ʿ��������ط�ʽ���ֲ�)" },
	{"���", "������ǧ��������֮ĸ�����ó���֮��������˫��׶��Եļ໤�ˣ�һ�й��صĿ����ߣ����������ף�ʱ����̨��������ߣ���ת������֮��"},
	{"Ͷι","&Ͷʳ"},
	{"Ͷʳ", "ͶʳShiki����ѡ��https://afdian.net/@dice_shiki\nͶʳ��䧣���ѡ��https://afdian.net/@suhuiw4123\nͶʳ{self}����ѡ�񡭡��䳬�᣿"},
	{"������λ", "����Ȼ�ĵط�������Ŀ�ĵ�ǰ�С�ϲ��������ս��������Ĵ����ˡ����õ����롣"},
	{
		"������λ",
		"ð�յ��ж���׷������ԣ��������룬�������ʵ���ʧ���뿪��԰�������������ˣ�Ϊ�������ж����ա�������顢���ʵ����顢�޷����ó�������Ǣ�С������İ�����ó̡��Ի����е��������˴˺�����ȡ��������˷���׹�밮�ӡ�Ϊ���˵ĸ������ˡ����鲻רһ��"
	},
	{"ħ��ʦ��λ", "����Ŀ�ʼ���ж��ĸı䣬�����ļ��������ɣ��᳹�ҵ���־��������Ȼ���������ﵽҰ�ġ�"},
	{"ħ��ʦ��λ", "��־����������ͷ�ѣ��������ķ���֪ʶ���㣬��ƭ��ʧ�ܡ�"},
	{"Ů��˾��λ", "���������ڵ�����Ǳ����ǰ;�������仯��Ԥ�ԣ���̵�˼��������Ķ�������׼ȷ��ֱ����"},
	{"Ů��˾��λ", "���ڽ�񱣬��֪��̰�ģ�Ŀ���ǳ�������Ĺ��ߣ�ƫ����жϣ�������ı������������"},
	{"Ů����λ", "�Ҹ����ɹ����ջ��������ǣ�Բ���ļ�ͥ������õĻ�������ò�������������Ȼ�Ӵ����������У����С�"},
	{"Ů����λ", "�����ã�ȱ���Ͻ��ģ�ɢ��������ϰ�ߣ��޷���������飬���ܿ����ɹ����������֣������ն�����˷������ס�"},
	{"�ʵ���λ", "���٣�Ȩ����ʤ���������쵼Ȩ����ǿ����־�����Ŀ�꣬���׵����Σ������ϵĹµ���"},
	{"�ʵ���λ", "���ɣ����������ã��������ԣ�ƽ����û�����ţ��ж������㣬��־��������֧�䡣"},
	{"�̻���λ", "Ԯ����ͬ�飬�������������ε��˸����Ȱ�棬���õ��������󣬵õ������ϵ����㣬���ع���־Ը�ߡ�"},
	{"�̻���λ", "�����ѶϢ������Ĺ�Ȱ���ϵ���Ԯ�����жϣ�Ը���޷���ɣ��������ã���������"},
	{"������λ", "��ϣ����飬���У���Ȥ������ϣ����δ�����������������ѡ�"},
	{"������λ", "�������ջ��������ȣ������޳�������䵭����룬�����������Ĵ�磬����Ѷϡ�"},
	{"ս����λ", "Ŭ������óɹ���ʤ�����˷��ϰ����ж��������������ԣ��������ţ��������ӣ���ͨ���ߣ������˴󼪡�"},
	{"ս����λ", "����ʧ�ܣ��������ף����ͣ�Υ���������������̵����ӣ�ͻȻ��ʧ�ܣ��������꣬���ۺ���˽������"},
	{"������λ", "�󵨵��ж����������ľ��ϣ��·�չ����ת�����춯������־��սʤ���ѣ���׳��Ů�ˡ�"},
	{"������λ", "��С�����ǿ�ߣ��������ջ�������Ȩ���볣ʶ֮�£�û��ʵ�������������٣�ų����û�����ԡ�"},
	{"������λ", "���ص���ʵ��������ж����������˵���������ܹ¶�������ľ��䣬�곤�ߣ��ܿ�Σ�գ��游��������"},
	{"������λ", "���޹¶����Ա������ģ�����˼�룬�������ص���ʧ�ܣ�ƫ��������С�"},
	{"����֮����λ", "�ؼ��Ե��¼������µĻ��ᣬ��ĳ����������ı仯�����˵Ŀ��ˣ�״����ת��������������֮���١�"},
	{"����֮����λ", "���ۣ��ƻ��������ϰ����޷�����������������չ������ѭ�����жϡ�"},
	{"������λ", "��������������ʵ������̹����������һ������ְ��׷�������Э���ߡ��뷨���йء���������Ľ��������������"},
	{"������λ", "ʧ�⡢ƫ�������š����ϡ�����ר�С������������޷���ȫ�����ﲻһ����Ů�Ը񲻺ϡ���в��ۡ����������µ����顣"},
	{"��������λ", "���ܿ��顢�ж����ޡ���������η�������������ա���ʧ���еá���ȡ�����ѵ��ԡ���������㷺ѧϰ�����׵İ���"},
	{"��������λ", "��ν�����������ۡ����ˡ�����Ŭ�����������ơ����ԡ����������ߡ�ȱ�����ġ��ܳͷ����ӱܰ��顢û�н�������顣"},
	{"������λ", "ʧ�ܡ��ӽ�����������ʧҵ��ά��ͣ��״̬���������𺦡�����ֹͣ�������������롢���¿�ʼ��˫���к���ĺ蹵��������ֹ��"},
	{"������λ", "����һ��ϣ������������������ת�⡢���ѵ���״̬��������������念����ͻȻ�ı�ƻ����ӱ���ʵ��ն����˿�����������ꡣ"},
	{"������λ", "������������ƽ˳�����ݻ������ø�תΪ���⡢���������"},
	{"������λ", "���ġ��½���ƣ�͡���ʧ������������Ǣ���������϶Ȳ��ѡ�"},
	{"��ħ��λ", "�����������䡢���������⡢�����������ķ�²�����ɿ��ܵ��ջ��Ƿϵ������ծ���ա����ɸ��˵����ܡ�˽�����顣"},
	{"��ħ��λ", "���������������š�������ʹ������ȥ����ͣ�����롢�ܾ��ջ�����˽��������ʱ�̡����޽��ӵ����顣"},
	{"����λ", "�Ʋ����澳���������������������Ĵ�����޴�ı䶯����ǣ�����������������Է١����Ų��ϡ�ͻȻ���룬����İ���"},
	{"����λ", "��������ڧ�����ȵ�״̬��״�����ѡ������ȶ��������Դ󽫸������ۡ���ˮһս�������Ԥ�С�����Σ����"},
	{"������λ", "ǰ;����������ϣ�����������������������롢����Ը����ˮ׼��ߡ�����Ķ������õ����顣"},
	{"������λ", "���ۡ�ʧ�����ø���Զ�������쿪���ֻ�ʧ�롢����ԸΥ��������˳�ġ�������ۡ��������顢ȱ�ٰ������"},
	{"������λ", "�������Ի󡢶�ҡ�����ԡ���ƭ���������ϡ������İ������ǹ�ϵ��"},
	{"������λ", "����ƭ�֡������ᡢ״����ת��Ԥ֪Σ�ա��ȴ������Ӱ�����ѷ졣"},
	{"̫����λ", "��Ծ���ḻ���������������������������桢����˳���������������Ҹ��Ļ����������Ľ��ʡ�"},
	{"̫����λ", "�������������ѡ�ȱ�������ԡ�������������������˼ʹ�ϵ���á����鲨������顣"},
	{"������λ", "�����ϲ�á�������̹�ס�����Ϣ������������¶��â�����յİ����طꡢ�����漣��"},
	{"������λ", "һ�겻�񡢻�������������Ϣ���޷�������ȱ��Ŀ�ꡢû�н�չ���������������ᡣ"},
	{"������λ", "��ɡ��ɹ���������ȱ���������ϡ����񿺷ܡ�ӵ�б����ܶ���Ŀ�ꡢ���ʹ�������˽��١����ֵĽ�����ģ�����¡�"},
	{"������λ", "δ��ɡ�ʧ�ܡ�׼�����㡢äĿ���ܡ�һʱ��˳������;���ϡ������Ƿϡ�����״̬����ı��̬�Ȳ�����Ǣ�������ܴ졣"},
};

const std::string getComment(const std::string& key) {
	if (auto it{ GlobalComment.find(key) };it!= GlobalComment.end())return it->second;
	return {};
}
