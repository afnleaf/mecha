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

---

# fuller game idea

what I really like:

the world is a simulation, you are up against an ASI (artificial super intelligence) that is basically the god of the simulation. this can't be stated explicitly only shown through gameplay and art. diagetic style. meta narrative, fourth wall thing. you are playing a game that is itself a simulation and the AI is the game logic that the player doesn't control. True ending from beating the game with all the different classes is a good idea.

Mecha is all about customization and upgrades. The randomness of roguelikes feels like a step in the wrong direction. Whereas it works for a game like Hades because it's very room based, you want the player to interact with all the gods, and you completely reset on each run. 

You want your mech to feel like its yours. I prefer more a more deterministic shop style that is closer to moba. You could have slight randomness of some item/ability drops you find (unlock new cosmetics?). They shouldn't be massively run altering, more like a cool bonus. This would have almost an extraction shooter feel to the game when youre going back to your mechanic's shop/garage.

MOBA Roguelike, Cores = Genre
- Gunner (Shoot em up)
- Sword (Rhythm, parry, dodge, hit)
- Mastermind (deckbuilder is yawn imo, but strategy/tactics are cool)
- ???

The reuse of assets is clever.

Also isometric would be cool but I think I want to stick to bird's eye 2d for now, it's quite a lot easier to develop for. At least for a prototype.

But a tough proposition because it becomes 3/4 games. That means a lot of overlapping logic that could become wonky without literally designing them each from the ground up. That's spreading design and development across 4 different gameplay loops rather than refining one really well. (For a first game.) 

The 3 proposed classes have me thinking about roles in moba like tank, damage and support. When you play overwatch or deadlock or league/dota those roles exist and you play quite differently, but all the systems are the same.

When you play a souls-like, you can choose spell caster or big sword or other, etc, but the fundamental mechanics of dodge hit dodge hit are the same. 













