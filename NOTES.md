# problem solving space

feel free to use these urls to explore
- [Cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [Math Sheet](https://www.raylib.com/cheatsheet/raymath_cheatsheet.html)
- [raylib.h](https://github.com/raysan5/raylib/blob/master/src/raylib.h) inside ./raylib/src/raylib.h
- [FAQ Technical](https://github.com/raysan5/raylib/wiki/Frequently-Asked-Questions)
- [FAQ Generic](https://github.com/raysan5/raylib/blob/master/FAQ.md

joint implementation between claude and myself the user! 🤝🫡

``` C
// this is THE piece of data that we are operating on
// it sits inside of our pipeline
typedef struct GameState {
    Player player;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    Particle particles[MAX_PARTICLES];
    Camera2D camera;
    int score;
    float spawnTimer;
    float spawnInterval;
    int enemiesKilled;
    bool gameOver;
    bool paused;
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
- [] fix hitmask
- [x] (claude) add a pentagon enemy type. they are a pentagon, neon green. They shoot two big rows of 5 bullets each.
- [x] (claude) add rhombus enemy
- [] add hexagon enemy
- [] add octagon enemy
- [] add trapezoid mini boss
- [] add circle big boss
- [] figure out how to separate particle/weapon animation from game logic update?
- [x] (user) add green particles on heal 
- [x] (user) separate update camera and window resize logic
- [] give each enemy their own score
- [] (user) make the player model HSV, or RBG, so the texture is generated algorithmically by going through each possible value of HSV or RBH u know 3 u8s right uint8_t in C? it needs to look like a rainbow. still not working properly but close
- [] big refactor of all hardcoded numbers
- [] (user) refactor anything badly named and organized (LLMisms)
- [] proceduraly gen textures for debug, enemies, background (grass? diff biomes?) 
- [] (user) damage system design (not implementation, what is the math behind it)
- [] (user) asset blob data design, how to parse? how to create?
- [] physics as deterministic for multiplayer future? (FPS TARGET)
- [] shadows under entity
- [] understand particles better (this is an art thing...)
- [] make the sword flash with particles
- [] linger effect refactor
- [] shaders GLSL
- [] bullet pool scaling separation player and enemy, also ownership
- [] ~~~fix what dash follows, mouse or wasd? what takes prio?~~~

## Research
- [x] read many parts of raylib.h
- [] study vector 2 implementation (raymath.h)
- [x] Vector2Lerp: what is it doing exactly? how much can we fiddle with the lerp constant (8.0f)? (game.c:696-705)
- [] GetScreenToWorld2D / GetMousePosition: understand the screen-to-world coordinate conversion pipeline (game.c:381-382)
- [x] GetFrameTime: why clamp at 0.05? is it a max frame time for physics stability? (game.c:724-728)
- [] SetWindowSize: can we add dynamic resize and fullscreen support? what APIs exist? (game.c:716)
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

## Build system
Highkey the wasm target with histos in the pipeline is just goated.
- [x] linux (eassss)
- [] macos
    - emcc
    - clang
- [] windows
    - ????
    - mingw, how does raylib build, etc

idea: a docker image for dev environment
- can be spun up on any OS
- get to practice NixOS config?
- nvim, tmux
- all build tools auto installed
- ???

## Damage interaction design
The game is clearly borrowing from the moba genre, where there are tons of different damage types.

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

## Claude thoughts

### Bug: hitMask only covers 64 enemies
`uint64_t hitMask` on Sword and Spin only tracks enemies at index 0-63 via bit shifts. MAX_ENEMIES is 1024. Any enemy at index 64+ gets hit every frame of a swing instead of once. Doesn't manifest at current spawn rates but will once the arena fills up. Options: a second parallel bool array `bool hitThisSwing[MAX_ENEMIES]` inside GameState (reset on swing start), or a generation counter per-enemy that gets stamped when hit.

### Enemy-enemy separation
Enemies stack into the same pixel chasing the player. A simple repulsion pass in UpdateEnemies (for each pair within overlap distance, push apart along the delta vector) would make the swarm look way better and give the player readable silhouettes to aim at. Doesn't need to be a full physics solve — just a nudge.

### Spawn position hardcoded +-500
Line 297 etc. The random offset from player position is a magic number independent of map size or screen size. Tying it to SPAWN_MARGIN or a fraction of MAP_SIZE would be more robust as the map scales.

### Pool scanning is linear
SpawnBullet/SpawnEnemy/SpawnParticle all do O(n) scans for a free slot. Fine at 1024. If pools ever grow, a free-list (track `int nextFree` and chain free slots) is a cheap upgrade. Also relevant to the bullet pool separation todo — when you split player/enemy bullets, each pool gets its own free-list.

### OBB collision duplication
The RECT oriented bounding box check appears in 4 places: sword hit, spin hit, bullet hit, contact damage. Each new enemy shape (hexa, octa, trap) will multiply this. A single `bool EnemyHitTest(Enemy *e, Vector2 point, float radius)` that switches on `e->type` would centralize the collision logic. The data is the same — it's just the shape test that varies per type.

### Spin lifesteal scaling
`SPIN_DAMAGE * 0.05` = 2hp per hit, cast to int. Works now, but as damage numbers grow this will always round down. Consider storing heal as float accumulator and only applying integer part, or just bumping the percentage. Minor, but worth knowing the truncation is there.

### Gameplay depth vs code ratio
The ratio of mechanical depth to lines of code is high right now — 4 enemy types, 4 player abilities with meaningful interactions (dash-slash combo, retroactive upgrade, spin lifesteal), charge systems, and a progression ramp in ~1700 lines. This is the sweet spot to be in before the skeleton prototype. The instinct to stop feature creep and understand the data transformation is correct.








