//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class to be implemented by each protocoll to use
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


#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__


#include "definitions.h"
#include "map.h"
#include "texcept.h"

#include <string>

#include "player.h"


// base class to represent different protokolls...
class Protocol
{
public:
  Protocol();
  virtual ~Protocol();

	virtual int sendInventory(){return 0;}

  void setPlayer(Player* p);


  virtual bool CanSee(int x, int y) = 0;

  virtual void sendNetworkMessage(NetworkMessage *msg) = 0;

  virtual void sendThingMove(const Player *player, const Thing *thing, const Position *oldPos, unsigned char oldStackPos) = 0;
  virtual void sendCreatureAppear(const Creature *creature) = 0;
  virtual void sendCreatureDisappear(const Creature *creature, unsigned char stackPos) = 0;
  virtual void sendCreatureTurn(const Creature *creature, unsigned char stackPos) = 0;
  virtual void sendCreatureSay(const Creature *creature, unsigned char type, const string &text) = 0;
  virtual void sendSetOutfit(const Creature* creature) = 0;

protected:
  Map    *map;
  Player *player;
};


#endif  // #ifndef __PROTOCOL_H__
