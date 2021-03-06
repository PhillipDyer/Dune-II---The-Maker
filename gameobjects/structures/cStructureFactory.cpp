#include "../../include/d2tmh.h"

cStructureFactory *cStructureFactory::instance = NULL;

cStructureFactory::cStructureFactory() {
	cellCalculator = new cCellCalculator(&map);
}

cStructureFactory::~cStructureFactory() {
	delete cellCalculator;
}

cStructureFactory *cStructureFactory::getInstance() {
	if (instance == NULL) {
		instance = new cStructureFactory();
	}
	return instance;
}

cAbstractStructure *cStructureFactory::createStructureInstance(int type) {
	// Depending on type, create proper derived class. The constructor
    // will take care of the rest
    if (type == CONSTYARD)		return new cConstYard;
    if (type == STARPORT)		return new cStarPort;
    if (type == WINDTRAP)		return new cWindTrap;
    if (type == SILO)			return new cSpiceSilo;
    if (type == REFINERY)		return new cRefinery;
    if (type == RADAR)			return new cOutPost;
    if (type == PALACE)			return new cPalace;
    if (type == HIGHTECH)		return new cHighTech;
    if (type == LIGHTFACTORY)      return new cLightFactory;
    if (type == HEAVYFACTORY)      return new cHeavyFactory;
    if (type == TURRET)      return new cGunTurret;
    if (type == RTURRET)      return new cRocketTurret;
    if (type == REPAIR)      return new cRepairFacility;
    if (type == IX)      return new cIx;
    if (type == WOR)      return new cWor;
    if (type == BARRACKS)      return new cBarracks;
	return NULL;
}

void cStructureFactory::deleteStructureInstance(cAbstractStructure *structure) {
    // notify building list updater
    cPlayer * pPlayer = &player[structure->getPlayerId()];
    cBuildingListUpdater * buildingListUpdater = pPlayer->getBuildingListUpdater();
    if (buildingListUpdater) {
        buildingListUpdater->onStructureDestroyed(structure->getType());
    }

    // delete memory acquired
    delete structure;
}


/**
	Shorter version, creates structure at full health.
**/
cAbstractStructure* cStructureFactory::createStructure(int iCell, int iStructureType, int iPlayer) {
	return createStructure(iCell, iStructureType, iPlayer, 100);
}

/**
	Create a structure, place it and return a reference to this created class.

	This method will return NULL when either an error occurred, or the creation
	of a non-structure type (ie SLAB/WALL) is done.
**/
cAbstractStructure* cStructureFactory::createStructure(int iCell, int iStructureType, int iPlayer, int iPercent) {
	assert(iPlayer >= 0);
	assert(iPlayer <= MAX_PLAYERS);

	int iNewId = getFreeSlot();

	// fail
    if (iNewId < 0) {
        cLogger::getInstance()->log(LOG_INFO, COMP_STRUCTURES, "create structure", "No free slot available, returning NULL");
        return nullptr;
    }

    if (iPercent > 100) iPercent = 100;

	// When 100% of the structure is blocked, this method is never called
	// therefore we can assume that SLAB4 can be placed always partially
	// when here.
	int result = getSlabStatus(iCell, iStructureType, -1);

	// we may not place it, GUI messed up
    if (result < -1 && iStructureType != SLAB4) {
        cLogger::getInstance()->log(LOG_INFO, COMP_STRUCTURES, "create structure", "cannot create structure: slab status < -1, and type != SLAB4, returning NULL");
        return nullptr;
    }

	float fPercent = (float)iPercent/100; // divide by 100 (to make it 0.x)

    int hp = structures[iStructureType].hp;
    if (hp < 0) {
        cLogger::getInstance()->log(LOG_INFO, COMP_STRUCTURES, "create structure", "Structure to create has no hp, aborting creation.");
        return nullptr;
    }

    updatePlayerCatalogAndPlaceNonStructureTypeIfApplicable(iCell, iStructureType, iPlayer);
	clearFogForStructureType(iCell, iStructureType, 2, iPlayer);

	// SLAB and WALL is not a real structure. The terrain is manipulated
	// therefore quit here, as we won't place real structure.
    if (iStructureType == SLAB1 || iStructureType == SLAB4 || iStructureType == WALL) {
		return NULL;
	}

	cAbstractStructure *str = createStructureInstance(iStructureType);

	if (str == NULL) {
        cLogger::getInstance()->log(LOG_INFO, COMP_STRUCTURES, "create structure", "cannot create structure: createStructureInstance returned NULL");
		return NULL; // fail
	}

    // calculate actual health
    float fHealth = hp * fPercent;

    if (DEBUGGING) {
        char msg2[255];
        sprintf(msg2, "Structure with id [%d] has [%d] hp , fhealth is [%f]", iStructureType, hp, fHealth);
        logbook(msg2);
    }

    int structureSize = structures[iStructureType].bmp_width * structures[iStructureType].bmp_height;

    // assign to array
	structure[iNewId] = str;

    // Now set it up for location & player
    str->setCell(iCell);
    str->setOwner(iPlayer);
    str->setBuildingFase(1); // prebuild
    str->TIMER_prebuild = std::min(structureSize/16, 250); // prebuild timer. A structure of 64x64 will result in 256, bigger structure has longer timer
    str->TIMER_damage = rnd(1000)+100;
    str->fConcrete = (1 - fPercent);
	str->setHitPoints((int)fHealth);
    str->setFrame(rnd(1)); // random start frame (flag)
    str->setStructureId(iNewId);

	str->setWidth(structures[str->getType()].bmp_width/TILESIZE_WIDTH_PIXELS);
	str->setHeight(structures[str->getType()].bmp_height/TILESIZE_HEIGHT_PIXELS);

 	// clear fog around structure
	clearFogForStructureType(iCell, str);

	// additional forces: (UNITS)
	if (iStructureType == REFINERY) {
		REINFORCE(iPlayer, HARVESTER, iCell+2, iCell+2);
	}

    // handle update
    cPlayer * pPlayer = &player[iPlayer];
    cBuildingListUpdater * buildingListUpdater = pPlayer->getBuildingListUpdater();
    if (buildingListUpdater) {
        buildingListUpdater->onStructureCreated(iStructureType);
    }

    structureUtils.putStructureOnDimension(MAPID_STRUCTURES, str);

    return str;
}


/**
	If this is a SLAB1, SLAB4, or WALL. Make changes in terrain.
    Also updates player catalog of structure types.
**/
void cStructureFactory::updatePlayerCatalogAndPlaceNonStructureTypeIfApplicable(int iCell, int iStructureType, int iPlayer) {
    // add this structure to the array of the player (for some score management)
	player[iPlayer].increaseStructureAmount(iStructureType);

	if (iStructureType == SLAB1) {
		mapEditor.createCell(iCell, TERRAIN_SLAB, 0);
		return; // done
	}

    if (iStructureType == SLAB4) {
		if (map.occupied(iCell) == false) {
			if (map.getCellType(iCell) == TERRAIN_ROCK) {
				mapEditor.createCell(iCell, TERRAIN_SLAB, 0);
			}
		}

        int leftToCell = iCell + 1;
        if (map.occupied(leftToCell) == false) {
			if (map.getCellType(leftToCell) == TERRAIN_ROCK) {
				mapEditor.createCell(leftToCell, TERRAIN_SLAB, 0);
			}
		}

        int oneRowBelowCell = iCell + game.map_width;
        if (map.occupied(oneRowBelowCell) == false) {
			if (map.getCellType(oneRowBelowCell) == TERRAIN_ROCK) {
				mapEditor.createCell(oneRowBelowCell, TERRAIN_SLAB, 0);
			}
		}

        int rightToRowBelowCell = oneRowBelowCell + 1;
        if (map.occupied(rightToRowBelowCell) == false) {
			if (map.getCellType(rightToRowBelowCell) == TERRAIN_ROCK) {
				mapEditor.createCell(rightToRowBelowCell, TERRAIN_SLAB, 0);
			}
		}

		return;
    }

    if (iStructureType == WALL) {
    	mapEditor.createCell(iCell, TERRAIN_WALL, 0);
		// change surrounding walls here
        mapEditor.smoothAroundCell(iCell);
		return;
    }
}



/**
	Clear fog around structure, using a cAbstractStructure class.
**/
void cStructureFactory::clearFogForStructureType(int iCell, cAbstractStructure *str) {
	if (str == NULL) {
		return;
	}

	clearFogForStructureType(iCell, str->getType(), structures[str->getType()].sight, str->getOwner());
}

/**
	Clear the cells of a structure on location of iCell, the size that is cleared is
	determined from the iStructureType
**/
void cStructureFactory::clearFogForStructureType(int iCell, int iStructureType, int iSight, int iPlayer) {
	int iWidth = structures[iStructureType].bmp_width / 32;;
	int iHeight = structures[iStructureType].bmp_height / 32;

	int iCellX = iCellGiveX(iCell);
	int iCellY = iCellGiveY(iCell);
	int iCellXMax = iCellX + iWidth;
	int iCellYMax = iCellY + iHeight;

	for (int x = iCellX; x < iCellXMax; x++) {
		for (int y = iCellY; y < iCellYMax; y++) {
			map.clear_spot(iCellMake(x, y), iSight, iPlayer);
		}
	}
}

/**
	return free slot in array of structures.
**/
int cStructureFactory::getFreeSlot() {
	for (int i=0; i < MAX_STRUCTURES; i++) {
		if (structure[i] == NULL) {
			return i;
		}
	}

    return -1; // NONE
}

/**
	This function will check if at iCell (the upper left corner of a structure) a structure
	can be placed of type "iStructureType". If iUnitIDTOIgnore is > -1, then if any unit is
	supposidly 'blocking' this structure from placing, it will be ignored.

	Ie, you will use the iUnitIDToIgnore value when you want to create a Const Yard on the
	location of an MCV.

	Returns:
	-2  = ERROR / Cannot be placed at this location with the params given.
	-1  = PERFECT / Can be placed, and entire structure has pavement (slabs)
	>=0 = GOOD but it is not slabbed all (so not perfect)
**/
int cStructureFactory::getSlabStatus(int iCell, int iStructureType, int iUnitIDToIgnore) {
    // checks if this structure can be placed on this cell
    int w = structures[iStructureType].bmp_width/32;
    int h = structures[iStructureType].bmp_height/32;

    int slabs=0;
    int total=w*h;
    int x = iCellGiveX(iCell);
    int y = iCellGiveY(iCell);

    for (int cx=0; cx < w; cx++)
        for (int cy=0; cy < h; cy++)
        {
            int cll=iCellMake(cx+x, cy+y); // <-- some evil global thing that calculates the cell...

			// check if terrain allows it.
            if (map.getCellType(cll) != TERRAIN_ROCK &&
                map.getCellType(cll) != TERRAIN_SLAB) {
				logbook("getSlabStatus will return -2, reason: terrain is not rock or slab.");
                return -2; // cannot place on sand
            }

			// another structure found on this location, return -2 meaning "blocked"
            if (map.getCellIdStructuresLayer(cll) > -1) {
				logbook("getSlabStatus will return -2, reason: another structure found on one of the cells");
                return -2;
            }

			// unit found on location where structure wants to be placed. Check if
			// it may be ignored, if not, return -2.
            int idOfUnitAtCell = map.getCellIdUnitLayer(cll);
            if (idOfUnitAtCell > -1)
            {
                if (iUnitIDToIgnore > -1)
                {
                    if (idOfUnitAtCell == iUnitIDToIgnore) {
                        // ok; this may be ignored.
					} else {
						// not the unit to be ignored.
						logbook("getSlabStatus will return -2, reason: unit found that blocks placement; is not ignored");
						return -2;
					}
				} else {
					// no iUnitIDToIgnore given, this means always -2
					logbook("getSlabStatus will return -2, reason: unit found that blocks placement; no id to ignore");
                    return -2;
				}
            }

            // now check if the 'terrain' type is 'slab'. If that is true, increase value of found slabs.
			if (map.getCellType(cll) == TERRAIN_SLAB) {
                slabs++;
			}
        }


		// if the amount of slabs equals the amount of total slabs possible, return a perfect status.
		if (slabs >= total) {
			return -1; // perfect
		}

    return slabs; // ranges from 0 to <max slabs possible of building (ie height * width in cells)
}

void cStructureFactory::createSlabForStructureType(int iCell, int iStructureType) {
	assert(iCell > -1);

	int height = structures[iStructureType].bmp_height / 32;
	int width = structures[iStructureType].bmp_width / 32;

	int cellX = cellCalculator->getX(iCell);
	int cellY = cellCalculator->getY(iCell);

	int endCellX = cellX + width;
	int endCellY = cellY + height;
	for (int x = cellX; x < endCellX; x++) {
		for (int y = cellY; y < endCellY; y++) {
			int cell = cellCalculator->getCellWithMapDimensions(x, y, game.map_width, game.map_height);
			mapEditor.createCell(cell, TERRAIN_SLAB, 0);
		}
	}
}

void cStructureFactory::clearAllStructures() {
	for (int i=0; i < MAX_STRUCTURES; i++) {
		// clear out all structures
		if (structure[i]) {
			cStructureFactory::getInstance()->deleteStructureInstance(structure[i]);
		}

		// clear pointer
		structure[i] = nullptr;
	}
}
