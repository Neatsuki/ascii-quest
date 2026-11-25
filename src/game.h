/*
 * ASCII QUEST - shared declarations.
 *
 * The project is intentionally split into small translation units:
 *
 *   data.c      static content tables (maps, monsters, gear, NPCs)
 *   ui.c        ANSI output buffer, line composition, terminal raw mode
 *   render.c    every screen the player can see
 *   world.c     floor loading, visibility, movement, pickups, stairs
 *   dungeon.c   procedural floor generation for --random
 *   combat.c    turn-based combat, traits, poison, XP
 *   inventory.c supplies, weapons, affixes, accessories, shop
 *   npc.c       dialogue and side quests
 *   save.c      versioned binary save file
 *   main.c      CLI, event loop and key handlers
 */
#ifndef ASCII_QUEST_GAME_H
#define ASCII_QUEST_GAME_H

#include <stddef.h>

/* ------------------------------------------------------------------------- */
/*  Layout and limits                                                        */
/* ------------------------------------------------------------------------- */
#define MAP_W        31
#define MAP_H        15
#define FLOORS       14
#define SCREEN_W     66
#define SCREEN_H     23
#define LOG_LINES    3
#define MAX_ENEMIES  24
#define MAX_ITEMS    24
#define NPC_COUNT    2
#define QUEST_COUNT  2
#define WEAPON_COUNT 6
#define AFFIX_COUNT  5
#define ACCESSORY_COUNT 4
#define SAVE_VERSION 2u

/* ------------------------------------------------------------------------- */
/*  ANSI colours                                                             */
/* ------------------------------------------------------------------------- */
#define C_RESET   "\x1b[0m"
#define C_BOLD    "\x1b[1m"
#define C_DIM     "\x1b[90m"
#define C_WHITE   "\x1b[97m"
#define C_RED     "\x1b[91m"
#define C_GREEN   "\x1b[92m"
#define C_YELLOW  "\x1b[93m"
#define C_MAGENTA "\x1b[95m"
#define C_CYAN    "\x1b[96m"

/* ------------------------------------------------------------------------- */
/*  Assembly helpers (32-bit cdecl)                                          */
/* ------------------------------------------------------------------------- */
extern long asm_write(int fd, const void *buf, unsigned long count);
extern long asm_read(int fd, void *buf, unsigned long count);
extern unsigned int asm_random(void);
extern void asm_srandom(unsigned int seed);
extern unsigned int asm_get_random_state(void);
extern void asm_set_random_state(unsigned int state);
extern void asm_fill32(void *dst, unsigned int value, unsigned long count);

/* ------------------------------------------------------------------------- */
/*  Enumerations                                                             */
/* ------------------------------------------------------------------------- */
typedef enum {
    T_FLOOR = 0,
    T_WALL,
    T_DOOR,
    T_STAIRS,
    T_SHOP
} Terrain;

typedef enum {
    IT_POTION = 0,
    IT_GOLD,
    IT_ACCESSORY
} ItemType;

typedef enum {
    E_SLIME = 0,
    E_GOBLIN,
    E_BAT,
    E_SKELETON,
    E_RAT,
    E_SPIDER,
    E_IMP,
    E_GHOUL,
    E_WRAITH,
    E_GOLEM,
    E_LICH,
    E_DRAGON,
    E_KING,
    E_CULTIST,
    E_WYRM,
    E_COUNT
} EnemyType;

typedef enum {
    SUP_POTION = 0,
    SUP_BOMB,
    SUP_ELIXIR,
    SUP_COUNT
} SupplyType;

typedef enum {
    MODE_TITLE = 0,
    MODE_MAP,
    MODE_COMBAT,
    MODE_HELP,
    MODE_SHOP,
    MODE_INV,
    MODE_DIALOGUE,
    MODE_QUIT_CONFIRM,
    MODE_DEAD,
    MODE_WIN
} Mode;

/* ------------------------------------------------------------------------- */
/*  Static content definitions                                               */
/* ------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    char glyph;
    int hp;
    int atk;
    int def;
    int xp;
    int gold_min;
    int gold_max;
    const char *color;
    const char *desc;
    const char *art[9];
} EnemyDef;

typedef struct {
    int dodge;
    int drain;
    int poison;
    int fire;
    int slow;
    int pierce;
} EnemyTrait;

typedef struct {
    const char *name;
    int bonus;
    int price;
} WeaponDef;

typedef struct {
    const char *name;
    int price;
    int cap;
    char key;
    const char *desc;
} SupplyDef;

/* A random modifier rolled onto a weapon when it is bought/found. */
typedef struct {
    const char *name;
    const char *short_name;
    int atk;
    int def;
    int lifesteal;
    int crit;
    const char *desc;
} AffixDef;

typedef struct {
    const char *name;
    int atk;
    int def;
    int lifesteal;
    int price;
    const char *desc;
} AccessoryDef;

typedef struct {
    const char *name;
    int floor;   /* zero based */
    int x;
    int y;
} NpcDef;

/* ------------------------------------------------------------------------- */
/*  Mutable game objects                                                     */
/* ------------------------------------------------------------------------- */
typedef struct {
    int x, y;
    EnemyType type;
    int hp;
    int max_hp;
    int alive;
} Enemy;

typedef struct {
    int x, y;
    ItemType type;
    int amount;  /* gold amount, or accessory index */
    int taken;
} Item;

typedef struct {
    int hp, max_hp;
    int atk, def;
    int level, xp, xp_next;
    int gold;

    int supplies[SUP_COUNT];

    int weapon;                         /* equipped weapon tier */
    int best_weapon;                    /* highest tier owned */
    unsigned int weapons_owned;         /* bit per tier */
    int weapon_affix[WEAPON_COUNT];     /* -1 = plain */

    int accessory;                      /* -1 = empty */
    unsigned int accessories_owned;     /* bit per accessory */

    int combat_buff;                    /* elixir attack bonus */
    int poison_turns;

    int px, py;
    int turns;

    int floor;
    Terrain map[MAP_H][MAP_W];
    unsigned int seen[MAP_H][MAP_W];
    unsigned int visible[MAP_H][MAP_W];

    Enemy enemies[MAX_ENEMIES];
    int enemy_count;
    Item items[MAX_ITEMS];
    int item_count;

    Mode mode;
    int running;
    int combat_enemy;
    int combat_turn;

    int log_count;
    char log[LOG_LINES][160];
    char shop_msg[160];
    char inv_msg[160];
    char notice[160];

    unsigned int seed;
    int random_mode;

    int quest_state[QUEST_COUNT];    /* 0 locked, 1 open, 2 active, 3 done */
    int quest_progress[QUEST_COUNT];
    int talked_npc[NPC_COUNT];
    int npc_x[NPC_COUNT];            /* current floor position, -1 = absent */
    int npc_y[NPC_COUNT];

    int dialogue_npc;
    int dialogue_page;

    char save_path[128];
} Game;

/* ------------------------------------------------------------------------- */
/*  Content tables (data.c)                                                  */
/* ------------------------------------------------------------------------- */
extern const EnemyDef ENEMY_DEFS[E_COUNT];
extern const EnemyTrait ENEMY_TRAITS[E_COUNT];
extern const WeaponDef WEAPONS[WEAPON_COUNT];
extern const SupplyDef SUPPLY_DEFS[SUP_COUNT];
extern const AffixDef AFFIXES[AFFIX_COUNT];
extern const AccessoryDef ACCESSORIES[ACCESSORY_COUNT];
extern const char *const MAPS[FLOORS][MAP_H];
extern const char *const FLOOR_NAMES[FLOORS];
extern const NpcDef NPCS[NPC_COUNT];

#define POTION_CAP   (SUPPLY_DEFS[SUP_POTION].cap)
#define POTION_PRICE (SUPPLY_DEFS[SUP_POTION].price)

/* ------------------------------------------------------------------------- */
/*  UI: output buffer, lines, terminal and input (ui.c)                      */
/* ------------------------------------------------------------------------- */
void out_begin(void);
void out_add(const char *s, int n);
void out_str(const char *s);
void out_flush(void);

typedef struct {
    char s[8192];
    int len;
    int vis;
} Line;

void line_init(Line *l);
void lraw(Line *l, const char *s);
void ltxt(Line *l, const char *s);
void lcol(Line *l, const char *color);
void lfmt(Line *l, const char *fmt, ...);
void lpad(Line *l, int col);
void out_line(Line *l, int last);

int term_setup(void);
void term_restore(void);
int read_key(void);

enum {
    KEY_EOF = -1,
    KEY_UP = 256,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_F5,
    KEY_F9
};

/* ------------------------------------------------------------------------- */
/*  Rendering (render.c)                                                     */
/* ------------------------------------------------------------------------- */
void render_game(const Game *g);

/* ------------------------------------------------------------------------- */
/*  Core helpers and world (world.c)                                         */
/* ------------------------------------------------------------------------- */
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iabs(int a) { return a < 0 ? -a : a; }

int rand_range(int n);
const EnemyDef *enemy_def(EnemyType type);
const EnemyTrait *enemy_trait(EnemyType type);
void log_add(Game *g, const char *fmt, ...);

const char *floor_name(const Game *g, int floor_index);
int player_atk(const Game *g);
int player_def(const Game *g);
int player_lifesteal(const Game *g);
int player_crit(const Game *g);
int combat_atk(const Game *g);

int enemy_at(const Game *g, int x, int y);
int item_at(const Game *g, int x, int y);
int npc_at(const Game *g, int x, int y);

void update_visibility(Game *g);
void load_floor(Game *g, int floor_index);
void new_run(Game *g, int floor_index, unsigned int seed, int random_mode);
void descend(Game *g);
void try_move(Game *g, int dx, int dy);
void wait_turn(Game *g);
void map_drink_potion(Game *g);
void pickup_item(Game *g, int index);
void move_enemies(Game *g);

/* ------------------------------------------------------------------------- */
/*  Procedural dungeon (dungeon.c)                                           */
/* ------------------------------------------------------------------------- */
void generate_floor(Game *g, int floor_index);

/* ------------------------------------------------------------------------- */
/*  Combat (combat.c)                                                        */
/* ------------------------------------------------------------------------- */
void level_up(Game *g);
void start_combat(Game *g, int enemy_index, int ambush);
void combat_player_attack(Game *g);
void combat_use_supply(Game *g, SupplyType type);
void combat_flee(Game *g);

/* ------------------------------------------------------------------------- */
/*  Inventory, gear and shop (inventory.c)                                   */
/* ------------------------------------------------------------------------- */
void open_shop(Game *g);
void shop_buy_supply(Game *g, SupplyType type);
void shop_buy_weapon(Game *g);
void shop_buy_accessory(Game *g);
void equip_weapon(Game *g, int index);
void cycle_accessory(Game *g);
void open_inventory(Game *g);
void inv_use_supply(Game *g, SupplyType type);
int grant_accessory(Game *g, int index, int announce);

/* ------------------------------------------------------------------------- */
/*  NPC dialogue and quests (npc.c)                                          */
/* ------------------------------------------------------------------------- */
void open_dialogue(Game *g, int npc_index);
void handle_dialogue(Game *g, int key);
const char *dialogue_title(const Game *g);
const char *dialogue_line(const Game *g, int page);
int dialogue_pages(const Game *g);
int dialogue_can_accept(const Game *g);
void quest_on_kill(Game *g, EnemyType type);
void quest_refresh(Game *g);

/* ------------------------------------------------------------------------- */
/*  Save file (save.c)                                                       */
/* ------------------------------------------------------------------------- */
int save_game(const Game *g, const char *path);
int load_game(Game *g, const char *path);

#endif /* ASCII_QUEST_GAME_H */
