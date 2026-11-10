## Sopwith TODO and ideas list

### New features

* Save window size + location + display#
* Demo recording and playback

### Custom levels

* Built-in map editor
* Campaigns (multi-level mods with stories)
* New gameplay modes
  - Sidescroller mode
  - Bombing run
  - Balloon busting
  - Stop the pigeon
* New object types:
  - "Ace" planes
  - Zeppelins
  - Bridges
  - Trains
  - Trees

### Multiplayer

* More than two players
  - Extra player color combos - cyan+white, white+magenta, etc.
  - Player names become essential so you know who's who.
* Better sync code via PID loop
* Network disconnect should bring you back to the title screen
  - Also on failure to connect

### Code cleanup

* Restructure the game to be state based - eliminate use of longjmp
* Delete ob_xnext/ob_xprev/insertx/deletex
* Rename functions to be more readable
* Code reformatting w/clang-format
