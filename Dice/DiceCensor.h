#pragma once

#include <string>
#include "filesystem.hpp"
#include "STLExtern.hpp"
using std::string;

class Censor {
	string dirWords;
public:
	enum class Level :size_t {
		Ignore,		//����
		Notice,		//��0��֪ͨ
		Caution,		//��1��֪ͨ
		Warning,		//�����û��Ҿܾ��ƺ�����1��֪ͨ
		Danger,		//�����û��Ҿܾ�ָ���3��֪ͨ
		Critical,		//��ռλ��������
	};
	dict_ci<Level> words;
	dict_ci<Level> CustomWords;
	Level get_level(const string&);
	void insert(const string& word, Level);
	void add_word(const string& word, Level);
	bool rm_word(const string& word);
	void build();
	void save();
	size_t size()const { return words.size(); }
	int search(const string& text, vector<string>& res);
};

extern Censor censor;

int load_words(const std::filesystem::path& path, Censor& cens);