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
#include "otpch.h"

#include "monsters.h"
#include "monster.h"
#include "container.h"
#include "tools.h"
#include "spells.h"
#include "combat.h"
#include "luascript.h"
#include "weapons.h"
#include "configmanager.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern Spells* g_spells;
extern Monsters g_monsters;
extern ConfigManager g_config;

MonsterType::MonsterType()
{
	reset();
}

void MonsterType::reset()
{
	isSummonable = false;
	isIllusionable = false;
	isConvinceable = false;
	isAttackable = true;
	isHostile = true;
	race = RACE_BLOOD;
	experience = 0;

	defense = 0;
	armor = 0;

	canPushItems = false;
	staticAttackChance = 95;
	maxSummons = 0;
	targetDistance = 1;
	runAwayHealth = 0;
	pushable = true;
	base_speed = 200;
	
	health = 100;
	health_max = 100;
	damageImmunities = 0;
	conditionImmunities = 0;
	lightLevel = 0;
	lightColor = 0;

	manaCost = 0;
	summonList.clear();
	lootItems.clear();

	for(SpellList::iterator it = spellAttackList.begin(); it != spellAttackList.end(); ++it){
		if(it->combatSpell){
			delete it->spell;
			it->spell = NULL;
		}
	}

	spellAttackList.clear();

	for(SpellList::iterator it = spellDefenseList.begin(); it != spellDefenseList.end(); ++it){
		if(it->combatSpell){
			delete it->spell;
			it->spell = NULL;
		}
	}

	spellDefenseList.clear();

	yellSpeedTicks = 0;
	yellChance = 0;
	voiceVector.clear();

	changeTargetSpeed = 0;
	changeTargetChance = 0;

	attackStrength = 100;
	defenseStrength = 0;
}

MonsterType::~MonsterType()
{
	//
}

uint32_t Monsters::getLootRandom()
{
	return random_range(0, MAX_LOOTCHANCE)/g_config.getNumber(ConfigManager::RATE_LOOT);
}

void MonsterType::createLoot(Container* corpse)
{
	for(LootItems::const_iterator it = lootItems.begin(); it != lootItems.end() && (corpse->capacity() - corpse->size() > 0); it++){
		Item* tmpItem = createLootItem(*it);
		if(tmpItem){
			//check containers
			if(Container* container = tmpItem->getContainer()){
				createLootContainer(container, *it);
				if(container->size() == 0){
					delete container;
				}
				else{
					corpse->__internalAddThing(tmpItem);
				}
			}
			else{
				corpse->__internalAddThing(tmpItem);
			}
		}
	}

	corpse->__startDecaying();
}

Item* MonsterType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = NULL;
	if(Item::items[lootBlock.id].stackable){
		uint32_t randvalue = Monsters::getLootRandom();
		if(randvalue < lootBlock.chance){
			uint32_t n = randvalue % lootBlock.countmax + 1;
			tmpItem = Item::CreateItem(lootBlock.id, n);
		}
	}
	else{
		if(Monsters::getLootRandom() < lootBlock.chance){
			tmpItem = Item::CreateItem(lootBlock.id, 0);
		}
	}
	
	if(tmpItem){
		if(lootBlock.subType != -1){
			tmpItem->setItemCountOrSubtype(lootBlock.subType);
		}

		if(lootBlock.actionId != -1){
			tmpItem->setActionId(lootBlock.actionId);
		}

		if(lootBlock.text != ""){
			tmpItem->setText(lootBlock.text);
		}

		return tmpItem;
	}

	return NULL;
}

void MonsterType::createLootContainer(Container* parent, const LootBlock& lootblock)
{
	if(parent->size() < parent->capacity()){
		LootItems::const_iterator it;
		for(it = lootblock.childLoot.begin(); it != lootblock.childLoot.end(); it++){
			Item* tmpItem = createLootItem(*it);
			if(tmpItem){
				if(Container* container = tmpItem->getContainer()){
					createLootContainer(container, *it);
					if(container->size() == 0){
						delete container;
					}
					else{
						parent->__internalAddThing(container);
					}
				}
				else{
					parent->__internalAddThing(tmpItem);
				}
			}
		}
	}
}

Monsters::Monsters()
{
	loaded = false;
}

bool Monsters::loadFromXml(const std::string& _datadir,bool reloading /*= false*/)
{	
	loaded = false;	
	datadir = _datadir;
	
	std::string filename = datadir + "monster/monsters.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		unsigned long id = 0;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monsters") != 0){
			xmlFreeDoc(doc);
			loaded = false;
			return false;
		}

		p = root->children;
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"monster") == 0){
				std::string file;
				std::string name;

				if(readXMLString(p, "file", file) && readXMLString(p, "name", name)){
					file = datadir + "monster/" + file;
					
					std::string lowername = name;
					toLowerCaseString(lowername);
						
					MonsterType* mType = loadMonster(file, name, reloading);
					if(mType){
						id++;
						monsterNames[lowername] = id;
						monsters[id] = mType;
					}
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}

	return loaded;
}

bool Monsters::reload()
{
	return loadFromXml(datadir, true);
}

ConditionDamage* Monsters::getDamageCondition(ConditionType_t conditionType, int32_t maxDamage, int32_t minDamage, int32_t startDamage)
{
	ConditionDamage* condition = dynamic_cast<ConditionDamage*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, 0, 0));
	condition->setParam(CONDITIONPARAM_MINVALUE, minDamage);
	condition->setParam(CONDITIONPARAM_MAXVALUE, maxDamage);
	condition->setParam(CONDITIONPARAM_STARTVALUE, startDamage);
	condition->setParam(CONDITIONPARAM_DELAYED, 1);

	return condition;
}

bool Monsters::deserializeSpell(xmlNodePtr node, spellBlock_t& sb)
{
	sb.chance = 100;
	sb.speed = 2000;
	sb.range = 0;
	sb.minCombatValue = 0;
	sb.maxCombatValue = 0;
	sb.combatSpell = false;

	std::string name = "";
	if(!readXMLString(node, "name", name)){
		return false;
	}

	int intValue;
	std::string strValue;
	if(readXMLInteger(node, "speed", intValue) || readXMLInteger(node, "interval", intValue)){
		sb.speed = std::max(1, intValue);
	}

	if(readXMLInteger(node, "chance", intValue)){
		if(intValue < 0 || intValue > 100){
			intValue = 100;
		}

		sb.chance = intValue;
	}

	if(readXMLInteger(node, "range", intValue)){
		if(intValue < 0 ){
			intValue = 0;
		}

		if(intValue > Map::maxViewportX * 2){
			intValue = Map::maxViewportX * 2;
		}

		sb.range = intValue;
	}

	if(readXMLInteger(node, "min", intValue)){
		sb.minCombatValue = intValue;
	}

	if(readXMLInteger(node, "max", intValue)){
		sb.maxCombatValue = intValue;

		//normalize values
		if(std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue)){
			int32_t value = sb.maxCombatValue;
			sb.maxCombatValue = sb.minCombatValue;
			sb.minCombatValue = value;
		}
	}

	if(sb.spell = g_spells->getSpellByName(name)){
		return true;
	}

	Combat* combat = new Combat;
	bool needTarget = false;
	bool needDirection = false;

	if(readXMLInteger(node, "length", intValue)){
		int32_t length = intValue;

		if(length > 0){
			int32_t spread = 3;

			//need direction spell
			if(readXMLInteger(node, "spread", intValue)){
				spread = std::max(0, intValue);
			}

			AreaCombat* area = new AreaCombat();
			area->setupArea(length, spread);
			combat->setArea(area);

			needDirection = true;
		}
	}

	if(readXMLInteger(node, "radius", intValue)){
		int32_t radius = intValue;

		//target spell
		if(readXMLInteger(node, "target", intValue)){
			needTarget = (intValue != 0);
		}

		AreaCombat* area = new AreaCombat();
		area->setupArea(radius);
		combat->setArea(area);
	}

	if(name == "melee"){
		int attack = 0;
		int skill = 0;
		if(readXMLInteger(node, "attack", attack)){
			if(readXMLInteger(node, "skill", skill)){
				sb.minCombatValue = 0;
				sb.maxCombatValue = -Weapons::getMaxWeaponDamage(skill, attack);
			}
		}
		
		ConditionType_t conditionType = CONDITION_NONE;
		int32_t minDamage = 0;
		int32_t maxDamage = 0;
		int32_t startDamage = 0;

		if(readXMLInteger(node, "fire", intValue)){
			conditionType = CONDITION_FIRE;

			minDamage = intValue;
			maxDamage = intValue;
		}
		else if(readXMLInteger(node, "poison", intValue)){
			conditionType = CONDITION_POISON;

			minDamage = intValue;
			maxDamage = intValue;
		}
		else if(readXMLInteger(node, "energy", intValue)){
			conditionType = CONDITION_ENERGY;

			minDamage = intValue;
			maxDamage = intValue;
		}

		if(conditionType != CONDITION_NONE){
			Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage);
			combat->setCondition(condition);
		}

		sb.range = 1;
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
		combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
		combat->setParam(COMBATPARAM_BLOCKEDBYSHIELD, 1);
	}
	else if(name == "physical"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
		combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
	}
	else if(name == "poison"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_POISONDAMAGE);
	}
	else if(name == "fire"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FIREDAMAGE);
	}
	else if(name == "energy"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ENERGYDAMAGE);
	}
	else if(name == "lifedrain"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_LIFEDRAIN);
	}
	else if(name == "manadrain"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_MANADRAIN);
	}
	else if(name == "healing"){
		combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HEALING);
		combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
	}
	else if(name == "speed"){
		int32_t speedChange = 0;
		int32_t duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			duration = intValue;
		}

		if(readXMLInteger(node, "speedchange", intValue)){
			speedChange = intValue;

			if(speedChange < -1000){
				//cant be slower than 100%
				speedChange = -1000;
			}
		}

		ConditionType_t conditionType;
		if(speedChange > 0){
			conditionType = CONDITION_HASTE;
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
		}
		else{
			conditionType = CONDITION_PARALYZE;
		}

		ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, duration, 0));
		condition->setFormulaVars(speedChange / 1000.0, 0, speedChange / 1000.0, 0);
		combat->setCondition(condition);
	}
	else if(name == "outfit"){
		int32_t duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			duration = intValue;
		}

		if(readXMLString(node, "monster", strValue)){
			MonsterType* mType = g_monsters.getMonsterType(strValue);
			if(mType){
				ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
				condition->addOutfit(mType->outfit);
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
				combat->setCondition(condition);
			}
		}
		else if(readXMLInteger(node, "item", intValue)){
			Outfit_t outfit;
			outfit.lookTypeEx = intValue;

			ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
			condition->addOutfit(outfit);
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
			combat->setCondition(condition);
		}
	}
	else if(name == "invisible"){
		int32_t duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			duration = intValue;
		}

		Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration, 0);
		combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
		combat->setCondition(condition);
	}
	else if(name == "drunk"){
		int32_t duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			duration = intValue;
		}

		Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration, 0);
		combat->setCondition(condition);
	}
	else if(name == "firefield"){
		combat->setParam(COMBATPARAM_CREATEITEM, 1487);
	}
	else if(name == "poisonfield"){
		combat->setParam(COMBATPARAM_CREATEITEM, 1490);
	}
	else if(name == "energyfield"){
		combat->setParam(COMBATPARAM_CREATEITEM, 1491);
	}
	else if(name == "firecondition" || name == "poisoncondition" || name == "energycondition"){
		ConditionType_t conditionType;

		if(name == "firecondition"){
			conditionType = CONDITION_FIRE;
		}
		else if(name == "poisoncondition"){
			conditionType = CONDITION_POISON;
		}
		else{
			conditionType = CONDITION_ENERGY;
		}

		int32_t minDamage = std::abs(sb.minCombatValue);
		int32_t maxDamage = std::abs(sb.maxCombatValue);
		int32_t startDamage = 0;

		if(readXMLInteger(node, "start", intValue)){
			intValue = std::abs(intValue);

			if(intValue <= minDamage){
				startDamage = intValue;
			}
		}

		Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage);
		combat->setCondition(condition);
	}
	else{
		delete combat;
		return false;
	}

	xmlNodePtr attributeNode = node->children;

	while(attributeNode){
		if(readXMLString(attributeNode, "key", strValue)){
			if(strcasecmp(strValue.c_str(), "shootEffect") == 0){
				if(readXMLString(attributeNode, "value", strValue)){
					if(strValue == "spear"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_SPEAR);
					}
					if(strValue == "bolt"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_BOLT);
					}
					if(strValue == "arrow"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_ARROW);
					}
					if(strValue == "fire"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_FIRE);
					}
					if(strValue == "energy"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_ENERGY);
					}
					if(strValue == "poisonarrow"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_POISONARROW);
					}
					if(strValue == "burstarrow"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_BURSTARROW);
					}
					if(strValue == "throwingstar"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_THROWINGSTAR);
					}
					if(strValue == "throwingknife"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_THROWINGKNIFE);
					}
					if(strValue == "smallstone"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_SMALLSTONE);
					}
					if(strValue == "suddendeath"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_SUDDENDEATH);
					}
					if(strValue == "largerock"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_LARGEROCK);
					}
					if(strValue == "snowball"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_SNOWBALL);
					}
					if(strValue == "powerbolt"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_POWERBOLT);
					}
					if(strValue == "poison"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_FLYPOISONFIELD);
					}
					if(strValue == "infernalbolt"){
						combat->setParam(COMBATPARAM_DISTANCEEFFECT, NM_ANI_INFERNALBOLT);
					}
				}
			}
			else if(strcasecmp(strValue.c_str(), "areaEffect") == 0){
				if(readXMLString(attributeNode, "value", strValue)){
					if(strValue == "redspark"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_DRAW_BLOOD);
					}
					else if(strValue == "bluebubble"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_LOSE_ENERGY);
					}
					else if(strValue == "poff"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_PUFF);
					}
					else if(strValue == "yellowspark"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_BLOCKHIT);
					}
					else if(strValue == "explosion"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_EXPLOSION_AREA);
					}
					else if(strValue == "explosionarea"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_EXPLOSION_DAMAGE);
					}
					else if(strValue == "firearea"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_FIRE_AREA);
					}
					else if(strValue == "yellowbubble"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_YELLOW_RINGS);
					}
					else if(strValue == "blackspark"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_HIT_AREA);
					}
					else if(strValue == "energyarea"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_ENERGY_AREA);
					}
					else if(strValue == "energy"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_ENERGY_DAMAGE);
					}
					else if(strValue == "blueshimmer"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_MAGIC_ENERGY);
					}
					else if(strValue == "redshimmer"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_MAGIC_BLOOD);
					}
					else if(strValue == "greenshimmer"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_MAGIC_POISON);
					}
					else if(strValue == "yellowspark"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_HITBY_FIRE);
					}
					else if(strValue == "greenspark"){    
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_POISON);
					}
					else if(strValue == "mortarea"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_MORT_AREA);
					}
					else if(strValue == "greennote"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_SOUND_GREEN);
					}
					else if(strValue == "rednote"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_SOUND_RED);
					}
					else if(strValue == "poison"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_POISON_AREA);
					}
					else if(strValue == "yellownote"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_SOUND_YELLOW);
					}
					else if(strValue == "purplenote"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_SOUND_PURPLE);
					}
					else if(strValue == "bluenote"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_SOUND_BLUE);
					}
					else if(strValue == "whitenote"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_SOUND_WHITE);
					}
					else if(strValue == "bubbles"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_BUBBLES);
					}
					else if(strValue == "bubblearea"){
						combat->setParam(COMBATPARAM_EFFECT, NM_ME_CRAPS);
					}
				}
			}
		}

		attributeNode = attributeNode->next;
	}

	sb.spell = new CombatSpell(combat, needTarget, needDirection);
	sb.combatSpell = true;
	return true;
}

MonsterType* Monsters::loadMonster(const std::string& file,const std::string& monster_name, bool reloading /*= false*/)
{
	bool monsterLoad;
	MonsterType* mType = NULL;
	bool new_mType = true;
	
	if(reloading){
		unsigned long id = getIdByName(monster_name);
		if(id != 0){
			mType = getMonsterType(id);
			if(mType != NULL){
				new_mType = false;
				mType->reset();
			}
		}
	}
	if(new_mType){
		mType = new MonsterType();
	}
	
	monsterLoad = true;
	xmlDocPtr doc = xmlParseFile(file.c_str());

	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monster") != 0){
			std::cerr << "Malformed XML: " << file << std::endl;
		}

		int intValue;
		std::string strValue;

		p = root->children;

		if(readXMLString(root, "name", strValue)){
			mType->name = strValue;
		}
		else
			monsterLoad = false;

		if(readXMLString(root, "nameDescription", strValue)){
			mType->nameDescription = strValue;
		}
		else{
			mType->nameDescription = "a " + mType->name;
			toLowerCaseString(mType->nameDescription);
		}

		if(readXMLString(root, "race", strValue)){
			if((strcasecmp(strValue.c_str(), "venom") == 0) || (atoi(strValue.c_str()) == 1)){
				mType->race = RACE_VENOM;
			}
			else if((strcasecmp(strValue.c_str(), "blood") == 0) || (atoi(strValue.c_str()) == 2)){
				mType->race = RACE_BLOOD;
			}
			else if((strcasecmp(strValue.c_str(), "undead") == 0) || (atoi(strValue.c_str()) == 3)){
				mType->race = RACE_UNDEAD;
			}
			else if((strcasecmp(strValue.c_str(), "fire") == 0) || (atoi(strValue.c_str()) == 4)){
				mType->race = RACE_FIRE;
			}
		}

		if(readXMLInteger(root, "experience", intValue)){
			mType->experience = intValue;
		}

		if(readXMLInteger(root, "speed", intValue)){
			mType->base_speed = intValue;
		}

		if(readXMLInteger(root, "manacost", intValue)){
			mType->manaCost = intValue;
		}

		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0){

				if(readXMLInteger(p, "now", intValue)){
					mType->health = intValue;
				}
				else
					monsterLoad = false;

				if(readXMLInteger(p, "max", intValue)){
					mType->health_max = intValue;
				}
				else
					monsterLoad = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"flags") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"flag") == 0){

						if(readXMLInteger(tmpNode, "summonable", intValue)){
							mType->isSummonable = (intValue != 0);
						}
						
						if(readXMLInteger(tmpNode, "attackable", intValue)){
							mType->isAttackable = (intValue != 0);
						}
						
						if(readXMLInteger(tmpNode, "hostile", intValue)){
							mType->isHostile = (intValue != 0);
						}		

						if(readXMLInteger(tmpNode, "illusionable", intValue)){
							mType->isIllusionable = (intValue != 0);
						}
						
						if(readXMLInteger(tmpNode, "convinceable", intValue)){
							mType->isConvinceable = (intValue != 0);
						}

						if(readXMLInteger(tmpNode, "pushable", intValue)){
							mType->pushable = (intValue != 0);
						}

						if(readXMLInteger(tmpNode, "canpushitems", intValue)){
							mType->canPushItems = (intValue != 0);
						}

						if(readXMLInteger(tmpNode, "staticattack", intValue)){
							if(intValue < 0){
								intValue = 0;
							}

							if(intValue > 100){
								intValue = 100;
							}
							
							mType->staticAttackChance = intValue;
						}

						if(readXMLInteger(tmpNode, "lightlevel", intValue)){
							mType->lightLevel = intValue;
						}

						if(readXMLInteger(tmpNode, "lightcolor", intValue)){
							mType->lightColor = intValue;
						}

						if(readXMLInteger(tmpNode, "targetdistance", intValue)){
							mType->targetDistance = std::max(1, intValue);
						}

						if(readXMLInteger(tmpNode, "runonhealth", intValue)){
							mType->runAwayHealth = intValue;
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"targetchange") == 0){

				if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue)){
					mType->changeTargetSpeed = std::max(1, intValue);
				}

				if(readXMLInteger(p, "chance", intValue)){
					mType->changeTargetChance = intValue;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"strategy") == 0){

				if(readXMLInteger(p, "attack", intValue)){
					mType->attackStrength = intValue;
				}

				if(readXMLInteger(p, "defense", intValue)){
					mType->defenseStrength = intValue;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){

				if(readXMLInteger(p, "type", intValue)){
					mType->outfit.lookType = intValue;

					if(readXMLInteger(p, "head", intValue)){
						mType->outfit.lookHead = intValue;
					}

					if(readXMLInteger(p, "body", intValue)){
						mType->outfit.lookBody = intValue;
					}

					if(readXMLInteger(p, "legs", intValue)){
						mType->outfit.lookLegs = intValue;
					}

					if(readXMLInteger(p, "feet", intValue)){
						mType->outfit.lookFeet = intValue;
					}
					
					if(readXMLInteger(p, "addons", intValue)){
						mType->outfit.lookAddons = intValue;
					}
				}
				else if(readXMLInteger(p, "typeex", intValue)){
					mType->outfit.lookTypeEx = intValue;
				}

				if(readXMLInteger(p, "corpse", intValue)){
					mType->lookcorpse = intValue;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"attacks") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"attack") == 0){

						spellBlock_t sb;
						if(deserializeSpell(tmpNode, sb)){
							mType->spellAttackList.push_back(sb);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"defenses") == 0){
				if(readXMLInteger(p, "defense", intValue)){
					mType->defense = intValue;
				}

				if(readXMLInteger(p, "armor", intValue)){
					mType->armor = intValue;
				}

				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"defense") == 0){
						
						spellBlock_t sb;
						if(deserializeSpell(tmpNode, sb)){
							mType->spellDefenseList.push_back(sb);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"immunities") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"immunity") == 0){

						if(readXMLString(tmpNode, "name", strValue)){

							if(strcasecmp(strValue.c_str(), "physical") == 0){
								mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
								//mType->conditionImmunities |= CONDITION_PHYSICAL;
							}
							else if(strcasecmp(strValue.c_str(), "energy") == 0){
								mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
								mType->conditionImmunities |= CONDITION_ENERGY;
							}
							else if(strcasecmp(strValue.c_str(), "fire") == 0){
								mType->damageImmunities |= COMBAT_FIREDAMAGE;
								mType->conditionImmunities |= CONDITION_FIRE;
							}
							else if(strcasecmp(strValue.c_str(), "poison") == 0){
								mType->damageImmunities |= COMBAT_POISONDAMAGE;
								mType->conditionImmunities |= CONDITION_POISON;
							}
							else if(strcasecmp(strValue.c_str(), "lifedrain") == 0){
								mType->damageImmunities |= COMBAT_LIFEDRAIN;
								mType->conditionImmunities |= CONDITION_LIFEDRAIN;
							}
							else if(strcasecmp(strValue.c_str(), "paralyze") == 0){
								mType->conditionImmunities |= CONDITION_PARALYZE;
							}
							else if(strcasecmp(strValue.c_str(), "outfit") == 0){
								mType->conditionImmunities |= CONDITION_OUTFIT;
							}
							else if(strcasecmp(strValue.c_str(), "drunk") == 0){
								mType->conditionImmunities |= CONDITION_DRUNK;
							}
							else if(strcasecmp(strValue.c_str(), "invisible") == 0){
								mType->conditionImmunities |= CONDITION_INVISIBLE;
							}
						}
						else if(readXMLInteger(tmpNode, "physical", intValue)){
							if(intValue != 0){
								mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
								//mType->conditionImmunities |= CONDITION_PHYSICAL;
							}
						}
						else if(readXMLInteger(tmpNode, "energy", intValue)){
							if(intValue != 0){
								mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
								mType->conditionImmunities |= CONDITION_ENERGY;
							}
						}
						else if(readXMLInteger(tmpNode, "fire", intValue)){
							if(intValue != 0){
								mType->damageImmunities |= COMBAT_FIREDAMAGE;
								mType->conditionImmunities |= CONDITION_FIRE;
							}
						}
						else if(readXMLInteger(tmpNode, "poison", intValue)){
							if(intValue != 0){
								mType->damageImmunities |= COMBAT_POISONDAMAGE;
								mType->conditionImmunities |= CONDITION_POISON;
							}
						}
						else if(readXMLInteger(tmpNode, "lifedrain", intValue)){
							if(intValue != 0){
								mType->damageImmunities |= COMBAT_LIFEDRAIN;
								mType->conditionImmunities |= CONDITION_LIFEDRAIN;
							}
						}
						else if(readXMLInteger(tmpNode, "paralyze", intValue)){
							if(intValue != 0){
								mType->conditionImmunities |= CONDITION_PARALYZE;
							}
						}
						else if(readXMLInteger(tmpNode, "outfit", intValue)){
							if(intValue != 0){
								mType->conditionImmunities |= CONDITION_OUTFIT;
							}
						}
						else if(readXMLInteger(tmpNode, "drunk", intValue)){
							if(intValue != 0){
								mType->conditionImmunities |= CONDITION_DRUNK;
							}
						}
						else if(readXMLInteger(tmpNode, "invisible", intValue)){
							if(intValue != 0){
								mType->conditionImmunities |= CONDITION_INVISIBLE;
							}
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"voices") == 0){
				xmlNodePtr tmpNode = p->children;
				
				if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue)){
					mType->yellSpeedTicks = intValue;
				}

				if(readXMLInteger(p, "chance", intValue)){
					mType->yellChance = intValue;
				}

				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"voice") == 0){

						voiceBlock_t vb;
						vb.text = "";
						vb.yellText = false;

						if(readXMLString(tmpNode, "sentence", strValue)){
							vb.text = strValue;
						}

						if(readXMLInteger(tmpNode, "yell", intValue)){
							vb.yellText = (intValue != 0);
						}

						mType->voiceVector.push_back(vb);
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"loot") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					LootBlock lootBlock;
					if(loadLootItem(tmpNode, lootBlock)){
						mType->lootItems.push_back(lootBlock);
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"summons") == 0){

				if(readXMLInteger(p, "maxSummons", intValue) || readXMLInteger(p, "max", intValue)){
					mType->maxSummons = std::min(intValue, 100);
				}

				xmlNodePtr tmpNode = p->children;
				while(tmpNode){

					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"summon") == 0){
						int32_t chance = 100;
						int32_t speed = 1000;

						if(readXMLInteger(tmpNode, "speed", intValue) || readXMLInteger(tmpNode, "interval", intValue)){
							speed = intValue;
						}

						if(readXMLInteger(tmpNode, "chance", intValue)){
							chance = intValue;
						}

						if(readXMLString(tmpNode, "name", strValue)){
							summonBlock_t sb;
							sb.name = strValue;
							sb.speed = speed;
							sb.chance = chance;							

							mType->summonList.push_back(sb);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	else{
		monsterLoad = false;
	}

	if(monsterLoad){
		return mType;
	}
	else{
		delete mType;
		return NULL;
	}
}

bool Monsters::loadLootItem(xmlNodePtr node, LootBlock& lootBlock)
{
	int intValue;
	std::string strValue;

	if(readXMLInteger(node, "id", intValue)){
		lootBlock.id = intValue;
	}
	
	if(lootBlock.id == 0){
		return false;
	}
	
	if(readXMLInteger(node, "countmax", intValue)){
		lootBlock.countmax = intValue;

		if(lootBlock.countmax > 100){
			lootBlock.countmax = 100;
		}
	}
	else{
		//std::cout << "missing countmax for loot id = "<< lootBlock.id << std::endl;
		lootBlock.countmax = 1;
	}

	if(readXMLInteger(node, "chance", intValue) || readXMLInteger(node, "chance1", intValue)){
		lootBlock.chance = intValue;

		if(lootBlock.chance > MAX_LOOTCHANCE){
			lootBlock.chance = MAX_LOOTCHANCE;
		}
	}
	else{
		//std::cout << "missing chance for loot id = "<< lootBlock.id << std::endl;
		lootBlock.chance = MAX_LOOTCHANCE;
	}

	if(Item::items[lootBlock.id].isContainer()){
		loadLootContainer(node, lootBlock);
	}

	//optional
	if(readXMLInteger(node, "subtype", intValue)){
		lootBlock.subType = intValue;
	}

	if(readXMLInteger(node, "actionId", intValue)){
		lootBlock.actionId = intValue;
	}

	if(readXMLString(node, "text", strValue)){
		lootBlock.text = strValue;
	}

	return true;
}

bool Monsters::loadLootContainer(xmlNodePtr node, LootBlock& lBlock)
{
	if(node == NULL){
		return false;
	}
	
	xmlNodePtr tmpNode = node->children;
	xmlNodePtr p;

	if(tmpNode == NULL){
		return false;
	}

	while(tmpNode){
		if(xmlStrcmp(tmpNode->name, (const xmlChar*)"inside") == 0){
			p = tmpNode->children;
			while(p){
				LootBlock lootBlock;
				if(loadLootItem(p, lootBlock)){
					lBlock.childLoot.push_back(lootBlock);
				}
				p = p->next;
			}
			return true;
		}//inside

		tmpNode = tmpNode->next;
	}

	return false;	
}

MonsterType* Monsters::getMonsterType(const std::string& name)
{
	uint32_t mId = getIdByName(name);
	if(mId == 0){
		return NULL;
	}

	return getMonsterType(mId);
}

MonsterType* Monsters::getMonsterType(unsigned long mid)
{
	MonsterMap::iterator it = monsters.find(mid);
	if(it != monsters.end()){
		return it->second;
	}
	else{
		return NULL;
	}
}

unsigned long Monsters::getIdByName(const std::string& name)
{
	std::string lower_name = name;
	std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), tolower);
	MonsterNameMap::iterator it = monsterNames.find(lower_name);
	if(it != monsterNames.end()){
		return it->second;
	}
	else{
		return 0;
	}
}

Monsters::~Monsters()
{
	for(MonsterMap::iterator it = monsters.begin(); it != monsters.end(); it++)
		delete it->second;
}
