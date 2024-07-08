# Death Replay

Small mod that adds ability to view all of your deaths recorded by this mod while playing a level. You can control how this mod behaves by enabling/disabling stuff.

Level on logo: **Light Spectrum**

# Changelog

# v1.2.4
### Gameplay Changes
 * Small bugfix
 * Maximum ghost offset is lowered to `3` seconds
### Mod Description Changes
 * Fixed formatting in `changelog.md` and `README.md`

# v1.2.3
### Gameplay Changes
 * Fixed critical bug **(known as a Practice Mode Bug)**

# v1.2.2
### Gameplay Changes
 * Ghost offset now can be changed
 * All deaths caused by online players (e.g. Geode players) are now blue-tinted
 * Fixed crash found by `orange059487` and `MasterGameY`
 * Death marks are now a bit smaller
### Techinal Changes
 * New parameter has been added to a player death data: **Player Type**
   * `0` - **Offline Player**;
   * `1` - **Globed Player**.
 * SDK has been updated to `v3.0.0`

# v1.2.1
### Gameplay Changes
 * Ghost replay has been improved
 * Bugfixes

# v1.2.0
### Gameplay Changes
 * Ghost has been rewritten and enabled again. If you don't what is Ghost I'll give you some info about it
   - This Ghost shows after your death or level completion and replays previous inputs.
   - Ghost is shown by default. You can disable it in the mod settings.
   - If you experience inconsistent framerate try to disable Ghost in the mod settings.
   - **Ghost cannot be attached to the online players. (Globed players, for example)**

# v1.2.0 Beta 11
### Technical Changes
 * SDK has been updated to `v3.0.0-beta.1`

# v1.2.0 Beta 10
### Technical Changes
 * Fixed ABI bug on Android

# v1.2.0 Beta 9
### Technical Changes
 * SDK has been updated to `v3.0.0-alpha.2`
 * Added Android support

# v1.2.0 Beta 8
### Technical Changes
 * Fixed typo `(RayDeeUx)`
 * 'Remove Death Replay data' button is now placed correctly in the Level Info menu `(RayDeeUx)`

# v1.2.0 Beta 7
### Technical Changes
 * Try to port mod to Android (failed)

# v1.2.0 Beta 6
### Technical Changes
 * Try to port mod to Android (failed)

# v1.2.0 Beta 5
### Gameplay Changes
 * New setting: `Record Players in Globed`
   - This setting allows Death Replay to record online players if enabled.
   - **Disabled by default**.
   - Fixes `#6`.
### Technical Changes
#### Since v1.2.0 Beta 1
 * `playTime` parameter is fully ignored if level was not released and updated since 2.2+ release
### Mod Description Changes
 * Changelog in `README.md` has been updated

# v1.2.0 Beta 4
### Gameplay Changes
 * 'X' sprite is now half visible
### Techinical Changes
 * Code is cleaned up a bit
### Mod Description Changes
 * Repository link is updated to use proper repo name

# v1.2.0 Beta 3
### Gameplay Changes
 * Pressing ESC on removing Death Replay file popup now doesn't do anything (#2)
### Techinical Changes
 * Death particles are now spawned properly
 * SDK has been updated to `v2.0.0-beta.12`
 * Ghost's recording now includes Player scaling

# v1.2.0 Beta 2
### Gameplay Changes
 * Now you can remove Death Replay file for any level by clicking to red back arrow button
### Technical Changes
 * SDK has been updated to `v2.0.0-beta.11`
 * Bugfixes
 * playTime parameter is now fully ignored if played level has been created/updated last time before 2.2
 * Debug output has been removed
### Mod Description Changes
 * `changelog.md` formatting has been changed a little bit

# v1.2.0 Beta 1
### Gameplay Changes
 * Ghost has been temporarily disabled; it's very broken in 2.2
### Technical Changes
 * SDK has been updated to `v2.0.0-beta.9`
 * Ghost now waste less CPU time when its hidden than in DR v1.1.0
 * Death Replay level file now has new parameter - playTime
   - `playTime` parameter is fully ignored while being in Platformer mode.
   - Normal Mode can use playTime parameter if it possible.

# v1.1.1
### Gameplay Changes
 * Fixed bug where Ghost didn't get hidden when he was said to
 * Ghost now can be attached to any number of players so the Dual Mode is now working properly
### Technical Changes
 * SDK has been updated to v1.3.5
 * Ghost now doesn't waste CPU time and RAM when its hidden
### Mod Description Changes
 * `README.md` now has mod information and changelog

# v1.1.0
### Gameplay Changes
 * Added the Ghost player
   - This Ghost shows after your death or level completion and replays previous inputs.
   - Ghost is hidden by default. You can make it visible in the mod settings.
### Techincal Changes
 * SDK has been updated to v1.3.3
 * Source code now has Logo's GIMP project file
 * Mod now doesn't use child indexer
### Mod Description Changes
 * Changelog now looks better

# v1.0.2
### Mod Description Changes
 * Added changelog

# v1.0.1
### Techincal Changes
 * Removed debug printf
 * Changed save path to geode's

# v1.0.0
 * Initial release