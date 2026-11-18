/*
 * save.c - versioned binary save files.
 *
 * The format is deliberately explicit instead of fwrite(&game, sizeof game):
 * enums and padding differ between compilers, and a save that silently breaks
 * after a struct change is worse than no save at all.  Bump SAVE_VERSION when
 * the field list changes.
 */
#include "game.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC 0x41515356u   /* "AQSV" */

static int wr_u32(FILE *f, uint32_t v) {
    return fwrite(&v, sizeof(v), 1, f) == 1;
}

static int rd_u32(FILE *f, uint32_t *v) {
    return fread(v, sizeof(*v), 1, f) == 1;
}

static int wr_i(FILE *f, int v) {
    return wr_u32(f, (uint32_t)v);
}

static int rd_i(FILE *f, int *v) {
    uint32_t u;
    if (!rd_u32(f, &u)) return 0;
    *v = (int)u;
    return 1;
}

static int wr_str(FILE *f, const char *s) {
    char buf[160];
    memset(buf, 0, sizeof(buf));
    if (s) snprintf(buf, sizeof(buf), "%s", s);
    return fwrite(buf, 1, sizeof(buf), f) == sizeof(buf);
}

static int rd_str(FILE *f, char *dst, size_t size) {
    char buf[160];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf)) return 0;
    buf[sizeof(buf) - 1] = '\0';
    snprintf(dst, size, "%s", buf);
    return 1;
}

static int wr_map(FILE *f, const Game *g) {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            uint8_t cell = (uint8_t)g->map[y][x];
            if (fwrite(&cell, 1, 1, f) != 1) return 0;
        }
    return 1;
}

static int rd_map(FILE *f, Game *g) {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            uint8_t cell;
            if (fread(&cell, 1, 1, f) != 1) return 0;
            if (cell > T_SHOP) return 0;
            g->map[y][x] = (Terrain)cell;
        }
    return 1;
}

static int wr_flags(FILE *f, const unsigned int flags[MAP_H][MAP_W]) {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            if (!wr_u32(f, (uint32_t)flags[y][x])) return 0;
    return 1;
}

static int rd_flags(FILE *f, unsigned int flags[MAP_H][MAP_W]) {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            uint32_t v;
            if (!rd_u32(f, &v)) return 0;
            flags[y][x] = (unsigned int)v;
        }
    return 1;
}

int save_game(const Game *g, const char *path) {
    if (!g || !path || !path[0]) return -1;

    char tmp[192];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;

    int ok = 1;
    ok &= wr_u32(f, SAVE_MAGIC);
    ok &= wr_u32(f, SAVE_VERSION);
    ok &= wr_u32(f, (uint32_t)asm_get_random_state());
    ok &= wr_u32(f, (uint32_t)g->seed);
    ok &= wr_i(f, g->random_mode);

    ok &= wr_i(f, g->floor);
    ok &= wr_i(f, g->px);
    ok &= wr_i(f, g->py);
    ok &= wr_i(f, g->turns);
    ok &= wr_i(f, g->hp);
    ok &= wr_i(f, g->max_hp);
    ok &= wr_i(f, g->atk);
    ok &= wr_i(f, g->def);
    ok &= wr_i(f, g->level);
    ok &= wr_i(f, g->xp);
    ok &= wr_i(f, g->xp_next);
    ok &= wr_i(f, g->gold);

    for (int i = 0; i < SUP_COUNT; i++) ok &= wr_i(f, g->supplies[i]);
    ok &= wr_i(f, g->weapon);
    ok &= wr_i(f, g->best_weapon);
    ok &= wr_u32(f, (uint32_t)g->weapons_owned);
    for (int i = 0; i < WEAPON_COUNT; i++) ok &= wr_i(f, g->weapon_affix[i]);
    ok &= wr_i(f, g->accessory);
    ok &= wr_u32(f, (uint32_t)g->accessories_owned);
    ok &= wr_i(f, g->combat_buff);
    ok &= wr_i(f, g->poison_turns);

    for (int i = 0; i < QUEST_COUNT; i++) ok &= wr_i(f, g->quest_state[i]);
    for (int i = 0; i < QUEST_COUNT; i++) ok &= wr_i(f, g->quest_progress[i]);
    for (int i = 0; i < NPC_COUNT; i++) {
        ok &= wr_i(f, g->talked_npc[i]);
        ok &= wr_i(f, g->npc_x[i]);
        ok &= wr_i(f, g->npc_y[i]);
    }

    ok &= wr_map(f, g);
    ok &= wr_flags(f, g->seen);
    ok &= wr_flags(f, g->visible);

    ok &= wr_i(f, g->enemy_count);
    for (int i = 0; i < g->enemy_count && i < MAX_ENEMIES; i++) {
        const Enemy *e = &g->enemies[i];
        ok &= wr_i(f, e->x);
        ok &= wr_i(f, e->y);
        ok &= wr_i(f, (int)e->type);
        ok &= wr_i(f, e->hp);
        ok &= wr_i(f, e->max_hp);
        ok &= wr_i(f, e->alive);
    }

    ok &= wr_i(f, g->item_count);
    for (int i = 0; i < g->item_count && i < MAX_ITEMS; i++) {
        const Item *it = &g->items[i];
        ok &= wr_i(f, it->x);
        ok &= wr_i(f, it->y);
        ok &= wr_i(f, (int)it->type);
        ok &= wr_i(f, it->amount);
        ok &= wr_i(f, it->taken);
    }

    ok &= wr_i(f, g->log_count);
    for (int i = 0; i < LOG_LINES; i++) ok &= wr_str(f, g->log[i]);
    ok &= wr_str(f, g->shop_msg);
    ok &= wr_str(f, g->inv_msg);
    ok &= wr_str(f, g->notice);

    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        remove(tmp);
        return -1;
    }
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return -1;
    }
    return 0;
}

int load_game(Game *g, const char *path) {
    if (!g || !path || !path[0]) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    Game t;
    memcpy(&t, g, sizeof(t));

    uint32_t magic, version, rng, seed;
    int ok = 1;
    int value;

    ok &= rd_u32(f, &magic);
    ok &= rd_u32(f, &version);
    if (!ok || magic != SAVE_MAGIC || version != SAVE_VERSION) {
        fclose(f);
        return -1;
    }

    ok &= rd_u32(f, &rng);
    ok &= rd_u32(f, &seed);
    ok &= rd_i(f, &t.random_mode);

    ok &= rd_i(f, &t.floor);
    ok &= rd_i(f, &t.px);
    ok &= rd_i(f, &t.py);
    ok &= rd_i(f, &t.turns);
    ok &= rd_i(f, &t.hp);
    ok &= rd_i(f, &t.max_hp);
    ok &= rd_i(f, &t.atk);
    ok &= rd_i(f, &t.def);
    ok &= rd_i(f, &t.level);
    ok &= rd_i(f, &t.xp);
    ok &= rd_i(f, &t.xp_next);
    ok &= rd_i(f, &t.gold);

    for (int i = 0; i < SUP_COUNT; i++) ok &= rd_i(f, &t.supplies[i]);
    ok &= rd_i(f, &t.weapon);
    ok &= rd_i(f, &t.best_weapon);
    ok &= rd_u32(f, (uint32_t *)&t.weapons_owned);
    for (int i = 0; i < WEAPON_COUNT; i++) ok &= rd_i(f, &t.weapon_affix[i]);
    ok &= rd_i(f, &t.accessory);
    ok &= rd_u32(f, (uint32_t *)&t.accessories_owned);
    ok &= rd_i(f, &t.combat_buff);
    ok &= rd_i(f, &t.poison_turns);

    for (int i = 0; i < QUEST_COUNT; i++) ok &= rd_i(f, &t.quest_state[i]);
    for (int i = 0; i < QUEST_COUNT; i++) ok &= rd_i(f, &t.quest_progress[i]);
    for (int i = 0; i < NPC_COUNT; i++) {
        ok &= rd_i(f, &t.talked_npc[i]);
        ok &= rd_i(f, &t.npc_x[i]);
        ok &= rd_i(f, &t.npc_y[i]);
    }

    ok &= rd_map(f, &t);
    ok &= rd_flags(f, t.seen);
    ok &= rd_flags(f, t.visible);

    ok &= rd_i(f, &value);
    if (value < 0 || value > MAX_ENEMIES) ok = 0;
    t.enemy_count = ok ? value : 0;
    for (int i = 0; i < t.enemy_count; i++) {
        Enemy *e = &t.enemies[i];
        ok &= rd_i(f, &e->x);
        ok &= rd_i(f, &e->y);
        ok &= rd_i(f, &value);
        if (value < 0 || value >= E_COUNT) ok = 0;
        e->type = (EnemyType)value;
        ok &= rd_i(f, &e->hp);
        ok &= rd_i(f, &e->max_hp);
        ok &= rd_i(f, &e->alive);
    }

    ok &= rd_i(f, &value);
    if (value < 0 || value > MAX_ITEMS) ok = 0;
    t.item_count = ok ? value : 0;
    for (int i = 0; i < t.item_count; i++) {
        Item *it = &t.items[i];
        ok &= rd_i(f, &it->x);
        ok &= rd_i(f, &it->y);
        ok &= rd_i(f, &value);
        if (value < 0 || value > IT_ACCESSORY) ok = 0;
        it->type = (ItemType)value;
        ok &= rd_i(f, &it->amount);
        ok &= rd_i(f, &it->taken);
    }

    ok &= rd_i(f, &t.log_count);
    if (t.log_count < 0 || t.log_count > LOG_LINES) ok = 0;
    for (int i = 0; i < LOG_LINES; i++) ok &= rd_str(f, t.log[i], sizeof(t.log[i]));
    ok &= rd_str(f, t.shop_msg, sizeof(t.shop_msg));
    ok &= rd_str(f, t.inv_msg, sizeof(t.inv_msg));
    ok &= rd_str(f, t.notice, sizeof(t.notice));

    fclose(f);

    if (!ok) return -1;
    if (t.floor < 0 || t.floor >= FLOORS) return -1;
    if (t.px < 0 || t.px >= MAP_W || t.py < 0 || t.py >= MAP_H) return -1;
    if (t.weapon < 0 || t.weapon >= WEAPON_COUNT) return -1;
    if (t.accessory < -1 || t.accessory >= ACCESSORY_COUNT) return -1;

    char keep_path[sizeof(t.save_path)];
    snprintf(keep_path, sizeof(keep_path), "%s", t.save_path);
    t.seed = seed;
    t.running = 1;
    t.mode = MODE_MAP;
    t.combat_enemy = -1;
    t.combat_turn = 0;
    t.combat_buff = 0;
    t.poison_turns = 0;
    t.dialogue_npc = -1;
    t.dialogue_page = 0;
    snprintf(t.save_path, sizeof(t.save_path), "%s", keep_path);

    asm_set_random_state(rng);
    update_visibility(&t);
    snprintf(t.notice, sizeof(t.notice), "Loaded %s.", path);

    *g = t;
    return 0;
}
