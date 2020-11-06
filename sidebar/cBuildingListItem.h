/**
 * a cListItem is  used within a BuildingList , it has a picture and any unit or
 * structure ID attached. At the same time this item is responsible for 'building'
 * the item it represents. It has therefor also other properties; progress a timer, etc.
 *
 *
 */
#ifndef CBUILDINGLISTITEM
#define CBUILDINGLISTITEM

class cBuildingListItem {

public:
    ~cBuildingListItem();

	// uber constructoe
    cBuildingListItem(eBuildType type, int buildId, int cost, int icon, int totalBuildTime, cBuildingList *list, int subList);

	// easier constructors
    cBuildingListItem(int theID, s_Structures entry, int subList);
    cBuildingListItem(int theID, s_UnitP entry, int subList);
    cBuildingListItem(int theID, s_Upgrade entry, int subList);

	// gettters
	int getTotalBuildTime() { return totalBuildTime; }
	int getIconId() { return icon; }
	int getBuildId() { return buildId; }
	int getSubList() { return subList; }
	eBuildType getBuildType() { return type; }
	int getBuildCost() const { return cost; }
	int getProgress() { return progress; }
	bool isBuilding() { return building; }
	bool isState(eBuildingListItemState value) { return state == value; }
	bool isAvailable() { return isState(eBuildingListItemState::AVAILABLE); }
	int getTimesToBuild() { return timesToBuild; }
	int getSlotId() { return slotId; } // return index of items[] array (set after adding item to list, default is < 0)
	int getTimesOrdered() { return timesOrdered; }

	int getCosts();

	float getCreditsPerProgressTime() { return creditsPerProgressTime; }

	float getRefundAmount();

	bool shouldPlaceIt() { return placeIt; }

	// setters
	void setIconId(int value) { icon = value; }
	void setBuildCost(int value) { cost = value; }
	void setProgress(int value) { progress = value; }
	void setIsBuilding(bool value) { building = value; }
	void setIsAvailable(bool value) { value ? state = eBuildingListItemState::AVAILABLE : eBuildingListItemState::UNAVAILABLE; }
	void setTimesToBuild(int value) { timesToBuild = value; }
	void setTimesOrdered(int value) { timesOrdered = value; }
	void increaseTimesToBuild() { timesToBuild++; }
	void decreaseTimesToBuild();
	void increaseTimesOrdered() { timesOrdered++; }
	void decreaseTimesOrdered() { timesOrdered--; }
	void setSlotId(int value) { slotId = value; }
	void setPlaceIt(bool value) { placeIt = value; }
	void setList(cBuildingList *theList) { myList = theList; }

	cBuildingList *getList() { return myList; }	// returns the list it belongs to

    void increaseProgress(int byAmount);

    int getBuildTime();

    bool isDoneBuilding();

private:
	int icon;				// the icon ID to draw (from datafile)
	int buildId;			// the ID to build .. (ie TRIKE, or CONSTYARD)
	eBuildType type;		// .. of this type of thing (ie, UNIT or STRUCTURE)
	int cost;				// price
	bool building;			// building this item? (default = false)
	eBuildingListItemState state;
	int progress;			// progress building this item
	int timesToBuild;		// the amount of times to build this item (queueing) (meaning, when building = true, this should be 1...)
	int timesOrdered;		// the amount of times this item has been ordered (starport related)
	int slotId;			 	// return index of items[] array (set after adding item to list, default is < 0)

	float creditsPerProgressTime; // credits to pay for each progress point. (calculated at creation)
	bool placeIt;			// when true, this item is ready for placement.

	int totalBuildTime;		// total time it takes to build.
	int subList;            // subList id's allow us to distinguish built items within the same buildingList.

	cBuildingList *myList;

    cBuildingListItem(int theID, s_Structures entry, cBuildingList* list, int subList);
    cBuildingListItem(int theID, s_UnitP entry, cBuildingList* list, int subList);
    cBuildingListItem(int theID, s_Upgrade entry, cBuildingList* list, int subList);
};

#endif
