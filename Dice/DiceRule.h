#pragma once
#include <memory>
#include <optional>
#include "DiceMsgReply.h"
class DiceRule {
	dict_ci<> manual;
	friend class DiceRuleSet;
	friend class DiceMod;
	void clear() {
		subrules.clear();
	}
public:
	ptr<DiceRule> meta;
	dict_ci<ptr<DiceRule>> subrules;
	DiceReplyUnit orders;
	void merge(const DiceRule& other);
	std::optional<string> traceManual(const string&);
	std::optional<string> searchManual(const string&);
	std::optional<string> getOwnManual(const string&);
	std::optional<string> getManual(const string&);
	bool listen_order(DiceEvent*);
};
class DiceRuleSet {
	//ptr<DiceRule> root{ std::make_shared<DiceRule>() };
public:
	dict_ci<ptr<DiceRule>> rules;
	multidict_ci<> manual_index;
	void merge(const dict_ci<DiceRule>&);
	size_t build();
	void clear() {
		manual_index.clear();
		for (auto& [name, rule] : rules) {
			rule->clear();
		}
		rules.clear();
	}
	bool has_rule(const string& name)const { return rules.count(name); }
	ptr<DiceRule> get_rule(const string& name)const { return rules.count(name) ? rules.at(name) : ptr<DiceRule>(); }
	std::optional<string> getManual(const string&)const;
	std::optional<string> getManual(const string&, const string&)const;
};
extern ptr<DiceRuleSet> ruleset;