/*
 * world.c - floor loading, visibility, movement and turn flow.
 * This is the "board": it knows where things are, not how they look.
 */
#include "game.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int rand_range(int n) {
    if (n <= 1) return 0;
    return (int)(asm_random() % (unsigned int)n);
}

const EnemyDef *enemy_def(EnemyType type) {
    return &ENEMY_DEFS[(int)type];
}

const EnemyTrait *enemy_trait(EnemyType type) {
    return &ENEMY_TRAITS[(int)type];
}

int player_atk(const Game *g) {
    int bonus = WEAPONS[g->weapon].bonus;
    int affix = g->weapon_affix[g->weapon];
    if (affix >= 0) bonus += AFFIXES[affix].atk;
    if (g->accessory >= 0) bonus += ACCESSORIES[g->accessory].atk;
    return g->atk + bonus;
}

int player_def(const Game *g) {
    int bonus = 0;
    int affix = g->weapon_affix[g->weapon];
    if (affix >= 0) bonus += AFFIXES[affix].def;
    if (g->accessory >= 0) bonus += ACCESSORIES[g->accessory].def;
    return g->def + bonus;
}

int player_lifesteal(const Game *g) {
    int steal = 0;
    int affix = g->weapon_affix[g->weapon];
    if (affix >= 0) steal += AFFIXES[affix].lifesteal;
    if (g->accessory >= 0) steal += ACCESSORIES[g->accessory].lifesteal;
    return steal;
}

int player_crit(const Game *g) {
    int crit = 12;
    int affix = g->weapon_affix[g->weapon];
    if (affix >= 0) crit += AFFIXES[affix].crit;
    return crit;
}

int combat_atk(const Game *g) {
    return player_atk(g) + g->combat_buff;
}

const char *floor_name(const Game *g, int floor_index) {
    if (g->random_mode) return "Shifting Halls";
    if (floor_index < 0) floor_index = 0;
    if (floor_index >= FLOORS) floor_index = FLOORS - 1;
    return FLOOR_NAMES[floor_index];
}

int npc_at(const Game *g, int x, int y) {
    for (int i = 0; i < NPC_COUNT; i++) {
        if (g->npc_x[i] == x && g->npc_y[i] == y)
            return i;
    }
    return -1;
}

void log_add(Game *g, const char *fmt, ...) {
    if (g->log_count >= LOG_LINES) {
        memmove(g->log[0], g->log[1],
                sizeof(g->log[0]) * (size_t)(LOG_LINES - 1));
        g->log_count = LOG_LINES - 1;
    }
    char *dst = g->log[g->log_count];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(dst, sizeof(g->log[0]), fmt, ap);
    va_end(ap);
    g->log_count++;
}

int enemy_at(const Game *g, int x, int y) {
    for (int i = 0; i < g->enemy_count; i++) {
        if (g->enemies[i].alive &&
            g->enemies[i].x == x && g->enemies[i].y == y)
            return i;
    }
    return -1;
}

int item_at(const Game *g, int x, int y) {
    for (int i = 0; i < g->item_count; i++) {
        if (!g->items[i].taken &&
            g->items[i].x == x && g->items[i].y == y)
            return i;
    }
    return -1;
}

static int line_of_sight(const Game *g, int x0, int y0, int x1, int y1) {
    int dx = iabs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -iabs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        if (x0 == x1 && y0 == y1) return 1;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
        if (x0 == x1 && y0 == y1) return 1;
        if (g->map[y0][x0] == T_WALL) return 0;
    }
}

void update_visibility(Game *g) {
    asm_fill32(g->visible, 0, (unsigned long)(MAP_W * MAP_H));

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            int dx = x - g->px;
            int dy = y - g->py;
            if (dx * dx + dy * dy <= 26 &&
                line_of_sight(g, g->px, g->py, x, y)) {
                g->visible[y][x] = 1;
                g->seen[y][x] = 1;
            }
        }
    }
    g->visible[g->py][g->px] = 1;
    g->seen[g->py][g->px] = 1;
}

void load_floor(Game *g, int floor_index) {
    if (floor_index < 0) floor_index = 0;
    if (floor_index >= FLOORS) floor_index = FLOORS - 1;

    g->floor = floor_index;

    if (g->random_mode) {
        generate_floor(g, floor_index);
        g->floor = floor_index;
        log_add(g, "Floor %d: %s.", floor_index + 1, floor_name(g, floor_index));
        quest_refresh(g);
        update_visibility(g);
        return;
    }

    g->enemy_count = 0;
    g->item_count = 0;
    g->log_count = 0;
    g->px = 1;
    g->py = 1;

    asm_fill32(g->map, (unsigned int)T_WALL,
               (unsigned long)(MAP_W * MAP_H));
    asm_fill32(g->seen, 0, (unsigned long)(MAP_W * MAP_H));
    asm_fill32(g->visible, 0, (unsigned long)(MAP_W * MAP_H));

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            char c = MAPS[floor_index][y][x];
            switch (c) {
            case '#':
                g->map[y][x] = T_WALL;
                break;
            case '+':
                g->map[y][x] = T_DOOR;
                break;
            case '>':
                g->map[y][x] = T_STAIRS;
                break;
            case 'S':
                g->map[y][x] = T_SHOP;
                break;
            case '@':
                g->map[y][x] = T_FLOOR;
                g->px = x;
                g->py = y;
                break;
            case 's':
            case 'g':
            case 'b':
            case 'k':
            case 'r':
            case 'x':
            case 'i':
            case 'z':
            case 'w':
            case 'o':
            case 'L':
            case 'c':
            case 'W':
            case 'D': {
                EnemyType t = E_SLIME;
                if (c == 'g') t = E_GOBLIN;
                else if (c == 'b') t = E_BAT;
                else if (c == 'k') t = E_SKELETON;
                else if (c == 'r') t = E_RAT;
                else if (c == 'x') t = E_SPIDER;
                else if (c == 'i') t = E_IMP;
                else if (c == 'z') t = E_GHOUL;
                else if (c == 'w') t = E_WRAITH;
                else if (c == 'o') t = E_GOLEM;
                else if (c == 'L') t = E_LICH;
                else if (c == 'c') t = E_CULTIST;
                else if (c == 'W') t = E_WYRM;
                else if (c == 'D') t = E_DRAGON;
                if (g->enemy_count < MAX_ENEMIES) {
                    Enemy *e = &g->enemies[g->enemy_count++];
                    e->x = x;
                    e->y = y;
                    e->type = t;
                    e->hp = enemy_def(t)->hp;
                    e->max_hp = e->hp;
                    e->alive = 1;
                }
                g->map[y][x] = T_FLOOR;
                break;
            }
            case '!':
            case '$': {
                if (g->item_count < MAX_ITEMS) {
                    Item *it = &g->items[g->item_count++];
                    it->x = x;
                    it->y = y;
                    it->type = (c == '!') ? IT_POTION : IT_GOLD;
                    it->amount = (it->type == IT_GOLD)
                                     ? 4 + floor_index * 3 + rand_range(8)
                                     : 1;
                    it->taken = 0;
                }
                g->map[y][x] = T_FLOOR;
                break;
            }
            default:
                g->map[y][x] = T_FLOOR;
                break;
            }
        }
    }

    for (int i = 0; i < NPC_COUNT; i++) {
        if (NPCS[i].floor == floor_index) {
            g->npc_x[i] = NPCS[i].x;
            g->npc_y[i] = NPCS[i].y;
        } else {
            g->npc_x[i] = -1;
            g->npc_y[i] = -1;
        }
    }

    log_add(g, "Floor %d: %s.", floor_index + 1, floor_name(g, floor_index));
    update_visibility(g);
}

void new_run(Game *g, int floor_index, unsigned int seed, int random_mode) {
    memset(g, 0, sizeof(*g));
    asm_srandom(seed);
    g->random_mode = random_mode;

    g->hp = 24;
    g->max_hp = 24;
    g->atk = 5;
    g->def = 1;
    g->level = 1;
    g->xp = 0;
    g->xp_next = 10;
    g->gold = 0;
    g->supplies[SUP_POTION] = 2;
    g->supplies[SUP_BOMB] = 1;
    g->supplies[SUP_ELIXIR] = 0;
    g->weapon = 0;
    g->best_weapon = 0;
    g->weapons_owned = 1u;
    for (int i = 0; i < WEAPON_COUNT; i++) g->weapon_affix[i] = -1;
    g->accessory = -1;
    g->accessories_owned = 0;
    for (int i = 0; i < QUEST_COUNT; i++) {
        g->quest_state[i] = 0;
        g->quest_progress[i] = 0;
    }
    g->running = 1;
    g->seed = seed;
    g->mode = MODE_MAP;

    load_floor(g, floor_index);
    quest_refresh(g);
    g->log_count = 0;
    log_add(g, "The gate seals behind you. Only the stairs lead on.");
    log_add(g, "Floor %d: %s.", g->floor + 1, floor_name(g, g->floor));
    if (floor_index == FLOORS - 1)
        log_add(g, "The dragon stirs at the far end of the hall.");
    else if (floor_index == 5)
        log_add(g, "A crowned lich guards the way deeper.");
    else if (floor_index == 9)
        log_add(g, "Heat pours from the foundry cracks.");
    else if (floor_index == 11)
        log_add(g, "The crimson gate hums with old fire.");
}

void descend(Game *g) {
    if (g->floor >= FLOORS - 1) return;
    int next = g->floor + 1;
    load_floor(g, next);
    g->log_count = 0;
    log_add(g, "You descend the narrow stair. The air grows colder.");
    log_add(g, "Floor %d: %s.", next + 1, floor_name(g, next));
    if (next == FLOORS - 1)
        log_add(g, "Something enormous breathes in the dark.");
    else if (next == 5)
        log_add(g, "A cold crown glitters beyond the gate.");
    else if (next == 9)
        log_add(g, "The foundry's heat presses against your skin.");
    else if (next == 11)
        log_add(g, "The gate drinks the light. Something waits beyond.");
    quest_refresh(g);
    update_visibility(g);
    if (g->save_path[0]) save_game(g, g->save_path);
}

void pickup_item(Game *g, int index) {
    Item *it = &g->items[index];
    it->taken = 1;

    if (it->type == IT_POTION) {
        if (g->supplies[SUP_POTION] < POTION_CAP) {
            g->supplies[SUP_POTION]++;
            log_add(g, "You pick up a potion. (Potions: %d)",
                    g->supplies[SUP_POTION]);
        } else {
            g->gold += 5;
            log_add(g, "Your pack is full. You trade the potion for 5 gold.");
        }
    } else if (it->type == IT_ACCESSORY) {
        if (!grant_accessory(g, it->amount, 1)) {
            g->gold += 40;
            log_add(g, "You already own that charm. You sell it for 40 gold.");
        }
    } else {
        g->gold += it->amount;
        log_add(g, "You pick up %d gold.", it->amount);
    }
}

static int enemy_blocked(const Game *g, int x, int y, int self) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return 1;
    if (g->map[y][x] == T_WALL) return 1;
    if (npc_at(g, x, y) >= 0) return 1;
    for (int i = 0; i < g->enemy_count; i++) {
        if (i != self && g->enemies[i].alive &&
            g->enemies[i].x == x && g->enemies[i].y == y)
            return 1;
    }
    return 0;
}

void move_enemies(Game *g);

void map_drink_potion(Game *g) {
    if (g->supplies[SUP_POTION] <= 0) {
        log_add(g, "You have no potions left!");
        return;
    }
    if (g->hp >= g->max_hp) {
        log_add(g, "You are already at full health.");
        return;
    }

    g->supplies[SUP_POTION]--;
    int healed = imin(12, g->max_hp - g->hp);
    g->hp += healed;
    log_add(g, "You drink a potion and recover %d HP.", healed);
    g->turns++;
    update_visibility(g);
    move_enemies(g);
}

void move_enemies(Game *g) {
    for (int i = 0; i < g->enemy_count; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->alive) continue;

        int dist = iabs(e->x - g->px) + iabs(e->y - g->py);

        /* The dragon guards its gate and never gives chase. */
        if (e->type == E_DRAGON) continue;

        int chase = (dist <= 6 && rand_range(100) < 65);
        int moved = 0;

        if (chase) {
            int prefer_x = iabs(g->px - e->x) >= iabs(g->py - e->y);
            int order[2][2];
            if (prefer_x) {
                order[0][0] = (g->px > e->x) ? 1 : -1; order[0][1] = 0;
                order[1][0] = 0; order[1][1] = (g->py > e->y) ? 1 : -1;
            } else {
                order[0][0] = 0; order[0][1] = (g->py > e->y) ? 1 : -1;
                order[1][0] = (g->px > e->x) ? 1 : -1; order[1][1] = 0;
            }
            for (int k = 0; k < 2 && !moved; k++) {
                if (order[k][0] == 0 && order[k][1] == 0) continue;
                int nx = e->x + order[k][0];
                int ny = e->y + order[k][1];
                if (nx == g->px && ny == g->py) {
                    start_combat(g, i, 1);
                    return;
                }
                if (!enemy_blocked(g, nx, ny, i)) {
                    e->x = nx;
                    e->y = ny;
                    moved = 1;
                }
            }
        } else if (rand_range(100) < 22) {
            int dir = rand_range(4);
            int dx = 0, dy = 0;
            if (dir == 0) dx = 1;
            else if (dir == 1) dx = -1;
            else if (dir == 2) dy = 1;
            else dy = -1;
            int nx = e->x + dx;
            int ny = e->y + dy;
            if (!(nx == g->px && ny == g->py) &&
                !enemy_blocked(g, nx, ny, i)) {
                e->x = nx;
                e->y = ny;
            }
        }
    }
    update_visibility(g);
}

void try_move(Game *g, int dx, int dy) {
    int nx = g->px + dx;
    int ny = g->py + dy;

    if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) return;
    if (g->map[ny][nx] == T_WALL) {
        log_add(g, "The wall blocks your way.");
        return;
    }

    int ni = npc_at(g, nx, ny);
    if (ni >= 0) {
        open_dialogue(g, ni);
        return;
    }

    int ei = enemy_at(g, nx, ny);
    if (ei >= 0) {
        start_combat(g, ei, 0);
        return;
    }

    if (g->map[ny][nx] == T_SHOP) {
        open_shop(g);
        return;
    }

    if (g->map[ny][nx] == T_STAIRS) {
        if (g->floor >= FLOORS - 1) {
            log_add(g, "The stair is sealed. The dragon still breathes.");
            return;
        }
        descend(g);
        return;
    }

    g->px = nx;
    g->py = ny;
    g->turns++;

    int ii = item_at(g, nx, ny);
    if (ii >= 0) pickup_item(g, ii);

    update_visibility(g);
    move_enemies(g);
}

void wait_turn(Game *g) {
    g->turns++;
    log_add(g, "You listen to the dark...");
    update_visibility(g);
    move_enemies(g);
}
