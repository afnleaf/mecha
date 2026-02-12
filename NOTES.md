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

## v(-2)
add all the weapons and enemies

## v(-1)
a room with the first 4 enemies, defeat them

game pauses you pick an extra weapon out of some options

same room with more enemies, defeat them

game pauses and you pick another weapon

mini boss fight

game end

## v(0)
pick a chassis type to start with

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
- [x] (claude) fix hitmask — replaced with sweep line + lastHitAngle
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

refactor
- [] refactorin time
- [] bullet to projectile or base struct
- [] figure out how to do hitscan, what are the other weapon types?
- [] big refactor of all hardcoded numbers
- [] understand particles better (this is an art thing...)
- [] (user) figure out how to separate particle/weapon animation from game logic update?
- [] (user) refactor anything badly named and organized (LLMisms)
- [] (user) proceduraly gen textures for debug, background (grass? biomes?) 
- [] physics as deterministic for multiplayer future? (FPS TARGET)
- [x] (claude) linger effect refactor
- [] make it easy to adjust game feel
- [] ~~~bullet pool scaling separation player and enemy, also ownership~~~
- [] ~~~fix what dash follows, mouse or wasd? what takes prio?~~~

other features
- [] (user) damage system design (not implementation, what is the math behind it)
- [] (user) asset blob data design, how to parse? how to create?
- [] shaders GLSL
- [] shadows under entity

enemy types
- [x] (claude) add a pentagon enemy type. they are a pentagon, neon green. They shoot two big rows of 5 bullets each.
- [x] (claude) add rhombus enemy (chase)
- [x] (claude) add hexagon enemy (fan shooter)
- [x] (user) add octagon enemy (stopper)
- [] add mini boss (shape?) multiple attacks
- [] add big boss circle

weapons
- [x] (claude) shotgun (knockback)
- [x] (user + claude) rocket launcher (explosion)
- [] laser (hitscan)
- [] grenade launcher (delayed explosion)
- [] railgun (histcan big damage)
- [] big gun wip (BFG10k)

abilities
- [x] dash
- [x] spin
- [] grenades?
- are weapons just abilities in a mecha? buttons

levels
- [] rounds/rooms
- [] pick new item between rounds]

idea: two characters? the cube and the tetrahedron? rest of platonic solids: octahedron, dodecahedron, icosahedron. Make the animation it rolling on each side? (I like this). 2d shapes vs 3d shape, you leave your shadow in this 2d world.

## Research
- [x] read many parts of raylib.h
- [x] study vector 2 implementation (raymath.h)
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
- [x] linux
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

## Music Flow

rooms = zones = objectives, a level to challenge your mechanics against

like a song, the addition of elements to the music, through loops simple combinations of instruments together (enemies) the absence and removal of a sound is just as artful. this can be programmatic. the enemies and music go together. that is techno. phylyps trak - basic channel. that is that music programming language.

## Claude review
Moved to `claude_review.md`.


