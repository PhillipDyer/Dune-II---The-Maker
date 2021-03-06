/*
 * cMapDrawer.h
 *
 *  Created on: 10-aug-2010
 *      Author: Stefan
 */

#ifndef CMAPDRAWER_H_
#define CMAPDRAWER_H_

class cMapDrawer {
	public:
		cMapDrawer(cMap * theMap, const cPlayer& thePlayer, cMapCamera * theCamera);
		~cMapDrawer();

		void drawTerrain(int startX, int startY);
		void drawShroud(int startX, int startY);

	protected:


	private:
		cMap * map;
		const cPlayer& player;
		cMapCamera * camera;
		cCellCalculator * cellCalculator;

		// bitmap for drawing tiles, and possibly stretching (depending on zoom level)
        BITMAP *bmp_temp;

    int determineWhichShroudTileToDraw(int cll, int playerId) const;
};

#endif /* CMAPDRAWER_H_ */
