Dune II - The Maker
===================
A.k.a. D2TM. Is a [Dune 2](http://en.wikipedia.org/wiki/Dune_II) clone. At this moment the most known/stable version is [DEMO4 and DEMO 3.5](http://dune2themaker.fundynamic.com/?page_id=11) which are very old and using
Allegro & other libs. I am proud of that project, but it is time to rewrite it and build a game mostly focussing on skirmish
and networking gaming.

This branch is an attempt to rewrite the game from scratch using SDL as game library. 

I do this purely for fun, and try to keep it multi-platform. I test it often on Mac OSX and Ubuntu. I sometimes test it
on Windows (MinGW32 and MSYS). 

At this moment there is nothing built enough to call this a game, see the list "TODO" below what needs to be done. (list
will continue to grow).

Compiling
=========
This project has makefiles for Windows (mingw32), Linux (ubuntu) and Mac OSX.

Dependencies:
- SDL
- SDL Image

After you check out this project, copy the appropiate makefile (with the extension matching your OS) as 'makefile'. After that hit make.

Example, OSX && [homebrew](http://mxcl.github.io/homebrew/):
------------------------------------------------------------
- Install XCode CLI tools
- Using brew, install dependencies: `brew install SDL && brew install sdl_image`
- Then: `cp makefile.osx makefile`
- `make`

Example, Ubuntu (inspired by [Lazy Foo](http://lazyfoo.net/SDL_tutorials/lesson01/linux/))
------------------------------------------------------------------------------------------
- Fetch SDL and its dependencies with apt-get (`apt-get install libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev`)
- `cp makefile.ubuntu makefile`
- `make`

TODO: ((as a player I) should be able to...)
============================================
- move the camera around (working)
 - make keyboard keys repeat
 - mouse borders
 - hold mouse key?
- see units (draw them)
- select one unit
- select multiple units
- order a unit to move to a position
- order a unit to attack a position
- explore the terrain by clearing a Fog of War
- enjoy the terrain smoothened (smoothing algorithm)
- see structures (draw them)
- select my structure
- build another structure from a Construction Yard
- build infantry from a Barracks
- build troopers from a WOR
- build light vehicles from a Light Factory
- build heavy vehicles from a Heavy Factory
- build air units from a High-Tech
- upgrade my structures to gain access to other units
- upgrade my units (??)
- receive reinforcements from time to time
- mine spice with a harvester
- get my units transported much faster by a carry-all



Controls
========
Thus far the following controls work:

Press `q` to quit anytime.
Press movement keys to move around.
