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


#include "definitions.h"

#include <string>
#include <iostream>

using namespace std;

#include <stdlib.h>

#include "protocol.h"
#include "player.h"



Player::Player(const char *name, int _sex, Protocol *p) : Creature(name)
{
  client     = p;

	sex        = _sex;
	looktype = sex?PLAYER_MALE_1:PLAYER_FEMALE_1;

	voc        = 0;

  cap        = 300;

  mana       = 250;
  manamax    = 250;

  level      = 1;
  experience = 3000;

  maglevel   = 50;

  access     = 0;

  for(int i = 0; i < 7; i++)
  {
    skills[i][SKILL_LEVEL] = 1;
    skills[i][SKILL_TRIES] = 0;
  }

  attackedCreature = 0;

  useCount = 0;
} 


Player::~Player()
{
  delete client;
}


Item* Player::getItem(int pos)
{
//		if(pos>0 && pos <11)
//			return player.items[pos];
		return NULL;
}


int Player::sendInventory(){
	client->sendInventory();
	return true;
}


int Player::addItem(Item* item, int pos){
//		std::cout << "Should add item at " << pos <<std::endl;
//		if(pos>0 && pos <11)
//			player.items[pos]=item;
//		client->sendInventory();
	return true;
}


void Player::setAttackedCreature(unsigned long id)
{
  attackedCreature = id;
}


bool Player::CanSee(int x, int y)
{
  return client->CanSee(x, y);
}


void Player::sendNetworkMessage(NetworkMessage *msg)
{
  client->sendNetworkMessage(msg);
};


void Player::onThingMove(const Player *player, const Thing *thing, const Position *oldPos, unsigned char oldstackpos)
{
  client->sendThingMove(player, thing, oldPos, oldstackpos);
}


void Player::onCreatureAppear(const Creature *creature)
{
  client->sendCreatureAppear(creature);
}


void Player::onCreatureDisappear(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureDisappear(creature, stackPos);
}


void Player::onCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureTurn(creature, stackPos);
}


void Player::onCreatureSay(const Creature *creature, unsigned char type, const string &text)
{
  client->sendCreatureSay(creature, type, text);
}

void Player::onCreatureChangeOutfit(const Creature* creature) {
		  client->sendSetOutfit(creature);
}
