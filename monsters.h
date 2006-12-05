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

#ifndef __OTSERV_MONSTERS_H__
#define __OTSERV_MONSTERS_H__

#include <string>
#include "creature.h"

#define CHANCE_MAX  100000
struct LootBlock{
	unsigned short id;
	unsigned short countmax;
	unsigned long chance1;
	unsigned long chancemax;
	typedef std::list<LootBlock> LootItems;
	LootItems childLoot;
	LootBlock(){
		id = 0;
		countmax = 0;
		chance1 = 0;
		chancemax = 0;
	}
};	

struct summonBlock_t{
	std::string name;
	uint32_t chance;
	uint32_t speed;
};

class Spell;

struct spellBlock_t{
	Spell* spell;
	uint32_t chance;
	uint32_t speed;
	uint32_t range;
	int32_t minCombatValue;
	int32_t maxCombatValue;
};

typedef std::list<LootBlock> LootItems;
typedef std::list<summonBlock_t> SummonList;
typedef std::list<spellBlock_t> SpellList;
typedef std::vector<std::string> VoiceVector;

class MonsterType{
public:
	MonsterType();
	~MonsterType();
	
	void reset();
	
	std::string name;
	std::string nameDescription;
	int experience;

	//int attackSkill;
	//int attackValue;
	int attackPower;

	int defenseSkill;
	int defense;
	int armor;

	bool canPushItems;
	unsigned long staticAttack;
	int maxSummons;
	int targetDistance;
	int runAwayHealth;
	bool pushable;
	int base_speed;
	int health;
	int health_max;
	
	Outfit_t outfit;
	int32_t lookcorpse;
	int conditionImmunities;
	int damageImmunities;
	RaceType_t race;
	bool isSummonable;
	bool isIllusionable;
	bool isConvinceable;
	bool isAttackable;
	bool isHostile;
	
	int lightLevel;
	int lightColor;
		
	uint32_t manaCost;
	SummonList summonList;
	LootItems lootItems;
	SpellList spellAttackList;
	SpellList spellDefenseList;

	int32_t combatMeleeMin;
	int32_t combatMeleeMax;
	uint32_t combatMeleeSpeed;

	uint32_t yellChance;
	uint32_t yellSpeedTicks;
	VoiceVector voiceVector;

	int32_t changeTargetSpeed;
	int32_t changeTargetChance;

	void createLoot(Container* corpse);
	void createLootContainer(Container* parent, const LootBlock& lootblock);
	Item* createLootItem(const LootBlock& lootblock);
};

class Monsters{
public:
	Monsters();
	~Monsters();
	
	bool loadFromXml(const std::string& _datadir, bool reloading = false);
	bool isLoaded(){return loaded;}	
	bool reload();
	
	MonsterType* getMonsterType(unsigned long mid);
	unsigned long getIdByName(const std::string& name);

	static uint32_t getLootRandom();
	
private:
	MonsterType* loadMonster(const std::string& file, const std::string& monster_name, bool reloading = false);

	bool loadLootContainer(xmlNodePtr, LootBlock&);
	bool loadLootItem(xmlNodePtr, LootBlock&);

	typedef std::map<std::string, unsigned long> MonsterNameMap;
	MonsterNameMap monsterNames;
	
	typedef std::map<unsigned long, MonsterType*> MonsterMap;
	MonsterMap monsters;
	
	bool loaded;
	std::string datadir;
		
};

#endif
