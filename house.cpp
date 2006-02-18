//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include <sstream>
#include <algorithm>

#include "house.h"
#include "ioplayer.h"
#include "game.h"

extern Game g_game;

House::House()
{
	houseName = "OTServ headquarter (Flat 1, Area 42)";
	houseOwner = 0;
	posEntry.x = 0;
	posEntry.y = 0;
	posEntry.z = 0;
}

House::~House()
{
	//
}

void House::addTile(HouseTile* tile)
{
	tile->setFlag(TILESTATE_PROTECTIONZONE);
	houseTiles.push_back(tile);
}

void House::setHouseOwner(uint32_t guid)
{
	houseOwner = guid;
	Item* iiItem = NULL;
	
	std::stringstream houseDescription;
	houseDescription << "It belongs to house '" << houseName << "'. " << std::endl;

	std::string name;
	if(guid != 0 && IOPlayer::instance()->getNameByGuid(guid, name)){
		houseDescription << name;
	}
	else{
		houseDescription << "Nobody";
	}
	
	houseDescription << " owns this house." << std::endl;
	
	HouseDoorList::iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		(*it)->setSpecialDescription(houseDescription.str());
	}
}
/*
ReturnValue House::addGuest(const Player* player)
{
	if(!isInvited(player->getGUID())){
		guestList.push_back(player->getGUID());
		return RET_NOERROR;
	}

	return RET_NOTPOSSIBLE;
}

ReturnValue House::addGuest(const std::string& name)
{
	unsigned long guid = 0;
	unsigned long access = 0;
	std::string dbName = name;

	if(IOPlayer::instance()->getGuidByName(guid, access, dbName)){
		if(!isInvited(guid)){
			guestList.push_back(guid);
			return RET_NOERROR;
		}
	}

	return RET_NOTPOSSIBLE;
}

ReturnValue House::removeGuest(const std::string& name)
{
	unsigned long guid = 0;
	unsigned long access = 0;
	std::string dbName = name;

	if(IOPlayer::instance()->getGuidByName(guid, access, dbName)){
		if(isInvited(guid)){
			InviteList::iterator it = std::find(guestList.begin(), guestList.end(), guid);
			guestList.erase(it);

			Player* uninvitedPlayer = g_game.getPlayerByName(dbName);
			if(uninvitedPlayer){
				HouseTile* houseTile = dynamic_cast<HouseTile*>(uninvitedPlayer->getTile());

				if(houseTile && houseTile->getHouse() == this){
					if(g_game.internalTeleport(uninvitedPlayer, getEntryPosition()) == RET_NOERROR){
						g_game.AddMagicEffectAt(getEntryPosition(), NM_ME_ENERGY_AREA);
					}
				}
			}

			return RET_NOERROR;
		}
	}

	return RET_NOTPOSSIBLE;
}
*/

AccessHouseLevel_t House::getHouseAccessLevel(const Player* player)
{
	if(player->access > 2)
		return HOUSE_OWNER;
	
	if(player->getGUID() == houseOwner)
		return HOUSE_OWNER;
	
	if(subOwnerList.isInList(player))
		return HOUSE_SUBOWNER;
	
	if(guestList.isInList(player))
		return HOUSE_GUEST;
	
	return HOUSE_NO_INVITED;
}

bool House::kickPlayer(Player* player, const std::string& name)
{
	Player* kickingPlayer = g_game.getPlayerByName(name);
	if(kickingPlayer){
		HouseTile* houseTile = dynamic_cast<HouseTile*>(kickingPlayer->getTile());
		
		if(houseTile && houseTile->getHouse() == this){
			if(getHouseAccessLevel(player) >= getHouseAccessLevel(kickingPlayer)){
				if(g_game.internalTeleport(kickingPlayer, getEntryPosition()) == RET_NOERROR){
					g_game.AddMagicEffectAt(getEntryPosition(), NM_ME_ENERGY_AREA);
				}
				return true;
			}
		}
	}
	return false;
}

void House::setAccessList(unsigned long listId, const std::string& textlist)
{
	if(listId == GUEST_LIST){
		guestList.parseList(textlist);
	}
	else if(listId == SUBOWNER_LIST){
		subOwnerList.parseList(textlist);
	}
	else{
		Door* door = getDoorByNumber(listId);
		if(door){
			door->setAccessList(textlist);
		}
		else{
			#ifdef __DEBUG_HOUSES__
			std::cout << "Failure: [House::setAccessList] door == NULL, listId = " << listId <<std::endl;
			#endif
			return;
		}
	}
	HouseTileList::iterator it;
	for(it = houseTiles.begin();it != houseTiles.end(); ++it){
	
	}
		
}

bool House::getAccessList(unsigned long listId, std::string& list)
{
	if(listId == GUEST_LIST){
		guestList.getList(list);
		return true;
	}
	else if(listId == SUBOWNER_LIST){
		subOwnerList.getList(list);
		return true;
	}
	else{
		Door* door = getDoorByNumber(listId);
		if(door){
			return door->getAccessList(list);
		}
		else{
			#ifdef __DEBUG_HOUSES__
			std::cout << "Failure: [House::getAccessList] door == NULL, listId = " << listId <<std::endl;
			#endif
			return false;
		}
	}
	return false;
}

bool House::isInvited(const Player* player)
{
	if(getHouseAccessLevel(player) != HOUSE_NO_INVITED){
		return true;
	}
	else{
		return false;
	}
}

void House::addDoor(Door* door)
{
	doorList.push_back(door);
	door->setHouse(this);
}

Door* House::getDoorByNumber(unsigned long doorId)
{
	HouseDoorList::iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getDoorId() == doorId){
			return *it;
		}
	}
	return NULL;
}

Door* House::getDoorByPosition(const Position& pos)
{
	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getPosition() == pos){
			return *it;
		}
	}

	return NULL;
}

bool House::canEditAccessList(unsigned long listId, const Player* player)
{
	switch(getHouseAccessLevel(player)){
	case HOUSE_OWNER:
		return true;
		break;
	case HOUSE_SUBOWNER:
		if(listId == GUEST_LIST){
			return true;
		}
		else{
			return false;
		}
		break;
	default:
		return false;	
	}
}

AccessList::AccessList()
{
	//
}

AccessList::~AccessList()
{
	//
}

bool AccessList::parseList(const std::string& _list)
{
	playerList.clear();
	guildList.clear();
	expressionList.clear();
	regExList.clear();
	
	std::stringstream listStream(list);
	std::string line;
	while(getline(listStream, line)){
		std::transform(line.begin(), line.end(), line.begin(), tolower);
		if(line.substr(1,1) == "#")
			continue;
		
		//TODO. strip spaces, validate input,...
		if(line.find("!") || line.find("*") || line.find("?")){
			addExpression(line);
		}
		else if(std::string::size_type pos = line.find("@")){
			addGuild(line.substr(pos + 1), "");
		}
		else{
			addPlayer(line);
		}
	}
	return false;
}

bool AccessList::addPlayer(const std::string& name)
{
	unsigned long access;
	unsigned long guid;
	if(IOPlayer::instance()->getGuidByName(guid, access, name)){
		if(playerList.find(guid) == playerList.end()){
			playerList.insert(guid);
			return true;
		}
	}
	return false;
}

bool AccessList::addGuild(const std::string& guildName, const std::string& rank)
{
	unsigned long guildId;
	if(IOPlayer::instance()->getGuilIdByName(guildId, guildName)){
		if(guildId != 0 && guildList.find(guildId) == guildList.end()){
			guildList.insert(guildId);
			return true;
		}
	}
	return false;
}

bool AccessList::addExpression(const std::string& expression)
{
	ExpressionList::iterator it;
	for(it = expressionList.begin(); it != expressionList.end(); ++it){
		if((*it) == expression){
			return false;
		}
	}
	expressionList.push_back(expression);
	if(expression.substr(1,1) == "!"){
		regExList.push_back(std::make_pair(boost::regex(expression.substr(2)), false));
	}
	else{
		regExList.push_back(std::make_pair(boost::regex(expression), true));
	}
	return true;
}

bool AccessList::isInList(const Player* player)
{
	PlayerList::iterator playerIt = playerList.find(player->getGUID());
	if(playerIt != playerList.end())
		return true;

	GuildList::iterator guildIt = guildList.find(player->getGuildId());
	if(guildIt != guildList.end())
		return true;
	
	RegExList::iterator it;
	std::string name = player->getName();
	for(it = regExList.begin(); it != regExList.end(); ++it){
		if(boost::regex_match(name, it->first , boost::match_default) != 0){
			if(it->second){
				return true;
			}
			else{
				return false;
			}
		}
	}
	return false;
}
	
void AccessList::getList(std::string& _list)
{
	_list = list;
}

Door::Door(uint16_t _type):
Item(_type)
{
	house = NULL;
	accessList = NULL;
	doorId = 0;
}

Door::~Door()
{
	if(accessList)
		delete accessList;
}

int Door::unserialize(xmlNodePtr p)
{
	Item::unserialize(p);

	char* tmp = (char*)xmlGetProp(p, (const xmlChar *) "doorId");
	if(tmp){
		setDoorId(atoi(tmp));
		xmlFreeOTSERV(tmp);
	}
	
	return 0;
}

void Door::setHouse(House* _house)
{
	if(house != NULL){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Warning: [Door::setHouse] house != NULL" << std::endl;
		#endif
		return;
	}
	house = _house;
	accessList = new AccessList();
}

bool Door::canUse(const Player* player)
{
	if(!house){
		return true;
	}
	if(house->getHouseAccessLevel(player) == HOUSE_OWNER)
		return true;
	
	return accessList->isInList(player);
}
	
void Door::setAccessList(const std::string& textlist)
{
	if(!house){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Failure: [Door::setAccessList] house == NULL" << std::endl;
		#endif
		return;
	}
	accessList->parseList(textlist);
}

bool Door::getAccessList(std::string& list)
{
	if(!house){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Failure: [Door::getAccessList] house == NULL" << std::endl;
		#endif
		return false;
	}
	accessList->getList(list);
	return true;
}


bool Houses::loadHousesXML(std::string filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		xmlNodePtr root, houseNode;
		char* nodeValue = NULL;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*) "houses") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		houseNode = root->children;
		while(houseNode){
			if(xmlStrcmp(houseNode->name,(const xmlChar*) "house") == 0){
				int _houseid = 0;
				Position entryPos;

				nodeValue = (char*)xmlGetProp(houseNode, (const xmlChar *) "houseid");

				if(!nodeValue){
					houseNode = houseNode->next;
					return false;
				}

				_houseid = atoi(nodeValue);
				House* house = Houses::getInstance().getHouse(_houseid);
				if(!house){
					return false;
				}

				nodeValue = (char*)xmlGetProp(houseNode, (const xmlChar *) "name");
				if(nodeValue){
					house->setName(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(houseNode, (const xmlChar *) "entryx");
				if(nodeValue){
					entryPos.x = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(houseNode, (const xmlChar *) "entryy");
				if(nodeValue){
					entryPos.y = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(houseNode, (const xmlChar *) "entryz");
				if(nodeValue){
					entryPos.z = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				
				house->setEntryPos(entryPos);
				house->setHouseOwner(0);
			}

			houseNode = houseNode->next;
		}

		return true;
	}

	return false;
}
