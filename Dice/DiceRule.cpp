#include "DiceRule.h"
ptr<DiceRuleSet> ruleset{ std::make_shared<DiceRuleSet>() };

void DiceRule::merge(const DiceRule& other) {
	map_merge(manual, other.manual);
}
std::optional<string> DiceRule::getManual(const string& key) {
	if (manual.count(key))return manual.at(key);
	else if(meta)return meta->getManual(key);
	return {};
}
void DiceRuleSet::merge(const dict_ci<DiceRule>& m) {
	for (auto& [name, book] : m) {
		if (!rules.count(name))rules.emplace(name, std::make_shared<DiceRule>());
		rules[name]->merge(book);
	}
}
size_t DiceRuleSet::build() {
	for (auto& [rulename, book] : rules) {
		if (size_t pos{ rulename.rfind('-') }; pos != string::npos) do {
			string parent{ rulename.substr(0,pos) };
			if (rules.count(parent)){
				book->meta = rules[parent];
				break;
			}
		} while ((pos = rulename.rfind('-', pos - 1)) != string::npos);
		for (auto& [key, doc] : book->manual) {
			manual_index.emplace(key, rulename);
		}
	}
	return rules.size();
}
std::optional<string> DiceRuleSet::getManual(const string& key)const {
	if (auto pos{ key.find(':') }; pos != string::npos) {
		if (string rule{ key.substr(0,pos) }; rules.count(rule))return rules.at(rule)->getManual(key.substr(pos + 1));
	}
	else if (auto cnt = manual_index.count(key)) {
		if (cnt == 1) {
			return rules.at(manual_index.find(key)->second)->getManual(key);
		}
		else {
			ShowList li;
			for (auto& [k, rule] : multi_range(manual_index, key)) {
				if (auto item{ rules.at(rule)->getManual(key) })li << rule + ":" + key + "\n" + *item;
			}
			return li.show("\f");
		}
	}
	return {};
}
std::optional<string> DiceRuleSet::getManual(const string& key, const string& rule)const {

}