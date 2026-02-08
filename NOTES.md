# problem solving space

joint implementation between claude and myself the user! 🤝🫡

**todo**

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

I'm surprised the binary executable is 1.1mb while the full index.html single file mode is 300kb.

Anyway I typed out a lot of questions inside the source file itself.








