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
	BlackMark(const BlackMark& mark, std::string IDKey) {
		fromID = llMap[IDKey] = mark.llMap.find(IDKey)->second;
		*this << mark;
	};
	void set(std::string Key, std::string Val) {
		if (Key == "note")while (Val.find('\"') != std::string::npos)Val.replace(Val.find('\"'), 1, "\'");
		strMap[Key] = Val;
	}
	void set(std::string Key, long long Val) {
		llMap[Key] = Val;
		if (Key != "DiceMaid" && Key != "masterQQ")fromID = Val;
	}
	BlackMark& operator<<(const BlackMark& MarkTmp) {
		for (auto it : MarkTmp.strMap) {
			strMap[it.first] = it.second;
		}
		if (MarkTmp.count("DiceMaid"))llMap["DiceMaid"] = MarkTmp.llMap.find("DiceMaid")->second;
		if (MarkTmp.count("masterQQ"))llMap["masterQQ"] = MarkTmp.llMap.find("masterQQ")->second;
		strWarning = MarkTmp.strWarning;
		getWarning();
		return *this;
	}
	const bool operator==(const BlackMark& MarkTmp) {
		if (!count("note") && !MarkTmp.count("note"))return true;
		if (strMap.count("note") && MarkTmp.count("note") && strMap["note"] == MarkTmp.strMap.find("note")->second)return true;
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
	const std::string getJson() const{
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
	const std::string& getWarning() {
		if (strWarning.empty())strWarning = "!warning" + getJson();
		return strWarning;
	}
	const bool isErased() const {
		return strMap.find("type")->second == "erase";
	}
	void erase() {
		strMap["type"] = "erase";
		strWarning.clear();
	}
	const bool isVal(std::string Key, std::string Val) {
		return strMap.count(Key) && strMap[Key] == Val;
	}
	const bool isVal(std::string Key, long long Val) {
		return llMap.count(Key) && llMap[Key] == Val;
	}
	const bool isNoteEmpty() {
		return !strMap.count("note") || strMap["note"].empty();
	}
	const bool count(std::string strKey) const {
		return (strMap.count(strKey) && !strMap.find(strKey)->second.empty()) || (llMap.count(strKey) && llMap.find(strKey)->second);
	}
	const bool hasType() {
		return strMap["type"] == "kick" || strMap["type"] == "ban" || strMap["type"] == "spam" || strMap["type"] == "ruler" || strMap["type"] == "erase";
	}
};