/*
 * main.c - command line, event loop and per-mode key handling.
 * The rules live in world.c/combat.c; this file only routes input.
 */
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int g_start_floor;
static unsigned int g_base_seed;
static int g_random_mode;
static char g_save_path[128];

static int key_is_up(int k) {
    return k == KEY_UP || k == 'w' || k == 'W' || k == 'k' || k == 'K';
}
static int key_is_down(int k) {
    return k == KEY_DOWN || k == 's' || k == 'S' || k == 'j' || k == 'J';
}
static int key_is_left(int k) {
    return k == KEY_LEFT || k == 'a' || k == 'A' || k == 'h' || k == 'H';
}
static int key_is_right(int k) {
    return k == KEY_RIGHT || k == 'd' || k == 'D' || k == 'l' || k == 'L';
}

static void start_new_run(Game *g, int floor_index) {
    char path[sizeof(g->save_path)];
    snprintf(path, sizeof(path), "%s", g_save_path);
    new_run(g, floor_index, g_base_seed, g_random_mode);
    snprintf(g->save_path, sizeof(g->save_path), "%s", path);
    if (g_random_mode)
        snprintf(g->notice, sizeof(g->notice),
                 "The halls shift each time you descend.");
    else
        snprintf(g->notice, sizeof(g->notice), "The crypt waits.");
}

static void notice_save(Game *g) {
    if (!g->save_path[0]) {
        snprintf(g->notice, sizeof(g->notice),
                 "No save file. Start with --save PATH.");
        return;
    }
    if (save_game(g, g->save_path) == 0)
        snprintf(g->notice, sizeof(g->notice), "Saved to %s.", g->save_path);
    else
        snprintf(g->notice, sizeof(g->notice), "Could not write %s.", g->save_path);
}

static void notice_load(Game *g) {
    if (!g->save_path[0]) {
        snprintf(g->notice, sizeof(g->notice),
                 "No save file. Start with --save PATH.");
        return;
    }
    if (load_game(g, g->save_path) != 0)
        snprintf(g->notice, sizeof(g->notice), "No readable save at %s.", g->save_path);
}

static void handle_title(Game *g, int key) {
    if (key == KEY_EOF || key == 'q' || key == 'Q' || key == 3) {
        g->running = 0;
    } else if (key == '\n' || key == ' ') {
        start_new_run(g, g_start_floor);
    }
}

static void handle_map(Game *g, int key) {
    if (key == 3) { g->running = 0; return; }
    if (key == 'q' || key == 'Q') { g->mode = MODE_QUIT_CONFIRM; return; }
    if (key == '?') { g->mode = MODE_HELP; return; }
    if (key == 'p' || key == 'P') { map_drink_potion(g); return; }
    if (key == 'i' || key == 'I' || key == 'b' || key == 'B') {
        open_inventory(g);
        return;
    }
    if (key == KEY_F5) { notice_save(g); return; }
    if (key == KEY_F9) { notice_load(g); return; }

    if (key_is_up(key)) try_move(g, 0, -1);
    else if (key_is_down(key)) try_move(g, 0, 1);
    else if (key_is_left(key)) try_move(g, -1, 0);
    else if (key_is_right(key)) try_move(g, 1, 0);
    else if (key == ' ') wait_turn(g);
}

static void handle_quit_confirm(Game *g, int key) {
    if (key == 'y' || key == 'Y' || key == 3) g->running = 0;
    else g->mode = MODE_MAP;
}

static void handle_help(Game *g, int key) {
    (void)key;
    g->mode = MODE_MAP;
}

static void handle_inv(Game *g, int key) {
    if (key == KEY_EOF || key == 3) {
        g->running = 0;
        return;
    }
    if (key >= '1' && key <= '6') {
        equip_weapon(g, key - '1');
    } else if (key == 'p' || key == 'P') {
        inv_use_supply(g, SUP_POTION);
    } else if (key == 'b' || key == 'B') {
        inv_use_supply(g, SUP_BOMB);
    } else if (key == 'e' || key == 'E') {
        inv_use_supply(g, SUP_ELIXIR);
    } else if (key == 'a' || key == 'A') {
        cycle_accessory(g);
    } else if (key == 'i' || key == 'I' || key == 'q' || key == 'Q' ||
               key == 27 || key == '\n' || key == ' ') {
        g->mode = MODE_MAP;
    }
}

static void handle_shop(Game *g, int key) {
    if (key == KEY_EOF || key == 3) {
        g->running = 0;
        return;
    }
    if (key == '1') {
        shop_buy_supply(g, SUP_POTION);
    } else if (key == '2') {
        shop_buy_supply(g, SUP_BOMB);
    } else if (key == '3') {
        shop_buy_supply(g, SUP_ELIXIR);
    } else if (key == '4') {
        shop_buy_weapon(g);
    } else if (key == '5') {
        shop_buy_accessory(g);
    } else if (key == '0' || key == 'q' || key == 'Q' ||
               key == 27 || key == '\n' || key == ' ') {
        g->mode = MODE_MAP;
    }
}

static void handle_combat(Game *g, int key) {
    if (key == 3) { g->running = 0; return; }
    if (key == 'a' || key == 'A' || key == '\n') combat_player_attack(g);
    else if (key == 'p' || key == 'P') combat_use_supply(g, SUP_POTION);
    else if (key == 'b' || key == 'B') combat_use_supply(g, SUP_BOMB);
    else if (key == 'e' || key == 'E') combat_use_supply(g, SUP_ELIXIR);
    else if (key == 'f' || key == 'F') combat_flee(g);
}

static void handle_dead(Game *g, int key) {
    if (key == 'q' || key == 'Q' || key == KEY_EOF || key == 3) {
        g->running = 0;
    } else if (key == 'r' || key == 'R' || key == '\n') {
        start_new_run(g, g_start_floor);
    }
}

static void handle_win(Game *g, int key) {
    if (key == 'q' || key == 'Q' || key == KEY_EOF || key == 3) {
        g->running = 0;
    } else if (key == 'r' || key == 'R' || key == '\n') {
        start_new_run(g, g_start_floor);
    }
}

static void usage(const char *prog) {
    printf("ASCII QUEST - a tiny 32-bit x86 terminal RPG\n\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("  --seed N       fixed RNG seed (repeatable dungeon)\n");
    printf("  --floor N      start on floor 1..%d (development shortcut)\n", FLOORS);
    printf("  --random       generate every floor procedurally\n");
    printf("  --save PATH    save file used by F5 and autosave\n");
    printf("  --load PATH    load PATH and start there\n");
    printf("  --help         show this help\n\n");
    printf("Controls: WASD/arrows move, bump a monster to fight,\n");
    printf("          A attack, P potion, B bomb, E elixir, F flee,\n");
    printf("          I backpack, N talk (walk into N), ? help, Q quit.\n");
    printf("          F5 save, F9 load (with --save PATH).\n");
}

int main(int argc, char **argv) {
    unsigned int seed = 0;
    int start_floor = 1;
    const char *load_path = NULL;
    const char *save_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--floor") == 0 && i + 1 < argc) {
            start_floor = atoi(argv[++i]);
            if (start_floor < 1) start_floor = 1;
            if (start_floor > FLOORS) start_floor = FLOORS;
        } else if (strcmp(argv[i], "--random") == 0) {
            g_random_mode = 1;
        } else if (strcmp(argv[i], "--save") == 0 && i + 1 < argc) {
            save_path = argv[++i];
        } else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            load_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s (try --help)\n", argv[i]);
            return 2;
        }
    }

    if (seed == 0)
        seed = (unsigned int)time(NULL) ^ ((unsigned int)getpid() << 16);

    g_base_seed = seed;
    g_start_floor = start_floor - 1;
    if (save_path) snprintf(g_save_path, sizeof(g_save_path), "%s", save_path);
    else if (load_path) snprintf(g_save_path, sizeof(g_save_path), "%s", load_path);

    if (term_setup() != 0) return 1;

    Game g;
    memset(&g, 0, sizeof(g));
    g.mode = MODE_TITLE;
    g.running = 1;
    g.combat_enemy = -1;
    g.dialogue_npc = -1;
    snprintf(g.save_path, sizeof(g.save_path), "%s", g_save_path);

    if (load_path) {
        if (load_game(&g, load_path) != 0) {
            term_restore();
            fprintf(stderr, "Could not load %s.\n", load_path);
            return 1;
        }
        snprintf(g.save_path, sizeof(g.save_path), "%s", g_save_path);
        snprintf(g.notice, sizeof(g.notice), "Loaded %s.", load_path);
    }

    while (g.running) {
        render_game(&g);
        int key = read_key();
        if (key == KEY_EOF) break;

        switch (g.mode) {
        case MODE_TITLE: handle_title(&g, key); break;
        case MODE_MAP: handle_map(&g, key); break;
        case MODE_COMBAT: handle_combat(&g, key); break;
        case MODE_HELP: handle_help(&g, key); break;
        case MODE_SHOP: handle_shop(&g, key); break;
        case MODE_INV: handle_inv(&g, key); break;
        case MODE_DIALOGUE: handle_dialogue(&g, key); break;
        case MODE_QUIT_CONFIRM: handle_quit_confirm(&g, key); break;
        case MODE_DEAD: handle_dead(&g, key); break;
        case MODE_WIN: handle_win(&g, key); break;
        default: break;
        }
    }

    term_restore();
    return 0;
}
