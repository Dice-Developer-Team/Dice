#include "DiceMod.h"
ptr<DiceRuleSet> ruleset{ std::make_shared<DiceRuleSet>() };

void DiceRule::merge(const DiceRule& other) {
	map_merge(manual, other.manual);
	map_merge(orders.items, other.orders.items);
	map_merge(cassettes, other.cassettes);
}
std::optional<string> DiceRule::traceManual(const string& key) {
	if (manual.count(key))return manual.at(key);
	else if (meta)return meta->traceManual(key);
	return std::nullopt;
}
std::optional<string> DiceRule::searchManual(const string& key) {
	for (auto& [name, sub] : subrules) {
		if (auto item{ sub->searchManual(key) })return item;
	}
	return std::nullopt;
}
std::optional<string> DiceRule::getOwnManual(const string& key) {
	if (manual.count(key))return manual.at(key);
	return std::nullopt;
}
std::optional<string> DiceRule::getManual(const string& key) {
	if (manual.count(key))return manual.at(key);
	else if (auto item{ searchManual(key) })return item;
	else if (meta)return meta->traceManual(key);
	return std::nullopt;
}
bool DiceRule::listen_order(DiceEvent* eve) {
	if (orders.listen(eve, 0b100))return true;
	/*else for (auto& [name, sub] : subrules) {
		if (sub->listen_order(eve))return true;
	}*/
	if (meta)return meta->listen_order(eve);
	return false;
}
bool DiceRule::listen_cassette(const string& tape, DiceEvent* eve)const{
	if (auto t{ cassettes.find(tape) }; t != cassettes.end() && t->second) {
		call_event(eve->shared_from_this(), t->second);
		return eve->is("blocked");
	}
	return false;
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
				rules[parent]->subrules[rulename] = book;
				break;
			}
		} while ((pos = rulename.rfind('-', pos - 1)) != string::npos);
		for (auto& [key, doc] : book->manual) {
			manual_index.emplace(key, rulename);
		}
		book->orders.build();
	}
	return rules.size();
}
std::optional<string> DiceRuleSet::getManual(const string& key)const {
	if (auto pos{ key.find(':') }; pos != string::npos) {
		if (string rule{ key.substr(0,pos) }; rules.count(rule))return rules.at(rule)->getManual(key.substr(pos + 1));
	}
	else if (auto cnt = manual_index.count(key)) {
		if (cnt == 1) {
			if (auto rule{ get_rule(manual_index.find(key)->second)})return rule->getOwnManual(key);
		}
		else {
			ShowList li;
			for (auto& [k, rulename] : multi_range(manual_index, key)) {
				if (auto rule{ get_rule(rulename) })li << rulename + ":" + key + "\n" + rule->getOwnManual(key).value_or("[DATA EXPUNGED]");
			}
			return li.show("\f");
		}
	}
	return std::nullopt;
}