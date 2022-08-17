#include "DiceSelfData.h"
#include "DiceFile.hpp"
#include "Jsonio.h"
#include "DiceExtensionManager.h"
dict<ptr<SelfData>> selfdata_byFile;
dict<ptr<SelfData>> selfdata_byStem;

SelfData::SelfData(const std::filesystem::path& p) :pathFile(p) {
	mkDir(p.parent_path());
	if (p.extension() == ".json")type = Json;
	if (std::filesystem::exists(pathFile = p)) {
		switch (type) {
		case Json:
			from_json(freadJson(pathFile), *data);
			break;
		case Bin:
			if (std::ifstream fs{ pathFile })data.readb(fs);
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
		fwriteJson(pathFile, to_json(*data), 0);
		break;
	case Bin:
		if (std::ofstream fs{ pathFile })data.writeb(fs);
		break;
	default:
		break;
	}
}