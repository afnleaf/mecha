# problem solving space

feel free to use these urls to explore
- https://www.raylib.com/cheatsheet/cheatsheet.html
- raylib.h inside ./raylib/src/raylib.h
- https://github.com/raysan5/raylib/wiki/Frequently-Asked-Questions

joint implementation between claude and myself the user! 🤝🫡

maybe we should refactor the huge update and draw functions? but, not sure what the proper way to approach this problem is. we are at the point where if we keep adding new stuff to the game, our current approach would be unmaintainable, but it works right now in this super early prototype version. This is a good spot to be in from a single "vibe coding" session, but continuing on with feature creep would be a massive mistake. We need to understand the data transformation problem better before we tackle a re design. 

draw vs update

so draw is obviously handling the visuals

update should be updating the game state

the update happens before the draw

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


## todo
- [x] optimized native build /usaeadded
- [x] iframe granted by dash at start. 
- [x] add background and grid color to default
- [x] refactor draw to World and HUD.
- [x] make the player model an cube that rotates but in 2d but it looks 3dish? if you catch my drift
- [x] dash animation, ghosting through enemies
- [x] make dash longer
- [] add enemy type, the debug texture as a rectangle, bigger than triangle, more hp and shoots bullets every half second at your current location
- [] bullet pool scaling separation player and enemy, also ownership
- [] refactor update (design clean approach to separating this like draw world and draw hud)
- [] make the player model HSV, or RBG, so the texture is generated algorithmically by going through each possible value of HSV or RBH u know 3 u8s right uint8_t in C? it needs to look like a rainbow. still not working properly but close
- [] fix what dash follows, mouse or wasd? what takes prio?
- [] damage system design (not implementation, what is the math behind it)
- [] asset blob data design, how to parse? how to create?
- [] physics as deterministic for multiplayer future? (FPS TARGET)
- [] shadows under entity
- [] proceduraly gen textures for enemies, background (grass? diff biomes?)
- [] understand particles better (this is an art thing...)
- [] make the sword flash with particles
- [] linger effect refactor
- [] shaders GLSL
- [] study vector 2 implementation
- [] reading raylib.h





