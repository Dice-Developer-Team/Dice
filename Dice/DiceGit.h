#pragma once
#include <string>
#include "filesystem.hpp"
#include "git2.h"
using std::string;
class DiceRepo {
	git_repository* repo{ nullptr };
	git_remote* remote{ nullptr };
	//string remote_url;
	DiceRepo& init(const string&);
	DiceRepo& open(const string&);
	DiceRepo& clone(const string&, const string&);
public:
	DiceRepo(const std::filesystem::path&);
	DiceRepo(const std::filesystem::path&, const string&);
	~DiceRepo() { if (repo)git_repository_free(repo); }
	operator bool()const { return repo; }
	DiceRepo& url(const string&);
	DiceRepo& update();
};