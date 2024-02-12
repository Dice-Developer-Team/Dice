#include "DiceSelfData.h"
#include "DiceFile.hpp"
#include "Jsonio.h"
dict<ptr<SelfData>> selfdata_byFile;
dict<ptr<SelfData>> selfdata_byStem;

SelfData::SelfData(const std::filesystem::path& p) :pathFile(p) {
	mkDir(p.parent_path());
	if (p.extension() == ".json")type = Json;
	else if (p.extension() == ".toml")type = Toml;
	else if (p.extension() == ".yaml"|| p.extension() == ".yml")type = Yaml;
	if (std::filesystem::exists(pathFile = p)) {
		switch (type) {
		case Json:
			data = AttrVar(freadJson(pathFile));
			break;
		case Bin:
			if (std::ifstream fs{ pathFile })data.readb(fs);
			break;
		case Toml:
			if (std::ifstream fs{ pathFile })data = toml::parse(fs);
			break;
		case Yaml:
			data = YAML::LoadFile(getNativePathString(pathFile));
			break;
		default:
			break;
		}
	}
}
void SelfData::save() {
	std::lock_guard<std::mutex> lock(exWrite);
	switch (type) {
	case Json:
		fwriteJson(pathFile, data.to_json(), 1);
		break;
	case Bin:
		if (std::ofstream fs{ pathFile })data.writeb(fs);
		break;
	case Toml:
		if (std::ofstream fs{ pathFile }) {
			if (data.is_table())fs << data.to_obj()->to_toml();
			else fs << data.to_json();
		}
		break;
	case Yaml:
		if (std::ofstream fs{ pathFile })fs << data.to_yaml();
		break;
	default:
		break;
	}
}