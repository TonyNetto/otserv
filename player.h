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

#ifndef __player_h_
#define __player_h_


#include "creature.h"
#include "container.h"
#include <vector>
#include <algorithm>

class Protocol;

enum slots_t {
	SLOT_WHEREEVER=0,
	SLOT_HEAD=1,
	SLOT_NECKLACE=2,
	SLOT_BACKPACK=3,
	SLOT_ARMOR=4,
	SLOT_RIGHT=5,
	SLOT_LEFT=6,
	SLOT_LEGS=7,
	SLOT_FEET=8,
	SLOT_RING=9,
	SLOT_AMMO=10
};

enum skills_t {
    SKILL_FIST,
    SKILL_CLUB,
    SKILL_SWORD,
    SKILL_AXE,
    SKILL_DIST,
    SKILL_SHIELD,
    SKILL_FISH
};

enum skillsid_t {
    SKILL_LEVEL,
    SKILL_TRIES
};

class NetworkMessage;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature
{
public:
	Player(const char *name, Protocol* p);
	virtual ~Player();

  void speak(const std::string &text);

	int addItem(Item* item, int pos);
	unsigned int getContainerCount() {return (uint32_t)vcontainers.size();}; //returns the current number of containers open
	Container* getContainer(unsigned char containerid);
	unsigned char getContainerID(Container* container);
	void addContainer(unsigned char containerid, Container *container);
	void closeContainer(unsigned char containerid);
	int sendInventory();

	Item* getItem(int pos);

	std::string getName(){return name;};
	
  int sex, voc;
  int cap;
  int food;
  bool cancelMove;
  virtual int getWeaponDamage() const;
  char fightMode, followMode;
  int accountNumber;
  
  std::string password;

  unsigned int skills[7][2];
  
  //reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
  unsigned short CapGain[5];          //for level advances
  unsigned short ManaGain[5];
  unsigned short HPGain[5];
  
  //for skill advances
  unsigned int getReqSkilltries (int skill, int level, int voc);
  
  //for magic level advances
  unsigned int getReqMana(int maglevel, int voc); 
  

  //items
  Item* items[11]; //equipement of the player
	typedef std::pair<unsigned char, Container*> containerItem;
	typedef std::vector<containerItem> containerLayout;
	containerLayout vcontainers;

  void    usePlayer() { useCount++; };
  void    releasePlayer() { useCount--; if (useCount == 0) delete this; };
	unsigned long getIP() const;
	//void kickPlayer(bool banIP = false);
	//const SOCKET getSocket() const;

  fight_t getFightType();
	subfight_t getSubFightType();

  void sendIcons();
  bool CanSee(int x, int y, int z) const;
  void addSkillTry(int skilltry);
  void sendNetworkMessage(NetworkMessage *msg);
  void sendCancelAttacking();
  void sendChangeSpeed(Creature* creature);
  void savePlayer(std::string &name);
  void sendToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId);
  void die();      //player loses exp/skills/maglevel on death

  
  virtual void sendCancel(const char *msg);
  virtual void sendCancelWalk(const char *msg);
  virtual void setAttackedCreature(unsigned long id);
  virtual bool isAttackable() { return (access == 0); };

protected:
  int useCount;

  virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  virtual void onCreatureChangeOutfit(const Creature* creature);
  virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos); 
  virtual int onThink(int& newThinkTicks);
  virtual std::string getDescription(bool self = false) const;
	virtual void onTileUpdated(const Position &pos);
	virtual void onContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool remove);
	Protocol *client;

	// we need our name
	std::string name;
};


#endif // __player_h_
