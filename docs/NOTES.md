# problem solving space

feel free to use these urls to explore
- [Cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [Math Sheet](https://www.raylib.com/cheatsheet/raymath_cheatsheet.html)
- [raylib.h](https://github.com/raysan5/raylib/blob/master/src/raylib.h) inside ./raylib/src/raylib.h
- [FAQ Technical](https://github.com/raysan5/raylib/wiki/Frequently-Asked-Questions)
- [FAQ Generic](https://github.com/raysan5/raylib/blob/master/FAQ.md)

joint implementation between claude and myself the user! 🤝🫡 ^~~~^

``` C
// this is THE piece of data that we are operating on
// it sits inside of our pipeline
typedef struct GameState {
    // the player entity
    Player player;
    Projectile projectiles[MAX_PROJECTILES];
    Enemy enemies[MAX_ENEMIES];
    Deployable deployables[MAX_DEPLOYABLES];
    LightningChain lightning;
    // vfx event buffer — update writes, draw reads
    VfxState vfx;
    //
    Camera2D camera;
    int score;
    float spawnTimer;
    float spawnInterval;
    int podValue;
    int enemiesKilled;
    GamePhase phase;
    int level;
    bool gameOver;
    bool paused;
    bool gamepadActive;     // true = last input from gamepad
    int selectIndex;
    int selectPhase;    // 0 = picking primary, 1 = picking secondary
    float selectDemoTimer;
    float selectSwordTimer;  // sword arc demo in select screen
    float selectSwordAngle;
    Vector2 selectPedestals[NUM_PRIMARY_WEAPONS];
    float transitionTimer;   // >0 = fade transition active
    // shop
    int shopIndex;           // highlighted shop pedestal (-1 = none)
    Vector2 shopPedestals[ABILITY_SLOTS];
    float spawnDelay;        // countdown after leaving base
    // cheats
    bool infiniteMoney;
} GameState;
// the static struct and the emscripten void requirement are important
// futher study needed
static GameState g;
```

This probably fits really nicely inside cache. Also it's still tiny, so duh.

I'm surprised the binary executable is 1.1mb while the full index.html single file mode is 300kb. This is because the native build links everything from the full build of make platform_desktop inside the lib.a. While emscripten -Os build is aggressive and can strip all the functions you don't use. 

maybe we should refactor the huge update and draw functions? but, not sure what the proper way to approach this problem is. we are at the point where if we keep adding new stuff to the game, our current approach would be unmaintainable, but it works right now in this super early prototype version. This is a good spot to be in from a single "vibe coding" session, but continuing on with feature creep would be a massive mistake. We need to understand the data transformation problem better before we tackle a re design. 
draw vs update

so draw is obviously handling the visuals

update should be updating the game state

the update happens before the draw

there are a lot of extra lines of C code just to fit inside the 80 col limit

## v(-2)
add the main weapons, enemies and abilities

pick two weapon types to start with

## v(-1)
massive refactor

a room with the first 4 enemies, defeat them

game pauses you pick an extra weapon out of some options

same room with more enemies, defeat them

game pauses and you pick another weapon

mini boss fight

game end

## v(0)
massive refactor

assets, art, music

asset loader

## todo
- [x] (claude) optimized native build
- [x] (claude) iframe granted by dash at start. 
- [x] (claude) add background and grid color to default
- [x] (claude) refactor draw to World and HUD.
- [x] (claude) make the player model an cube that rotates but in 2d but it looks 3dish? if you catch my drift
- [x] (claude) dash animation, ghosting through enemies
- [x] (claude) make dash longer
- [x] (claude) add enemy type, the debug texture as a rectangle, bigger than triangle, more hp and shoots bullets every half second at your current location
- [x] (claude) add double dash that recharges (like tracer blinks)
- [x] (user) refactor update game (design clean approach to separating this like draw world and draw hud)
- [x] (user) changed crosshair to classic green with gap
- [x] (claude) fix dash slash and slash color, add trailing sword arc effect
- [x] (claude) fix RECT hitbox (added oriented bounding box)
- [x] (user) added game title text
- [x] (claude) treat spin to the sword trailing animation treatment
- [x] (user) added healing to spin attack
- [x] (claude) fix hitmask - replaced with sweep line + lastHitAngle
- [x] (user) clamp enemies to map size rather than -500, 500
- [x] (user) add green particles on heal 
- [x] (user) separate update camera and window resize logic
- [x] (claude) make the sword flash with particles
- [x] (user) fix esc quit on native
- [x] (user) dynamic resize in native
- [x] (user) give each enemy their own score
- [x] (claude) redesign and refactor collision logic
- [x] (claude) make the character model a tetrahedron pyramid
- [x] (gemini + gpt5.2 + claude) make the player model HSV, or RBG, so the texture is generated algorithmically by going through each possible value of HSV or RBH u know 3 u8s right uint8_t in C? it needs to look like a rainbow. still not working properly but close.
- [x] move keybind instructions UI to pause menu.
- [x] launch screen shows keybinds
- [x] sent to primary monitor on launch
- [x] fullscreen, set toggle
- [x] mouse visible on top of crosshair on native (fixed)
- [x] move reload and overheat bars somehwere more visible than the left side
- [x] cleanup controls hud menu to be properly visible and programmatic
- [x] arrow keys/okl; to aim for keyboard only accessibility, right enter+shift as m2/m1
- [x] controller support
- [x] waves/phases after screen/phase refactor
- [x] map change, one big map with base and combat zone
- [x] add money it is the `int score`, already have it working.
- [x] add shop pay with score
- [x] add cheat toggles, infinite money, buy all/sell all abilities
- [] able to remap abilities (tab/alt modifier)


### refactorin time
- [x] ok let's do a header file and remake the build
- [x] (user + claude) bullet to projectile or base struct
- [x] (claude) big refactor of all hardcoded numbers
- [x] figure out how to do hitscan, what are the other weapon types?
- [x] time to make src/ folder and split up the project?
- [x] enemy damage calculation being done in a bunch of for loops going over each enemy? fixed
- [x] make weapon select menu accept enter and m1 for picking a chassis
- [x] game camera does a wonky teleport to the proper game map from the weapon select zone, also there are artifacts from the weapon seleection still on the screen like exploding rockets
- [x] enemy debuff whack
-- gap --
- [] understand particles better (this is an art thing...)
- [] (user) figure out how to separate particle/weapon animation from game logic update?
- [] (user) refactor anything badly named and organized (LLMisms)
- [] (user) proceduraly gen textures for debug, background (grass? biomes?) 
- [] physics as deterministic for multiplayer future? (FPS TARGET)
- [x] (claude) linger effect refactor
- [] make it easy to adjust game feel
- [] ~~~bullet pool scaling separation player and enemy, also ownership~~~
- [] ~~~fix what dash follows, mouse or wasd? what takes prio?~~~

### other features
- [] (user) damage system design (not implementation, what is the math behind it)
- [] (user) asset blob data design, how to parse? how to create?
- [] shaders GLSL
- [x] shadows under entity (the shadow is a 2d projection of the 3d player onto the game world, this is a meta concept of the game for players to figure out, it needs to work well. The shadow must slightly lag behind the player and be underneath them, to give a sense of depth. The shadows also need to be mathematically correct according to their shape. Enemies do not have 2d shadows as they are 2d objects.)

### enemy types
- [x] (claude) add a pentagon enemy type. they are a pentagon, neon green. They shoot two big rows of 5 bullets each.
- [x] (claude) add rhombus enemy (chase)
- [x] (claude) add hexagon enemy (fan shooter)
- [x] (user) add octagon enemy (stopper)
- [x] add mini boss, trapezoid, multiple attacks
- [x] design enemy value table (number of sides they have lol?)
- [x] endless waves of pods
- [] add big boss circle
- [] enemies receive a tell for their attacks
- [] enemies have a range for their aggro
- [] enemies don't attack immediately after spawning
- [] trap becomes elite unit
- [] pod value increase function
- [] cutting out weaker enemies at high values

waves of pods of enemies, different pods. assign enemy value, each pod has a set value. Pod with value x is made up of enemies whose value sum is equal to pod value.

We make endless mode, start at some pod value, x. Clear wave, next pod value is x + 1.

high value encouter = pod(s) with high value enemies

mirror of this is economic system (gold/parts/scrap/bolts/screws/etc)

factory buildings, making robots, i guess we doing shapes now

weak point

enemies spawn around the building, they keep respawning around the factory while its active

factory level sets pod value


### weapons
- [x] laser (hitscan)
- [x] choose your weapon screen, you get 5 options (machine gun, laser, revolver, sword, ???)
- [x] revolver (6 shooter) (m1, m2 fan the hammer)
- [x] sniper (no pierce critical damage, fast bullet, weakens opponent, slows them, long cd)
- [x] rocket launcher (explosion)
- [x] minigun (slows you down, wind up time, many bullets)
- [x] remove laser? (put on backburner, redefine something that feels good about it)
- [x] "reloading" like active reload mechanic from deadlock (press button timing = bonus)
- [x] revolver reload timing m1 regular buff dash bonus buff (hidden quirks with weapon on dash?)
- [x] "overheat" mechanic for infinite shooting guns, with a reheat mechanic?
- [x] gun modifier button (ctrl+m1/m2) (maybe not there is a decision to make here)
- [x] decide what the actual 5? primary weapons should be? (sword, revolver, machine gun, sniper, ??? (mingun?, explosives?))
- [x] figure out where to put keybindings because GRENADE_KEY = KEY_C is not it, the keys need to be flexible and reslotable.
- [x] unbind all other keys except for the primary.
- [x] wider railgun
- [x] sniper hipfire way less accurate
- [x] flamethrower range longer
- [x] rocket goes max distance, m2 explodes it, just remove rocket explodes where m1 was pressed (better for controller/kb only)
- [x] make multiple rockets possible at one time
- [] remove lunge and replace with heavy sword attack (more damage, slower)

if this is a mecha game, then down the line we want two arms = two weapons, or replace weapon with dedicated ability, etc. do we want m1 m2 to be from the same weapon but two different modes of firing or is m1 m2 arm1 arm2 weapon1, weapon2.

maybe each primary weapon should have a quirk where a basic other ability upgrades it or another mechanic exectured properly. a reload is kind of like this, overheat too. dash slash, fan the hammer is just an m2. We should think about what is m2 for the others. Machine Gun = gunner = two weapons on one arm? Sword = Special sword ability? Is spin for the sword only? Then we want something else on shift. 
minigun could be the m2 for machine gun

explosives as primary? is that possible? (yes)

variations of each weapon?

### abilities
- [x] dash
- [x] super dash, like deadlock dash timing on the second dash gives you something
- [x] fix dash slash
- [x] are weapons just abilities in a mecha? buttons? (yes)
- [x] (fix arc, not green but dark orange) grenade launcher (delayed explosion)
- [x] big gun (BFG10k) (massive arc lighting that spreads likes tendrils to every enemy, from another enemy based on distance)
- [x] bfg should charge up not as a cooldown but like an overwatch ultimate (maybe each character has one? or you can choose it? bfg is just like an epic ult you can pick, yeee that would sick)
- [x] sniper over laser as primary?
- [x] shotgun bullets ricochet?
- [x] shotgun (knockback)
- [x] railgun (histcan big damage with pierce)
- [x] decoy (you leave your shadow on the ground, and the enemies aggro it)
- [x] spin that deflects bullets
- [x] shield (delete projectiles in a radius around the player)
- [x] parry (longer?) kinda works like deadlock melee parry
- [] summons (npcs who take aggro for you) 
    - [x] turret needs to be targetable by enemies and healable by your field
    - [] damage drone
    - [] heal drone
- [x] root mines when exploded aoe web effect 
- [x] ground slam (what should this do?) less damage more stun, like reinhardt mini slam
- [x] fire (more like a flamethrower that you can spread infront on the ground)
- [x] heal field? (current is fine)
- [x] blink dagger (but its a tiny cutting dagger teleport and do damge effect, only usable when not damaged)
- [x] shotgun further range
- [] dva matrix

### players
each chassis is a platonic solid and has an associated element (sacred geometry)
- [x] tetrahedron (fire) (sword)
- [x] cube (earth) (revolver)
- [x] octahedron (air) (gun)
- [x] dodecahedron (aether) (sniper)
- [x] icosahedron (water) (rocket)
- [x] decide who gets which primary weapon
- [x] do you get to pick two shapes and morph between them? yes (primary/secondary press 1 to switch between forms, transformation of shape)
- [x] consistent player size depending on shape
- [] fix player shadow not rotating properly or being the direct shadow shape

idea: two characters? the cube and the tetrahedron? rest of platonic solids: octahedron, dodecahedron, icosahedron. Make the animation it rolling on each side? (I like this). 2d shapes vs 3d shape, you leave your shadow in this 2d world.

the different games idea, each chassis has one really powerful identity that enables you to play a game a certain way. While most of the game remains the same, this one unique spin is creates quite a change. 

what can you customize? all your buttons, its a mecha game... some remain for basics. Obviously we can reslot any of these buttons. 

non changing controls
- WASD - move
- m1/m2 - main weapon 
    - [x] sword (short range)
        - m1 sweep (hold m1 = spin?)
        - m2 lunge (pierces through enemies forward cone) (hold m2 == ?)
        - quirk dash slash + dash lunge
    - [x] revolver (mid range precision)
        - m1 shoot regular
        - m2 fan the hammer
        - quirk reload timing (r + dash bonus)
    - [x] machine gun (mid range power)
        - m1 normal shooting (short overheat)
        - m2 mini gun (should this be m2 for machine gun?) (longer overheat + movement penalty)
        - quirk overheat mechanics (r + dash bonus=movespeed buff)
    - [x] sniper (long range precision)
        - m1 hip fire less accurate
        - m2 slow down like aiming down sights, more accurate more damage?
        - quirk debuff (critical opening or slowness?) (dash into m2 timing = super duper shot hidden mechanic)
    - [x] rocket (area denial)
        - m1 rocket (one in flight at a time)
        - m2 detonate in-flight rocket
        - quirk rocket jump (self-damage, half-dash push, scales with distance from center)
    - ~~laser~~
- space - dash (2 charges)
- ctrl - switch weapon formes
- P / esc - pause
- 0 - quit
- tab - free (score board?/map??/diagetic menu)
- alt - free (modifier? level up?)
- alt keyboard mode OKL; movement right shift m1, enter m2
- controller?

customizeable controls, 12 normal abilities and 1 ult
- shift - spin (lifesteal)
- Q - shotgun (2 blasts)
- E - railgun (pierce all)
- F - parry (melee)
- R - ult BGG10k 
- Z - heal field
- X - projectile shield
- C - Grenade launcher
- V - flamethrower
- 1 - ground slam stun
- 2 - blink dagger
- 3 - turret
- 4 - root mine

figure out the classes of abilities


## Levels
we have one level. level 0 is where you can pick your weapon diagetically. every menu in the game should be a level.

regarding the current weapon select menu. The player character should be a sphere. When you walk up close to a weapon, it should fire its attack.

levels
- [x] weapon select
- [] pick new item between rounds

scratch this, one map. Combine weapon select and the arena. 

Take the endless waves of pods, that becomes our sandbox for perfecting game mechanics, weapons, abilities, enemies and all of their interactions.

Two modes, sandbox, gives you all the abilities to choose from and shop mode.

We start to add gold to the game. You can buy between every x number of waves. 

## HUD
- [x] hud fix level placement
- [x] redesign hud simple
- [x] diagetic dash cd
- [] more diagetic effects for weapons/abilities

were going to make the dash cooldown diagetic, it will be 3 orbs moving, rotating, counter-clockwise around the player, they represent the dash cooldown via their color, and alpha. going from gray to the same blue color currently used by the dash HUD.

keep hp bar and score, kills, level revolve/gun where they are, move the game title to only be visible in the top middle in weapon select menu and pause screen. cooldowns go to the bottom we have a full bottom bar free with the title moved. save bottom corners for minimap on either left or right.

I find that when you play this game you stare at your cross hair, which moves around a lot left and right. You're looking for the enemy. And also looking around where your character is moving. In an FPS or 3rd person action game your crosshair is part of the movement. 

Looking at the HUD in this game is not natural is what I am trying to say.

Even the quick time events on your character are hard to time. You often miss out on knowing when your last revolver shot is to prime yourself for the event. This is a problem to solve, I think it can be done diagetically.

## Research
- [x] read many parts of raylib.h
- [x] study vector 2 implementation (raymath.h)
- [x] Vector2Lerp: what is it doing exactly? how much can we fiddle with the lerp constant (8.0f)? (game.c:696-705)
- [] GetScreenToWorld2D / GetMousePosition: understand the screen-to-world coordinate conversion pipeline (game.c:381-382)
- [x] GetFrameTime: why clamp at 0.05? is it a max frame time for physics stability? (game.c:724-728)
- [x] SetWindowSize: can we add dynamic resize and fullscreen support? what APIs exist? (game.c:716)
- [] SetTargetFPS: minimum 60 even on 30fps screens? internal game logic fps vs draw fps? (game.c:1209-1212)
- [] Camera2D: how do target and offset interact? smooth follow pattern (game.c:696-718)
- [] emscripten_set_main_loop: why does the static GameState + void(void) pattern matter? (game.c:139-141)
- [] 3D projection Z ordering: smallest Z is "far"? Z+ is near? understand raylib coordinate system (game.c:844)
- [] Vector2Subtract, Vector2Length, Vector2Normalize, Vector2Scale, Vector2Add: read implementations in raylib.h (game.c:360-362, 571)
- [] GetRandomValue: range behavior and distribution (game.c:325, 463)
- [] IsKeyDown vs IsKeyPressed: when to use which for input handling (game.c:398, 732)
- [] CheckCollisionCircles: how collision detection works under the hood (game.c:~580+)
- [] DrawCircleSector / DrawRingSector: understand arc drawing for sword swing visuals (game.c:~490+)
- [] finishing the textbook math for programmers.

## Raylib.h: Further Questions
Currently OpenGL is the backend graphics accelerator. Is it possible to build a custom render engine? Thinking along the line of: ray tracing, voxels, webgpu, etc. Would probably be a lot of work though. Worth looking into the rlgl implementation.

GLSL is worth learning. This is for WebGL.

Why is Matrix a 4by4? This an OpenGL thing?

Ok I gotta learn what mipsmaps are exactly. How Image/Texture are stored is important for when I design the custom asset format.

I see there is some level of gesture support? is this for touch? worth exploring for mobile support.

Row major vs Column major math? Why is it cache friendly?

What are the implications of input parameters received by value only? 

## Damage/Delivery System / Interaction Design
The game is clearly borrowing from the moba genre, where there are tons of different damage types.

The core question to figuring out the weapons is designing some kind of damage calculating system. what types of damage are there, gun, explosive, pierce melee, ability (ap), true/pure damage what does it scale with (deadlock style). Is there bullet/spirit resist on enemies, do you want to build shred. 

Types of damage: Explosive, Balistic, Hitscan, Melee(Pierce/Blunt/Sharp), Ability, Pure

Types of damaging entities: Sword, Bullets, Lasers, Rockets

Armor is Bullet Resist, mecha games you fight enemies that are robots? they have armor?

What matters most is building the abstraction of DeliveryType(needs a better name) and a DamageType

## Build system
Highkey the wasm target with histos in the pipeline is just goated.
- [x] linux
- [x] macos
    - [] emcc working but have to source emsdk_env.sh every time
    - clang
- [x] windows
    - oquay

idea: a docker image for dev environment (0QUAY)
- can be spun up on any OS (ubuntu)
- ~~get to practice NixOS config?~~
- nvim, tmux
- all build tools auto installed
- ??? (it works were in it right now)

## Music Flow
rooms = zones = objectives, a level to challenge your mechanics against

like a song, the addition of elements to the music, through loops simple combinations of instruments together (enemies) the absence and removal of a sound is just as artful. this can be programmatic. the enemies and music go together. that is techno. phylyps trak - basic channel. that is that music programming language.

## Custom asset blob encoder/loader
The point of implementing this is that we can have one asset blob that is packed inside our game binary. 

from [Raylib FAQ](https://github.com/raysan5/raylib/blob/master/FAQ.md)
```
raylib can load data from multiple standard file formats:
    Image/Textures: PNG, BMP, TGA, JPG, GIF, QOI, PSD, DDS, HDR, KTX, ASTC, PKM, PVR
    Fonts:          FNT (sprite font), TTF, OTF
    Models/Meshes:  OBJ, IQM, GLTF, VOX, M3D
    Audio:          WAV, OGG, MP3, FLAC, XM, MOD, QOA
```

Make an enum for each of the 28 file types. That fits inside 5 bits.

Taking a lesson from the ethernet frame, Header + Payload. The header will tell you the length of the payload and the type of file, what else would be important to put inside the header? The filename?

Pulling back. I am a stack enjoyer, so we want to pre allocate an array for the assets? Meaning we have a maximum number of assets? We should also have a max asset size.

Is it worth creating a type alias header so that I don't have to do uint8_t and can do u8?

### Table of Contents (ToC)
A way to store the position of the assets inside the blob.

Blob
    - Header (fixed size) same as info
    - Table of Contents
    - String table
    - Payload

See asset_blob/blobbert.c for details

