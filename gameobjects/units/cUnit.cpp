/*

  Dune II - The Maker

  Author : Stefan Hendriks
  Contact: stefanhen83@gmail.com
  Website: http://dune2themaker.fundynamic.com

  2001 - 2020 (c) code by Stefan Hendriks

  */

#include <math.h>
#include "../../include/d2tmh.h"
#include "cUnit.h"


// Path creation definitions / var
#define CLOSED        -1
#define OPEN          0

struct ASTAR
{
  int cost;
  int parent;
  int state;
};

// Temp map
ASTAR temp_map[MAX_CELLS];


// Class specific on top
// Globals on bottom

void cUnit::init(int i) {

	fExperience=0;

    iID = i;

    iCell=0;          // cell of unit

    iType=0;          // type of unit

    iHitPoints=-1;     // hitpoints of unit
    iTempHitPoints=-1; // temp hold back for 'reinforced' / 'dumping' and 'repairing' units

    iPlayer=-1;        // belongs to player

    iGroup=-1;

    // Movement
    iNextCell=-1;      // where to move to (next cell)
    iGoalCell=-1;      // the goal cell (goal of path)
    iCellX=-1;
    iCellY=-1;
    iOffsetX=0;       // X offset
    iOffsetY=0;       // Y offset
    memset(iPath, -1, sizeof(iPath));    // path of unit
    iPathIndex=-1;     // where are we?
    iPathFails=0;
    bCalculateNewPath=false;

	bCarryMe=false;		// carry this unit when moving it around?
    iCarryAll=-1;		// any carry-all that will pickup this unit... so this unit knows
						// it should wait when moving, etc, etc


    iAction = ACTION_GUARD;

    iAttackUnit=-1;      // attacking unit id
    iAttackStructure=-1; // attack structure id
    iAttackCell=-1;

    // selected
    bSelected=false;

    // Action given code
    iUnitID=-1;        // Unit ID to attack/pickup, etc
    iStructureID=-1;   // structure ID to attack/bring to (refinery)

	// Carry-All specific
	iTransferType=-1;	// -1 = none, 0 = new , 1 = carrying existing unit
						// iUnitID = unit we CARRY (when TransferType == 1)
						// iTempHitPoints = hp of unit when transfertype = 1

	iCarryTarget=-1;	// Unit ID to carry, but is not carried yet
	iBringTarget=-1;	// Where to bring the carried unit (when iUnitID > -1)
	iNewUnitType=-1;	// new unit that will be brought, will be this type
	bPickedUp=false;		// picked up the unit?

    // harv
    iCredits=0;

    // Drawing
    iBodyFacing=0;    // Body of tanks facing
    iHeadFacing=0;    // Head of tanks facing
    iBodyShouldFace=iBodyFacing;    // where should the unit body look at?
    iHeadShouldFace=iHeadFacing;    // where should th eunit look at?

    iFrame=0;

    // TIMERS
	TIMER_blink=0;
    TIMER_move=0;
    TIMER_movewait=0;
    TIMER_thinkwait=0;    // wait with normal thinking..
    TIMER_turn=0;
    TIMER_frame=0;
    TIMER_harvest=0;
    TIMER_guard=0;    // guard scanning timer
    TIMER_bored=0;    // how long are we bored?
    TIMER_attack=0;
    TIMER_wormeat=0;


}

void cUnit::recreateDimensions() {
    if (dimensions != nullptr) {
        delete dimensions;
    }

    // set up dimensions
    dimensions = new cRectangle(draw_x(), draw_y(), getBmpWidth(), getBmpHeight());
}

void cUnit::die(bool bBlowUp, bool bSquish) {
    // Animation / Sound
	// TODO: update statistics player

    int iDieX=pos_x() + getUnitType().bmp_width/2;
    int iDieY=pos_y() + getUnitType().bmp_height/2;

    // when HARVESTER, check if there are any friends , if not, then deliver one
    if (iType == HARVESTER && // a harvester died
        player[iPlayer].hasAtleastOneStructure(REFINERY)) { // and its player still has a refinery

        // check if the player has any harvester left
        bool bFoundHarvester=false;
        for (int i=0; i < MAX_UNITS; i++) {
            if (i != iID) {
                cUnit &theUnit = unit[i];

                if (theUnit.isValid() && theUnit.iType == HARVESTER) {
                    if (theUnit.iPlayer == iPlayer) {
                        bFoundHarvester = true;
                        break;
                    }
                }
            }
        }

        // No harvester found, deliver one
        if (!bFoundHarvester) {
            // deliver

            cAbstractStructure * refinery = nullptr;
            for (int k=0; k < MAX_STRUCTURES; k++) {
                cAbstractStructure *theStructure = structure[k];
                if (!theStructure) continue;
                if (!theStructure->getOwner() == iPlayer) continue;
                if (!theStructure->getType() == REFINERY) continue;

                // found!
                refinery = theStructure;
                break;
            }

            // found a refinery, deliver harvester to that
            if (refinery) {
                REINFORCE(iPlayer, HARVESTER, refinery->getCell(), -1);
            }
        }
    } // a harvester died, check if we have to deliver a new one to the player

    if (bBlowUp) {
        if (iType == TRIKE || iType == RAIDER || iType == QUAD) {
            // play quick 'boom' sound and show animation
            PARTICLE_CREATE(iDieX, iDieY, EXPLOSION_TRIKE, -1, -1);
            play_sound_id_with_distance(SOUND_TRIKEDIE, distanceBetweenCellAndCenterOfScreen(iCell));

            if (rnd(100) < 30) {
                PARTICLE_CREATE(iDieX, iDieY - 24, OBJECT_SMOKE, -1, -1);
            }
        }

        if (iType == SIEGETANK || iType == DEVASTATOR && rnd(100) < 25) {
            if (iBodyFacing == FACE_UPLEFT ||
                iBodyFacing == FACE_DOWNRIGHT) {
                PARTICLE_CREATE(iDieX, iDieY, OBJECT_SIEGEDIE, iPlayer, -1);
            }
        }

        if (iType == TANK || iType == SIEGETANK || iType == SONICTANK || iType == LAUNCHER || iType == DEVIATOR ||
            iType == HARVESTER || iType == ORNITHOPTER || iType == MCV) {
            // play quick 'boom' sound and show animation
            if (rnd(100) < 50) {
                PARTICLE_CREATE(iDieX, iDieY, EXPLOSION_TANK_ONE, -1, -1);
                play_sound_id_with_distance(SOUND_TANKDIE2, distanceBetweenCellAndCenterOfScreen(iCell));
            } else {
                PARTICLE_CREATE(iDieX, iDieY, EXPLOSION_TANK_TWO, -1, -1);
                play_sound_id_with_distance(SOUND_TANKDIE, distanceBetweenCellAndCenterOfScreen(iCell));
            }

            if (rnd(100) < 30)
                PARTICLE_CREATE(iDieX, iDieY - 24, OBJECT_SMOKE, -1, -1);

            if (iType == HARVESTER) {
                game.TIMER_shake = 25;
                mapEditor.createField(iCell, TERRAIN_SPICE, ((iCredits + 1) / 7));
            }

            if (iType == ORNITHOPTER) {
                PARTICLE_CREATE(iDieX, iDieY, EXPLOSION_ORNI, -1, -1);
            }
        }

        if (iType == DEVASTATOR) {
            int iOrgDieX = iDieX;
            int iOrgDieY = iDieY;


            // create a cirlce of explosions (big ones)
            iDieX -= 32;
            iDieY -= 32;

            for (int cx = 0; cx < 3; cx++)
                for (int cy = 0; cy < 3; cy++) {


                    for (int i = 0; i < 2; i++)
                        PARTICLE_CREATE(iDieX + (cx * 32), iDieY + (cy * 32), EXPLOSION_STRUCTURE01 + rnd(2), -1, -1);

                    if (rnd(100) < 35)
                        play_sound_id_with_distance(SOUND_TANKDIE + rnd(2),
                                                    distanceBetweenCellAndCenterOfScreen(iCell));

                    // calculate cell and damage stuff around this
                    int cll = iCellMake((iCellX - 1) + cx, (iCellY - 1) + cy);

                    if (cll == iCell)
                        continue; // do not do own cell

                    if (map.getCellType(cll) == TERRAIN_WALL) {
                        // damage this type of wall...
                        map.cellTakeDamage(cll, 150);

                        if (map.getCellHealth(cll) < 0) {
                            // remove wall, turn into smudge:
                            mapEditor.createCell(cll, TERRAIN_ROCK, 0);

                            mapEditor.smoothAroundCell(cll);

                            map.smudge_increase(SMUDGE_WALL, cll);
                        }
                    }

                    // damage surrounding units
                    int idOfUnitAtCell = map.getCellIdUnitLayer(cll);
                    if (idOfUnitAtCell > -1) {
                        int id = idOfUnitAtCell;

                        if (unit[id].iHitPoints > 0) {

                            unit[id].iHitPoints -= 150;

                            // NO HP LEFT, DIE
                            if (unit[id].iHitPoints <= 1)
                                unit[id].die(true, false);
                        } // only die when the unit is going to die
                    }

                    int idOfStructureAtCell = map.getCellIdStructuresLayer(cll);
                    if (idOfStructureAtCell > -1) {
                        // structure hit!
                        int id = idOfStructureAtCell;

                        if (structure[id]->getHitPoints() > 0) {

                            int iDamage = 150 + rnd(100);
                            structure[id]->damage(iDamage);

                            int iChance = 10;

                            if (structure &&
                                structure[id]->getHitPoints() < (structures[structure[id]->getType()].hp / 2))
                                iChance = 30;

                            if (rnd(100) < iChance) {
                                long x = pos_x() + (mapCamera->getViewportStartX()) + 16 + (-8 + rnd(16));
                                long y = pos_y() + (mapCamera->getViewportStartY()) + 16 + (-8 + rnd(16));
                                PARTICLE_CREATE(x, y, OBJECT_SMOKE, -1, -1);
                            }
                        }
                    }


                    int cellType = map.getCellType(cll);
                    if (cellType == TERRAIN_ROCK) {
                        if (cellType != TERRAIN_WALL)
                            map.cellTakeDamage(cll, 30);

                        if (map.getCellHealth(cll) < -25) {
                            map.smudge_increase(SMUDGE_ROCK, cll);
                            map.cellGiveHealth(cll, rnd(25));
                        }
                    } else if (cellType == TERRAIN_SAND ||
                               cellType == TERRAIN_HILL ||
                               cellType == TERRAIN_SPICE ||
                               cellType == TERRAIN_SPICEHILL) {
                        if (cellType != TERRAIN_WALL)
                            map.cellTakeDamage(cll, 30);

                        if (map.getCellHealth(cll) < -25) {
                            map.smudge_increase(SMUDGE_SAND, cll);
                            map.cellGiveHealth(cll, rnd(25));
                        }
                    }
                }


            PARTICLE_CREATE(iOrgDieX, iOrgDieY, OBJECT_BOOM02, -1, -1);

            PARTICLE_CREATE(iOrgDieX - 32, iOrgDieY, OBJECT_BOOM02, -1, -1);
            PARTICLE_CREATE(iOrgDieX + 32, iOrgDieY, OBJECT_BOOM02, -1, -1);
            PARTICLE_CREATE(iOrgDieX, iOrgDieY - 32, OBJECT_BOOM02, -1, -1);
            PARTICLE_CREATE(iOrgDieX, iOrgDieY + 32, OBJECT_BOOM02, -1, -1);

        }

        if (iType == TROOPER || iType == SOLDIER) {
            // create particle of dead body

            PARTICLE_CREATE(iDieX, iDieY, OBJECT_DEADINF02, iPlayer, -1);

            play_sound_id_with_distance(SOUND_DIE01 + rnd(5), distanceBetweenCellAndCenterOfScreen(iCell));
        }

        if (iType == TROOPERS || iType == INFANTRY) {
            // create particle of dead body

            PARTICLE_CREATE(iDieX, iDieY, OBJECT_DEADINF01, iPlayer, -1);

            play_sound_id_with_distance(SOUND_DIE01 + rnd(5), distanceBetweenCellAndCenterOfScreen(iCell));
        }
    } // blow up
    else {


    }

    if (bSquish) {

        // when we do not 'blow up', we died by something else. Only infantry will be 'squished' here now.
        if (iType == SOLDIER || iType == TROOPER) {
            PARTICLE_CREATE(iDieX, iDieY, EXPLOSION_SQUISH01 + rnd(2), iPlayer, iFrame);
            play_sound_id_with_distance(SOUND_SQUISH, distanceBetweenCellAndCenterOfScreen(iCell));
        } else if (iType == TROOPERS || iType == INFANTRY) {
            PARTICLE_CREATE(iDieX, iDieY, EXPLOSION_SQUISH03, iPlayer, iFrame);
            play_sound_id_with_distance(SOUND_SQUISH, distanceBetweenCellAndCenterOfScreen(iCell));
        }

    }

    // NOW IT IS FREE FOR USAGE AGAIN

    if (iStructureID > -1) {
        if (structure[iStructureID])
            structure[iStructureID]->setAnimating(false);
    }

    // Anyone who was attacking this unit is on guard
    for (int i = 0; i < MAX_UNITS; i++) {
        if (unit[i].isValid())
            if (i != iID)
                if (unit[i].iAttackUnit == i) {
                    unit[i].iAttackUnit = -1;
                    unit[i].iGoalCell = unit[i].iCell;
                    unit[i].iAction = ACTION_GUARD;

                    // Ai will still move to this location
                    logbook("Another move to");
                    unit[i].move_to(iCell, -1, -1);
                }
    }

    init(iID);    // init

    for (int i = 0; i < MAPID_MAX; i++) {
        if (i != MAPID_STRUCTURES) {
            map.remove_id(iID, i);
        }
    }
}


bool cUnit::isValid()
{
    if (iPlayer <0)
        return false;

    // no hitpoints and not in a structure
    if (iHitPoints < 0  && iTempHitPoints < 0)
        return false;

    if (iCell < 0 || iCell >= MAX_CELLS)
        return false;

    return true;
}

int cUnit::pos_x() {
    return (iCellX * TILESIZE_WIDTH_PIXELS) + iOffsetX;
}

int cUnit::pos_y() {
    return (iCellY * TILESIZE_HEIGHT_PIXELS) + iOffsetY;
}

int cUnit::draw_x() {
    int bmpOffset = (TILESIZE_WIDTH_PIXELS - getBmpWidth()) / 2;
    return mapCamera->getWindowXPositionWithOffset(pos_x(), bmpOffset);
}

int cUnit::center_draw_x() {
    return draw_x() + (mapCamera->factorZoomLevel(getBmpWidth())/2);
}

int cUnit::center_draw_y() {
    return draw_y() + (mapCamera->factorZoomLevel(getBmpHeight())/2);
}

int cUnit::draw_y() {
    int bmpOffset = (TILESIZE_HEIGHT_PIXELS - getBmpHeight()) / 2;
    return mapCamera->getWindowYPositionWithOffset(pos_y(), bmpOffset);
}

int cUnit::getBmpHeight() const {
    return units[iType].bmp_height;
}

void cUnit::draw_spice() {
    float width_x = mapCamera->factorZoomLevel(getBmpWidth());
    int height_y = mapCamera->factorZoomLevel(4);
    int drawx = draw_x();
    int drawy = draw_y()-((height_y *2)+ 2);

    int max = getUnitType().credit_capacity;
    int w = health_bar(width_x, iCredits, max);

    // bar itself
    rectfill(bmp_screen, drawx, drawy, drawx + width_x, drawy + height_y, makecol(0,0,0));
    rectfill(bmp_screen, drawx, drawy, drawx + (w), drawy + height_y, makecol(255,91,1));

    // bar around it (only when it makes sense due zooming)
    if (height_y > 2) {
        rect(bmp_screen, drawx, drawy, drawx + width_x, drawy + height_y, makecol(255, 255, 255));
    }
}

int cUnit::getBmpWidth() const {
    return units[iType].bmp_width;
}

float cUnit::getHealthNormalized() {
    s_UnitP &unitType = getUnitType();
    float flMAX  = unitType.hp;
    return (iHitPoints / flMAX);
}

void cUnit::draw_health() {
    // draw units health
    float width_x = mapCamera->factorZoomLevel(getBmpWidth());
    int height_y = mapCamera->factorZoomLevel(4);
    int drawx = draw_x();
    int drawy = draw_y()-(height_y + 2);

    float healthNormalized = getHealthNormalized();

    int w = healthNormalized * width_x;
    int r = (1.1 - healthNormalized) * 255;
    int g = healthNormalized * 255;

    if (r > 255) r = 255;

    // bar itself
    rectfill(bmp_screen, drawx, drawy, drawx + width_x, drawy + height_y, makecol(0,0,0));
    rectfill(bmp_screen, drawx, drawy, drawx + (w-1), drawy + height_y, makecol(r,g,32));

    // bar around it (only when it makes sense due zooming)
    if (height_y > 2) {
        rect(bmp_screen, drawx, drawy, drawx + width_x, drawy + height_y, makecol(255, 255, 255));
    }

    // draw group
    if (iGroup > 0 &&
        iPlayer == HUMAN) {
        alfont_textprintf(bmp_screen, bene_font, drawx+26,drawy-11, makecol(0,0,0), "%d", iGroup);
        alfont_textprintf(bmp_screen, bene_font, drawx+26,drawy-12, makecol(255,255,255), "%d", iGroup);
    }

}

// this method returns the amount of percent extra damage may be done
float cUnit::fExpDamage()
{
	if (fExperience < 1) return 0; // no stars, no increasement

	// MAX EXPERIENCE = 10 (9 stars)

	// A unit can do 2x damage on full experience; being very powerfull.
	// it does take a long time to get there though.
	float fResult = (fExperience/10);

	// never make a unit weaker.
	if (fResult < 1.0) fResult = 1.0F;
	return fResult;
}

void cUnit::draw_experience()
{
	// draws experience above health

	int iStars= (int)fExperience;

	if (iStars < 1)
		return; // no stars to draw!

	int iStarType = 0;

	// twice correct and upgrade star type
	if (iStars > 2)
	{
		iStarType++;
		iStars-=3;
	}

	// red stars now, very much experience!
	if (iStars > 2)
	{
		iStarType++;
		iStars-=3;
	}

	// still enough experience! wow
	if (iStars > 2)
	{
		iStars=3;
	}


	int drawx = draw_x()+3;
    int drawy = draw_y()-19;

	// 1 star = 1 experience
	for (int i=0; i < iStars; i++)
	{
		draw_sprite(bmp_screen, (BITMAP *)gfxdata[OBJECT_STAR_01+iStarType].dat, drawx+i*9, drawy);
	}

}

void cUnit::draw_path() {
	// for debugging purposes
	if (iCell == iGoalCell)
		return;

	if (iPath[0] < 0)
		return;

    int halfTile = 16;
	int iPrevX  = mapCamera->getWindowXPositionFromCellWithOffset(iPath[0], halfTile);
	int iPrevY  = mapCamera->getWindowYPositionFromCellWithOffset(iPath[0], halfTile);

	for (int i=1; i < MAX_PATH_SIZE; i++) {
	    if (iPath[i] < 0) break;
        int iDx  = mapCamera->getWindowXPositionFromCellWithOffset(iPath[i], halfTile);
        int iDy  = mapCamera->getWindowYPositionFromCellWithOffset(iPath[i], halfTile);

        if (i == iPathIndex) { // current node we navigate to
            line(bmp_screen, iPrevX, iPrevY, iDx, iDy, makecol(255, 255, 255));
        } else if (iPath[i] == iGoalCell) {
            // end of path (goal)
            line(bmp_screen, iPrevX, iPrevY, iDx, iDy, makecol(255, 0, 0));
        } else {
            // everything else
            line(bmp_screen, iPrevX, iPrevY, iDx, iDy, makecol(255, 255, 64));
        }

        // draw a line from previous to current
        iPrevX = iDx;
        iPrevY = iDy;
	}
}

void cUnit::draw() {
    if (iTempHitPoints > -1) {
        // temp hitpoints filled, meaning it is not visible (but not dead). Ie, it is being repaired, or transfered
        // by carry-all
        return;
    }

    int ux = draw_x();
    int uy = draw_y();

    if (iType == SANDWORM) {
        Shimmer(20, ux+(mapCamera->getZoomedHalfTileSize()), uy + (mapCamera->getZoomedHalfTileSize()));
		return;
    }

    s_UnitP &unitType = getUnitType();
    int bmp_width = unitType.bmp_width;
    int bmp_height = unitType.bmp_height;

    // the multiplier we will use to draw the unit
    int bmp_head = convert_angle(iHeadFacing);
    int bmp_body = convert_angle(iBodyFacing);

    int start_x, start_y;

    // draw body first
    start_x = bmp_body * bmp_width;
    start_y = bmp_height * iFrame;

    // Selection box x, y position. Depends on unit size
    int iSelX = ux;
    int iSelY = uy;

    cPlayer &cPlayer = player[this->iPlayer];

    // Draw SHADOW
    BITMAP *shadow = cPlayer.getUnitShadowBitmap(iType, bmp_body, iFrame);
    float scaledWidth = mapCamera->factorZoomLevel(bmp_width);
    float scaledHeight = mapCamera->factorZoomLevel(bmp_height);

    if (shadow) {
        int destY = uy;

		if (iType == CARRYALL) {
		    // adjust X and Y so it looks like a carry-all is 'higher up in the air'
            destY = uy + 24; // TODO; do something with height here? the closer to target, the less distant the shadow?
		}

        allegroDrawer->maskedStretchBlit(shadow, bmp_screen, 0, 0, bmp_width, bmp_height,
                                         ux, destY,
                                         scaledWidth, scaledHeight);
		destroy_bitmap(shadow);
    }

    // Draw BODY
    BITMAP *bitmap = cPlayer.getUnitBitmap(iType);
    if (bitmap) {
        allegroDrawer->maskedStretchBlit(bitmap, bmp_screen, start_x, start_y, bmp_width, bmp_height,
                            ux, uy,
                            scaledWidth,
                            scaledHeight);
    } else {
        char msg[255];
        sprintf(msg, "unit of iType [%d] did not have a bitmap!?", iType);
        logbook(msg);
    }

    // Draw TOP
    BITMAP *top = cPlayer.getUnitTopBitmap(iType);
    if (top && iHitPoints > -1) {
        // recalculate start_x using head instead of body
        start_x = bmp_head * bmp_width;
        start_y = bmp_height * iFrame;

        allegroDrawer->maskedStretchBlit(top, bmp_screen, start_x, start_y, bmp_width, bmp_height, ux, uy, mapCamera->factorZoomLevel(bmp_width), mapCamera->factorZoomLevel(bmp_height));
    }

    // TODO: Fix this / Draw BLINKING (ie, when targeted unit)
//	if (TIMER_blink > 0)
//		if (rnd(100) < 15)
//			mask_to_color(temp, makecol(255,255,255));

	// when we want to be picked up..
	if (bCarryMe) {
        draw_sprite(bmp_screen, (BITMAP *) gfxdata[SYMB_PICKMEUP].dat, ux, uy - 7);
    }

    if (bSelected) {
//       draw_sprite(bmp_screen, (BITMAP *)gfxdata[FOCUS].dat, iSelX/*+startpixel*/-2, iSelY);
        BITMAP *focusBitmap = (BITMAP *) gfxdata[FOCUS].dat;
        int bmp_width = focusBitmap->w;
        int bmp_height = focusBitmap->h;
        allegroDrawer->maskedStretchBlit(focusBitmap, bmp_screen, 0, 0, bmp_width, bmp_height, iSelX/*+startpixel*/- 2, iSelY, mapCamera->factorZoomLevel(bmp_width), mapCamera->factorZoomLevel(bmp_height));
    }


}

// GLOBALS
void cUnit::poll()
{
    iCellX = iCellGiveX(iCell);
    iCellY = iCellGiveY(iCell);
}

void cUnit::move_to(int iCll, int iStrucID, int iUnitID)
{
	LOG("ORDERED TO MOVE");

    iGoalCell = iCll;
    iStructureID = iStrucID;
    iUnitID = iUnitID;
    iPathIndex=-1;
	iAttackStructure=-1;
    iAttackCell=-1;

	// only when not moving (half on tile) reset nextcell
	if (iOffsetX == 0 && iOffsetY == 0)
		iNextCell=-1;

	iAction = ACTION_MOVE;

	memset(iPath,-1, sizeof(iPath));

    if (iStrucID > -1)
    {
        if (structure[iStrucID])
        {
            structure[iStrucID]->iUnitID = iUnitID;
        }
    }

    char msg[255];
    sprintf(msg, "%s", units[iType].name);
    logbook(msg);
}


void cUnit::think_guard() {

    if (units[iType].airborn)
    {
        iAction = ACTION_MOVE; // fly around man
        return;
    }

    if (iType == HARVESTER)
    {
        iAction = ACTION_MOVE;
        return;

    }

    TIMER_bored++; // we are bored ow yeah
    TIMER_guard++; // scan time

    if (TIMER_guard > 5)
    {
        // scan area
        TIMER_guard = 0 - (rnd(5)); // do not scan all at the same time so do it like this

        poll();
        // scan
        int iDistance=9999;
        int iDanger=-1;

        // non airborn
        if (!units[iType].airborn)
        {
			if (iType == SANDWORM) {
				if (TIMER_wormeat > 0)
				{
					TIMER_wormeat--;
					return; // get back, not hungry just yet
				}

				for (int i=0; i < MAX_UNITS; i++)
				{
					if (unit[i].isValid() && i != iID)
					{
						// not ours and its visible
						if (unit[i].iPlayer != iPlayer &&
							units[unit[i].iType].airborn == false)
						{
                            int cellType = map.getCellType(unit[i].iCell);
                            if (cellType == TERRAIN_SAND ||
                                cellType == TERRAIN_HILL ||
                                cellType == TERRAIN_SPICE ||
                                cellType == TERRAIN_SPICEHILL)
							{
								int distance = ABS_length(iCellX, iCellY, unit[i].iCellX, unit[i].iCellY);

								if (distance <= units[iType].sight && distance < iDistance)
								{
									// ATTACK
									iDistance = distance;
									iDanger=i;
									// logbook("WORM FOUND ENEMY");
								}
							} // valid terrain
						}

					}
				}

			}
			else // not sandworm
			{

				for (int i=0; i < MAX_UNITS; i++)
				{
					if (unit[i].isValid() && i != iID)
					{
						bool bAlly=false;

						// When we are player 1 till 5 (6 = SANDWORM) then we have a lot of allies)
						//if (iPlayer >= 1 && iPlayer <= 5)
						//	if (unit[i].iPlayer >= 1 && unit[i].iPlayer <= 5)
						//		bAlly=true; // friend dude!


                        if (player[iPlayer].iTeam == player[unit[i].iPlayer].iTeam)
                            bAlly=true;

							if (iPlayer == 0 && player[0].getHouse() == ATREIDES)
							{
								// when the unit player == FREMEN
								if (player[unit[i].iPlayer].getHouse() == FREMEN && iType != SANDWORM)
									bAlly=true;
							}


							// not ours and its visible
							if (unit[i].iPlayer != iPlayer &&
									mapUtils->isCellVisibleForPlayerId(iPlayer, unit[i].iCell) && // is visible for ai as well?
								units[unit[i].iType].airborn == units[iType].airborn && bAlly == false)
							{
								int distance = ABS_length(iCellX, iCellY, unit[i].iCellX, unit[i].iCellY);

								// TODO: perhaps make this configurable, so you can set the 'aggresiveness' of units?
								int range = units[iType].sight + 3; // do react earlier than already in range.

								if (distance <= range && distance < iDistance)
								{
									// ATTACK
									iDistance = distance;
									iDanger=i;
								}
							}

					}
				}

			}


        }


        if (iDanger > -1)
        {

			if (units[unit[iDanger].iType].airborn)
				allegro_message("Choosen to attack airborn unit?");

            UNIT_ORDER_ATTACK(iID, unit[iDanger].iCell, iDanger, -1,-1);

			if (game.iMusicType == MUSIC_PEACE && iType != SANDWORM && iPlayer == 0)
			{
				playMusicByType(MUSIC_ATTACK);

				// warning... bla bla
				if (unit[iDanger].iType == SANDWORM)
					play_voice(SOUND_VOICE_10_ATR); // wormsign
				else
					play_voice(SOUND_VOICE_09_ATR); // enemy unit approaching
			}
            return;
        }
		else
		{
			// NOT SANDWORM AND NOT HUMAN PLAYER
			if (iType != SANDWORM && iPlayer > 0)
			{
				// nothing found yet to attack;
				// ai units will auto-attack structures nearby

				for (int i=0; i < MAX_STRUCTURES; i++)
				{
					if (structure[i] && i != iID)
					{
						bool bAlly=false;
/*
						// When we are player 1 till 5 (6 = SANDWORM) then we have a lot of allies)
						if (iPlayer >= 1 && iPlayer <= 5)
							if (structure[i].iPlayer >= 1 && structure[i].iPlayer <= 5)
								bAlly=true; // friend dude!*/

                        if (player[iPlayer].iTeam == player[structure[i]->getOwner()].iTeam)
                            bAlly=true;


							if (iPlayer == 0 && player[0].getHouse() == ATREIDES)
							{
								// when the unit player == FREMEN
								if (player[unit[i].iPlayer].getHouse() == FREMEN && iType != SANDWORM)
									bAlly=true;
							}


							// not ours and its visible
							if (structure[i]->getOwner() != iPlayer &&
								mapUtils->isCellVisibleForPlayerId(iPlayer, structure[i]->getCell()) &&
								bAlly == false)	{
								int distance = ABS_length(iCellX, iCellY, iCellGiveX(structure[i]->getCell()),
															iCellGiveY(structure[i]->getCell()));

								if (distance <= units[iType].sight && distance < iDistance)
								{
									// ATTACK
									iDistance = distance;
									iDanger=i;
								}
							}

					}
				}
			}

			if (iDanger > -1)
			{
				UNIT_ORDER_ATTACK(iID, structure[iDanger]->getCell(), -1, iDanger,-1);

				if (game.iMusicType == MUSIC_PEACE && iType != SANDWORM && iPlayer == 0)
				{
					playMusicByType(MUSIC_ATTACK);
				}

			}
		}

    }


    if (TIMER_bored > 3500)
    {
        TIMER_bored=0;
        iBodyShouldFace=rnd(8);
        iHeadShouldFace=rnd(8);
    }

    // When bored we turn our body and head sometimes, so do it here:

}

// NORMAL thinking
void cUnit::think() {
    if (TIMER_blink > 0)
		TIMER_blink--;

    if (iType == MCV)
    {
        if (iPlayer == 0)
        {
            if (bSelected)
                if (key[KEY_D])
                {
					int iGood = cStructureFactory::getInstance()->getSlabStatus(iCell, CONSTYARD, iID);

                    if (iGood >= -1)
                    {
                        int iLocation=iCell;

                        die(false, false);

                        // place const yard
						cStructureFactory::getInstance()->createStructure(iLocation, CONSTYARD, 0, 100);
                    }
                }
        }
    }

    // HEAD is not facing correctly
    if (units[iType].airborn == false)
    {
    if (iBodyFacing == iBodyShouldFace)
    {
     if (iHeadFacing != iHeadShouldFace)
     {

         TIMER_turn++;


        if (TIMER_turn > (units[iType].turnspeed))
        {
            TIMER_turn=0;

            int d = 1;

            int toleft = (iHeadFacing + 8) - iHeadShouldFace;
            if (toleft > 7) toleft -= 8;

            int toright = abs(toleft-8);

            if (toright == toleft) d = -1+(rnd(2));
            if (toleft  > toright) d = 1;
            if (toright > toleft)  d = -1;

            iHeadFacing += d;

            if (iHeadFacing < 0)
                iHeadFacing = 7;

            if (iHeadFacing > 7)
                iHeadFacing = 0;


        } // turn




     } // head facing

    }
    else
    {
        // BODY is not facing correctly
		        TIMER_turn++;


        if (TIMER_turn > (units[iType].turnspeed))
        {
            TIMER_turn=0;

            int d = 1;

            int toleft = (iBodyFacing + 8) - iBodyShouldFace;
            if (toleft > 7) toleft -= 8;

            int toright = abs(toleft-8);

            if (toright == toleft) d = -1+(rnd(2));
            if (toleft  > toright) d = 1;
            if (toright > toleft)  d = -1;

            iBodyFacing += d;

            if (iBodyFacing < 0)
                iBodyFacing = 7;

            if (iBodyFacing > 7)
                iBodyFacing = 0;


        } // turn
    }

    } // airborn stuff

    // when waiting.. wait
    if (TIMER_thinkwait > 0)
    {
        TIMER_thinkwait--;
        return;
    }

    if (iTempHitPoints > -1)
        return;

	// when any unit is on a spice bloom, you got a problem, you die!
    int cellType = map.getCellType(iCell);
    if (cellType == TERRAIN_BLOOM
        && units[iType].airborn == false) {
		// change type of terrain to sand
		mapEditor.createCell(iCell, TERRAIN_SAND, 0);

		mapEditor.createField(iCell, TERRAIN_SPICE, 25+(rnd(50)));

		// kill unit
		map.remove_id(iID, MAPID_UNITS);

		die(true, false);
		game.TIMER_shake=20;
		return;
	}


    // --- think
    if (iType == ORNITHOPTER) {
        // flap with your wings
        iFrame++;

        if (iFrame > 3)
            iFrame = 0;

        if ((iAttackUnit < 0 || iAttackStructure < 0) && TIMER_attack == 0) {
            // scan for enemy activities.
            int iDistance=9999;
            int iTarget=-1;

        	for (int i=0; i < MAX_UNITS; i++) {
					if (unit[i].isValid() && i != iID) {

                        if (player[iPlayer].iTeam == player[unit[i].iPlayer].iTeam)
                            continue;


                        if (iPlayer == 0 && player[0].getHouse() == ATREIDES)
                        {
                            // when the unit player == FREMEN
                            if (player[unit[i].iPlayer].getHouse() == FREMEN && iType != SANDWORM)
                                continue;
                        }


							// not ours and its visible
							if (unit[i].iPlayer != iPlayer &&
								mapUtils->isCellVisibleForPlayerId(iPlayer, unit[i].iCell) &&
								units[unit[i].iType].airborn == false) // do not attack airborn units!?
							{
								int distance = ABS_length(iCellX, iCellY, unit[i].iCellX, unit[i].iCellY);

								if (distance <= units[iType].range && distance < iDistance)
								{
									// ATTACK
									iDistance = distance;
									iTarget=i;
								}
							}

					}
				}

            // target known?
            if (iTarget > -1) {
                UNIT_ORDER_ATTACK(iID, unit[iTarget].iCell, iTarget, -1,-1);
				if (DEBUGGING)
					logbook("FOUND ENEMY TO ATTACK");
            } else {
                // no unit found, attack structure
                // scan for enemy activities.
                int iDistance=9999;
                int iTarget=-1;

                for (int i=0; i < MAX_STRUCTURES; i++) {
                    cAbstractStructure *pStructure = structure[i];
                    if (!pStructure) continue;
                    if (!pStructure->isValid()) continue;

                    // skip same team
                    if (player[iPlayer].iTeam == pStructure->getPlayer()->iTeam)
                        continue;

                    //
                    if (iPlayer == HUMAN && player[HUMAN].isHouse(ATREIDES))
                    {
                        // ATREIDES is allies with FREMEN
                        if (pStructure->getPlayer()->isHouse(FREMEN))
                            continue;
                    }

                    // not ours and its visible
                    if (pStructure->getPlayerId() != iPlayer && // enemy
                        mapUtils->isCellVisibleForPlayerId(iPlayer, pStructure->getCell()))
                    {
                        int cellX = iCellGiveX(pStructure->getCell());
                        int cellY = iCellGiveX(pStructure->getCell());
                        int distance = ABS_length(iCellX, iCellY, pStructure->getCell(), unit[i].iCellY);

                        // attack closest structure
                        if (distance < iDistance)
                        {
                            // ATTACK
                            iDistance = distance;
                            iTarget=i;
                        }
                    }
                }

                if (iTarget > -1) {
                    UNIT_ORDER_ATTACK(iID, unit[iTarget].iCell, -1, iTarget, -1);
                }
            }
        }
        else if (iAttackUnit < 0 && TIMER_attack < 0)
        {
            TIMER_attack++;
        }
    }


    // HARVESTERs logic here
    int idOfStructureAtCell = map.getCellIdStructuresLayer(iCell);
    if (iType == HARVESTER) {
        bool bFindRefinery=false;

        if (iCredits > 0 && bSelected && key[KEY_D]) {
            bFindRefinery = true;
        }

        // cell = goal cell (doing nothing)
        if (iCell == iGoalCell) {
            // when on spice, harvest
            if (cellType == TERRAIN_SPICE ||
                cellType == TERRAIN_SPICEHILL)
            {
                // do timer stuff
                if (iCredits < units[iType].credit_capacity)
                    TIMER_harvest++;
            } else {
                // not on spice, find a new location
                if (iCredits < units[iType].credit_capacity) {
                    // find harvest cell
                    move_to(UNIT_find_harvest_spot(iID), -1, -1);
                } else {
                    iFrame=0;
                    bFindRefinery=true;
                    // find a refinery
                }
            }

            if (iCredits >= units[iType].credit_capacity)
                bFindRefinery=true;

            // when we should harvest...
            cPlayerDifficultySettings *difficultySettings = player[iPlayer].getDifficultySettings();
            if (TIMER_harvest > (difficultySettings->getHarvestSpeed(units[iType].harvesting_speed)) &&
                iCredits < units[iType].credit_capacity) {
                TIMER_harvest=1;

                iFrame++;

                if (iFrame > 3)
                    iFrame = 1;

                iCredits += units[iType].harvesting_amount;
                map.cellTakeCredits(iCell, units[iType].harvesting_amount);

                // turn into sand/spice (when spicehill)
                if (map.getCellCredits(iCell) <= 0)
                {
                    if (cellType == TERRAIN_SPICEHILL)
                    {
                        map.cellChangeType(iCell, TERRAIN_SPICE);
                        map.cellGiveCredits(iCell, rnd(100));
                    }
                    else
                    {
                        map.cellChangeType(iCell, TERRAIN_SAND);
                        map.cellChangeTile(iCell, 0);
                    }

                    // create new path to this thingy
                    //iGoalCell =  UNIT_find_harvest_spot(iID);
                    //iPathIndex=-1; // create new path

                    move_to(UNIT_find_harvest_spot(iID), -1, -1);

                    mapEditor.smoothAroundCell(iCell);
                }
            }


            // refinery required, go find one that is available
            if (bFindRefinery) {
                iFrame=0;

                cStructureUtils structureUtils;
                char msg[255];
                sprintf(msg, "Going to look for a refinery, playerId [%d], cell [%d]", iPlayer, iCell);
                logbook(msg);
                int	refineryStructureId = structureUtils.findClosestStructureTypeWhereNoUnitIsHeadingToComparedToCell(
                        iCell, REFINERY,&player[iPlayer]);

                if (refineryStructureId < 0) {
                    // none found, wait
                    TIMER_thinkwait=10;
                    return;
                }

				// found a refinery, lets check if we can find a carry-all that will bring us
                cAbstractStructure *refinery = structure[refineryStructureId];
                refinery->setAnimating(true);

                // how? carry-all or ride?
                int r = CARRYALL_TRANSFER(iID, refinery->getCell() + 2);

                iStructureID = refineryStructureId;

                if (r < 0) {
                    char msg[255];
                    sprintf(msg, "Returning to refinery ID %d", refineryStructureId);
                    LOG(msg);
					move_to(refinery->getCell() + rnd(2) + (rnd(2) * 64), refineryStructureId, -1); // move yourself...
				} else {
					TIMER_movewait = 500; // wait for pickup!
					TIMER_thinkwait = 500;
				}

                TIMER_movewait=0;
            }

        } else {
            // ??
            iFrame = 0;
        }

		// we wanted to enter this structure, so do it immidiatly (else we just seem to
		// drive over the structure, which looks odd!
		if (iStructureID > -1 &&
            idOfStructureAtCell == iStructureID &&
            idOfStructureAtCell > -1)
		{

			// when this structure is not occupied
			if (structure[iStructureID]->iUnitID < 0)
			{
				// get in!
				structure[iStructureID]->setAnimating(false);
				structure[iStructureID]->iUnitID = iID;  // !!
				structure[iStructureID]->setFrame(0);

				// store this
				iTempHitPoints = iHitPoints;
				iHitPoints=-1; // 'kill' unit

				if (DEBUGGING)
					logbook("[UNIT] -> Enter refinery #1");

				map.remove_id(iID, MAPID_UNITS);
			} // enter..
		}
    }

    	// we wanted to enter this structure, so do it immidiatly (else we just seem to
		// drive over the structure, which looks odd!
		if (iStructureID > -1 &&
            idOfStructureAtCell == iStructureID &&
            idOfStructureAtCell > -1)
		{

			// when this structure is not occupied
			if (structure[iStructureID]->iUnitID < 0)
			{
				// get in!
				structure[iStructureID]->setAnimating(false);
				structure[iStructureID]->iUnitID = iID;  // !!
				structure[iStructureID]->setFrame(0);

				// store this
				iTempHitPoints = iHitPoints;
				iHitPoints=-1; // 'kill' unit

				if (DEBUGGING)
					logbook("[UNIT] -> Enter Structure");

				map.remove_id(iID, MAPID_UNITS);
			} // enter..
		}


	// When this is a carry-all, show proper animation when filled
	if (iType == CARRYALL) {
		// A carry-all has something when:
		// - it carries a unit (iUnitID > -1)
		// - it has the flag TRANSFER_NEW_

		if ((iTransferType == TRANSFER_NEW_STAY ||
			 iTransferType == TRANSFER_NEW_LEAVE ||
			 iTransferType == TRANSFER_PICKUP) || iUnitID > -1) {

			// when picking up a unit.. only draw when picked up
			if (iTransferType == TRANSFER_PICKUP && bPickedUp)
				iFrame=1;

			// any other transfer, means it is filled from start...
			if (iTransferType != TRANSFER_PICKUP)
				iFrame=1;
		} else {
            iFrame = 0;
        }
	}
}

// aircraft specific thinking
void cUnit::think_move_air() {
    if (iTempHitPoints > -1) {
        return;
    }

    if (TIMER_movewait > 0) {
        TIMER_movewait--;
        return;
    }

    iNextCell = isNextCell();

	if (bCellValid(iCell) == false)	{
		die(true, false);

		// KILL UNITS WHO SOMEHOW GET INVALID
		if (DEBUGGING)
			logbook("ERROR: Unit became invalid somehow, killed it");

		return;
	}

	if (bCellValid(iNextCell) == false)
		iNextCell = iCell;

	if (bCellValid(iGoalCell) == false)
		iGoalCell = iCell;

    // same cell (no goal specified or something)
    if (iNextCell == iCell) {
        bool bBordered=BORDER_POS(iCellGiveX(iCell), iCellGiveY(iCell));

		// reinforcement stuff happens here...

		if (iTransferType == TRANSFER_DIE) {
			// kill (probably reached border or something)
			die(false, false);
			return;
		}

		// transfer wants to pickup and drop a unit...
		if (iTransferType == TRANSFER_PICKUP) {
			if (iUnitID > -1) {
				// Not yet picked up the unit
                cUnit &unitToPickupOrDrop = unit[iUnitID];
                if (!bPickedUp)	{
                    if (!unitToPickupOrDrop.isValid()) {
                        logbook("THIS UNIT IS NOT VALID ANYMORE");
                        iTransferType = TRANSFER_NONE; // nope...
                        return;
                    }

                    // I believe this statement is always true, as we do not set flag
                    // on unit that is being picked up
                    if (unitToPickupOrDrop.bPickedUp == false) {
						// check where the unit is:
						int iTheGoal = unitToPickupOrDrop.iCell;

						if (iCell == iTheGoal) {
							// when this unit is NOT moving
							if (!unitToPickupOrDrop.isMovingBetweenCells())	{

								bPickedUp = true; // set state in aircraft, that it has picked up a unit

								// so we set the tempHitpoints so the unit 'dissapears' from the map without being
								// really dead.
								unitToPickupOrDrop.iTempHitPoints = unitToPickupOrDrop.iHitPoints;

								// now remove hitpoints (HACK HACK)
								unitToPickupOrDrop.iHitPoints = -1;

								// remove unit from map id (so it wont block other units)
                                map.cellResetIdFromLayer(iCell, MAPID_UNITS);

								// now move air unit to the 'bring target'
								iGoalCell = iBringTarget;

								// smoke puffs when picking up unit
                                int pufX=(pos_x() + getBmpWidth() / 2);
                                int pufY=(pos_y() + getBmpHeight() / 2);

                                PARTICLE_CREATE(pufX, pufY, OBJECT_CARRYPUFF, -1, -1);

								LOG("Pick up unit");
								return;
							}
						} else {
						    // goal/unit not yet reached

						    if (!unitToPickupOrDrop.bPickedUp) {
						        // keep updating goal as long as unit has not been picked up yet.
                                iGoalCell = unitToPickupOrDrop.iCell;
                                iCarryTarget = unitToPickupOrDrop.iCell;
                            } else {
                                // forget about this
                                iGoalCell=iCell;
                                iTransferType=TRANSFER_NONE;
                            }
							return;
						}
					} // is the unit to pick up, not yet picked up yet?
				} // has the aircraft (this) not yet picked up unit?
				else {
					// picked up unit! yay, we are at the destination where we had to
					// bring it... w00t

					if (iCell == iBringTarget) {

						// check if its valid for this unit...
						if (map.occupied(iCell, iUnitID) == false && bBordered) {
							// dump it here
							unitToPickupOrDrop.iCell = iCell;
                            unitToPickupOrDrop.iGoalCell = iCell;
                            unitToPickupOrDrop.poll(); // update cellx and celly
							map.cellSetIdForLayer(iCell, MAPID_UNITS, iUnitID);
                            unitToPickupOrDrop.iHitPoints = unitToPickupOrDrop.iTempHitPoints;
                            unitToPickupOrDrop.iTempHitPoints = -1;
                            unitToPickupOrDrop.TIMER_movewait = 0;
                            unitToPickupOrDrop.TIMER_thinkwait = 0;
                            unitToPickupOrDrop.iCarryAll = -1;

							// match facing of carryall
							unitToPickupOrDrop.iHeadFacing = iHeadFacing;
                            unitToPickupOrDrop.iHeadShouldFace = iHeadShouldFace;
                            unitToPickupOrDrop.iBodyFacing = iBodyFacing;
                            unitToPickupOrDrop.iBodyShouldFace = iBodyShouldFace;
                            unitToPickupOrDrop.iOffsetX=0;
                            unitToPickupOrDrop.iOffsetY=0;

							// clear spot
							map.clear_spot(iCell, units[unitToPickupOrDrop.iType].sight, iPlayer);

							int unitIdOfUnitThatHasBeenPickedUp = iUnitID;

							iUnitID = -1;		 // reset this
							iTempHitPoints = -1; // reset this
							iTransferType = TRANSFER_NONE; // done

							// WHEN DUMPING A HARVESTER IN A REFINERY
							// make it enter the refinery instantly
							if (unit[unitIdOfUnitThatHasBeenPickedUp].iType == HARVESTER) {
                                // valid structure
                                cAbstractStructure *structureUnitWantsToEnter = structure[unit[unitIdOfUnitThatHasBeenPickedUp].iStructureID];
                                if (structureUnitWantsToEnter && structureUnitWantsToEnter->isValid()) {
                                    // when this structure is not occupied
                                    if (structureUnitWantsToEnter->iUnitID < 0)	{
                                        // get in!
                                        structureUnitWantsToEnter->setAnimating(false);
                                        structureUnitWantsToEnter->iUnitID = unitIdOfUnitThatHasBeenPickedUp;  // !!
                                        structureUnitWantsToEnter->setFrame(0);

                                        // store this
                                        unitToPickupOrDrop.iTempHitPoints = unitToPickupOrDrop.iHitPoints;
                                        unitToPickupOrDrop.iHitPoints=-1; // 'kill' unit
                                        unitToPickupOrDrop.iCell = structureUnitWantsToEnter->getCell();
                                        unitToPickupOrDrop.poll();

                                        map.remove_id(unitIdOfUnitThatHasBeenPickedUp, MAPID_UNITS);
                                    } // enter..
                                }
							}

                            int pufX=(pos_x() + getBmpWidth() / 2);
                            int pufY=(pos_y() + getBmpHeight() / 2);
                            PARTICLE_CREATE(pufX, pufY, OBJECT_CARRYPUFF, -1, -1);

						}
						else
						{
							if (DEBUGGING)
								logbook("Could not dump here, searching other spot");

							// find a new spot
							poll();
							int rx = (iCellX - 2) + rnd(5);
							int ry = (iCellY - 2) + rnd(5);
							FIX_BORDER_POS(rx, ry);

							iGoalCell = iCellMakeWhichCanReturnMinusOne(rx, ry);

							iBringTarget = iGoalCell;
							return;
						}
					}
				}
			}
			else
			{
				iTransferType = TRANSFER_NONE; // unit is not valid?
				return;
			}
		}


		// transfer is to create a new unit
		if (iTransferType == TRANSFER_NEW_LEAVE ||
			iTransferType == TRANSFER_NEW_STAY) {
			// bring a new unit

			if (iType == FRIGATE) {

				int iStrucId = map.getCellIdStructuresLayer(iCell);

				if (iStrucId > -1) {
					iGoalCell = iFindCloseBorderCell(iCell);
					iTransferType = TRANSFER_DIE;

                    structure[iStrucId]->setFrame(4); // show package on this structure
					structure[iStrucId]->setAnimating(true); // keep animating
                    ((cStarPort *)structure[iStrucId])->setFrigateDroppedPackage(true);
				}

				return; // override for frigates
			}

			// first check if this cell is clear
			if (map.occupied(iCell, iID) == false && bBordered)	{
				// drop unit
				if (iNewUnitType > -1) {
				    int id=	UNIT_CREATE(iCell, iNewUnitType, iPlayer, true);

                    map.cellSetIdForLayer(iCell, MAPID_UNITS, id);

                    // when it is a TRANSFER_NEW_LEAVE , and AI, we auto-assign team number here
                    // - Normally done in AI.CPP, though from here it is out of our hands.
                    if (iTransferType == TRANSFER_NEW_LEAVE) {
                        if (iPlayer > HUMAN) {
                            if (id > -1) {
                                unit[id].iGroup = rnd(3) + 1; // assign group
                            }
                        }
                    }
                }

				// now make sure this carry-all will not be drawn as having a unit:
				iNewUnitType = -1;

				// depending on transfertype...
				if (iTransferType == TRANSFER_NEW_LEAVE) {
					iTransferType = TRANSFER_DIE;

					// find a new border cell close to us... to die
					iGoalCell = iFindCloseBorderCell(iCell);
					return;
				}
				else if (iTransferType == TRANSFER_NEW_STAY)
				{
					// reset transfertype:
					iTransferType = TRANSFER_NONE;
					return;
				}

			} else {
				// find a new spot for delivery
				poll();
				int rx = (iCellX - 4) + rnd(7);
				int ry = (iCellY - 4) + rnd(7);
				FIX_BORDER_POS(rx, ry);

				iGoalCell = iCellMakeWhichCanReturnMinusOne(rx, ry);
				return;
			}
		}




        // move randomly
        int rx=rnd(game.map_width);
        int ry=rnd(game.map_height);

        iGoalCell = iCellMakeWhichCanReturnMinusOne(rx, ry);
        return;
    }

    // goal cell == next cell (move straight to it)
    TIMER_move++;

    // now move
    int cx = iCellGiveX(iGoalCell);
    int cy = iCellGiveY(iGoalCell);

    int iSlowDown=0;

    // use this when picking something up

	if (iUnitID > -1 || (iTransferType != TRANSFER_DIE && iTransferType != TRANSFER_NONE)) {
		int iLength = ABS_length(iCellX, iCellY, cx, cy);

		if (iType != FRIGATE) {

            if (iLength > 8) {
                iLength = 8;
            } else {
                if (rnd(100) < 5) {
                    int cellType = map.getCellType(iCell);
                    if (cellType == TERRAIN_SAND ||
                        cellType == TERRAIN_SPICE ||
                        cellType == TERRAIN_HILL ||
                        cellType == TERRAIN_SPICEHILL) {
                        int pufX=(pos_x() + getBmpWidth() / 2);
                        int pufY=(pos_y() + getBmpHeight() / 2);
                        PARTICLE_CREATE(pufX, pufY, OBJECT_CARRYPUFF, -1, -1);
                    }
                }
            }

            iSlowDown = 8 - iLength;
		} else {
			if (iLength > 6) {
                iLength = 6;
            }

			iSlowDown = 12 - iLength;

		}
	}

	cPlayerDifficultySettings *difficultySettings = player[iPlayer].getDifficultySettings();

    if (TIMER_move < (difficultySettings->getMoveSpeed(iType) + iSlowDown)) {
        return;
    }

    TIMER_move=0;


    // step 1 : look to the correct direction
    int d = fDegrees(iCellX, iCellY, cx, cy);
    int f = face_angle(d); // get the angle

    iBodyFacing = f;
    iHeadFacing = f;


    float angle = fRadians(iCellX, iCellY, cx, cy);

    // now do some thing to make
    // 1/8 of a cell (2 pixels) per movement
    iOffsetX += cos(angle)*2;
    iOffsetY += sin(angle)*2;

    bool update_me=false;
    // update when to much on the right.
    if (iOffsetX > 31)
    {
        iOffsetX -= 32;
        map.cellResetIdFromLayer(iCell, MAPID_AIR);
        iCell++;
        update_me=true;
    }

    // update when to much on the left
    if (iOffsetX < -31)
    {
        iOffsetX += 32;
        map.cellResetIdFromLayer(iCell, MAPID_AIR);
        iCell--;
        update_me=true;
    }

                  // update when to much up
    if (iOffsetY < -31)
    {
        iOffsetY += 32;
        map.cellResetIdFromLayer(iCell, MAPID_AIR);
        iCell -= MAP_W_MAX;
        update_me=true;
    }

    // update when to much down
    if (iOffsetY > 31)
    {
        iOffsetY -= 32;
        map.cellResetIdFromLayer(iCell, MAPID_AIR);
        iCell += MAP_W_MAX;
        update_me=true;
    }

    if (iCell==iGoalCell)
        iOffsetX=iOffsetY=0;

    if (update_me)
    {
        if (bCellValid(iCell) == false)
        {
			if (DEBUGGING)
			{
				LOG("UNIT : Aircraft : ERROR : Correction applied in cell data");
			}

            if (iCell > (MAX_CELLS-1)) iCell = MAX_CELLS-1;
            if (iCell < 0) iCell = 0;
        }

        map.cellSetIdForLayer(iCell, MAPID_AIR, iID);

        poll();
    }


}

// Carryall-order
//
// Purpose:
// Order a carryall to pickup a unit, or send a new unit (depending on iTransfer)
//
void cUnit::carryall_order(int iuID, int iTransfer, int iBring, int iTpe)
{
	if (iTransferType > -1)
		return; // we cannot do multiple things at a time!!

	if (iTransfer == TRANSFER_NEW_STAY || iTransfer == TRANSFER_NEW_LEAVE) {

		// bring a new unit, depending on the iTransfer the carryall who brings this will be
		// removed after he brought the unit...

		// when iTranfer is 0 or 2, the unit is just created by REINFORCE() and this function
		// sets the target and such.

		iTransferType = iTransfer;

		// Go to this:
		iGoalCell = iBring;
		iBringTarget = iGoalCell;

		// carry is not valid
		iCarryTarget=-1;

		// unit we carry is none
		iUnitID = -1;

		// the unit type...
		iNewUnitType=iTpe;

        bPickedUp=false;


		// DONE!
	} else if (iTransfer == TRANSFER_PICKUP && iuID > -1) {

		// the carryall must pickup the unit, and then bring it to the iBring stuff
		if (unit[iuID].isValid()) {
			iTransferType = iTransfer;

			iGoalCell	 = unit[iuID].iCell; // first go to the target to pick it up
			iCarryTarget = unit[iuID].iCell; // same here...

			iNewUnitType = -1;

			// where to bring it to
			iBringTarget = iBring; // bring

			bPickedUp=false;

			// what unit do we carry? (in case the unit moves away)
			iUnitID = iuID;
		}
	}
}

void cUnit::shoot(int iShootCell) {
    // particles are rendered at the center, so do it here as well
    int iShootX=pos_x() + getBmpWidth() / 2;
    int iShootY=pos_y() + getBmpHeight() / 2;
    int bmp_head = convert_angle(iHeadFacing);

    if (iType == TANK) {
        PARTICLE_CREATE(iShootX, iShootY, OBJECT_TANKSHOOT, -1, bmp_head);
    } else if (iType == SIEGETANK) {
        PARTICLE_CREATE(iShootX, iShootY, OBJECT_SIEGESHOOT, -1, bmp_head);
    }

    create_bullet(units[iType].bullets, iCell, iShootCell, iID, -1);
}

int cUnit::isNextCell() {
    if (isAirbornUnit()) {
        // Aircraft
        if (iGoalCell == iCell)
            return iCell;

        return iGoalCell; // return the goal
    }

    if (iPathIndex < 0) {
        if (iNextCell < 0) {
            iOffsetX=0;
            iOffsetY=0;
            LOG("No pathindex & no nextcell, resetting unit");
            return iCell; // same as our location
        }

        return iNextCell;
    }

    // not valid OR same location
    if (iPath[iPathIndex] < 0) {
        iOffsetX=0;
        iOffsetY=0;
        LOG("No valid iPATH[pathindex]");
        return iCell; // same as our location
    }

    if (iPath[iPathIndex] == iCell) {
        if (iPath[iPathIndex++] > -1) {
            LOG("Odd Pathindex, sollution (HACK HACK)");
            iPathIndex++; // when accidently the index refers to our location, do this
        }
    }

    // now, we are sure it will be another location
    return iPath[iPathIndex];
}

// ouch, who shot me?
void cUnit::think_hit(int iShotUnit, int iShotStructure) {

    // only act when we are doing 'nothing'...
    if (iAction == ACTION_GUARD) {
        if (iShotUnit > -1) {
            UNIT_ORDER_ATTACK(iID, unit[iShotUnit].iCell, iShotUnit, -1, -1);
        }
    }

    if (iPlayer > HUMAN) {
        if (iShotUnit > -1) {
            if (iAction != ACTION_ATTACK) {
                UNIT_ORDER_ATTACK(iID, unit[iShotUnit].iCell, iShotUnit, -1, -1);
            } else {
                // we are attacking, but when target is very far away (out of range?) then we should not attack that but defend
                // ourselves
                int iDestCell = iAttackCell;

                if (iDestCell < 0)
                {
                    if (iAttackUnit > -1)
                        iDestCell = unit[iAttackUnit].iCell;

                    if (iAttackStructure > -1)
                        iDestCell = structure[iAttackStructure]->getCell();

                    if (ABS_length(iCellX, iCellY, iCellGiveX(iDestCell), iCellGiveY(iDestCell)) < units[iType].range)
                    {
                        // within range, do nothing
                    }
                    else
                    {
                        // fire back
                        UNIT_ORDER_ATTACK(iID, unit[iShotUnit].iCell, iShotUnit, -1,-1);
                    }
                }
            }
        }
    }

    // for infantry , infantry turns into soldier and troopers into trooper when on 50% damage.
    if (units[iType].infantry)
    {
        if (iType == INFANTRY || iType == TROOPERS)
        {
            // turn into soldier or trooper when on 50% health
            if (iHitPoints <= (units[iType].hp / 3))
            {
                // leave 2 dead bodies (of 3 ;))

                // turn into single one
                if (iType == INFANTRY)
                    iType = SOLDIER;

                if (iType == TROOPERS)
                    iType = TROOPER;

                iHitPoints = units[iType].hp;

                int half = 16;
                int iDieX=pos_x() + half;
                int iDieY=pos_y() + half;

                PARTICLE_CREATE(iDieX, iDieY, OBJECT_DEADINF01, iPlayer, -1);
                play_sound_id_with_distance(SOUND_DIE01 + rnd(5),
                                            distanceBetweenCellAndCenterOfScreen(iCell));

            }
        }
    }

}
void cUnit::LOG(const char *txt)
{
	// logs unit stuff, but gives unit information
	char msg[512];
	sprintf(msg, "[UNIT[%d]: %s - House %d, iCell = %d, iGoal = %d ] '%s'",iID,  units[iType].name, player[iPlayer].getHouse(), iCell, iGoalCell, txt);
	logbook(msg);
}

void cUnit::think_attack()
{
    poll();


    if (iType == SANDWORM)
    {
        if (iAttackUnit > -1)
        {
            iGoalCell = unit[iAttackUnit].iCell;
            if (iGoalCell == iCell)
            {
                // eat
                unit[iAttackUnit].die(false, false);
                int half = 16;
                int iParX=pos_x() + half;
                int iParY=pos_y() + half;

                PARTICLE_CREATE(iParX, iParY, OBJECT_WORMEAT, -1, -1);
                play_sound_id_with_distance(SOUND_WORM, distanceBetweenCellAndCenterOfScreen(iCell));
                iAction = ACTION_GUARD;
                iAttackUnit=-1;
                bCalculateNewPath=true;
                TIMER_wormeat += rnd(150);
                return;
            }
            else
            {
                int cellType = map.getCellType(unit[iAttackUnit].iCell);
                if (cellType != TERRAIN_SAND &&
                    cellType != TERRAIN_HILL &&
                    cellType != TERRAIN_SPICE &&
                    cellType != TERRAIN_SPICEHILL)
                {

                   iAction = ACTION_GUARD;
                   iAttackUnit=-1;
                }
                else
                {
                    iAction   = ACTION_CHASE;
					bCalculateNewPath = true;
                }


            }

        }
        else
            iAction = ACTION_GUARD;


        return;
    }

    // make sure the goalcell is correct
    if (iAttackUnit > -1)
    {
        iGoalCell = unit[iAttackUnit].iCell;
    }

    if (iAttackStructure > -1)
    {
        if (structure[iAttackStructure])
            iGoalCell = structure[iAttackStructure]->getCell();
        else
        {
            iAttackUnit=-1;
            iAttackStructure=-1;
            iGoalCell=iCell;
            iAction = ACTION_GUARD;
            return;
        }

    }

    if (iAttackCell > -1)
    {
        iGoalCell = iAttackCell;
        LOG("I am attacking a cell.. right");
    }

	if (iAttackUnit > -1) {
        if (unit[iAttackUnit].iHitPoints < 0)
        {
            iAttackUnit=-1;
            iGoalCell=iCell;
            iAction = ACTION_GUARD;
            LOG("Destroyed unit target");
            return;
        }
	}

	if (iAttackStructure > -1) {
	   if (structure[iAttackStructure]->getHitPoints() < 0)
	   {
		   iAttackStructure=-1;
		   iGoalCell=iCell;
		   iAction = ACTION_GUARD;
		   LOG("Destroyed structure target");
		   return;
	   }
	}

    if (iAttackCell > -1)
    {
        if (map.getCellType(iAttackCell) == TERRAIN_BLOOM)
        {
            // this is ok
        }
        else
        {
			if (map.getCellHealth(iAttackCell) < 0)
			{
                // it is destroyed
                iAttackCell=-1;
                iGoalCell=iCell;
                iAction = ACTION_GUARD;
			}
        }
    }
    int iDestX = iCellGiveX(iGoalCell);
    int iDestY = iCellGiveY(iGoalCell);

    // Distance check

    int distance = ABS_length(iCellX, iCellY, iDestX, iDestY);

    if (units[iType].airborn == false)
    {
        if (distance <= units[iType].range && iOffsetX == 0 && iOffsetY == 0)
        {
            // in range , fire and such

            // Facing
            int d = fDegrees(iCellX, iCellY, iCellGiveX(iGoalCell), iCellGiveY(iGoalCell));
            int f = face_angle(d); // get the angle

            // set body facing
            iBodyShouldFace = f;

            // HEAD faces goal directly
            d = fDegrees(iCellX, iCellY, iCellGiveX(iGoalCell), iCellGiveY(iGoalCell));
            f = face_angle(d); // get the angle

            iHeadShouldFace=f;

            if (iBodyShouldFace == iBodyFacing && iHeadShouldFace == iHeadFacing)
            {

                int iGc = iGoalCell;

                if (iType == LAUNCHER || iType == DEVIATOR)
                {
                    if (distance < units[iType].range)
                    {
                        int dx = iCellGiveX(iGc);
                        int dy = iCellGiveY(iGc);

                        int dif = (units[iType].range-distance) / 2;

                        dx -= (dif);
                        dx += rnd(dif*2);

                        dy -= (dif);
                        dy += rnd(dif*2);

                        iGc = iCellMakeWhichCanReturnMinusOne(dx, dy);
                    }
                }

                // facing ok?

                TIMER_attack++;
                if (TIMER_attack >= units[iType].attack_frequency)
                {
                    if (TIMER_attack == units[iType].attack_frequency)
                    {
                        if (iAttackUnit > -1)
                        {
                            if (unit[iAttackUnit].iPlayer == iPlayer)
                            {
                                // unit got converted
                                iAttackUnit=-1;
                                iAction = ACTION_GUARD;
                                return;
                            }
                        }

                        shoot(iGc);
                    }

                    if (units[iType].second_shot == false)
                    {
                        //shoot(iGoalCell);
                        TIMER_attack = 0;
                    }
                    else
                    {
                        if (TIMER_attack > (units[iType].attack_frequency + (units[iType].attack_frequency / 6)))
                        {
                            shoot(iGc);
                            TIMER_attack = 0;
                        }
                    }

                }

            }

        }
        else
        {

            // chase unit
            if (unit[iAttackUnit].iType != SANDWORM)
            {
                iAction   = ACTION_CHASE;
                bCalculateNewPath = true;
            }
            else
            {
                // do not chase sandworms, very ... inconvenient
                iAction = ACTION_GUARD;
				iGoalCell=iCell;
				iNextCell=iCell;
				iPathIndex=-1;
				// clear path
				memset(iPath, -1, sizeof(iPath));
            }

        }
    } // NON AIRBORN UNITS ATTACK THINKING
    else
    {
        if (distance <= units[iType].range)
        {
            // in range , fire and such

            // Facing
            int d = fDegrees(iCellX, iCellY, iCellGiveX(iGoalCell), iCellGiveY(iGoalCell));
            int f = face_angle(d); // get the angle

            // set body facing
            iBodyShouldFace = f;

            // HEAD faces goal directly
            d = fDegrees(iCellX, iCellY, iCellGiveX(iGoalCell), iCellGiveY(iGoalCell));
            f = face_angle(d); // get the angle

            iHeadShouldFace=f;

            if (iBodyShouldFace == iBodyFacing && iHeadShouldFace == iHeadFacing)
            {

                int iGc = iGoalCell;

                // facing ok?

                TIMER_attack++;
                if (TIMER_attack >= units[iType].attack_frequency)
                {
                    if (TIMER_attack == units[iType].attack_frequency)
                    {
                        if (iAttackUnit > -1)
                        {
                            if (unit[iAttackUnit].iPlayer == iPlayer)
                            {
                                // unit got converted
                                iAttackUnit=-1;
                                iAction = ACTION_MOVE;
                                return;
                            }
                        }

                        shoot(iGc);
                    }

                    if (units[iType].second_shot == false)
                    {
                        //shoot(iGoalCell);
                        TIMER_attack = 0;
                    }
                    else
                    {
                        if (TIMER_attack > (units[iType].attack_frequency + (units[iType].attack_frequency / 6)))
                        {
                            shoot(iGc);
                            TIMER_attack = -20;

                            int rx=iCellGiveX(iGc) - 16 + rnd(32);
                            int ry=iCellGiveY(iGc) - 16 + rnd(32);
                            iAttackUnit=-1;
                            iAttackStructure=-1;
                            iAction = ACTION_MOVE;
                            iGoalCell = iCellMakeWhichCanReturnMinusOne(rx, ry);
                        }
                    }

                }

            }

        }
        else
        {
            iAction = ACTION_MOVE;
            iAttackUnit=-1;
            iAttackStructure=-1;
        }
    }
}

s_UnitP& cUnit::getUnitType() {
    return units[iType];
}

// thinking about movement (which is called upon a faster rate)
void cUnit::think_move() {
    if (!isValid()) {
        assert(false && "Expected to have a valid unit calling think_move()");
    }

    if (iTempHitPoints > -1) {
        return;
    }

    if (TIMER_movewait > 0) {
        TIMER_movewait--;
        return;
    }

    // aircraft
    if (isAirbornUnit()) {
        return;
    }

    // when there is a valid goal cell (differs), then we go further
    if (iGoalCell == iCell) {
        iAction = ACTION_GUARD; // do nothing

		// clear path
		memset(iPath, -1, sizeof(iPath));

        return;
    }

    // soldiers
    /*
    if (iType == TROOPER ||
        iType == TROOPERS ||
        iType == INFANTRY ||
        iType == SOLDIER ||
        iType == SABOTEUR)
    {
        think_move_foot();
        return;
    }*/

    // everything from here is wheeled or tracked
    // QUAD, TRIKE, TANK, etc
	iNextCell = isNextCell();

    // Same cell? Get out of here
    if (iNextCell == iCell) {
        // clear path
        memset(iPath, -1, sizeof(iPath));
        iPathIndex=-1;

        // when we do have a different goal, we should get a path:
        if (iGoalCell != iCell)
        {
            char msg[255];
            int iResult = CREATE_PATH(iID, 0); // do not take units into account yet
            sprintf(msg, "Create path ... result = %d",  iResult);
            LOG(msg);

            // On fail:
            if (iPathIndex < 0)
            {
                // simply failed
                if (iResult == -1)
                {
                    logbook("Reason: Could not find path (goal unreachable)");

                    // Check why, is our goal cell occupied?
                    int uID =  map.getCellIdUnitLayer(iGoalCell);
                    int sID =  map.getCellIdStructuresLayer(iGoalCell);

                    // Other unit is on goal cell, do something about it.

                    // uh oh
                    if (uID > -1 && uID != iID)
                    {
                        // occupied, not by self
                        // find a goal cell near to it
                        int iNewGoal = RETURN_CLOSE_GOAL(iGoalCell, iCell, iID);

                        if (iNewGoal == iGoalCell)
                        {
                            // same goal, cant find new, stop
                            iGoalCell= iCell;
							LOG("Could not find alternative goal");
                            iPathIndex=-1;
                            iPathFails=0;
                            return;

                        }
                        else
                        {
                            iGoalCell = iNewGoal;
                            TIMER_movewait=rnd(20);
							LOG("Found alternative goal");
                            return;
                        }
                    }
                    else if (sID > -1 && iStructureID > -1 && iStructureID != sID)
                    {
                        LOG("Want to enter structure, yet ID's do not match");
                        LOG("Resetting structure id and such to redo what i was doing?");
                        iStructureID = -1;
                        iGoalCell=iCell;
                        iPathIndex=-1;
                        iPathFails=0;


                    }
                    else
                    {
                        LOG("Something else blocks path, but goal itself is not occupied.");
                        iGoalCell=iCell;
                        iPathIndex=-1;
                        iPathFails=0;
                        iStructureID=-1;

                        // random move around
                        int iF=UNIT_FREE_AROUND_MOVE(iID);

                        if (iF > -1)
                        {
                            logbook("Order move #1");
                            UNIT_ORDER_MOVE(iID, iF);
                        }
                    }
                }

                if (iResult == -2)
                {
                    // we have a problem here, units/stuff around us blocks the unit
					if (DEBUGGING)
						logbook("Units surround me, what to do?");


                }

                if (iResult == -3)
                {
					if (DEBUGGING)
						logbook("I just failed, ugh");

                    return;
                }

                // We failed
                iPathFails++;

                if (iPathFails > 2)
                {
                    // stop trying
                    iGoalCell=iCell;
                    iPathFails=0;
                    iPathIndex=-1;
                    if (TIMER_movewait <=0 )
                        TIMER_movewait=100;
                }


            }
            else
            {
                //logbook("SUCCES");
            }


        }
        return;
    }

    // Facing
    int d = fDegrees(iCellX, iCellY, iCellGiveX(iNextCell), iCellGiveY(iNextCell));
    int f = face_angle(d); // get the angle

    // set body facing
    iBodyShouldFace = f;

    // HEAD faces goal directly
    d = fDegrees(iCellX, iCellY, iCellGiveX(iGoalCell), iCellGiveY(iGoalCell));
    f = face_angle(d); // get the angle

    iHeadShouldFace=f;


    // check
    bool bOccupied=false;

    int idOfUnitAtNextCell = map.getCellIdUnitLayer(iNextCell);

    if (idOfUnitAtNextCell > -1 &&
        idOfUnitAtNextCell != iID)
    {
        // get it
        int iUID = idOfUnitAtNextCell;

        bOccupied=true;
        // when enemy infantry
        if (units[iType].squish)
            if (unit[iUID].iPlayer != iPlayer)
                if (units[unit[iUID].iType].infantry == true)
                    bOccupied=false;



    }

    // structure is NOT matching our structure ID, then its blocking us
    int idOfStructureAtNextCell = map.getCellIdStructuresLayer(iNextCell);
    if (iStructureID > -1 &&
        idOfStructureAtNextCell != iStructureID &&
        idOfStructureAtNextCell > -1)
        bOccupied=true;


    if (iStructureID < 0 && idOfStructureAtNextCell > -1)
    {
        bOccupied = true;
    }

    if (iStructureID > -1 && idOfStructureAtNextCell == iStructureID)
    {
        // we may enter, only if its empty
        if (structure[iStructureID]->iUnitID > -1)
        {
            // already occupied, find alternative
            cStructureUtils structureUtils;
            int	iNewID = structureUtils.findClosestStructureTypeWhereNoUnitIsHeadingToComparedToCell(iCell,
                                                                                                        structure[iStructureID]->getType(),
                                                                                                        &player[iPlayer]);

            if (iNewID > -1 && iNewID != iStructureID)
            {
                iStructureID = iNewID;
                move_to(structure[iNewID]->getCell(), iNewID, -1);
            }
            else
            {
                iNextCell=iCell;
                TIMER_movewait=100; // we wait
                return;
            }
        }
    }

    // When not infantry:
    int cellTypeAtNextCell = map.getCellType(iNextCell);
    if (units[iType].infantry == false)
        if (cellTypeAtNextCell == TERRAIN_MOUNTAIN)
            bOccupied=true;

    if (cellTypeAtNextCell == TERRAIN_WALL)
            bOccupied=true;

    if (iType == SANDWORM)
    {
        bOccupied=false;
    }

    // check if id is not possessed already:
    if (bOccupied)
    {
        // it is taken by someone else
        if (iNextCell == iGoalCell)
        {
            // it is our goal cell, modify goal cell
            iGoalCell=iCell;

            memset(iPath, -1, sizeof(iPath));
            iPathIndex=-1;
            return;
        }
        else if (idOfStructureAtNextCell > -1)
        {
            // new path, structure obstructs the path (only when it has been built AFTER
            // we created our path)
            memset(iPath, -1, sizeof(iPath));
            iPathIndex=-1;
            TIMER_movewait=100;
			iNextCell=iCell;
        }
        else
        {
            // From here, a unit is standing in our way. First check if this unit will
            // move. If so, we wait until it has moved.
            int uID = idOfUnitAtNextCell;

            // Wait when the obstacle is moving, perhaps it will clear our way
            if (unit[uID].TIMER_movewait <= 0 && unit[uID].iGoalCell != unit[uID].iCell)
            {
                // wait!
                TIMER_movewait=50;
                return;
            }
            else
            {
                // the obstacle will not move, try to go to path

                // create new path
                iNextCell=iCell;

                memset(iPath, -1, sizeof(iPath));
                iPathIndex=-1;
                TIMER_movewait=100;

                /*
                // on fail
                if (iPathIndex < 0)
                {
                    // We failed
                    iPathFails++;

                    if (iPathFails > 2)
                    {
                        // stop trying
                        iGoalCell=iCell;
                        iPathFails=0;
                        iPathIndex=-1;
                        TIMER_movewait=100;
                    }
                }*/

            }
        }

        return;
    }
	else
	{
		// we wanted to enter this structure, so do it immediately (else we just seem to
		// drive over the structure, which looks odd!
		if (iStructureID > -1 &&
            idOfStructureAtNextCell == iStructureID &&
            idOfStructureAtNextCell > -1)
		{

			// when this structure is not occupied
			if (structure[iStructureID]->iUnitID < 0) {
				// get in!
				structure[iStructureID]->setAnimating(false);
				structure[iStructureID]->iUnitID = iID;  // !!
				structure[iStructureID]->setFrame(0);

				// store this
				iTempHitPoints = iHitPoints;
				iHitPoints=-1; // 'kill' unit

				char msg[255];
				sprintf(msg, "MY ID = %d", iID);
				logbook(msg);

				map.remove_id(iID, MAPID_UNITS);
                iCell = structure[iStructureID]->getCell();

				LOG("-> Enter structure #3");
			} // enter..
			else
			{
				// looks like it is occupied

			}
		}
	}

    poll();

    if (iBodyShouldFace == iBodyFacing)
    {

    // When we should move:
    int bToLeft=-1;         // 0 = go left, 1 = go right
    int bToDown=-1;         // 0 = go down, 1 = go up

    int iNextX=iCellGiveX(iNextCell);
    int iNextY=iCellGiveY(iNextCell);

    // Compare X, Y coordinates
    if (iNextX < iCellX)
        bToLeft=0; // we head to the left

    if (iNextX > iCellX)
        bToLeft=1; // we head to the right

    // we go up
    if (iNextY < iCellY)
        bToDown=1;

    // we go down
    if (iNextY > iCellY)
        bToDown=0;



    // done, since we already have the other stuff set
    TIMER_move++;

    int iSlowDown=1;

    // Influenced by the terrain type
    int cellType = map.getCellType(iCell);
    if (cellType == TERRAIN_SAND)
        iSlowDown=2;

    // mountain is very slow
    if (cellType == TERRAIN_MOUNTAIN)
        iSlowDown=5;

    if (cellType == TERRAIN_HILL)
        iSlowDown=3;

    if (cellType == TERRAIN_SPICEHILL)
        iSlowDown=3;

    if (cellType == TERRAIN_ROCK)
        iSlowDown=1;

    if (cellType == TERRAIN_SLAB)
        iSlowDown=0;


    cPlayerDifficultySettings *difficultySettings = player[iPlayer].getDifficultySettings();
    if (TIMER_move < ((difficultySettings->getMoveSpeed(iType))+iSlowDown)) {
        return; // get out
    }

    TIMER_move=0; // reset to 0

    // get it

    // from here on, set the map id, so no other unit can take its place
    if (iType != SANDWORM) {
        // note, no AIRBORN here
        map.cellSetIdForLayer(iNextCell, MAPID_UNITS, iID);
    } else {
        map.cellSetIdForLayer(iNextCell, MAPID_WORMS, iID);

        // when sandworm, add particle stuff
        int iOffX = abs(iOffsetX);
        int iOffY = abs(iOffsetY);
        if ((iOffX == 8 || iOffX == 16 || iOffX == 24 || iOffX == 32) ||
            (iOffY == 8 || iOffY == 16 || iOffY == 24 || iOffY == 32))
        {
            int half = 16;
            int iParX=pos_x() + half;
            int iParY=pos_y() + half;

            PARTICLE_CREATE(iParX, iParY, OBJECT_WORMTRAIL, -1, -1);
        }
    }


    // 100% on cell.
    if (iOffsetX == 0 && iOffsetY == 0)
    {
        int half = 16;
        int iParX=pos_x() + half;
        int iParY=pos_y() + half;


        // add particle tracks

        if (cellType != TERRAIN_ROCK &&
            cellType != TERRAIN_MOUNTAIN &&
            cellType != TERRAIN_WALL &&
            cellType != TERRAIN_SLAB &&
            units[iType].infantry == false && iType != SANDWORM)
        {

            // horizontal when only going horizontal
            if (bToLeft > -1 && bToDown < 0)
                PARTICLE_CREATE(iParX, iParY, TRACK_HOR, -1, -1);

            // vertical, when only going vertical
            if (bToDown > -1 && bToLeft < 0)
                PARTICLE_CREATE(iParX, iParY, TRACK_VER, -1, -1);

            // diagonal when going both ways
            if (bToDown > -1 && bToLeft > -1)
            {
                if (bToDown == 0)
                {
                    // going up
                    if (bToLeft == 1)
                        PARTICLE_CREATE(iParX, iParY, TRACK_DIA, -1, -1);
                    else
                        PARTICLE_CREATE(iParX, iParY, TRACK_DIA2, -1, -1);

                }
                else
                {
                    if (bToLeft == 0)
                        PARTICLE_CREATE(iParX, iParY, TRACK_DIA, -1, -1);
                    else
                        PARTICLE_CREATE(iParX, iParY, TRACK_DIA2, -1, -1);
                }

            }
        }
    }


    // movement in pixels
    if (bToLeft == 0)
        iOffsetX--;
    else if (bToLeft == 1)
        iOffsetX++;

    if (bToDown == 0)
        iOffsetY++;
    else if (bToDown == 1)
        iOffsetY--;

    // When moving, infantry has some animation
    if (units[iType].infantry)
    {
        TIMER_frame++;

        if (TIMER_frame > 3)
        {

            iFrame++;
            if (iFrame > 3)
                iFrame=0;

            TIMER_frame=0;
        }
    }

    // take care of this:
    if (iOffsetX > 31 || iOffsetX < -31 || iOffsetY < -31 || iOffsetY > 31)
    {
        // when we are chasing, we now set on attack...
        if (iAction == ACTION_CHASE)
        {
            iAction = ACTION_ATTACK;
            // next time we think, will be checking for distance, etc
        }

        // movement to cell complete
        if (iType == SANDWORM) {
            map.cellResetIdFromLayer(iCell, MAPID_WORMS);
        } else {
            map.cellResetIdFromLayer(iCell, MAPID_UNITS);
        }


        /*
        int iUID = map.cell[iNextCell].id[MAPID_UNITS];

        if (iUID > -1)
        {
        // when we may squish
        if (units[iType].squish)
            if (unit[iUID].iPlayer != iPlayer) // other player
               if (units[unit[iUID].iType].infantry == true) // and it is squishable infantry
        {
            // we squish them and they are dead!
            unit[iUID].die(false);
        }
        }*/


        iCell=iNextCell;
        iOffsetX=0.0f;
        iOffsetY=0.0f;
        iPathIndex++;
        iPathFails=0; // we change this to 0... every cell

		// POLL now
		poll();

        // quick scan for infantry we squish
        for (int iq=0; iq < MAX_UNITS; iq++) {
            if (unit[iq].isValid())
            	if (unit[iq].iType != SANDWORM) // sandworms do not squish (TODO: units[unit[iq].iType].canSquish)
                if (units[unit[iq].iType].infantry)
                    if (unit[iq].iPlayer != iPlayer)
                        if (unit[iq].iCell == iCell)
                        {
                            // die
                            unit[iq].die(false, true);
                        }

        }


        map.clear_spot(iCell, units[iType].sight, iPlayer);

        // The goal did change probably, or something else forces us to reconsider
        if (bCalculateNewPath)
        {
            logbook("SHOULD CALCULATE NEW PATH");
            iPathIndex=-1;
            memset(iPath, -1, sizeof(iPath));



            //logbook("C");
            //CREATE_PATH(iID, 0);
        }

		if (iCarryAll > -1)
		{
			logbook("A carry all is after me...");
			iPathIndex=-1;
			memset(iPath, -1, sizeof(iPath));
			TIMER_movewait=9999;
			TIMER_thinkwait=9999;
			return; // wait a second will ya!
		}

        // Just arrived at goal cell
        if (iCell == iGoalCell)
        {
            // when this was a harvester, going back to a refinery...
            if (iType == HARVESTER)
            {
                // structure id match!
				if (iStructureID > -1) {
                    int idOfStructureAtCell = map.getCellIdStructuresLayer(iCell);

                    if (iStructureID == idOfStructureAtCell)
                    {
                        logbook("Enter structure");
                        // when this structure is not occupied
                        if (structure[iStructureID]->iUnitID < 0)
                        {
                            // get in!
                            structure[iStructureID]->setAnimating(false);
                            structure[iStructureID]->iUnitID = iID;
                            structure[iStructureID]->setFrame(0);

                            // store this
                            iTempHitPoints = iHitPoints;
                            iHitPoints=-1; // 'kill' unit
                        }
                    }
                }

            }
        }

        TIMER_movewait=2 + ((units[iType].speed+iSlowDown)*3);
    }



    }


}

bool cUnit::isInfantryUnit() {
    return units[iType].infantry;
}

cUnit::cUnit() {
    dimensions = nullptr;
}

/**
 * Poor man solution to frequently update the dimensions of unit, better would be using events?
 * (onMove, onViewportMove, onViewportZoom?)
 */
void cUnit::think_position() {
    // keep updating dimensions
    dimensions->move(draw_x(), draw_y());
    dimensions->resize(mapCamera->factorZoomLevel(getBmpWidth()),
                       mapCamera->factorZoomLevel(getBmpHeight()));

}

bool cUnit::isMovingBetweenCells() {
    return iOffsetX != 0 || iOffsetY != 0;
}

// return new valid ID
int UNIT_NEW()
{
    for (int i=0; i < MAX_UNITS; i++)
        if (unit[i].isValid() == false)
            return i;

    return -1; // NONE
}

// removes unit ID
int UNIT_REMOVE(int iID)
{
    // STEP 1: make it invalid
    unit[iID].init(iID); // initialize it

    // STEP 2: remove from cell map data
    map.remove_id(iID, MAPID_UNITS);
    map.remove_id(iID, MAPID_WORMS);
    map.remove_id(iID, MAPID_AIR);

    return 0; // success
}


int UNIT_CREATE(int iCll, int iTpe, int iPlyr, bool bOnStart) {
	if (bCellValid(iCll) == false) {
		logbook("UNIT_CREATE: Invalid cell as param");
		return -1;
	}

	int mapIdIndex = MAPID_UNITS;
	if (units[iTpe].airborn) mapIdIndex = MAPID_AIR;
	if (iTpe == SANDWORM) mapIdIndex = MAPID_WORMS;

	// check if unit already exists on location
	if (map.cellGetIdFromLayer(iCll, mapIdIndex) > -1) {
		return -1; // cannot place unit
	}

    // check if placed on invalid terrain type
    int cellType = map.getCellType(iCll);
    if (iTpe == SANDWORM) {
        if (cellType != TERRAIN_SAND &&
            cellType != TERRAIN_SPICE &&
            cellType != TERRAIN_HILL &&
            cellType != TERRAIN_SPICEHILL)
            return -1;
    }


    // not airborn, and not infantry, may not be placed on walls and mountains.
    if (!units[iTpe].infantry && !units[iTpe].airborn) {
		if (cellType == TERRAIN_MOUNTAIN || cellType == TERRAIN_WALL) {
			return -1;
		}
    }

    int iNewId = UNIT_NEW();

    if (iNewId < 0)
	{
		logbook("UNIT_CREATE:Could not find new unit index");
        return -1;
	}

    cUnit &newUnit = unit[iNewId];
    newUnit.init(iNewId);

    newUnit.iCell=iCll;
    newUnit.iBodyFacing=rnd(8);
    newUnit.iHeadFacing=rnd(8);

    newUnit.iBodyShouldFace = newUnit.iBodyFacing;
    newUnit.iHeadShouldFace = newUnit.iHeadFacing;

    newUnit.iType = iTpe;
    //unit[iNewId].iHitPoints = rnd(units[iTpe].hp);
    newUnit.iHitPoints = units[iTpe].hp;
    newUnit.iGoalCell = iCll;

    newUnit.bSelected=false;

    newUnit.TIMER_bored = rnd(3000);
    newUnit.TIMER_guard = -20 + rnd(70);
    newUnit.recreateDimensions();


    if (iPlyr > 0 && iPlyr < AI_WORM && units[iTpe].airborn == false && bOnStart == false)
    {
        int iF=UNIT_FREE_AROUND_MOVE(iNewId);

        if (iF > -1)
        {
            logbook("Order move #2");
            UNIT_ORDER_MOVE(iNewId, iF);
        }
    }


    // set player id:
    int iPlayer = iPlyr;
    if (iTpe == SANDWORM) iPlayer = AI_WORM;

    newUnit.iPlayer = iPlayer;
    // Put on map too!:
    map.cellSetIdForLayer(iCll, mapIdIndex, iNewId);

    if (iTpe == SANDWORM) {
        // sandworms are controlled by the last player
        map.cellSetIdForLayer(iCll, MAPID_WORMS, iNewId);
    } else if (
            iTpe != ORNITHOPTER && iTpe != FRIGATE && iTpe != CARRYALL // not airborn
            ) {
        newUnit.iPlayer = iPlyr;
    } else {
        // aircraft
        newUnit.iPlayer = iPlyr;
    }

	newUnit.poll();
    map.clear_spot(iCll, units[iTpe].sight, iPlyr);

    return iNewId;
}

/*
  Pathfinder

  Last revision: 19/02/2006

  The pathfinder will first eliminate any possibility that it will fail.
  It will check if the unit is free to move, if the timer is set correctly
  and so forth.

  Then the actualy FDS path finder starts. Which will output a 'reversed' tracable
  path.

  Eventually this path is converted back to a waypoint string for units, and also
  optimized (skipping attaching cells which are reachable and closer to goal or further on
  path string).

  Return possibilities:
  -1 = FAIL (goalcell = cell, or cannot find path)
  -2 = Cannot move, surrounded by a lot of units
  -3 = Too many paths created
  -4 = Offset is not 0

  */
int CREATE_PATH(int iID, int iPathCountUnits)
{

    // Too many paths where created , so we wait a little.
    if (game.paths_created > 40)
    {
        unit[iID].TIMER_movewait = (50 + rnd(50));
        return -3;
    }

    int iCell = unit[iID].iCell; // current cell

    // do not start calculating anything before we are on 0,0 x,y wise on a cell
    if (unit[iID].iOffsetX != 0 || unit[iID].iOffsetY != 0)
    {
        return -4; // no calculation before we are straight on a cell baby
    }

    // when all around the unit there is no space, dont even bother
    if (map.occupied(CELL_LEFT(iCell), iID)    &&
        map.occupied(CELL_RIGHT(iCell), iID)   &&
        map.occupied(CELL_ABOVE(iCell), iID)   &&
        map.occupied(CELL_UNDER(iCell), iID)   &&
        map.occupied(CELL_L_LEFT(iCell), iID)  &&
        map.occupied(CELL_L_RIGHT(iCell), iID) &&
        map.occupied(CELL_U_RIGHT(iCell), iID) &&
        map.occupied(CELL_U_LEFT(iCell), iID))
    {
        unit[iID].TIMER_movewait = 30 + rnd(50);
       return -2;
    }

    // Now start create path

    // Clear unit path settings (index & path string)
	//for (int i=0; i < MAX_PATH_SIZE; i++)
	//	unit[iID].iPath[i] = -1;

    memset(unit[iID].iPath, -1, sizeof(unit[iID].iPath));

    unit[iID].iPathIndex=-1;

    // When the goal == cell, then skip.
    if (unit[iID].iCell == unit[iID].iGoalCell)
    {
        logbook("ODD: The goal = cell?");
        return -1;
    }


    // Search around a cell:
    int cx, cy, the_cll, ex, ey;
    int goal_cell = unit[iID].iGoalCell;
    int controller = unit[iID].iPlayer;

    game.paths_created++;
    memset(temp_map, -1, sizeof(temp_map));

    /*
    for (int i=0; i < MAX_CELLS; i++)
        temp_map[i].state = CLOSED;*/

    the_cll=-1;
    ex=-1;
    ey=-1;
    cx = iCellGiveX(iCell);
    cy = iCellGiveY(iCell);

    // set very first... our start cell
    temp_map[iCell].cost = ABS_length(cx, cy, iCellGiveX(goal_cell), iCellGiveY(goal_cell));
    temp_map[iCell].parent = -1;
    temp_map[iCell].state = CLOSED;

    bool valid         = true;
    bool succes        = false;
    bool found_one     = false;

    int sx, sy;
    double cost        = -1;
    bool is_worm       = false;

    // Sandworm
    if (unit[iID].iType == SANDWORM)
        is_worm=true;

    // WHILE VALID TO RUN THIS LOOP
    while (valid)
    {
        // iCell reached Goal Cell
        if (iCell == goal_cell)
        {
            valid=false;
            succes=true;
            break;
        }

        if (unit[iID].iStructureID > -1 || unit[iID].iAttackStructure > -1)
        {
            int idOfStructureAtCell = map.cellGetIdFromLayer(iCell, MAPID_STRUCTURES);
            if (unit[iID].iStructureID > -1)
            if (idOfStructureAtCell == unit[iID].iStructureID)
            {
                valid=false;
                succes=true;
                logbook("Found structure ID");
                break;
            }

            if (unit[iID].iAttackStructure > -1)
            if (idOfStructureAtCell == unit[iID].iAttackStructure)
            {
                valid=false;
                succes=true;
                logbook("Found attack structure ID");
                break;
            }
        }

        cx = iCellGiveX(iCell);
        cy = iCellGiveY(iCell);

        // starting position is cx-1 and cy-1
        sx = cx - 1;
        sy = cy - 1;

        // check for not going under zero
        ex = cx+1;
        ey = cy+1;

        // boundries
        FIX_BORDER_POS(sx, sy);
        FIX_BORDER_POS(ex, ey);

        if (ex <= cx)
            logbook("CX = EX");
        if (ey <= cy)
            logbook("CY = EY");

        cost=999999999;
        the_cll=-1;
        found_one=false;

        // go around the cell we are checking!
        bool bail_out=false;

        // circle around cell X wise
        for (cx = sx; cx <= ex; cx++)
        {
          // circle around cell Y wise
            for (cy = sy; cy <= ey; cy++)
            {
                // only check the 'cell' that is NOT the current cell.
                int cll = iCellMakeWhichCanReturnMinusOne(cx, cy);

                // skip invalid cell
                if (cll < 0)
                    continue;

                // DO NOT CHECK SELF
                if (cll == iCell)
                    continue;

                // Determine if its a good cell to use or not:
                bool good = false; // not good

                // not a sandworm
                int cellType = map.getCellType(cll);
                if (is_worm == false)
                {
                    // Step by step determine if its good
                    // 2 fases:
                    // 1 -> Occupation by unit/structures
                    // 2 -> Occupation by terrain (but only when it is visible, since we do not want to have an
                    //      advantage or some unknowingly super intelligence by units for unknown territories!)
                    int idOfUnitAtCell = map.getCellIdUnitLayer(cll);
                    int idOfStructureAtCell = map.getCellIdStructuresLayer(cll);

                    if (idOfUnitAtCell == -1 && idOfStructureAtCell == -1)
                    {
                        // there is nothing on this cell, that is good
                        good=true;
                    }

                    if (idOfStructureAtCell > -1)
                    {
                        // when the cell is a structure, and it is the structure we want to attack, it is good


                        if (unit[iID].iAttackStructure > -1)
                            if (idOfStructureAtCell == unit[iID].iAttackStructure)
                            good=true;

                        if (unit[iID].iStructureID > -1)
                            if (idOfStructureAtCell == unit[iID].iStructureID)
                            good=true;

                    }

					// blocked by other then our own unit
					if (idOfUnitAtCell > -1)
                    {
                        // occupied by a unit
                        if (idOfUnitAtCell != iID)
						{
                            int iUID = idOfUnitAtCell;

							if (iPathCountUnits!=0)
								   if (iPathCountUnits <= 0)
								   {
									   if (idOfUnitAtCell != -1 &&
                                           idOfUnitAtCell != iID) // occupied by a unit
										   good=false;
								   }


                            if (unit[iUID].iPlayer != unit[iID].iPlayer)
                                if (units[unit[iUID].iType].infantry &&
                                    units[unit[iID].iType].squish) // and the current unit can squish
                                    good = true; // its infantry we want to run over, so don't be bothered!

                            //good=false; // it is not good, other unit blocks
						} else {
							good= true;
						}
                    }


                    // is not visible, always good (since we dont know yet if its blocked!)
                    if (mapUtils->isCellVisibleForPlayerId(controller, cll) == false) {
                        good=true;
                    } else {
                        // walls stop us
                        if (cellType == TERRAIN_WALL) {
                            good = false;
                        }

                        // When we are infantry, we move through mountains. However, normal units do not
                        if (units[unit[iID].iType].infantry == false) {
                            if (cellType == TERRAIN_MOUNTAIN) {
                                good=false;
                            }
                        }
                    }
             }
             else if (is_worm)
             {
                 // when not on sand, on spice or on sandhill, it is BAD
                 if (cellType == TERRAIN_SAND ||
                     cellType == TERRAIN_SPICE ||
                     cellType == TERRAIN_HILL ||
                     cellType == TERRAIN_SPICEHILL)
                 {
                     good=true;
                 }
                 else
                     good=false;
                 //good=true;
             }

             if (cll == goal_cell)
                 good=true;


             // it is the goal cell
             if (cll == goal_cell && good)
             {
                 the_cll = cll;
                 cost = 0;
                 succes=true;
                 found_one=true;
                 bail_out=true;
                 logbook("CREATE_PATH: Found the goal cell, succes, bailing out");
                 break;
             }

             // when the cell (the attached one) is NOT the cell we are on and
             // the cell is CLOSED (not checked yet)
             if (cll != iCell &&                            // not checking on our own
                 temp_map[cll].state == CLOSED &&          // and is closed (else its not valid to check)
                 (good)) // and its not occupied (but may be occupied by own id!

             {
                 int gcx = iCellGiveX(goal_cell);
                 int gcy = iCellGiveY(goal_cell);

                 // calculate the cost
                 double d_cost = ABS_length(cx, cy, gcx, gcy) + temp_map[cll].cost;

                 // when the cost is lower then we had before
                 if (d_cost < cost)
                 {
                     // when the cost is lower then the previous cost, then we set the new cost and we set the cell
                     the_cll=cll;
                     cost=d_cost;
                     // found a new cell, now decrease ipathcountunits
                     iPathCountUnits--;
                 }


/*
                 char msg[255];
                 sprintf(msg, "Waypoint found : cell %d - goalcell = %d", cll, goal_cell);
                 logbook(msg);*/

             } // END OF LOOP #2
   } // Y thingy

   // bail out
   if (bail_out)
   {
       //logbook("BAIL");
       break;
   }

 } // X thingy

 // When found a new c(e)ll;
     if (the_cll > -1)
     {
       // Open this one, so we do not check it again
       temp_map[the_cll].state = OPEN;
       temp_map[the_cll].parent = iCell;
       temp_map[the_cll].cost = cost;

       // Now set c to the cll
       iCell = the_cll;
       found_one=true;

       // DEBUG:
       // draws a SLAB when the cell is assigned for our path
       //map.cell[c].type = TERRAIN_SPICE;
       //map.cell[c].tile = 0;

       if (iCell == goal_cell)
       {
        valid=false;
        succes=true;
       }
     }

     if (found_one == false)
     {
       valid=false;
       succes=false;
       logbook("FAILED TO CREATE PATH - nothing found to continue");
       break;
     }

 } // valid to run loop (and try to create a path)

 if (succes)
 {
  // read path!
  int temp_path[MAX_PATH_SIZE];

  //logbook("NEW PATH FOUND");
  memset(temp_path, -1, sizeof(temp_path));
  //for (int copy=0; copy < PATH_MAX; copy++)
    // temp_path[copy] = -1;

  bool cp=true;

  int sc = iCell;
  int pi = 0;

  temp_path[pi] = sc;
  pi++;

  // while we should create a path
  while (cp)
  {
    int tmp = temp_map[sc].parent;
    if (tmp > -1)
    {
        // found terminator (PARENT=CURRENT)
        if (tmp == sc)
        {
        cp=false;
        continue;
        }
        else
        {
         temp_path[pi] = tmp;
         sc = temp_map[sc].parent;
         pi++;
        }
    }
    else
     cp=false;

   if (pi >= MAX_PATH_SIZE)
     cp=false;

   if (sc==unit[iID].iCell)
     cp=false;
 }

 // reverse
 int z=MAX_PATH_SIZE-1;
 int a=0;
 int iPrevCell=-1;

 while (z > -1)
 {
   if (temp_path[z] > -1)
   {
     // check if any other cell of temp_path also borders to the previous given cell, as that will safe us time
     if (iPrevCell > -1)
     {
         int iGoodZ=-1;

         for (int sz=z; sz > 0; sz--)
         {
             if (temp_path[sz] > -1)
             {

                 if (CELL_BORDERS(iPrevCell, temp_path[sz]))
                 {
                     iGoodZ=sz;
                 }
                 //if (ABS_length(iCellGiveX(iPrevCell), iCellGiveY(iPrevCell), iCellGiveX(temp_path[sz]), iCellGiveY(temp_path[sz])) <= 1)
                   //  iGoodZ=sz;
             }
             else
                 break;
         }

         if (iGoodZ < z && iGoodZ > -1)
             z = iGoodZ;
     }

     unit[iID].iPath[a] = temp_path[z];
     iPrevCell = temp_path[z];
     a++;
   }
   z--;
 }

 // optimize path
 //nextcell=cell;
 unit[iID].iPathIndex=1;

 // take the closest bordering cell as 'far' away to start with
 for (int i=1; i < MAX_PATH_SIZE; i++)
 {
	if (unit[iID].iPath[i] > -1)
	{
		if (CELL_BORDERS(unit[iID].iCell, unit[iID].iPath[i]))
			unit[iID].iPathIndex = i;
	}
 }



 /*
 // debug debug
 for (int i=0; i <	MAX_PATH_SIZE; i++)
 {
	if (unit[iID].iPath[i] > -1)
	{
		char msg[256];
		sprintf(msg, "WAYPOINT %d = %d ", iID, i, unit[iID].iPath[i]);
		unit[iID].LOG(msg);
	}
 }*/


 unit[iID].poll();
 unit[iID].bCalculateNewPath=false;


 //logbook("SUCCES");
 return 0; // succes!

 }
 else
 {

   // perhaps we can get closer when we DO take units into account?
   //path_id=-1;
 }

  logbook("CREATE_PATH: Failed to create path!");
  return -1;
}


// find
int RETURN_CLOSE_GOAL(int iCll, int iMyCell, int iID)
{
    //
    int iSize=1;
    int iStartX=iCellGiveX(iCll)-iSize;
    int iStartY=iCellGiveY(iCll)-iSize;
    int iEndX = iCellGiveX(iCll) + iSize;
    int iEndY = iCellGiveX(iCll) + iSize;

    float dDistance=9999;

    int ix=iCellGiveX(iMyCell);
    int iy=iCellGiveY(iMyCell);

    bool bSearch=true;

    int iTheClosest=-1;

    while (bSearch)
    {
        iStartX=iCellGiveX(iCll)-iSize;
        iStartY=iCellGiveY(iCll)-iSize;
        iEndX = iCellGiveX(iCll) + iSize;
        iEndY = iCellGiveY(iCll) + iSize;

        // Fix boundries
        FIX_BORDER_POS(iStartX, iStartY);
        FIX_BORDER_POS(iEndX, iEndY);

        // search
        for (int iSX=iStartX; iSX < iEndX; iSX++)
            for (int iSY=iStartY; iSY < iEndY; iSY++)
            {
                // find an empty cell
                int cll = iCellMakeWhichCanReturnMinusOne(iSX, iSY);

                float dDistance2 = ABS_length(iSX, iSY, ix, iy);

                int idOfStructureAtCell = map.getCellIdStructuresLayer(cll);
                int idOfUnitAtCell = map.getCellIdUnitLayer(cll);

                if ((idOfStructureAtCell < 0) && (idOfUnitAtCell < 0)) // no unit or structure at cell
                {
                    // depending on unit type, do not choose walls (or mountains)
                    int cellType = map.getCellType(cll);
                    if (units[unit[iID].iType].infantry)
                    {
                        if (cellType == TERRAIN_MOUNTAIN)
                            continue; // do not use this one
                    }

                    if (cellType == TERRAIN_WALL)
                            continue; // do not use this one

                    if (dDistance2 < dDistance)
                    {
                        dDistance = dDistance2;
                        iTheClosest=cll;
                    }
                }
            }

         if (iTheClosest > -1)
             return iTheClosest;

        iSize++;

        if (iSize > 9)
        {
            bSearch=false;
            break; // get out
        }
    }

    // fail
    return iCll;
}



int CLOSE_SPICE_BLOOM(int iCell) {

    // closest spicebloom
	if (rnd(100) < 40)	{
        int iDistance=16;

        if (iCell < 0) {
            iDistance=9999;
            iCell = iCellMakeWhichCanReturnMinusOne(32,32);
        }

        int cx, cy;
        int iBloom=-1;
        cx = iCellGiveX(iCell);
        cy = iCellGiveY(iCell);

        for (int i=0; i < MAX_CELLS; i++) {
            int cellType = map.getCellType(i);
            if (cellType == TERRAIN_BLOOM)
            {
                int d = ABS_length(cx, cy, iCellGiveX(i), iCellGiveY(i));

                if (d < iDistance)
                {
                    iBloom=i;
                    iDistance=d;
                }
            }
        }

        // when finished, return bloom
        return iBloom;
	}

	int iTargets[10];
	memset(iTargets, -1, sizeof(iTargets));
	int iT=0;

	for (int i=0; i < MAX_CELLS; i++) {
        int cellType = map.getCellType(i);
        if (cellType == TERRAIN_BLOOM)
        {
            iTargets[iT] = i;
            iT++;
        }
    }

	// when finished, return bloom
	return iTargets[rnd(iT)];
}

int UNIT_find_harvest_spot(int id)
{
  // finds the closest harvest spot
  unit[id].poll();
  int cx = unit[id].iCellX;
  int cy = unit[id].iCellY;

  int TargetSpice=-1;
  int TargetSpiceHill=-1;

  // distances
  int TargetSpiceDistance=40;
  int TargetSpiceHillDistance=40;


  for (int i=0; i < (MAX_CELLS); i++)
    if (map.getCellCredits(i) > 0 && i != unit[id].iCell)
    {
      // check if its not out of reach
      int dx = iCellGiveX(i);
      int dy = iCellGiveY(i);

      // skip bordered ones
      if (BORDER_POS(dx, dy) == false)
          continue;

      /*
	  if (dx <= 1) continue;
	  if (dy <= 1) continue;

      if (dx >= (game.map_width-1))
        continue;

      if (dy >= (game.map_height-1))
        continue;*/

      int idOfUnitAtCell = map.getCellIdUnitLayer(i);
      if (idOfUnitAtCell > -1)
        continue;

      if (map.occupied(i, id))
          continue; // occupied

      int d = ABS_length(cx, cy, iCellGiveX(i), iCellGiveY(i));

        int cellType = map.getCellType(i);
        if (cellType == TERRAIN_SPICE)
	  {
		  if (d < TargetSpiceDistance)
		  {
			  TargetSpice=i;
			  TargetSpiceDistance=d; // update distance
		  }
	  }
	  else if (cellType == TERRAIN_SPICEHILL)
	  {
		  if (d < TargetSpiceHillDistance)
		  {
			  TargetSpiceHill=i;
			  TargetSpiceHillDistance=d; // update distance
		  }
	  }

	  if (TargetSpiceHillDistance < 3 && TargetSpiceDistance < 3)
		  break;

	}

	// when distance is greater then 3 (meaning 'far' away)
	if (TargetSpiceHillDistance >= 3 && TargetSpiceDistance >= 3)
	{
		return TargetSpice;
	}
	else
	{
		// when both are close, prefer spice hill
		if (TargetSpiceHill > -1 && (TargetSpiceHillDistance <= TargetSpiceDistance))
			return TargetSpiceHill;
	}

	return TargetSpice;
}

void SPAWN_FRIGATE(int iPlr, int iCll)
{
	// send out a frigate to this structure
	if (iPlr < 0)
		return;

	// iCll = structure start cell (up left), since we must go to the center
	// of the cell:

	if (bCellValid(iCll) == false)
		return;

	int iCell = iCll;

	int iX=iCellGiveX(iCell);
	int iY=iCellGiveY(iCell);
	iX++;
	iY++;

	iCell = iCellMakeWhichCanReturnMinusOne(iX, iY);

	// step 1
	int iStartCell = iFindCloseBorderCell(iCell);

	if (iStartCell < 0)
	{
		logbook("ERROR (reinforce): Could not figure a startcell");
		return;
	}

	// STEP 2: create frigate
	int iUnit = UNIT_CREATE(iStartCell, FRIGATE, iPlr, true);


	// STEP 3: assign order to frigate (use carryall order function)
	unit[iUnit].carryall_order(-1, TRANSFER_NEW_LEAVE, iCll, -1);


}


// Reinforce:
// create a new unit by sending it:
// arguments:
// iPlr = player index
// iTpe = unit type
// iCll = location where to bring it

void REINFORCE(int iPlr, int iTpe, int iCll, int iStart) {

	// handle invalid arguments
	if (iPlr < 0 || iTpe < 0)
		return;

	//if (iCll < 0 || iCll >= MAX_CELLS)
	if (bCellValid(iCll) == false)
		return;

	if (iStart < 0)
		iStart = iCll;

	int iStartCell = iFindCloseBorderCell(iStart);

	if (iStartCell < 0)
	{
		iStart += rnd(64);
        if (iStart >= MAX_CELLS)
            iStart -= 64;

		iStartCell = iFindCloseBorderCell(iStart);
	}

	if (iStartCell < 0)
	{
		logbook("ERROR (reinforce): Could not figure a startcell");
		return;
	}

	char msg[255];
	sprintf(msg, "REINFORCE: Starting from cell %d, going to cell %d", iStartCell, iCll);
	logbook(msg);

	// STEP 2: create carryall
	int iUnit = UNIT_CREATE(iStartCell, CARRYALL, iPlr, true);

	// STEP 3: assign order to carryall
	unit[iUnit].carryall_order(-1, TRANSFER_NEW_LEAVE, iCll, iTpe);


}

/**
 * Finds a free carryall of the same player as unit iuID. Returns > -1 which is the ID of the
 * carry-all which is going to pick up the unit. Or < 0 if no carry-all has been found to transfer unit.
 * @param iuID
 * @param iGoal
 * @return
 */
int CARRYALL_TRANSFER(int iuID, int iGoal) {
	// find a free carry all, and bring unit to goal..
	for (int i=0; i < MAX_UNITS; i++) {
        cUnit &cUnit = unit[i];
        if (!cUnit.isValid()) continue;
        if (cUnit.iPlayer != unit[iuID].iPlayer) continue;
        if (cUnit.iType != CARRYALL) continue; // skip non-carry-all units
        if (cUnit.iTransferType != TRANSFER_NONE) continue; // skip busy carry-alls


        // assign an order
        cUnit.carryall_order(iuID, TRANSFER_PICKUP, iGoal, -1);
        return i;
	}

	return -1; // fail
}

/**
 * Deselect all units (not player specific)
 * TODO: Move to unitUtils class.
 *
 */
void UNIT_deselect_all() {
	for (int i=0; i < MAX_UNITS; i++) {
		if (unit[i].isValid()) {
			unit[i].bSelected=false;
		}
	}
}

void UNIT_ORDER_ATTACK(int iUnitID, int iGoalCell, int iUnit, int iStructure, int iAttackCell)
{
    // basically the same as move, but since we use iAction as ATTACK
    // it will think first in attack mode, determining if it will be CHASE now or not.
    // if not, it will just fire.

	char msg[255];
	sprintf(msg, "Attacking UNIT ID [%d], STRUCTURE ID [%d], ATTACKCLL [%d], GoalCell [%d]", iUnit, iStructure, iAttackCell, iGoalCell);
	unit[iUnitID].LOG(msg);

	if (iUnit < 0 && iStructure < 0 && iAttackCell < 0)
	{
		unit[iUnitID].LOG("What is this? Ordered to attack but no target?");
		return;
	}

    unit[iUnitID].iAction   = ACTION_ATTACK;
	unit[iUnitID].iGoalCell = iGoalCell;
    unit[iUnitID].iNextCell = -1;
	unit[iUnitID].bCalculateNewPath = true;
    unit[iUnitID].iAttackStructure = iStructure;
    unit[iUnitID].iAttackUnit = iUnit;
    unit[iUnitID].iAttackCell = iAttackCell;
}



void UNIT_ORDER_MOVE(int iUnitID, int iGoalCell)
{
	unit[iUnitID].move_to(iGoalCell, -1, -1);
}



///////////////////////////////////////////////////////////////////////
// REINFORCEMENT STUFF
///////////////////////////////////////////////////////////////////////
void INIT_REINFORCEMENT()
{
 for (int i=0; i < MAX_REINFORCEMENTS; i++)
 {
    reinforcements[i].iCell=-1;
    reinforcements[i].iPlayer=-1;
    reinforcements[i].iSeconds=-1;
    reinforcements[i].iUnitType=-1;
 }
}

// done every second
void THINK_REINFORCEMENTS()
{
    for (int i=0; i < MAX_REINFORCEMENTS; i++)
    {
        if (reinforcements[i].iCell > -1)
        {
            if (reinforcements[i].iSeconds > 0)
            {
                reinforcements[i].iSeconds--;
                continue; // next one
            }
            else
            {
                // deliver
                REINFORCE(reinforcements[i].iPlayer, reinforcements[i].iUnitType, reinforcements[i].iCell, player[reinforcements[i].iPlayer].focus_cell);

                // and make this unvalid
                reinforcements[i].iCell=-1;
            }
        }
    }
}

// returns next free reinforcement index
int NEXT_REINFORCEMENT()
{
 for (int i=0; i < MAX_REINFORCEMENTS; i++)
 {
     // new one yey
    if (reinforcements[i].iCell < 0)
        return i;
 }

 return -1;
}

// set reinforcement
void SET_REINFORCEMENT(int iCll, int iPlyr, int iTime, int iUType)
{
    int iIndex=NEXT_REINFORCEMENT();

	// do not allow falsy indexes.
	if (iIndex < 0)
		return;

	if (iCll < 0)
	{
		logbook("REINFORCEMENT: Cannot set; invalid cell given");
		return;
	}

	if (iPlyr < 0)
	{
		logbook("REINFORCEMENT: Cannot set; invalid plyr");
		return;
	}

	if (iTime < 0)
	{
		logbook("REINFORCEMENT: Cannot set; invalid time given");
		return;
	}

	if (iUType < 0)
	{
		logbook("REINFORCEMENT: Cannot set; invalid unit type given");
		return;
	}

	char msg[255];
	sprintf(msg, "[%d] Reinforcement: Controller = %d, House %d, Time %d, Type = %d", iIndex, iPlyr, player[iPlyr].getHouse(), iTime, iUType);
	logbook(msg);

	// DEBUG DEBUG
    if (!DEBUGGING)
        iTime *= 15;

    reinforcements[iIndex].iCell	= iCll;
    reinforcements[iIndex].iPlayer	= iPlyr;
    reinforcements[iIndex].iUnitType= iUType;
    reinforcements[iIndex].iSeconds = iTime;
}

int UNIT_FREE_AROUND_MOVE(int iUnit)
{
	if (iUnit < 0) {
		logbook("Invalid unit");
		return -1;
	}

    cUnit &cUnit = unit[iUnit];

    int iStartX = iCellGiveX(cUnit.iCell);
	int iStartY = iCellGiveY(cUnit.iCell);

    int iWidth=rnd(4);

    if (cUnit.iType == HARVESTER)
        iWidth=2;

	int iEndX = (iStartX + 1) + iWidth;
	int iEndY = (iStartY + 1) + iWidth;

	iStartX-=iWidth;
	iStartY-=iWidth;

    int iClls[50];
    memset(iClls, -1, sizeof(iClls));
    int iC=0;

	for (int x = iStartX; x < iEndX; x++)
		for (int y = iStartY; y < iEndY; y++)
		{
			int cll = iCellMakeWhichCanReturnMinusOne(x, y);

			if (map.occupied(cll) == false)
            {
                iClls[iC]=cll;
                iC++;
            }

		}


	return (iClls[rnd(iC)]); // fail
}

