/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2024 String.Empty
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
#if !defined(DICE_PYTHON) && !defined(__ANDROID__)
#define DICE_PYTHON
#include <memory>
#include "filesystem.hpp"
#include "DiceAttrVar.h"
class DiceEvent;
class PyGlobal {
public:
	PyGlobal();
	~PyGlobal();
	int runFile(const std::filesystem::path&);
	bool runFile(const std::filesystem::path& s, const AttrObject& context);
	int runString(const std::string&);
	bool execString(const std::string&, const AttrObject& = {});
	AttrVar evalString(const std::string& , const AttrObject& = {});
	bool call_reply(DiceEvent* , const AttrVar&);
};
extern std::unique_ptr<PyGlobal> py; 
bool py_call_event(const AttrObject& eve, const AttrVar& py);
#endif //DICE_PYTHON