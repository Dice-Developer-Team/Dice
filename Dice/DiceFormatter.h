/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Msg Formatter
 * Copyright (C) 2018-2021 w4123 Suhui
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
#include "DiceAttrVar.h"
#include "STLExtern.hpp"
//Plain Node
class MarkNode {
protected:
	string leaf;
public:
	ptr<MarkNode> next;
	MarkNode(const std::string_view& s) :leaf(s){}
	virtual AttrVar format(const AttrObject& ctx, bool = true, const dict_ci<>& = {})const { return AttrVar::parse(leaf); }
	AttrVar concat(const AttrObject& ctx, bool isTrust = true, const dict_ci<string>& global = {})const {
		return next ? format(ctx, isTrust, global) + next->concat(ctx).to_str()
			: format(ctx, isTrust, global);
	}
};

//ptr<MarkNode> buildFormatter(const std::string_view&); 
AttrVar formatMsg(const string& s, const AttrObject& ctx, bool isTrust = true, const dict_ci<string>& global = {});