# mecha bullet moba-like
Build your war-machine (mecha) to dominate the battlefield.

you play as the pilot of a highly customizable mecha

choose what type of gun identity (chassis) you want to pilot
- machine gun
- sword
- sniper
- shotgun
- rocket launcher
- laser
- etc

(should each chassis have a signature move?)

2 factions fighting

you have 10 slots to customize your mecha

you upgrade in the hangar

abilities/items, software/hardware (systems?)

active/passive abilities or stat sticks

top down style or isometric?

bullet hell + action 

single player moba

roguelike drops (eh)

big battlefield style map, have to spend time moving to different zones

zones contain npc minion skirmishes, or jungle (stacking?) for farm

some elite or special npcs

randomized objectives can contain different modes required to unlock the final objective (big boss)
- boss
- capture
- defend
- tower/building

### during a battle
- level up, get a random drop/boon (roguelite)
- enemies drop loot, special systems, rarer enemy, better loot

### between battles (runs), you return to the hangar
potential ideas:
- unlock new mecha identity guns, systems?
- choose a global power up? (5% bonus health, starting gold, etc)

## development process

springboard from failure, string and ductape, make it work, make it nice later

multiple iterations, prototype and v1 as two main targets

## prototype

### mini prototype [[**we are here**]]
we do this first

get a square (player) to move around and shoot machine gun and sword vs a triangle (enemy) on an empty map

camera moves with square

maybe add some scenery or clear map boundaries

### skeleton prototype
skeleton assets, extremely barebones placeholders for real sized spritesheets

missing texture style patterns or colours for each entity

explore movement mechanics

explore adding upgrades/abilities/items

adding npc minion battles

adding one type of objective

### asset prototype

add assets to game and make the animations, and feel work together

polish gameplay mechanics

## v1

add more of everything, assets, chassis, items, abilities, etc

explore adding some mechanics (but i also want the feel of the game to be pretty clear in skeleton prototype)

polish, polish, polish

sound, music

---

## technical

### game engine
We chose raylib and C right now. Compiled by Emscripten for single file usage as wasm as base64 autoloaded via js glue. Final index.html is a single file packed together via the Histos utility.

lighter:
- bevy (rust)
- love2d (lua)
- raylib (c)

more tools:
- godot (c#)
- unity

### similar games to research
- vampire survivor
- enter the gungeon
- into the breach









