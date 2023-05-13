#pragma once
#include <memory>
#include <optional>
#include "STLExtern.hpp"
class DiceRule {
	dict_ci<> manual;
	friend class DiceRuleSet;
	friend class DiceMod;
public:
	ptr<DiceRule> meta;
	void merge(const DiceRule& other);
	std::optional<string> getManual(const string&);
};
class DiceRuleSet {
public:
	dict_ci<ptr<DiceRule>> rules;
	multidict_ci<> manual_index;
	void merge(const dict_ci<DiceRule>&);
	size_t build();
	void clear() {
		manual_index.clear();
		rules.clear();
	}
	bool has_rule(const string& name)const { return rules.count(name); }
	ptr<DiceRule> get_rule(const string& name)const { return rules.count(name) ? rules.at(name) : ptr<DiceRule>(); }
	std::optional<string> getManual(const string&)const;
	std::optional<string> getManual(const string&, const string&)const;
};
extern ptr<DiceRuleSet> ruleset;