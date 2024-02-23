#include "DiceFormatter.h"
#include "ManagerSystem.h"
#include "DiceMod.h"
#include "DicePython.h"
#include "DDAPI.h"
ptr<MarkNode> buildFormatter(const std::string_view& exp);

class MarkReference : public MarkNode {
public:
	MarkReference(const std::string_view& s) :MarkNode(s) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		if (context && context->has(leaf)) {
			if (leaf == "res")return fmt->format(context->print(leaf), context, isTrust, global);
			else return context->print(leaf);
		}
		else if (AttrVar val{ getContextItem(context, leaf, isTrust) }) {
			return val;
		}
		else if (auto cit = global.find(leaf); cit != global.end()) {
			return fmt->format(cit->second, context, isTrust, global);
		}
		//语境优先于全局
		else if (auto sp = fmt->global_speech.find(leaf); sp != fmt->global_speech.end()) {
			val = fmt->format(sp->second.express(), context, isTrust, global);
			if (!isTrust && val == "\f")val = "\f> ";
			return val;
		}
		return {};
	}
};
class MarkGlobalIndexNode : public MarkNode {
	GlobalTex func;
public:
	MarkGlobalIndexNode(const std::string_view& s) :MarkNode(s),func(strFuncs.at(string(s))) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		return func();
	}
};
class MarkAtNode : public MarkNode {
	ptr<MarkNode> target;
public:
	MarkAtNode(const std::string_view& s) :MarkNode(s), target(buildFormatter(s)) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		if (auto tgt{ target->concat(context, isTrust, global).to_str()}; tgt == "self") {
			return "[CQ:at,id=" + to_string(console.DiceMaid) + "]";
		}
		else if (!tgt.empty()) {
			return "[CQ:at,id=" + tgt + "]";
		}
		else if (context->has("uid")) {
			return "[CQ:at,id=" + context->get_str("uid") + "]";
		}
		return "@" + leaf;
	}
};
class MarkPrintNode : public MarkNode {
	fifo_dict<string> paras;
public:
	MarkPrintNode(const std::string_view& s) :MarkNode(s), paras(splitPairs(string(s), '=', '&')) {
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string> & = {})const override {
		if (paras.count("uid")) {
			if (paras.at("uid").empty())return printUser(context->get_ll("uid"));
			else if(isTrust)return printUser(AttrVar(paras.at("uid")).to_ll());
		}
		else if (paras.count("gid")) {
			if (paras.at("gid").empty())return printGroup(context->get_ll("gid"));
			else if (isTrust)return printGroup(AttrVar(paras.at("gid")).to_ll());
		}
		else if (paras.count("master")) {
			return console ? printUser(console) : "[无主]";
		}
		else if (paras.count("self")) {
			return printUser(console.DiceMaid);
		}
		return leaf;
	}
};
class MarkSampleNode : public MarkNode {
	vector<ptr<MarkNode>> samples;
public:
	MarkSampleNode(const std::string& s) :MarkNode(s){
		for (auto& item : split(s, "|")) {
			samples.emplace_back(buildFormatter(item));
		}
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		if (!samples.empty()) {
			return samples[(size_t)RandomGenerator::Randint(0, samples.size() - 1)]->concat(context, isTrust, global);
		}
		return leaf;
	}
};
class MarkVarNode : public MarkNode {
	string field;
	ptr<MarkNode> exp;
public:
	MarkVarNode(const std::string_view& s) :MarkNode(s) {
		auto [name, val] = readini<string, string>(string(s), '?');
		field = name;
		exp = buildFormatter(val);
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		return context->at(field) = exp->concat(context, isTrust, global);
	}
};
class MarkDefineNode : public MarkNode {
	dict<ptr<MarkNode>> defines;
public:
	MarkDefineNode(const std::string_view& s) :MarkNode(s) {
		auto exps{ splitPairs(string(s),'=','&') };
		for (auto& [field, exp] : exps) {
			defines.emplace(field, buildFormatter(exp));
		}
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		for (auto& [field, exp] : defines) {
			context->set(field, exp->concat(context, isTrust, global));
		}
		return "";
	}
};
class MarkRandomNode : public MarkNode {
	ptr<MarkNode> min, max;
public:
	MarkRandomNode(const std::string_view& s) :MarkNode(s) {
		auto [min_exp, max_exp] = readini<string, string>(string(s), '~');
		min = buildFormatter(min_exp);
		max = buildFormatter(max_exp);
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		int l = min->concat(context, isTrust, global).to_int(), r = max->concat(context, isTrust, global).to_int();
		return l < r ? AttrVar(RandomGenerator::Randint(l, r)) : AttrVar();
	}
};
class MarkHelpNode : public MarkNode {
public:
	MarkHelpNode(const std::string_view& s) :MarkNode(s) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		return fmt->get_help(leaf, context);
		return "{" + leaf + "}";
	}
};
class MarkWaitNode : public MarkNode {
	ptr<MarkNode> arg;
public:
	MarkWaitNode(const std::string_view& s) :MarkNode(s), arg(buildFormatter(s)) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		if (long long ms{ arg->concat(context, isTrust, global).to_ll() }; 0 < ms && ms < 600000)
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		return {};
	}
};
class MarkCaseNode : public MarkNode {
	MarkReference field;
	dict<ptr<MarkNode>> cases;
	ptr<MarkNode> other;
public:
	MarkCaseNode(const std::string_view& s) :MarkNode(s), field(s.substr(0,s.find('?'))) {
		auto paras{ splitPairs(string(s.substr(s.find('?') + 1)),'=','&') };
		for (auto& [key, val] : paras) {
			if (key == "else")other = buildFormatter(val);
			else cases[key] = buildFormatter(val);
		}
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override{
		auto res{ field.concat(context, isTrust, global)};
		if (auto i{ cases.find(res.print()) }; i != cases.end()) {
			return i->second->concat(context, isTrust, global);
		}
		return other ? other->concat(context, isTrust, global) : AttrVar();
	}
};
class MarkGradeNode : public MarkNode {
	MarkReference field;
	grad_map<double, ptr<MarkNode>> grades;
public:
	MarkGradeNode(const std::string_view& s) :MarkNode(s), field(s.substr(0, s.find('?'))) {
		auto paras{ splitPairs(string(s.substr(s.find('?') + 1)),'=','&') };
		for (auto& [key, val] : paras) {
			if (key == "else")grades.set_else(buildFormatter(val));
			else if (isNumeric(key))grades.set_step(stod(key), buildFormatter(val));
		}
	}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		if (auto res{ field.concat(context, isTrust, global) };res.is_numberic()) {
			if (auto grade{ grades[res.to_ll()] })
				return grade->concat(context, isTrust, global);
		}
		else if (auto grade{ grades.get_else() })
			return grade->concat(context, isTrust, global);
		return AttrVar();
	}
};
class MarkJSNode : public MarkNode {
public:
	MarkJSNode(const std::string_view& s) :MarkNode(s) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
		return js_context_eval(leaf, context.p);
	}
};
class MarkPyNode : public MarkNode {
public:
	MarkPyNode(const std::string_view& s) :MarkNode(s) {}
	AttrVar format(const AttrObject& context, bool isTrust = true, const dict_ci<string>& global = {})const override {
#ifdef DICE_PYTHON
		if (py)return py->evalString(leaf, context);
#endif //DICE_PYTHON
		return {};
	}
};

static enumap<string> methods{ "var","print","help","sample","case","vary","grade","at","ran","wait","js","py","len" };
enum class FmtMethod { Var, Print, Help, Sample, Case, Vary, Grade, At, Ran, Wait, JS, Py, Len };
//Formatter
size_t find_close_brace(const std::string_view& s, size_t pos) {
	int diff = 1;
	while (pos < s.length()) {
		if (s[pos] == '{' && s[pos - 1] != '\\') {
			++diff;
		}
		else if (s[pos] == '}' && s[pos - 1] != '\\') {
			if (0 == --diff)return pos;
		}
		++pos;
	}
	return string::npos;
}
ptr<MarkNode> buildFormatter(const std::string_view& exp) {
	ptr<MarkNode> root;
	size_t preR{ 0 }, lastL{ 0 }, lastR{ 0 };
	ptr<MarkNode>* cur = &root;
	while ((lastL = exp.find('{', lastR)) != string::npos
		&& (lastR = find_close_brace(exp, lastL + 1)) != string::npos) {
		std::string_view field;
		//括号前加‘\’表示该括号内容不转义
		if (exp[lastR - 1] == '\\') {
			lastL = lastR - 1;
			field = exp.substr(lastR, 1);
		}
		else if (lastL > 0 && exp[lastL - 1] == '\\') {
			lastR = lastL--;
			field = exp.substr(lastR, 1);
		}
		else field = exp.substr(lastL + 1, lastR - lastL - 1);
		if (preR < lastL) {
			*cur = std::make_shared<MarkNode>(exp.substr(preR, lastL - preR));
			cur = &(*cur)->next;
		}
		preR = ++lastR;
		if (field == "}" || field == "{") {
			*cur = std::make_shared<MarkNode>(field);
			cur = &(*cur)->next;
			continue;
		}
		if (size_t colon{ field.find(':') }; colon != string::npos) {
			std::string_view method{ field.substr(0,colon) };
			std::string_view para{ field.substr(colon + 1) };
			if (method == "sample") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkSampleNode>(string(para)));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "at") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkAtNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "print") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkPrintNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "help") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkHelpNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "case") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkCaseNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "grade") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkGradeNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "var") {
				auto posQ = para.find('?'), posE = para.find('=');
				if (posQ < posE) {
					*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkVarNode>(para));
				}
				else {
					*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkDefineNode>(para));
				}
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "ran") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkRandomNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "wait") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkWaitNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "js") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkJSNode>(para));
				cur = &(*cur)->next;
				continue;
			}
			else if (method == "py") {
				*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkPyNode>(para));
				cur = &(*cur)->next;
				continue;
			}
		}
		if (auto func = strFuncs.find(string(field)); func != strFuncs.end()) {
			*cur = std::make_shared<MarkGlobalIndexNode>(field);
			cur = &(*cur)->next;
			continue;
		}
		*cur = std::static_pointer_cast<MarkNode>(std::make_shared<MarkReference>(field));
		cur = &(*cur)->next;
	}
	if (preR < exp.length())*cur = std::make_shared<MarkNode>(exp.substr(preR));
	return root;
}

static dict<ptr<MarkNode>> formatters;
AttrVar formatMsg(const string& s, const AttrObject& ctx, bool isTrust, const dict_ci<string>& global) {
	if (s.find('{') == string::npos)return s;
	if (!formatters.count(s)) {
		formatters.emplace(s, buildFormatter(s));
	}
	return formatters.at(s)->concat(ctx, isTrust, global);
}