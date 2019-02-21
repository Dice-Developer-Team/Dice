/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123ËÝä§
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef DICE_NAME_GENERATOR
#define DICE_NAME_GENERATOR
#include <string>
#include <vector>

namespace NameGenerator
{
	extern const std::vector<std::string> ChineseFirstName;
	extern const std::vector<std::string> ChineseSurname;
	extern const std::vector<std::string> JapaneseFirstName;
	extern const std::vector<std::string> JapaneseSurname;
	extern const std::vector<std::string> EnglishFirstName;
	extern const std::vector<std::string> EnglishLastName;
	extern const std::vector<std::string> EnglishLastNameChineseTranslation;
	extern const std::vector<std::string> EnglishFirstNameChineseTranslation;

	enum class Type
	{
		CN = 1,
		EN = 2,
		JP = 3,
		UNKNOWN = 0
	};

	std::string getChineseName();
	std::string getEnglishName();
	std::string getJapaneseName();
	std::string getRandomName(Type type = Type::UNKNOWN);
};
#endif /*DICE_NAME_GENERATOR*/


