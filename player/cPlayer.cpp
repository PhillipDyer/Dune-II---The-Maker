/*

  Dune II - The Maker

  Author : Stefan Hendriks
  Contact: stefanhen83@gmail.com
  Website: http://d2tm.duneii.com

  2001 - 2009 (c) code by Stefan Hendriks

  */

#include "..\d2tmh.h"

cPlayer::cPlayer() {
	itemBuilder = NULL;
}

cPlayer::~cPlayer() {
	if (itemBuilder) {
		delete itemBuilder;
	}
}

void cPlayer::setId(int theId) {
	assert(theId >= HUMAN);
	assert(theId <= MAX_PLAYERS);
	assert(&player[theId] == this); // check if the reference in the array is the same!
	id = theId;
}

void cPlayer::setSideBar(cSideBar *theSideBar) {
	assert(theSideBar);

	// delete old reference
	if (sidebar) {
		delete sidebar;
	}

	sidebar = theSideBar;
}

void cPlayer::setItemBuilder(cItemBuilder *theItemBuilder) {
	assert(theItemBuilder);

	// delete old reference
	if (itemBuilder) {
		delete itemBuilder;
	}

	itemBuilder = theItemBuilder;
}

void cPlayer::setStructurePlacer(cStructurePlacer * theStructurePlacer) {
	assert(theStructurePlacer);

	if (structurePlacer) {
		delete structurePlacer;
	}

	structurePlacer = theStructurePlacer;
}

void cPlayer::setBuildingListUpdater(cBuildingListUpdater *theBuildingListUpgrader) {
	assert(theBuildingListUpgrader);

	// delete old reference
	if (buildingListUpgrader) {
		delete buildingListUpgrader;
	}

	buildingListUpgrader = theBuildingListUpgrader;
}

void cPlayer::init()
{
	memcpy(pal, general_palette, sizeof(pal));
	house = GENERALHOUSE;

	difficultySettings = new cPlayerAtreidesDifficultySettings();

	// Reset structures amount
	memset(iStructures, 0, sizeof(iStructures));
	memset(iPrimaryBuilding, -1, sizeof(iPrimaryBuilding));

	credits	=	0;
	max_credits	=	credits;
	focus_cell	=	0;

	use_power=0;
	has_power=0;

    iTeam=-1;

    TIMER_think=rnd(10);        // timer for thinking itself (calling main routine)
    TIMER_attack=-1;			// -1 = determine if its ok to attack, > 0 is , decrease timer, 0 = attack

}


// set house
void cPlayer::setHouse(int iHouse)
{

  house = iHouse;      // use rules of this house

  // copy entire palette
  memcpy (pal, general_palette, sizeof(pal));

  // now set the different colors based uppon house
  if (houses[house].swap_color > -1) {
    int start = houses[house].swap_color;
    int s=144;                // original position (harkonnen)
    for (int j = start; j < (start+7);j++) {
      // swap everything from S with J
       pal[s] = pal[j];
       s++;
     }
  }

}

bool cPlayer::bEnoughPower()
{
    if (game.bSkirmish) {
       if (has_power >= use_power) return true;
    } else {
        int iMyID=-1;

		for (int i=0; i < MAX_PLAYERS; i++) {
            if (&player[i] == this)
            {
                iMyID=i;
                break;
            }
		}

        // AI cheats on power
        if (iMyID > 0) {
            // original dune 2 , the AI cheats. Unfortunatly D2TM has to cheat too, else the game will
            // be unplayable.
            if (iStructures[WINDTRAP] > 0) {
                // always enough power so it seems
                return true;
			} else {
                return false; // not enough power
			}
        }
        else
        {
            if (has_power >= use_power)
                return true;
        }
    }


    // return false on any other case
    return false;
}