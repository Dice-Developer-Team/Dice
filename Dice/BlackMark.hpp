/*
 * ºÚÃûµ¥Ã÷Ï¸
 * Copyright (C) 2019 String.Empty
 */
#include <string>
#include <map>
#include <vector>

class BlackMark {
public:
	long long fromID = 0;
	std::string strWarning;
	std::map<std::string, long long>llMap;
	std::map<std::string, std::string>strMap = { {"type",""} };
	BlackMark() {};
	BlackMark(long long llQQ) :fromID(llQQ) {};
	BlackMark(std::string IDKey, long long llID) :fromID(llID) {
		llMap[IDKey] = llID;
	};
	BlackMark(BlackMark &mark, std::string IDKey){
		fromID = llMap[IDKey] = mark.llMap[IDKey];
		*this << mark;
	};
	void set(std::string Key, std::string Val) {
		if (Key == "note")while (Val.find('\"') != std::string::npos)Val.replace(Val.find('\"'), 1, "\'");
		strMap[Key] = Val;
	}
	void set(std::string Key, long long Val) {
		llMap[Key] = Val;
		if (Key != "DiceMaid"&&Key != "masterQQ")fromID = Val;
	}
	BlackMark& operator<<(BlackMark& MarkTmp) {
		for (auto it : MarkTmp.strMap) {
			strMap[it.first]=it.second;
		}
		if (MarkTmp.count("DiceMaid"))llMap["DiceMaid"] = MarkTmp.llMap["DiceMaid"];
		if (MarkTmp.count("masterQQ"))llMap["masterQQ"] = MarkTmp.llMap["masterQQ"];
		strWarning = MarkTmp.getWarning();
		return *this;
	}
	const bool operator==(BlackMark& MarkTmp) {
		if (!count("note") && !MarkTmp.count("note"))return true;
		if (strMap.count("note") && MarkTmp.count("note") && strMap["note"] == MarkTmp.strMap["note"])return true;
		return false;
	}
	const char* getData() {
		std::string data = "fromQQ=" + std::to_string(llMap["fromQQ"]) + "&fromGroup=" + std::to_string(llMap["fromGroup"]) + "&DiceMaid=" + std::to_string(llMap["DiceMaid"]) + "&masterQQ=" + std::to_string(llMap["masterQQ"]);
		for (auto it : strMap) {
			if (it.first == "note") {
				continue;
			}
			data += "&" + it.first + "=" + it.second;
		}
		return data.data();
	}
	const std::string getJson() {
		std::string strJson = "{";
		if (llMap.empty())strJson += "\n\"fromQQ\":" + std::to_string(fromID) + ",";
		for (auto it : strMap) {
			strJson += "\n\"" + it.first + "\":\"" + it.second + "\",";
		}
		for (auto it : llMap) {
			strJson += "\n\"" + it.first + "\":" + std::to_string(it.second) + ",";
		}
		strJson.erase(strJson.end() - 1);
		strJson += "\n}";
		return strJson;
	}
	const std::string getWarning() {
		if (strWarning.empty())strWarning = "!warning" + getJson();
		return strWarning;
	}
	const bool isErased() {
		return strMap["type"] == "erase";
	}
	void erase() {
		strMap["type"] = "erase";
		strWarning.clear();
	}
	const bool isVal(std::string Key,std::string Val) {
		return strMap.count(Key) && strMap[Key] == Val;
	}
	const bool isVal(std::string Key,long long Val) {
		return llMap.count(Key) && llMap[Key] == Val;
	}
	const bool isNoteEmpty() {
		return !strMap.count("note") || strMap["note"].empty();
	}
	const bool count(std::string strKey) {
		return (strMap.count(strKey) && !strMap[strKey].empty()) || (llMap.count(strKey) && llMap[strKey]);
	}
	const bool hasType() {
		return strMap["type"] == "kick" || strMap["type"] == "ban" || strMap["type"] == "spam" || strMap["type"] == "erase";
	}
};