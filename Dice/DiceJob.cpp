#include "DiceJob.h"
#include "DiceConsole.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "S3PutObject.h"
#else
#include <curl/curl.h>
#endif
#include "StrExtern.hpp"
#include "DDAPI.h"
#include "CQTools.h"
#include "ManagerSystem.h"
#include "DiceCloud.h"
#include "BlackListManager.h"
#include "GlobalVar.h"
#include "CardDeck.h"
#include "DiceMod.h"
#include "DiceNetwork.h"
#include "DiceSession.h"
#include "DiceEvent.h"
#include "DiceZip.h"
#pragma warning(disable:28159)

using namespace std;

int sendSelf(const string& msg) {
	static long long selfQQ = DD::getLoginID();
	DD::sendPrivateMsg(selfQQ, msg);
	return 0;
}

void cq_exit(AttrObject& job) {
#ifdef _WIN32
	MsgNote(job, "����" + getMsg("self") + "��5�����ɱ", 1);
	std::this_thread::sleep_for(5s);
	dataBackUp();
	DD::killme();
#endif
}

#ifdef _WIN32
inline PROCESSENTRY32 getProcess(int pid) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	Process32First(hParentProcess, &pe32);
	return pe32;
}
#endif

void frame_restart(AttrObject& job) {
#ifdef _WIN32
	if (!job->get_ll("uid")) {
		if (console["AutoFrameRemake"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
		else if (int tWait{ console["AutoFrameRemake"] * 60 * 60 - int(time(nullptr) - llStartTime) }; tWait > 0) {
			sch.add_job_for(tWait, job);
			return;
		}
	}
	Enabled = false;
	dataBackUp();
	std::this_thread::sleep_for(3s);
	DD::remake();
#endif
}

void frame_reload(AttrObject& job) {
	if (DD::reload())
		MsgNote(job, "����" + getMsg("self") + "��ɡ�", 1);
	else
		MsgNote(job, "����" + getMsg("self") + "ʧ�ܡ�", 0b10);
}

void check_system(AttrObject& job) {
#ifdef _WIN32
	static int perRAM(0), perLastRAM(0);
	static double  perLastCPU(0), perLastDisk(0),
		perCPU(0), perDisk(0);
	static bool isAlarmRAM(false), isAlarmCPU(false), isAlarmDisk(false);
	static double mbFreeBytes = 0, mbTotalBytes = 0;
	//�ڴ���
	if (console["SystemAlarmRAM"] > 0) {
		perRAM = getRamPort();
		if (perRAM > console["SystemAlarmRAM"] && perRAM > perLastRAM) {
			console.log("���棺" + getMsg("strSelfName") + "����ϵͳ�ڴ�ռ�ô�" + to_string(perRAM) + "%", 0b1000, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = true;
		}
		else if (perLastRAM > console["SystemAlarmRAM"] && perRAM < console["SystemAlarmRAM"]) {
			console.log("���ѣ�" + getMsg("strSelfName") + "����ϵͳ�ڴ�ռ�ý���" + to_string(perRAM) + "%", 0b10, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = false;
		}
	}
	//CPU���
	if (console["SystemAlarmCPU"] > 0) {
		perCPU = getWinCpuUsage() / 10.0;
		if (perCPU > 99.9) {
			this_thread::sleep_for(30s);
			perCPU = getWinCpuUsage() / 10.0;
		}
		if (perCPU > console["SystemAlarmCPU"] && (!isAlarmCPU || perCPU > perLastCPU + 1)) {
			console.log("���棺" + getMsg("strSelfName") + "����ϵͳCPUռ�ô�" + toString(perCPU) + "%", 0b1000, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = true;
		}
		else if (perLastCPU > console["SystemAlarmCPU"] && perCPU < console["SystemAlarmCPU"]) {
			console.log("���ѣ�" + getMsg("strSelfName") + "����ϵͳCPUռ�ý���" + toString(perCPU) + "%", 0b10, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = false;
		}
	}
	//Ӳ�̼��
	if (console["SystemAlarmRAM"] > 0) {
		perDisk = getDiskUsage(mbFreeBytes, mbTotalBytes) / 10.0;
		if (perDisk > console["SystemAlarmDisk"] && (!isAlarmDisk || perDisk > perLastDisk + 1)) {
			console.log("���棺" + getMsg("strSelfName") + "����ϵͳӲ��ռ�ô�" + toString(perDisk) + "%", 0b1000, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = true;
		}
		else if (perLastDisk > console["SystemAlarmDisk"] && perDisk < console["SystemAlarmDisk"]) {
			console.log("���ѣ�" + getMsg("strSelfName") + "����ϵͳӲ��ռ�ý���" + toString(perDisk) + "%", 0b10, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = false;
		}
	}
	if (isAlarmRAM || isAlarmCPU || isAlarmDisk) {
		sch.add_job_for(5 * 60, job);
	}
	else {
		sch.add_job_for(30 * 60, job);
	}
#endif
}

void auto_save(AttrObject& job) {
	if (sch.is_job_cold("autosave"))return;
	DD::debugLog(printSTNow() + " �Զ�����");
	dataBackUp();
	//console.log(getMsg("strSelfName") + "���Զ�����", 0, printSTNow());
	if (console["AutoSaveInterval"] > 0) {
		sch.refresh_cold("autosave", time(NULL) + console["AutoSaveInterval"] * (time_t)60);
		sch.add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	}
}

//�����õ�ͼƬ�б�
void clear_image(AttrObject& job) {
	return;
	if (!job->has("uid")) {
		if (sch.is_job_cold("clrimage"))return;
		if (console["AutoClearImage"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
	}
	//MsgNote(job, "������image�ļ�" + to_string(cnt) + "��", 1);
	if (console["AutoClearImage"] > 0) {
		sch.refresh_cold("clrimage", time(NULL) + console["AutoClearImage"]);
		sch.add_job_for(console["AutoClearImage"] * 60 * 60, "clrimage");
	}
}

void clear_group(AttrObject& job) {
	console.log("��ʼ���Ⱥ��", 0, printSTNow());
	int intCnt = 0;
	ResList res;
	vector<long long> GrpDelete;
	time_t grpline{ console["InactiveGroupLine"] > 0 ? (tNow - console["InactiveGroupLine"] * (time_t)86400) : 0 };
	if (string mode{ job->get_str("clear_mode") };mode == "unpower") {
		for (auto& [id, grp] : ChatList) {
			if (grp->is("����") || !grp->getLst() || grp->is("����") || grp->is("Э����Ч"))continue;
			if (!DD::isGroupAdmin(id, console.DiceMaid, true)) {
				res << printGroup(id);
				time_t tLast{ grp->updated() };
				if (auto s{ sessions.get_if({ 0,id }) })
					tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
				if (tLast < grpline)GrpDelete.push_back(id);
				grp->leave(getMsg("strLeaveNoPower"));
				intCnt++;
				if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"])break;
				this_thread::sleep_for(3s);
			}
		}
		MsgNote(job, "ɸ��{strSelfName}��Ⱥ��Ⱥ��" + to_string(intCnt) + "��:" + res.show(), 0b10);
	}
	else if (!mode.empty() && isdigit(static_cast<unsigned char>(mode[0]))) {
		int intDayLim{ job->get_int("clear_mode") };
		time_t tNow{ time(nullptr) };
		for (auto& [id, grp] : ChatList) {
			if (grp->is("����") || grp->is("����") || grp->is("Э����Ч"))continue;
			time_t tLast{ grp->getLst() };
			if (auto s{ sessions.get_if({ 0,id }) };s && s->tUpdate > tLast)
				tLast = s->tUpdate;
			if (tNow - 86400LL * intDayLim < tLast)continue;
			if (long long tLMT{ DD::getGroupLastMsg(id, console.DiceMaid) };
				tLMT > tLast)grp->setLst(tLast = tLMT);
			if (tLast <= 0)continue;
			if (tLast < grpline && !grp->is("���"))GrpDelete.push_back(id);
			int intDay{ int((tNow - tLast) / 86400) };
			if (intDay > intDayLim) {
				job->at("day") = to_string(intDay);
				res << printGroup(id) + ":" + to_string(intDay) + "��\n";
				grp->leave(getMsg("strLeaveUnused", job));
				intCnt++;
				if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"])break;
				this_thread::sleep_for(3s);
			}
		}
		MsgNote(job, "��ɸ��{strSelfName}Ǳˮ{clear_mode}��Ⱥ��" + to_string(intCnt) + "����" + res.show(), 0b10);
	}
	else if (mode == "black") {
		try {
			set<long long> grps{ DD::getGroupIDList() };
			for (auto id : grps) {
				Chat& grp = chat(id).name(DD::getGroupName(id));
				if (grp.is("����") || grp.is("����") || grp.is("���") || grp.is("Э����Ч"))continue;
				if (blacklist->get_group_danger(id)) {
					time_t tLast{ grp.updated() };
					if (auto s{ sessions.get_if({ 0,id }) })
						tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
					if (tLast < grpline)GrpDelete.push_back(id);
					res << printGroup(id) + "��������Ⱥ";
					grp.leave(getMsg("strBlackGroup"));
				}
				set<long long> MemberList{ DD::getGroupMemberList(id) };
				AttrVar authSelf;
				for (auto eachQQ : MemberList) {
					if (blacklist->get_user_danger(eachQQ) > 1) {
						if (authSelf.is_null())authSelf = DD::getGroupAuth(id, console.DiceMaid, 1);
						if (auto authBlack{ DD::getGroupAuth(id, eachQQ, 1) }; authSelf.more(authBlack)) {
							continue;
						}
						else if (authSelf.less(authBlack)) {
							if (grp.updated() < grpline)GrpDelete.push_back(id);
							res << grp.print() + "��" + printUser(eachQQ) + "�Է�ȺȨ�޽ϸ�";
							grp.leave("���ֺ���������Ա" + printUser(eachQQ) + "\n" + getMsg("strSelfName") + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
						else if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"]) {
							if(intCnt == console["GroupClearLimit"])res << "*���������Ѵ�����*";
							res << grp.print() + "��" + printUser(eachQQ);
						}
						else if (console["LeaveBlackQQ"]) {
							if (grp.updated() < grpline)GrpDelete.push_back(id);
							res << grp.print() + "��" + printUser(eachQQ);
							grp.leave("���ֺ�������Ա" + printUser(eachQQ) + "\n" + getMsg("strSelfName") + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
					}
				}
			}
		} 		catch (...) {
			console.log("���ѣ�" + getMsg("strSelfName") + "��������Ⱥ��ʱ����", 0b10, printSTNow());
		}
		if (intCnt) {
			MsgNote(job, "�Ѱ�{strSelfName}���������Ⱥ��" + to_string(intCnt) + "����" + res.show(), 0b10);
		}
		else if (job->has("uid")) {
			reply(job, getMsg("strSelfName") + "��������δ���ִ����Ⱥ��");
		}
	}
	else if (mode == "preserve") {
		for (auto& [id, grp] : ChatList) {
			if (grp->is("����") || !grp->getLst() || grp->is("���ʹ��") || grp->is("����") || grp->is("Э����Ч"))continue;
			if (DD::isGroupAdmin(id, console, false)) {
				grp->set("���ʹ��");
				continue;
			}
			time_t tLast{ grp->updated() };
			if (auto s{ sessions.get_if({ 0,id }) })
				tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
			if (tLast < grpline)GrpDelete.push_back(id);
			res << printChat(*grp);
			grp->leave(getMsg("strPreserve"));
			intCnt++;
			if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"])break;
			this_thread::sleep_for(3s);
		}
		MsgNote(job, "ɸ��{strSelfName}�����Ⱥ��" + to_string(intCnt) + "����" + res.show(), 1);
	}
	else
		reply(job, "{�޷�ʶ��ɸѡ������}");
	if (!GrpDelete.empty()) {
		for (const auto& id : GrpDelete) {
			ChatList.erase(id);
			if (sessions.has_session({ 0,id }))sessions.over({ 0,id });
		}
		MsgNote(job, "���Ⱥ��ʱ���ղ���Ծ��¼" + to_string(GrpDelete.size()) + "��", 0b1);
	}
}
void list_group(AttrObject& job) {
	console.log("����Ⱥ�б�", 0, printSTNow());
	string mode{ job->get_str("list_mode") };
	if (mode.empty()) {
		reply(job, fmt->get_help("groups_list"));
	}
	if (mChatConf.count(mode)) {
		ResList res;
		for (auto& [id, grp] : ChatList) {
			if (grp->is(mode)) {
				res << printChat(*grp);
			}
		}
		reply(job, "{self}������" + mode + "Ⱥ��¼" + to_string(res.size()) + "��" + res.head(":").show());
	}
	else if (set<long long> grps(DD::getGroupIDList()); mode == "idle") {
		std::priority_queue<std::pair<time_t, string>> qDiver;
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			//if (grp->isGroup && !grps.empty() && !grps.count(id))grp->rmLst();
			if (!grp->getLst())continue;
			time_t tLast{ grp->updated() };
			if (long long tLMT{ DD::getGroupLastMsg(grp->ID, console.DiceMaid) };
				tLMT > 0 && tLMT > tLast)tLast = tLMT;
			if (!tLast)continue;
			int intDay{ int((tNow - tLast) / 86400) };
			qDiver.emplace(intDay, printGroup(id));
		}
		if (qDiver.empty()) {
			reply(job, "{self}��Ⱥ�Ļ�Ⱥ��Ϣ����ʧ�ܣ�");
			return;
		}
		size_t intCnt(0);
		ResList res;
		while (!qDiver.empty()) {
			res << qDiver.top().second + to_string(qDiver.top().first) + "��";
			qDiver.pop();
			if (++intCnt > 32 || qDiver.top().first < 7)break;
		}
		reply(job, "{self}��������Ⱥ�б�:" + res.show(1));
	}
	else if (job->at("list_mode") == "size") {
		std::priority_queue<std::pair<time_t, string>> qSize;
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (!grp->getLst())continue;
			GroupSize_t size(DD::getGroupSize(id));
			if (!size.currSize)continue;
			qSize.emplace(size.currSize, DD::printGroupInfo(id));
		}
		if (qSize.empty()) {
			reply(job, "{self}��Ⱥ�Ļ�Ⱥ��Ϣ����ʧ�ܣ�");
		}
		size_t intCnt(0);
		ResList res;
		while (!qSize.empty()) {
			res << qSize.top().second;
			qSize.pop();
			if (++intCnt > 32 || qSize.top().first < 7)break;
		}
		reply(job, "{self}���ڴ�Ⱥ�б�:" + res.show(1));
	}
}

//�������
void cloud_beat(AttrObject& job) {
	Cloud::heartbeat();
	sch.add_job_for(5 * 60, job);
}

void check_update(AttrObject& job) {
	string ret;
	if (!Network::GET("http://shiki.stringempty.xyz/DiceVer/update", ret)) {
		console.log("��ȡ�汾��Ϣʱ����: \n" + ret, 0);
		return;
	}
	string ver = isDev ? "dev" : "release";
	try {
		fifo_json jInfo(fifo_json::parse(ret));
		if (unsigned short nBuild{ jInfo[ver]["build"] }; nBuild > Dice_Build) {
			MsgNote(job, "����Dice!��{ver}�汾����:" + jInfo[ver]["ver"].get<string>() + "(" + to_string(nBuild) + ")\n����˵����" +
				UTF8toGBK(jInfo[ver]["changelog"].get<string>()), 1);
		}
	}
	catch (std::exception& e) {
		console.log(string("��ȡ����ʧ��:") + e.what(), 0);
	}
	sch.add_job_for(72 * 60 * 60, "check_update");
}
void dice_update(AttrObject& job) {
	string ret;
	if (!Network::GET("http://shiki.stringempty.xyz/DiceVer/update", ret)) {
		reply(job, "{self}��ȡ�汾��Ϣʱ����: \n" + ret);
		return;
	}
	string ver{ job->get_str("ver")};
	if (ver.empty())ver = isDev ? "dev" : "release";
	try {
		fifo_json jInfo(fifo_json::parse(ret));
		if (unsigned short nBuild{ jInfo[ver]["build"] }; nBuild > Dice_Build) {
			MsgNote(job, "����Dice!��{ver}�汾����:" + jInfo[ver]["ver"].get<string>() + "(" + to_string(nBuild) + ")\n����˵����" +
				UTF8toGBK(jInfo[ver]["changelog"].get<string>()), 1);
			if (DD::updateDice(ver, ret)) {
				MsgNote(job, "����Dice!{ver}�汾�ɹ����������غ�Ӧ�á�", 1);
				return;
			}
			else if(jInfo[ver].count("pkg")) {
				string pkg{ jInfo[ver]["pkg"] };
				if (Network::GET(pkg, ret)) {
					std::error_code ec1;
					Zip::extractZip(ret, dirExe / "Diceki");
					MsgNote(job, "����Dice!{ver}�汾�ɹ����������غ�Ӧ�á�", 1);
					return;
				}
			}
			reply(job, "{self}����ʧ�ܡ�" + ret);
		}
		else {
			reply(job, "{self}δ���ָ���{ver}�汾");
		}
	} catch (std::exception& e) {
		job->set("err", e.what());
		reply(job, "{self}��ȡ����ʧ�ܣ�{err}");
	}
}

//��ȡ�Ʋ�����¼
void dice_cloudblack(AttrObject& job) {
	bool isSuccess(false);
	MsgNote(job, "��ʼ��ȡ�Ʋ�����¼", 0);
	string strURL("https://shiki.stringempty.xyz/blacklist/checked.json?" + to_string(time(nullptr)));
	switch (Cloud::DownloadFile(strURL.c_str(), DiceDir / "conf" / "CloudBlackList.json")) {
	case -1: {
		string des;
		if (Network::GET("http://shiki.stringempty.xyz/blacklist/checked.json", des)) {
			ofstream fout(DiceDir / "conf" / "CloudBlackList.json");
			fout << des << endl;
			isSuccess = true;
		}
		else
			reply(job, "ͬ���Ʋ�����¼ͬ��ʧ��:" + des);
	}
		   break;
	case -2:
		reply(job, "{ͬ���Ʋ�����¼ͬ��ʧ��!�ļ�δ�ҵ�}");
		break;
	case 0:
		isSuccess = true;
		break;
	default:
		break;
	}
	if (isSuccess) {
		if (job->has("uid"))MsgNote(job, "ͬ���Ʋ�����¼�ɹ���" + getMsg("self") + "��ʼ��ȡ", 1);
		blacklist->loadJson(DiceDir / "conf" / "CloudBlackList.json", true);
	}
	if (console["CloudBlackShare"])
		sch.add_job_for(24 * 60 * 60, "cloudblack");
}

void log_put(AttrObject& job) {
	int cntExec{ job->get_int("retry") };
	if (!cntExec) {
		DD::debugLog("����log�ļ�:" + job->get_str("log_path"));
		if ((!job->get_ll("gid") || !DD::uploadGroupFile(job->get_ll("gid"), job->get_str("log_path")))
			&& job->get_ll("uid")) {
			DD::sendFriendFile(job->get_ll("uid"), job->get_str("log_path"));
		}
	}
	string nameLog{ Base64urlEncode(job->get_str("log_file")) };
#ifndef _WIN32
	auto curl = curl_easy_init();
	curl_slist* headers = NULL;
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_httppost* pFormPost = NULL;
	curl_httppost* pLastElem = NULL;
	curl_formadd(&pFormPost, &pLastElem,
		CURLFORM_COPYNAME, "key",
		CURLFORM_COPYCONTENTS, nameLog.c_str(),
		CURLFORM_END);
	curl_formadd(&pFormPost, &pLastElem,
		CURLFORM_COPYNAME, "file",
		CURLFORM_FILE, job->get_str("log_path").c_str(),
		CURLFORM_FILENAME, nameLog.c_str(),
		CURLFORM_END); 
	curl_easy_setopt(curl, CURLOPT_URL, "http://dicelogger.s3.ap-southeast-1.amazonaws.com");
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, pFormPost);
	string ret;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Network::curlWriteToString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
	auto res = curl_easy_perform(curl);
	if (res != CURLE_OK)job->set("ret", curl_easy_strerror(res));
	curl_formfree(pFormPost);
	curl_easy_cleanup(curl);
	if (res == CURLE_OK) {
#else
	job->at("ret") = put_s3_object("dicelogger",
		nameLog.c_str(),
		GBKtoLocal(job->get_str("log_path")).c_str(),
		"ap-southeast-1");
	if (job->at("ret") == "SUCCESS") {
#endif //_Win32
		job->at("log_file") = nameLog;
		job->at("log_url") = "https://logpainter.kokona.tech/?s3=" + nameLog;
		reply(job, "{strLogUpSuccess}");
	}
	else if (++cntExec > 2) {
		reply(job, "{strLogUpFailureEnd}");
	}
	else {
		job->inc("retry");
		reply(job, "{strLogUpFailure}");
		console.log(getMsg("strLogUpFailure", job), 1);
		sch.add_job_for(2 * 60, job);
	}
}

string print_master() {
	if (!console)return "��������";
	return printUser(console);
}

string list_deck() {
	return showKey(CardDeck::mPublicDeck);
}
string list_extern_deck() {
	return showKey(CardDeck::mExternPublicDeck);
}
string list_dice_sister() {
	std::set<long long>list{ DD::getDiceSisters() };
	if (list.size() <= 1)return {};
	else {
		list.erase(console.DiceMaid);
		ResList li;
		li << printUser(console.DiceMaid) + "�Ľ�����:";
		for (auto dice : list) {
			li << printUser(dice);
		}
		return li.show();
	}
}