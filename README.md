***Files for Cataclysm MUD.***

Cataclysm MUD is a continuation of tbaMUD/CircleMUD, which is built on DIKU MUD.
The code here is freeware to honor that tradition. Licensing and use should be based
on what was outlined previously. Any new code added here is released under the same
license.

Due to the sensitive nature of topics found in this setting, all characters and
players are 18+. The game world is derived from several inspirational sources,
most notably the former Armageddon MUD.

Roleplay is **highly** encouraged.

Changes from stock tbaMUD 2025 to Cataclysm MUD v1.0.0-alpha:

* The city of Caleran is available for exploration
* Experience points and levels are removed in favor of skill based progression
* Initial skills/spells based partly on tbaMUD code and 5e conversion (to be cleaned up in later release)
* Expanded emoting system for roleplay
* Permanent character death
* A hybrid "5e-like" system where:
 - [ ] Legacy THAC0 systems are removed in favor of the modern 5e system
 - [ ] Your skill level translates to a proficiency bonus on a per-skill level
 - [ ] Saving throws are based on 5e rules (if your class has them, you do)
 - [ ] Your spell save DC is 8 + skill profiency + ability mod for your class
 - [ ] AC and to hit d20 rolls typical of 5e, capped at +10 to hit and +8 to AC
 - [ ] Introduction of basic 5e skills which are used for multiple commands
 * QUIT room flags to designate a room safe to quit in, as well as quit ooc <message>
 * SAVE room flags for players to drop their loot in (such as an apartment or barracks)
 * NPC's can be equipped with items and their prototypes saved for quick future loading
 * Rooms can be saved with their objects and NPC's to be loaded on the next boot
 * Converted action descriptions to main descriptions for items
 * Mapping command now supports new terrain types
 * Players do not pay to keep their items when they log out
 * Furniture items now allow items to be placed on top of them (a mug on top of the bar)
 * Furniture items can now be stood at, sat on, rested on, and slept on
 * Furniture items in rooms can be reviewed with "look tables"
 * Think, feel, and OOC commands for roleplay purposes
 * Expanded say, talk, and whisper commands to allow for greater roleplay
 * Sneak/Hide have been updated and an opposed roll against observers occurs
 * Ability to stealthily put or get items from containers via Palm/Slip commands
 * Listening in on conversations nearby is possible
 * Modernized stat output for immortals
 * NPC's can now be assigned a class like a PC and inherit the relevant skills
 * PC's now use a short description for identification instead of name
 * Backgrounds are now available for PC's and NPC's
 * Account system for tracking players/characters over long periods of time

Changes in v1.1.0-alpha:

 * Cleaned up legacy practice system code
 * Added skill caps for classes to limit ability of everyone to reach skill level 100 (and respective proficiency)
 * Race/species selection and stat ranges (elves have higher dex, dwarves have higher str, etc)
 * Renamed move to stamina in code to reflect how much energy is used for certain actions
 * Species have base hit/mana/stamina now, plus their class modifier rolls
 * Prioritized stats during character generation
 * Ability to change ldesc of PC/NPC's
 * Ability to look in certain directions to see what is 1-3 rooms away
 * PC's and NPC's can now have an age set between 18-65
 * "audit armor" and "audit melee" commands for immortals (formerly "acaudit") to check non-compliant items
 * Minor score output change to only show quest status while on a quest, PC/NPC name, sdesc, and current ldesc
 * Added ability to reroll initial stats if they are not to player's liking, and undo reroll if needed
 * Removed alignment from game - no more GOOD/EVIL flags or restrictions on shops
 * Removed ANTI_ flags related to class restrictions on what objects they can use
 * Mounts added to help with long trips, and ability to use them as pack animals
 * Introduced AGENTS.md file to ensure code quality
 * Migration away from OLC with new commands "rcreate" and "rset" for builders to modify rooms
 * Migration away from OLC with new commands "ocreate", "oset", and "osave" for builders to modify objects
 * Migration away from OLC with new command "mcreate" and for builders to modify NPC's
 * Fixed issue with msave not saving items in containers on NPC's

Features that would have been implemented in the next few releases:

* Height and weight normalized to species
* Stables allow for purchasing of mounts
* Stables will take mounts and provide tickets to get them out
* Updated door code so that it can be closed/locked/saved with rsave code
* SECTOR/ROOM type changes to make terrain movement easier or more difficult
* Subclass selection to personalize character further
* Combat is slowed down so it isn't over in < 15 seconds (unless you're far outmatched)
* Wagons added to help with caravans
* BUILDING object type created to allow enter/leave
* Updated BUILDING object type so that it can be damaged and no longer enterable (but someone can leave at cost to health)
* Plantlife introduced, allowing a plant object to produce fruit or herbs every few hours/days
* Plantlife can be refreshed to spawn fruits/herbs more frequently by watering it
* Updated lockpicking skill
* Trap as a skill - one focused on city and one focused on desert
* Poisons and antidotes
* New alcohol and drugs/stimulants
* Skimmers/ships to traverse difficult terrain
* New elemental classes
* Introduction of gathering mana/magic for sorceror class
* Highly modified magic system
* Ranged weapons and ammo
* Components for some magical spells
* Reading/writing limited to specific castes of society
* Haggling and bartering system
* New calendar and moon cycles
* Heat based on time of day increases/decreases, changing hunger/thirst levels
* Weather updates and sandstorms limiting visibility
* Shaded rooms providing bonuses to regeneration
* Criminal system for cities and jails
* Basic Psionics
* Basic crafting system
* Apartment rentals for storing your loot
* Enhanced quest system
* Dialogue trees with NPC's
* Additional zones/cities based on Miranthas world map
* Resources on the world map can be claimed by different city-states or independent factions
* Claimed resources improve quality of armor/weapons/food/prices available
* Death from old age if you roll badly on your birthday after the expected lifespan of a species
* Attacks hit different parts of the body and have different damage effects
* Armor degradation based on damage taken per body part
* Weapon degradation based on damage dealt - potentially shattering weapons

...and down the road:

* Replace ASCII files in favor of SQL database on the backend
* Replace DG Scripts with a Python abstraction layer for modern scripting support
* Replace Oasis OLC with more modern interface for easing builder duties
* Discord server integration for ticketing and community
* Full documentation for admins and easy to follow improvement guides
* ...suggestions from anyone who uses this game
