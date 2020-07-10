/*

  Dune II - The Maker

  Author : Stefan Hendriks
  Contact: stefanhen83@gmail.com
  Website: http://d2tm.duneii.com

  2001 - 2009 (c) code by Stefan Hendriks


  ---
  New
  ---

  Giving a go for true OOP programming, using a base class and derived classes.
  */
#ifndef CABSTRACTSTRUCTURE_H_
#define CABSTRACTSTRUCTURE_H_

class cAbstractStructure {

	private:
		int armor;			// armor

		int iHitPoints;     // HitPoints this structure has

		int iCell;          // What cell it is (most upper left part of structure where
							// drawing starts)

		int iRallyPoint;    // rallypoint ...

		bool bAnimate;      // Do its special animation? (unit leaving building, starport
							// dropping something, etc)

		int id;				// the id within the structures[] array

	protected:
		int iWidth;			// width in cells (set by factory)
		int iHeight;		// height in cells (set by factory)

		int iFrame;          // Frame for animation (flag and other)

		int iPlayer;        // owner
    public:

		// Constructor & Destructor:
		cAbstractStructure(); // default constructor

		virtual ~cAbstractStructure();



		float fConcrete;     // how much concrete is *not* beneath this building (percentage)?
							 // meaning, when 0% , it is all concrete. But if 10%, it means 10% of the building
							 // is not covered.

		// Repairing stuff
		bool bRepair;       // repairing? (using timer + gives animation)
		int iRepairY;		// raising repair thingy
		int iRepairX;		// repair X position (changes everytime?)
		int iRepairAlpha;	// repair alpha

		int iUnitID;        // >-1 means ID to unit

		// Building animation
		int iBuildFase;

		// TIMERS that all structures use
		int TIMER_repair;   // repairing timer
		int TIMER_repairanimation; // repair animation timer
		int TIMER_flag;     // flag animation
		int TIMER_fade;     // fading timer

		int TIMER_damage;   // damaging stuff
		int TIMER_prebuild; // prebuild timer

		// -------------



		// Filled in by derived classes
		virtual void draw(int iStage)=0;              // draw
		virtual void think()=0;           // think
		virtual void think_animation()=0; // think animation stuff
		virtual void think_guard();       // think guard stuff

		virtual int getType()=0;					  // implementation gives type of structure
		void think_prebuild();            // prebuild animation
		void think_repair();              // repair thinking
		void think_damage();              // think about damaging through time
		void think_flag();				  // think method for flag animation

		void die();                       // die

		// drawing
		int iDrawX();
		int iDrawY();

		// Functionality
		int iFreeAround();

		// getters
		cPlayer * getPlayer();

		// convenience get method, which should eventually only be used in rare cases as all properties should be
		// TODO: copied from this struct to this class, so each structure can change the properties without
		// changing the entire game rules.
		s_Structures getS_StructuresType();

		int getArmor() { return armor; }
		int getWidth() { return iWidth; }
		int getHeight() { return iHeight; }
		int getHitPoints() { return iHitPoints; }
		int getCell() { return iCell; }
		int getOwner() { return iPlayer; } // return the player id who owns this structure
		int getRallyPoint() { return iRallyPoint; }
		int getFrame() { return iFrame; }
		int getStructureId() { return id; }
		int getMaxHP();
		int getSight();
		int getRange();
		int getPercentageNotPaved();
		int getPowerUsage();

		bool isAnimating() { return bAnimate; }
		bool isPrimary();

		void setHeight(int height);
		void setWidth(int width);
		void setHitPoints(int hp);
		void setCell(int cell);
		void setOwner(int player); // set owner (player id) of structure
		void setRallyPoint(int cell); // set rally point of structure
		void setAnimating(bool value); // set animation on / off
		void setFrame(int frame);
		void setStructureId(int theId) { id = theId; }

		void damage(int hp); // damage structure for x amount of hp
};

#endif
