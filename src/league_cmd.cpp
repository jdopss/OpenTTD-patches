/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file league_cmd.cpp Handling of league tables. */

#include "stdafx.h"
#include "league_base.h"
#include "command_type.h"
#include "command_func.h"
#include "industry.h"
#include "story_base.h"
#include "town.h"
#include "window_func.h"
#include "core/pool_func.hpp"
#include "company_base.h"

#include "safeguards.h"

LeagueTableElementPool _league_table_element_pool("LeagueTableElement");
INSTANTIATE_POOL_METHODS(LeagueTableElement)

LeagueTablePool _league_table_pool("LeagueTable");
INSTANTIATE_POOL_METHODS(LeagueTable)

/**
 * Checks whether a link is valid, i.e. has a valid target.
 * @param link the link to check
 * @return true iff the link is valid
 */
bool IsValidLink(Link link)
{
	switch (link.type) {
		case LT_NONE: return (link.target == 0);
		case LT_TILE: return IsValidTile(link.target);
		case LT_INDUSTRY: return Industry::IsValidID(link.target);
		case LT_TOWN: return Town::IsValidID(link.target);
		case LT_COMPANY: return Company::IsValidID(link.target);
		case LT_STORY_PAGE: return StoryPage::IsValidID(link.target);
		default: return false;
	}
	return false;
}

/**
 * Create a new league table.
 * @param flags type of operation
 * @param title Title of the league table
 * @param header Text to show above the table
 * @param footer Text to show below the table
 * @return the cost of this operation or an error
 */
std::tuple<CommandCost, LeagueTableID> CmdCreateLeagueTable(DoCommandFlag flags, const std::string &title, const std::string &header, const std::string &footer)
{
	if (_current_company != OWNER_DEITY) return { CMD_ERROR, INVALID_LEAGUE_TABLE };
	if (!LeagueTable::CanAllocateItem()) return { CMD_ERROR, INVALID_LEAGUE_TABLE };
	if (title.empty()) return { CMD_ERROR, INVALID_LEAGUE_TABLE };

	if (flags & DC_EXEC) {
		LeagueTable *lt = new LeagueTable();
		lt->title = title;
		lt->header = header;
		lt->footer = footer;
		return { CommandCost(), lt->index };
	}

	return { CommandCost(), INVALID_LEAGUE_TABLE };
}


/**
 * Create a new element in a league table.
 * @param flags type of operation
 * @param table Id of the league table this element belongs to
 * @param rating Value that elements are ordered by
 * @param company Company to show the color blob for or INVALID_COMPANY
 * @param text Text of the element
 * @param score String representation of the score associated with the element
 * @param link_type Type of the referenced object
 * @param link_target Id of the referenced object
 * @return the cost of this operation or an error
 */
std::tuple<CommandCost, LeagueTableElementID> CmdCreateLeagueTableElement(DoCommandFlag flags, LeagueTableID table, int64 rating, CompanyID company, const std::string &text, const std::string &score, LinkType link_type, LinkTargetID link_target)
{
	if (_current_company != OWNER_DEITY) return { CMD_ERROR, INVALID_LEAGUE_TABLE_ELEMENT };
	if (!LeagueTableElement::CanAllocateItem()) return { CMD_ERROR, INVALID_LEAGUE_TABLE_ELEMENT };
	Link link{link_type, link_target};
	if (!IsValidLink(link)) return { CMD_ERROR, INVALID_LEAGUE_TABLE_ELEMENT };
	if (company != INVALID_COMPANY && !Company::IsValidID(company)) return { CMD_ERROR, INVALID_LEAGUE_TABLE_ELEMENT };

	if (flags & DC_EXEC) {
		LeagueTableElement *lte = new LeagueTableElement();
		lte->table = table;
		lte->rating = rating;
		lte->company = company;
		lte->text = text;
		lte->score = score;
		lte->link = link;
		InvalidateWindowData(WC_COMPANY_LEAGUE, table);
		return { CommandCost(), lte->index };
	}
	return { CommandCost(), INVALID_LEAGUE_TABLE_ELEMENT };
}

/**
 * Update the attributes of a league table element.
 * @param flags type of operation
 * @param element Id of the element to update
 * @param company Company to show the color blob for or INVALID_COMPANY
 * @param text Text of the element
 * @param link_type Type of the referenced object
 * @param link_target Id of the referenced object
 * @return the cost of this operation or an error
 */
CommandCost CmdUpdateLeagueTableElementData(DoCommandFlag flags, LeagueTableElementID element, CompanyID company, const std::string &text, LinkType link_type, LinkTargetID link_target)
{
	if (_current_company != OWNER_DEITY) return CMD_ERROR;
	auto lte = LeagueTableElement::GetIfValid(element);
	if (lte == nullptr) return CMD_ERROR;
	if (company != INVALID_COMPANY && !Company::IsValidID(company)) return CMD_ERROR;
	Link link{link_type, link_target};
	if (!IsValidLink(link)) return CMD_ERROR;

	if (flags & DC_EXEC) {
		lte->company = company;
		lte->text = text;
		lte->link = link;
		InvalidateWindowData(WC_COMPANY_LEAGUE, lte->table);
	}
	return CommandCost();
}

/**
 * Update the score of a league table element.
 * @param flags type of operation
 * @param element Id of the element to update
 * @param rating Value that elements are ordered by
 * @param score String representation of the score associated with the element
 * @return the cost of this operation or an error
 */
CommandCost CmdUpdateLeagueTableElementScore(DoCommandFlag flags, LeagueTableElementID element, int64 rating, const std::string &score)
{
	if (_current_company != OWNER_DEITY) return CMD_ERROR;
	auto lte = LeagueTableElement::GetIfValid(element);
	if (lte == nullptr) return CMD_ERROR;

	if (flags & DC_EXEC) {
		lte->rating = rating;
		lte->score = score;
		InvalidateWindowData(WC_COMPANY_LEAGUE, lte->table);
	}
	return CommandCost();
}

/**
 * Remove a league table element.
 * @param flags type of operation
 * @param element Id of the element to update
 * @return the cost of this operation or an error
 */
CommandCost CmdRemoveLeagueTableElement(DoCommandFlag flags, LeagueTableElementID element)
{
	if (_current_company != OWNER_DEITY) return CMD_ERROR;
	auto lte = LeagueTableElement::GetIfValid(element);
	if (lte == nullptr) return CMD_ERROR;

	if (flags & DC_EXEC) {
		auto table = lte->table;
		delete lte;
		InvalidateWindowData(WC_COMPANY_LEAGUE, table);
	}
	return CommandCost();
}

CommandCost CmdCreateLeagueTable(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	std::string title, header, footer;
	text = StrConsumeToSeparator(title, text);
	if (text == nullptr) return CMD_ERROR;
	text = StrConsumeToSeparator(header, text);
	if (text == nullptr) return CMD_ERROR;
	text = StrConsumeToSeparator(footer, text);
	if (text != nullptr) return CMD_ERROR;

	auto [res, id] = CmdCreateLeagueTable(flags, title, header, footer);
	res.SetResultData(id);
	return res;
}

CommandCost CmdCreateLeagueTableElement(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, uint64 p3, const char *text, const CommandAuxiliaryBase *aux_data)
{
	std::string text_str, score;
	text = StrConsumeToSeparator(text_str, text);
	if (text == nullptr) return CMD_ERROR;
	text = StrConsumeToSeparator(score, text);
	if (text != nullptr) return CMD_ERROR;

	LeagueTableID table = GB(p1, 0, 8);
	int64 rating = p3;
	CompanyID company = (CompanyID)GB(p1, 8, 8);
	LinkType link_type = (LinkType)GB(p1, 16, 8);
	LinkTargetID link_target = (LinkTargetID)p2;

	auto [res, id] = CmdCreateLeagueTableElement(flags, table, rating, company, text_str, score, link_type, link_target);
	res.SetResultData(id);
	return res;
}

CommandCost CmdUpdateLeagueTableElementData(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	std::string title, header, footer;
	text = StrConsumeToSeparator(title, text);
	if (text == nullptr) return CMD_ERROR;
	text = StrConsumeToSeparator(header, text);
	if (text == nullptr) return CMD_ERROR;
	text = StrConsumeToSeparator(footer, text);
	if (text != nullptr) return CMD_ERROR;

	LeagueTableElementID element = GB(p1, 0, 16);
	CompanyID company = (CompanyID)GB(p1, 16, 8);
	LinkType link_type = (LinkType)GB(p1, 24, 8);
	LinkTargetID link_target = (LinkTargetID)p2;

	return CmdUpdateLeagueTableElementData(flags, element, company, text, link_type, link_target);
}

CommandCost CmdUpdateLeagueTableElementScore(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, uint64 p3, const char *text, const CommandAuxiliaryBase *aux_data)
{
	return CmdUpdateLeagueTableElementScore(flags, p1, p3, text);
}

CommandCost CmdRemoveLeagueTableElement(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	return CmdRemoveLeagueTableElement(flags, p1);
}
