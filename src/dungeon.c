/*
 * dungeon.c - procedural floors for --random.
 *
 * Rooms are carved as rectangles and connected in sequence, so the floor is
 * always connected by construction.  The layout is stored in the normal Game
 * map, which means saving a random run needs no special case.
 */
#include "game.h"

#include <string.h>

typedef struct {
    int x, y, w, h;
} Room;

static int room_cx(const Room *r) { return r->x + r->w / 2; }
static int room_cy(const Room *r) { return r->y + r->h / 2; }

static int overlaps(const Room *a, const Room *b) {
    return !(a->x + a->w + 1 < b->x || b->x + b->w + 1 < a->x ||
             a->y + a->h + 1 < b->y || b->y + b->h + 1 < a->y);
}

static int add_room(Room *rooms, int count, Room r) {
    for (int i = 0; i < count; i++)
        if (overlaps(&rooms[i], &r)) return count;
    rooms[count] = r;
    return count + 1;
}

static int carve_room(Game *g, const Room *r) {
    for (int y = r->y; y < r->y + r->h; y++)
        for (int x = r->x; x < r->x + r->w; x++) {
            if (x <= 0 || x >= MAP_W - 1 || y <= 0 || y >= MAP_H - 1) return 0;
            g->map[y][x] = T_FLOOR;
        }
    return 1;
}

static void carve_corridor(Game *g, int x0, int y0, int x1, int y1) {
    int x = x0, y = y0;
    if (rand_range(2)) {
        while (x != x1) { x += (x1 > x) ? 1 : -1; g->map[y][x] = T_FLOOR; }
        while (y != y1) { y += (y1 > y) ? 1 : -1; g->map[y][x] = T_FLOOR; }
    } else {
        while (y != y1) { y += (y1 > y) ? 1 : -1; g->map[y][x] = T_FLOOR; }
        while (x != x1) { x += (x1 > x) ? 1 : -1; g->map[y][x] = T_FLOOR; }
    }
}

static int free_cell(const Game *g, int x, int y) {
    if (x <= 0 || x >= MAP_W - 1 || y <= 0 || y >= MAP_H - 1) return 0;
    if (g->map[y][x] != T_FLOOR) return 0;
    if (x == g->px && y == g->py) return 0;
    if (npc_at(g, x, y) >= 0) return 0;
    if (enemy_at(g, x, y) >= 0) return 0;
    if (item_at(g, x, y) >= 0) return 0;
    return 1;
}

static int random_cell_in(const Game *g, const Room *r, int *x, int *y) {
    for (int tries = 0; tries < 80; tries++) {
        int tx = r->x + rand_range(r->w);
        int ty = r->y + rand_range(r->h);
        if (free_cell(g, tx, ty)) {
            *x = tx;
            *y = ty;
            return 1;
        }
    }
    return 0;
}

static void place_enemy(Game *g, EnemyType type, int x, int y) {
    if (g->enemy_count >= MAX_ENEMIES) return;
    Enemy *e = &g->enemies[g->enemy_count++];
    e->x = x;
    e->y = y;
    e->type = type;
    e->hp = enemy_def(type)->hp;
    e->max_hp = e->hp;
    e->alive = 1;
}

static void place_item(Game *g, ItemType type, int amount, int x, int y) {
    if (g->item_count >= MAX_ITEMS) return;
    Item *it = &g->items[g->item_count++];
    it->x = x;
    it->y = y;
    it->type = type;
    it->amount = amount;
    it->taken = 0;
}

static const EnemyType FLOOR_POOL[FLOORS][4] = {
    { E_SLIME,  E_RAT,    E_SLIME,  E_RAT    },
    { E_GOBLIN, E_BAT,    E_RAT,    E_SPIDER },
    { E_SKELETON, E_SPIDER, E_IMP,  E_GOBLIN },
    { E_IMP,    E_GHOUL,  E_SKELETON, E_SPIDER },
    { E_WRAITH, E_GHOUL,  E_IMP,    E_GOLEM  },
    { E_SKELETON, E_WRAITH, E_GOLEM, E_GHOUL },
    { E_WRAITH, E_GOLEM,  E_IMP,    E_GHOUL  },
    { E_WRAITH, E_GOLEM,  E_GHOUL,  E_IMP    },
    { E_GOLEM,  E_WRAITH, E_IMP,    E_GHOUL  },
    { E_CULTIST, E_IMP,   E_GOLEM,  E_CULTIST },
    { E_WRAITH, E_CULTIST, E_GHOUL, E_WRAITH },
    { E_CULTIST, E_WYRM,  E_WRAITH, E_GOLEM  },
    { E_GOLEM,  E_WRAITH, E_WYRM,   E_GOLEM  },
    { E_WYRM,   E_GOLEM,  E_CULTIST, E_WYRM  }
};

void generate_floor(Game *g, int floor_index) {
    Room rooms[8];
    int count = 0;

    memset(g->map, 0, sizeof(g->map));
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) g->map[y][x] = T_WALL;

    g->enemy_count = 0;
    g->item_count = 0;
    g->log_count = 0;
    for (int i = 0; i < NPC_COUNT; i++) {
        g->npc_x[i] = -1;
        g->npc_y[i] = -1;
    }

    for (int tries = 0; tries < 160 && count < 7; tries++) {
        Room r;
        r.w = 4 + rand_range(4);
        r.h = 3 + rand_range(2);
        r.x = 1 + rand_range(MAP_W - r.w - 2);
        r.y = 1 + rand_range(MAP_H - r.h - 2);
        count = add_room(rooms, count, r);
    }

    if (count < 5) {
        /* Guaranteed fallback: a 3x2 grid of rooms, still jittered. */
        count = 0;
        for (int gy = 0; gy < 2; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                Room r;
                r.w = 6;
                r.h = 4;
                r.x = 2 + gx * 10 + rand_range(2) - 1;
                r.y = 1 + gy * 8 + rand_range(2) - 1;
                rooms[count++] = r;
            }
        }
    }

    for (int i = 0; i < count; i++) carve_room(g, &rooms[i]);
    for (int i = 1; i < count; i++)
        carve_corridor(g, room_cx(&rooms[i - 1]), room_cy(&rooms[i - 1]),
                       room_cx(&rooms[i]), room_cy(&rooms[i]));

    g->px = room_cx(&rooms[0]);
    g->py = room_cy(&rooms[0]);
    g->map[g->py][g->px] = T_FLOOR;

    int sx = 0, sy = 0;
    const Room *last = &rooms[count - 1];
    if (!random_cell_in(g, last, &sx, &sy)) {
        sx = room_cx(last);
        sy = room_cy(last);
    }

    /* Boss first, then stairs on another free tile in the same room. */
    if (floor_index >= FLOORS - 2) {
        place_enemy(g, floor_index == FLOORS - 1 ? E_KING : E_DRAGON,
                    room_cx(last), room_cy(last));
        for (int tries = 0; tries < 40; tries++) {
            int tx = last->x + rand_range(last->w);
            int ty = last->y + rand_range(last->h);
            if (free_cell(g, tx, ty)) { sx = tx; sy = ty; break; }
        }
    } else if (floor_index == 5) {
        place_enemy(g, E_LICH, room_cx(last), room_cy(last));
        for (int tries = 0; tries < 40; tries++) {
            int tx = last->x + rand_range(last->w);
            int ty = last->y + rand_range(last->h);
            if (free_cell(g, tx, ty)) { sx = tx; sy = ty; break; }
        }
    }
    g->map[sy][sx] = T_STAIRS;

    int shops = (floor_index == 0 || floor_index == 2 || floor_index == 3 ||
                 floor_index == 4 || floor_index == 6 || floor_index == 7 ||
                 floor_index == 8 || floor_index == 9 || floor_index == 11);
    if (shops && count >= 3) {
        int ri = 1 + rand_range(count - 2);
        const Room *r = &rooms[ri];
        for (int tries = 0; tries < 50; tries++) {
            int tx = r->x + rand_range(r->w);
            int ty = r->y + rand_range(r->h);
            if (free_cell(g, tx, ty)) {
                g->map[ty][tx] = T_SHOP;
                break;
            }
        }
    }

    for (int i = 0; i < NPC_COUNT; i++) {
        if (NPCS[i].floor != floor_index) continue;
        int ri = (i == 0) ? 1 : count - 2;
        if (ri < 0) ri = 0;
        const Room *r = &rooms[ri];
        for (int tries = 0; tries < 50; tries++) {
            int tx = r->x + rand_range(r->w);
            int ty = r->y + rand_range(r->h);
            if (free_cell(g, tx, ty)) {
                g->npc_x[i] = tx;
                g->npc_y[i] = ty;
                break;
            }
        }
    }

    int monsters = 4 + floor_index / 2;
    for (int i = 0; i < monsters; i++) {
        int ri = 1 + rand_range(count > 1 ? count - 1 : 1);
        if (ri >= count) ri = count - 1;
        const Room *r = &rooms[ri];
        int tx, ty;
        if (random_cell_in(g, r, &tx, &ty))
            place_enemy(g, FLOOR_POOL[floor_index][rand_range(4)], tx, ty);
    }

    int loot = 3 + rand_range(3);
    for (int i = 0; i < loot; i++) {
        int ri = rand_range(count);
        const Room *r = &rooms[ri];
        int tx, ty;
        if (!random_cell_in(g, r, &tx, &ty)) continue;
        int roll = rand_range(100);
        if (roll < 10 && floor_index >= 2)
            place_item(g, IT_ACCESSORY, rand_range(ACCESSORY_COUNT), tx, ty);
        else if (roll < 45)
            place_item(g, IT_GOLD, 5 + floor_index * 4 + rand_range(10), tx, ty);
        else
            place_item(g, IT_POTION, 1, tx, ty);
    }

    g->floor = floor_index;
}
