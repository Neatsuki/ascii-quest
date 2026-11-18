/*
 * render.c - every screen. Frame helpers here are shared by map, shop,
 * backpack, dialogue and the end screens.
 */
#include "game.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/*  Screen drawing helpers                                                   */
/* ------------------------------------------------------------------------- */
static void frame_border(Line *l, char fill, const char *color) {
    lcol(l, color);
    ltxt(l, "+");
    char s[2] = { fill, 0 };
    for (int i = 0; i < SCREEN_W - 2; i++) ltxt(l, s);
    ltxt(l, "+");
    lcol(l, C_RESET);
}

static void frame_side(Line *l, const char *text, const char *color) {
    lcol(l, C_DIM);
    ltxt(l, "|");
    lcol(l, C_RESET);
    int start = 1 + (SCREEN_W - 2 - (int)strlen(text)) / 2;
    if (start < 1) start = 1;
    lpad(l, start);
    lcol(l, color);
    ltxt(l, text);
    lcol(l, C_RESET);
    lpad(l, SCREEN_W - 1);
    lcol(l, C_DIM);
    ltxt(l, "|");
    lcol(l, C_RESET);
}

static void frame_blank(Line *l) {
    lcol(l, C_DIM);
    ltxt(l, "|");
    lcol(l, C_RESET);
    lpad(l, SCREEN_W - 1);
    lcol(l, C_DIM);
    ltxt(l, "|");
    lcol(l, C_RESET);
}

static void hp_bar(Line *l, int hp, int max_hp, int width,
                   const char *fill_color) {
    int fill = 0;
    if (max_hp > 0)
        fill = (hp * width + max_hp - 1) / max_hp;
    if (fill < 0) fill = 0;
    if (fill > width) fill = width;

    ltxt(l, "[");
    for (int i = 0; i < width; i++) {
        lcol(l, i < fill ? fill_color : C_DIM);
        ltxt(l, i < fill ? "#" : "-");
    }
    lcol(l, C_RESET);
    ltxt(l, "]");
}

static void map_box_border(Line *l) {
    lcol(l, C_DIM);
    ltxt(l, "+-------------------------------+");
    lcol(l, C_RESET);
}

static void panel_border(Line *l) {
    lcol(l, C_DIM);
    ltxt(l, "+----------------------------+");
    lcol(l, C_RESET);
}

static void panel_start(Line *l) {
    lpad(l, 36);
    lcol(l, C_DIM);
    ltxt(l, "|");
    lcol(l, C_RESET);
    ltxt(l, " ");
}

static void panel_end(Line *l) {
    lpad(l, SCREEN_W - 1);
    lcol(l, C_DIM);
    ltxt(l, "|");
    lcol(l, C_RESET);
}

static void panel_text(Line *l, const char *text) {
    panel_start(l);
    ltxt(l, text);
    panel_end(l);
}

static const char *terrain_glyph(Terrain t) {
    switch (t) {
    case T_WALL: return "#";
    case T_DOOR: return "+";
    case T_STAIRS: return ">";
    case T_SHOP: return "S";
    default: return ".";
    }
}

static void draw_map_cell(Line *l, const Game *g, int x, int y) {
    if (!g->seen[y][x]) {
        ltxt(l, " ");
        return;
    }

    if (!g->visible[y][x]) {
        lcol(l, C_DIM);
        ltxt(l, terrain_glyph(g->map[y][x]));
        lcol(l, C_RESET);
        return;
    }

    if (x == g->px && y == g->py) {
        lcol(l, C_BOLD C_YELLOW);
        ltxt(l, "@");
        lcol(l, C_RESET);
        return;
    }

    int ni = npc_at(g, x, y);
    if (ni >= 0) {
        lcol(l, C_BOLD C_MAGENTA);
        ltxt(l, "N");
        lcol(l, C_RESET);
        return;
    }

    int ei = enemy_at(g, x, y);
    if (ei >= 0) {
        const EnemyDef *d = enemy_def(g->enemies[ei].type);
        lcol(l, d->color);
        char s[2] = { d->glyph, 0 };
        ltxt(l, s);
        lcol(l, C_RESET);
        return;
    }

    int ii = item_at(g, x, y);
    if (ii >= 0) {
        if (g->items[ii].type == IT_POTION) {
            lcol(l, C_GREEN);
            ltxt(l, "!");
        } else if (g->items[ii].type == IT_ACCESSORY) {
            lcol(l, C_CYAN);
            ltxt(l, "*");
        } else {
            lcol(l, C_YELLOW);
            ltxt(l, "$");
        }
        lcol(l, C_RESET);
        return;
    }

    switch (g->map[y][x]) {
    case T_WALL:
        lcol(l, C_WHITE);
        ltxt(l, "#");
        break;
    case T_DOOR:
        lcol(l, C_YELLOW);
        ltxt(l, "+");
        break;
    case T_STAIRS:
        lcol(l, C_BOLD C_MAGENTA);
        ltxt(l, ">");
        break;
    case T_SHOP:
        lcol(l, C_BOLD C_YELLOW);
        ltxt(l, "S");
        break;
    default:
        lcol(l, C_DIM);
        ltxt(l, ".");
        break;
    }
    lcol(l, C_RESET);
}

static void panel_content(Line *l, const Game *g, int row) {
    switch (row) {
    case 0:
        panel_start(l);
        lcol(l, C_BOLD C_YELLOW);
        ltxt(l, "@");
        lcol(l, C_RESET);
        ltxt(l, " HERO");
        lpad(l, 60);
        lfmt(l, "Lv %d", g->level);
        panel_end(l);
        break;

    case 1:
        panel_start(l);
        ltxt(l, "HP ");
        hp_bar(l, g->hp, g->max_hp, 12, C_GREEN);
        lfmt(l, " %d/%d", g->hp, g->max_hp);
        panel_end(l);
        break;

    case 2:
        panel_start(l);
        lfmt(l, "ATK %d (%d+%d) DEF %d", player_atk(g), g->atk,
             player_atk(g) - g->atk, player_def(g));
        panel_end(l);
        break;

    case 3:
        panel_start(l);
        lfmt(l, "XP %d/%-3d  Gold %d", g->xp, g->xp_next, g->gold);
        panel_end(l);
        break;

    case 4:
        panel_start(l);
        ltxt(l, "Items ");
        lcol(l, C_GREEN);
        lfmt(l, "P%d ", g->supplies[SUP_POTION]);
        lcol(l, C_YELLOW);
        lfmt(l, "B%d ", g->supplies[SUP_BOMB]);
        lcol(l, C_MAGENTA);
        lfmt(l, "E%d", g->supplies[SUP_ELIXIR]);
        lcol(l, C_RESET);
        panel_end(l);
        break;

    case 5:
        panel_start(l);
        lcol(l, C_CYAN);
        if (g->weapon_affix[g->weapon] >= 0)
            lfmt(l, "W: %.15s [%s]", WEAPONS[g->weapon].name,
                 AFFIXES[g->weapon_affix[g->weapon]].short_name);
        else
            lfmt(l, "W: %.20s", WEAPONS[g->weapon].name);
        lcol(l, C_RESET);
        panel_end(l);
        break;

    case 6:
        panel_start(l);
        lfmt(l, "Floor %d: %s", g->floor + 1, floor_name(g, g->floor));
        panel_end(l);
        break;

    case 7:
        panel_start(l);
        lfmt(l, "Turns %d", g->turns);
        panel_end(l);
        break;

    case 8:
        panel_start(l);
        if (g->accessory >= 0)
            lfmt(l, "Charm: %s", ACCESSORIES[g->accessory].name);
        else
            ltxt(l, "Charm: none");
        panel_end(l);
        break;

    case 9:
        panel_text(l, "LEGEND");
        break;

    case 10:
        panel_start(l);
        lcol(l, C_YELLOW); ltxt(l, "@"); lcol(l, C_RESET);
        ltxt(l, " you   ");
        lcol(l, C_GREEN); ltxt(l, "!"); lcol(l, C_RESET);
        ltxt(l, " potion  ");
        lcol(l, C_YELLOW); ltxt(l, "$"); lcol(l, C_RESET);
        ltxt(l, " gold");
        panel_end(l);
        break;

    case 11:
        panel_start(l);
        lcol(l, C_MAGENTA); ltxt(l, ">"); lcol(l, C_RESET);
        ltxt(l, " stairs  ");
        lcol(l, C_WHITE); ltxt(l, "#"); lcol(l, C_RESET);
        ltxt(l, " wall  ");
        lcol(l, C_YELLOW); ltxt(l, "+"); lcol(l, C_RESET);
        ltxt(l, " door");
        panel_end(l);
        break;

    case 12:
        panel_start(l);
        lcol(l, C_YELLOW); ltxt(l, "S"); lcol(l, C_RESET);
        ltxt(l, " shop   ");
        lcol(l, C_MAGENTA); ltxt(l, "N"); lcol(l, C_RESET);
        ltxt(l, " talk");
        panel_end(l);
        break;

    case 13:
        panel_text(l, "move: WASD/arrows  ? help");
        break;

    case 14:
        panel_text(l, "P/B/E items   I bag");
        break;

    default:
        panel_text(l, "");
        break;
    }
}

static void render_map(const Game *g) {
    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);

        if (r == 0) {
            frame_border(&l, '=', C_CYAN);
        } else if (r == 1) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "ASCII QUEST"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_DIM); ltxt(&l, "Crypt of the ASCII King"); lcol(&l, C_RESET);

            char right[48];
            snprintf(right, sizeof(right), "Floor %d/%d | Turn %d",
                     g->floor + 1, FLOORS, g->turns);
            int target = (SCREEN_W - 1) - 1 - (int)strlen(right);
            lpad(&l, target);
            lcol(&l, C_DIM); ltxt(&l, right); ltxt(&l, " "); lcol(&l, C_RESET);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 2) {
            frame_border(&l, '=', C_CYAN);
        } else if (r == 3) {
            map_box_border(&l);
            lpad(&l, 36);
            panel_border(&l);
        } else if (r >= 4 && r <= 18) {
            int y = r - 4;
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            for (int x = 0; x < MAP_W; x++) draw_map_cell(&l, g, x, y);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);

            panel_content(&l, g, y);
        } else if (r == 19) {
            map_box_border(&l);
            lpad(&l, 36);
            panel_border(&l);
        } else if (r == 20 || r == 21) {
            int which = r - 20;
            if (r == 21 && g->notice[0]) {
                char brief[63];
                snprintf(brief, sizeof(brief), "%.58s", g->notice);
                lcol(&l, C_GREEN);
                ltxt(&l, "  * ");
                ltxt(&l, brief);
                lcol(&l, C_RESET);
            } else {
                int idx = g->log_count - 2 + which;
                ltxt(&l, "  ");
                if (idx >= 0 && idx < g->log_count) {
                    char brief[63];
                    snprintf(brief, sizeof(brief), "%.62s", g->log[idx]);
                    lcol(&l, which == 0 ? C_DIM : C_CYAN);
                    ltxt(&l, "> ");
                    lcol(&l, C_RESET);
                    ltxt(&l, brief);
                }
            }
        } else {
            if (g->mode == MODE_QUIT_CONFIRM) {
                lcol(&l, C_BOLD C_RED);
                ltxt(&l, "  Quit the crypt? All progress will be lost. [y/N]");
                lcol(&l, C_RESET);
            } else {
                lcol(&l, C_DIM);
                ltxt(&l, "  WASD/arrows | bump fight | P potion | I bag | ? help | Q quit");
                lcol(&l, C_RESET);
            }
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_help(void) {
    static const struct {
        const char *text;
        const char *color;
    } rows[23] = {
        { NULL, NULL },
        { "HOW TO PLAY", C_BOLD C_YELLOW },
        { NULL, NULL },
        { NULL, NULL },
        { "MOVE          WASD or arrow keys", C_WHITE },
        { "FIGHT         walk into a monster", C_WHITE },
        { "ATTACK        A  (or Enter)", C_WHITE },
        { "ITEMS         P potion  B bomb  E elixir", C_WHITE },
        { "BACKPACK      I open  |  1-6 equip weapon", C_WHITE },
        { "FLEE          F  55% chance (not vs dragon)", C_WHITE },
        { "WAIT          Space lets monsters move", C_WHITE },
        { "SHOP          S to buy 1-3/4/5  |  N talk", C_WHITE },
        { "SAVE/LOAD     F5 save  F9 load (needs --save)", C_WHITE },
        { "GLYPHS", C_BOLD C_YELLOW },
        { "! potion    $ gold    > stairs    # wall    + door", C_WHITE },
        { "TIPS", C_BOLD C_YELLOW },
        { "Affixes roll on weapons. Charms fill your one slot.", C_DIM },
        { "Explore every room. Potions and gold hide in the dark.", C_DIM },
        { "The dragon breathes fire every third combat round.", C_DIM },
        { "Spiders poison. Liches drain. Golems grind slowly.", C_DIM },
        { "Press any key to return.", C_YELLOW },
        { NULL, NULL }
    };

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);
        if (r == 0 || r == 22) {
            frame_border(&l, '=', C_CYAN);
        } else if (r == 1) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "HOW TO PLAY"); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 2) {
            frame_border(&l, '-', C_DIM);
        } else if (rows[r].text) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, rows[r].color); ltxt(&l, rows[r].text); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_shop(const Game *g) {
    const WeaponDef *w = NULL;
    int next = g->best_weapon + 1;
    int acc = -1;
    char right[48];
    char msg[64];

    if (next < WEAPON_COUNT) w = &WEAPONS[next];
    for (int i = 0; i < ACCESSORY_COUNT; i++) {
        if (!(g->accessories_owned & (1u << i))) { acc = i; break; }
    }
    snprintf(right, sizeof(right), "Floor %d/%d | Turn %d",
             g->floor + 1, FLOORS, g->turns);
    snprintf(msg, sizeof(msg), "%.60s", g->shop_msg);

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);

        if (r == 0 || r == 2 || r == 22) {
            frame_border(&l, '=', C_CYAN);
        } else if (r == 1) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "THE PEDDLER"); lcol(&l, C_RESET);
            int target = 64 - (int)strlen(right);
            lpad(&l, target);
            lcol(&l, C_DIM); ltxt(&l, right); ltxt(&l, " "); lcol(&l, C_RESET);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 4) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_CYAN);
            ltxt(&l, "\"Steel or a swig? Both keep you breathing.\"");
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 6) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_YELLOW); lfmt(&l, "Gold %d", g->gold); lcol(&l, C_RESET);
            lpad(&l, 24);
            lcol(&l, C_GREEN); lfmt(&l, "HP %d/%d", g->hp, g->max_hp);
            lcol(&l, C_RESET);
            lpad(&l, 48);
            lcol(&l, C_WHITE);
            lfmt(&l, "P%d B%d E%d", g->supplies[SUP_POTION],
                 g->supplies[SUP_BOMB], g->supplies[SUP_ELIXIR]);
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 7) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_CYAN);
            lfmt(&l, "W: %s", WEAPONS[g->weapon].name);
            if (g->weapon_affix[g->weapon] >= 0)
                lfmt(&l, " [%s]", AFFIXES[g->weapon_affix[g->weapon]].name);
            if (g->accessory >= 0)
                lfmt(&l, "  Charm: %s", ACCESSORIES[g->accessory].name);
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 9 || r == 16) {
            frame_border(&l, '-', C_DIM);
        } else if (r == 10) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "FOR SALE"); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r >= 11 && r <= 13) {
            int type = r - 11;
            const SupplyDef *sp = &SUPPLY_DEFS[type];
            int poor = g->gold < sp->price;
            int full = g->supplies[type] >= sp->cap;
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, (poor || full) ? C_DIM : C_BOLD C_GREEN);
            lfmt(&l, "[%d]", type + 1);
            lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_WHITE); ltxt(&l, sp->name);
            lfmt(&l, " x%d", g->supplies[type]); lcol(&l, C_RESET);
            lpad(&l, 32);
            lcol(&l, poor ? C_RED : C_YELLOW);
            lfmt(&l, "%d gold", sp->price); lcol(&l, C_RESET);
            lpad(&l, 43);
            lcol(&l, C_DIM); ltxt(&l, full ? "pack full" : sp->desc);
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 14) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            if (!w) {
                lcol(&l, C_DIM); ltxt(&l, "[4]  Nothing sharper for sale");
                lcol(&l, C_RESET);
            } else {
                int poor = g->gold < w->price;
                lcol(&l, poor ? C_DIM : C_BOLD C_GREEN);
                ltxt(&l, "[4]"); lcol(&l, C_RESET);
                ltxt(&l, "  ");
                lcol(&l, C_WHITE); ltxt(&l, w->name); lcol(&l, C_RESET);
                lpad(&l, 32);
                lcol(&l, poor ? C_RED : C_YELLOW);
                lfmt(&l, "%d gold", w->price); lcol(&l, C_RESET);
                lpad(&l, 43);
                lcol(&l, C_GREEN); lfmt(&l, "ATK +%d", w->bonus);
                lcol(&l, C_RESET);
            }
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 15) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            if (acc < 0) {
                lcol(&l, C_DIM); ltxt(&l, "[5]  Every charm sold");
                lcol(&l, C_RESET);
            } else {
                const AccessoryDef *a = &ACCESSORIES[acc];
                int poor = g->gold < a->price;
                lcol(&l, poor ? C_DIM : C_BOLD C_MAGENTA);
                ltxt(&l, "[5]"); lcol(&l, C_RESET);
                ltxt(&l, "  ");
                lcol(&l, C_WHITE); ltxt(&l, a->name); lcol(&l, C_RESET);
                lpad(&l, 32);
                lcol(&l, poor ? C_RED : C_YELLOW);
                lfmt(&l, "%d gold", a->price); lcol(&l, C_RESET);
                lpad(&l, 43);
                lcol(&l, C_CYAN); ltxt(&l, a->desc); lcol(&l, C_RESET);
            }
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 17) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_YELLOW); ltxt(&l, "> "); lcol(&l, C_RESET);
            char brief[62];
            snprintf(brief, sizeof(brief), "%.58s", msg);
            ltxt(&l, brief);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 18) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  1-3 supplies  4 weapon  5 charm  0 leave");
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 19) {
            lcol(&l, C_BOLD C_WHITE); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  [0]  Leave the counter");
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 20) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            lcol(&l, C_DIM);
            ltxt(&l, "  The peddler watches your purse, not your face.");
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_inv(const Game *g) {
    char right[48];
    char msg[64];

    snprintf(right, sizeof(right), "Floor %d/%d | Turn %d",
             g->floor + 1, FLOORS, g->turns);
    snprintf(msg, sizeof(msg), "%.60s", g->inv_msg);

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);

        if (r == 0 || r == 2 || r == 22) {
            frame_border(&l, '=', C_CYAN);
        } else if (r == 1) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "BACKPACK"); lcol(&l, C_RESET);
            int target = 64 - (int)strlen(right);
            lpad(&l, target);
            lcol(&l, C_DIM); ltxt(&l, right); ltxt(&l, " "); lcol(&l, C_RESET);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 10) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_MAGENTA); ltxt(&l, "CHARM"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            if (g->accessory >= 0) {
                lcol(&l, C_WHITE);
                ltxt(&l, ACCESSORIES[g->accessory].name);
                lcol(&l, C_RESET);
                lcol(&l, C_DIM);
                lfmt(&l, "  %s", ACCESSORIES[g->accessory].desc);
                lcol(&l, C_RESET);
            } else {
                lcol(&l, C_DIM); ltxt(&l, "none  (A cycles owned)");
                lcol(&l, C_RESET);
            }
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 3 || r == 11) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW);
            ltxt(&l, r == 3 ? "EQUIPMENT" : "SUPPLIES");
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r >= 4 && r <= 9) {
            int index = r - 4;
            int owned = (g->weapons_owned & (1u << index)) != 0;
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, owned ? (g->weapon == index ? C_BOLD C_CYAN : C_WHITE)
                            : C_DIM);
            lfmt(&l, "[%d] %c ", index + 1,
                 g->weapon == index ? '>' : ' ');
            lfmt(&l, "%-12s", WEAPONS[index].name);
            lcol(&l, C_RESET);
            lpad(&l, 32);
            if (owned) {
                lfmt(&l, "+%d ATK", WEAPONS[index].bonus);
                lpad(&l, 34);
                if (g->weapon_affix[index] >= 0)
                    lcol(&l, C_MAGENTA);
                lfmt(&l, " [%s]", g->weapon_affix[index] >= 0
                         ? AFFIXES[g->weapon_affix[index]].name : "plain");
                lcol(&l, C_RESET);
            } else {
                ltxt(&l, "locked");
            }
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r >= 12 && r <= 14) {
            int type = r - 12;
            const SupplyDef *s = &SUPPLY_DEFS[type];
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, g->supplies[type] ? C_WHITE : C_DIM);
            lfmt(&l, "[%c]  ", s->key);
            lfmt(&l, "%-8s", s->name);
            lfmt(&l, "x%-2d", g->supplies[type]);
            lcol(&l, C_RESET);
            lpad(&l, 30);
            lcol(&l, C_DIM);
            ltxt(&l, s->desc);
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 15) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_YELLOW);
            lfmt(&l, "Gold %d", g->gold);
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 16) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_YELLOW); ltxt(&l, "> "); lcol(&l, C_RESET);
            char brief[62];
            snprintf(brief, sizeof(brief), "%.58s", msg);
            ltxt(&l, brief);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 18) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  1-6 equip  P potion  B bomb  E elixir  A charm  I close");
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 20) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  The pack remembers every bargain.");
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}


static void render_title(void) {
    static const char *art[6] = {
        "      /\\                                    /\\",
        "     /  \\        A S C I I   Q U E S T      /  \\",
        "    /    \\                                 /    \\",
        "   /______\\     Crypt of the ASCII King    /______\\",
        "   |  []  |                                |  []  |",
        "   |______|________________________________|______|"
    };

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);

        if (r == 0 || r == 22) {
            frame_border(&l, '=', C_CYAN);
        } else if (r >= 3 && r <= 8) {
            /* Drawn as one left-aligned block so the two towers line up. */
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            lpad(&l, 7);
            lcol(&l, C_DIM); ltxt(&l, art[r - 3]); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 10) {
            frame_side(&l, "a tiny terminal RPG", C_CYAN);
        } else if (r == 12) {
            frame_side(&l, "[ ENTER ]   Begin your descent", C_BOLD C_YELLOW);
        } else if (r == 14) {
            frame_side(&l, "WASD / arrows move      bump monsters to fight", C_WHITE);
        } else if (r == 15) {
            frame_side(&l, "! potions  $ gold  > stairs  S shop  N talk  I bag  ? help", C_DIM);
        } else if (r == 17) {
            frame_side(&l, "Tip: the peddler sells steel. The dragon sells death.", C_DIM);
        } else if (r == 19) {
            frame_side(&l, "x86-32 NASM + C  |  Linux", C_DIM);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_combat(const Game *g) {
    const Enemy *e = &g->enemies[g->combat_enemy];
    const EnemyDef *d = enemy_def(e->type);

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);

        if (r == 0 || r == 2 || r == 21) {
            frame_border(&l, '=', C_RED);
        } else if (r == 1) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            char title[64];
            snprintf(title, sizeof(title), "BATTLE  -  %s", d->name);
            int start = 1 + (64 - (int)strlen(title)) / 2;
            lpad(&l, start);
            lcol(&l, C_BOLD C_RED); ltxt(&l, title); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r >= 4 && r <= 11) {
            int i = r - 4;
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");

            const char *a = "";
            for (int k = 0; k < 9; k++) {
                if (d->art[k] == NULL) break;
                if (k == i) { a = d->art[k]; break; }
            }
            int apad = (34 - (int)strlen(a)) / 2;
            if (apad < 0) apad = 0;
            lpad(&l, 3 + apad);
            lcol(&l, d->color);
            ltxt(&l, a);
            lcol(&l, C_RESET);
            lpad(&l, 40);

            if (i == 0) {
                lcol(&l, C_BOLD C_RED);
                ltxt(&l, d->name);
                lcol(&l, C_RESET);
            } else if (i == 1) {
                ltxt(&l, "HP ");
                hp_bar(&l, e->hp, e->max_hp, 10, C_RED);
                lfmt(&l, " %d/%d", e->hp, e->max_hp);
            } else if (i == 2) {
                lfmt(&l, "ATK %d   DEF %d", d->atk, d->def);
            } else if (i == 3) {
                lfmt(&l, "XP %d   Gold %d", d->xp, d->gold_min);
            } else if (i == 4) {
                lcol(&l, C_DIM);
                ltxt(&l, d->desc);
                lcol(&l, C_RESET);
            }
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 12 || r == 15 || r == 19) {
            frame_border(&l, '-', C_DIM);
        } else if (r == 13) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  YOU   HP ");
            hp_bar(&l, g->hp, g->max_hp, 12, C_GREEN);
            lfmt(&l, " %d/%d", g->hp, g->max_hp);
            ltxt(&l, "   ");
            lcol(&l, C_GREEN);
            lfmt(&l, "P%d ", g->supplies[SUP_POTION]);
            lcol(&l, C_YELLOW);
            lfmt(&l, "B%d ", g->supplies[SUP_BOMB]);
            lcol(&l, C_MAGENTA);
            lfmt(&l, "E%d", g->supplies[SUP_ELIXIR]);
            if (g->poison_turns > 0) {
                lcol(&l, C_RED);
                lfmt(&l, " PSN%d", g->poison_turns);
            }
            lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 16 || r == 17 || r == 18) {
            int idx = g->log_count - 3 + (r - 16);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            if (idx >= 0 && idx < g->log_count) {
                lcol(&l, idx == g->log_count - 1 ? C_WHITE : C_DIM);
                ltxt(&l, "> ");
                lcol(&l, C_RESET);
                char msg[64];
                snprintf(msg, sizeof(msg), "%.58s", g->log[idx]);
                ltxt(&l, msg);
            }
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 20) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "[A]"); lcol(&l, C_RESET);
            ltxt(&l, "ttack  ");
            lcol(&l, C_BOLD C_GREEN); ltxt(&l, "[P]"); lcol(&l, C_RESET);
            ltxt(&l, "otion  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "[B]"); lcol(&l, C_RESET);
            ltxt(&l, "omb  ");
            lcol(&l, C_BOLD C_MAGENTA); ltxt(&l, "[E]"); lcol(&l, C_RESET);
            ltxt(&l, "lixir  ");
            lcol(&l, e->type == E_DRAGON ? C_DIM : C_BOLD C_CYAN);
            ltxt(&l, "[F]"); lcol(&l, C_RESET);
            ltxt(&l, "lee");
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 22) {
            ltxt(&l, "  A attack | P potion | B bomb | E elixir | F flee");
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_dead(const Game *g) {
    static const char *tomb[5] = {
        "        _______        ",
        "       /       \\       ",
        "      /  R.I.P.  \\      ",
        "      |  HERO    |      ",
        "      |__________|      "
    };
    char stats[128];
    snprintf(stats, sizeof(stats), "Floor %d  |  Level %d  |  %d gold  |  %d turns",
             g->floor + 1, g->level, g->gold, g->turns);

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);
        if (r == 0 || r == 22) {
            frame_border(&l, '=', C_RED);
        } else if (r == 2) {
            frame_side(&l, "YOU DIED", C_BOLD C_RED);
        } else if (r >= 4 && r <= 8) {
            frame_side(&l, tomb[r - 4], C_DIM);
        } else if (r == 11) {
            frame_side(&l, "The crypt keeps its secrets.", C_DIM);
        } else if (r == 12) {
            frame_side(&l, stats, C_WHITE);
        } else if (r == 14) {
            frame_side(&l, "Press R to rise again   |   Q to quit", C_YELLOW);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_win(const Game *g) {
    char stats[128];
    snprintf(stats, sizeof(stats), "Level %d  |  %d gold  |  %d turns",
             g->level, g->gold, g->turns);

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);
        if (r == 0 || r == 22) {
            frame_border(&l, '=', C_YELLOW);
        } else if (r == 2) {
            frame_side(&l, "*** VICTORY ***", C_BOLD C_YELLOW);
        } else if (r == 4) {
            frame_side(&l, "+-------------------------+", C_YELLOW);
        } else if (r == 5) {
            frame_side(&l, "|      THE DRAGON IS      |", C_YELLOW);
        } else if (r == 6) {
            frame_side(&l, "|          SLAIN          |", C_YELLOW);
        } else if (r == 7) {
            frame_side(&l, "+-------------------------+", C_YELLOW);
        } else if (r == 9) {
            frame_side(&l, "The gate opens. Sunlight pours in.", C_WHITE);
        } else if (r == 10) {
            frame_side(&l, "You escape the Crypt of the ASCII King.", C_DIM);
        } else if (r == 12) {
            frame_side(&l, stats, C_CYAN);
        } else if (r == 14) {
            frame_side(&l, "Press R to play again   |   Q to quit", C_YELLOW);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

static void render_dialogue(const Game *g) {
    char brief[64];
    const char *line = dialogue_line(g, g->dialogue_page);
    int pages = dialogue_pages(g);
    int q = g->dialogue_npc;
    const char *quest = q == 0 ? "The Hermit's Pests" : "The Crown of Ash";
    char status[72];

    snprintf(brief, sizeof(brief), "%.58s", line);
    if (g->quest_state[q] == 1)
        snprintf(status, sizeof(status), "Quest available: %s", quest);
    else if (g->quest_state[q] == 2 && q == 0)
        snprintf(status, sizeof(status), "%s  %d/5", quest, g->quest_progress[0]);
    else if (g->quest_state[q] == 2)
        snprintf(status, sizeof(status), "%s  crown: %s", quest,
                 g->quest_progress[1] ? "ash" : "still worn");
    else
        snprintf(status, sizeof(status), "%s  complete", quest);

    out_begin();
    for (int r = 0; r < 23; r++) {
        Line l;
        line_init(&l);

        if (r == 0 || r == 2 || r == 22) {
            frame_border(&l, '=', C_MAGENTA);
        } else if (r == 1) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_BOLD C_YELLOW); ltxt(&l, "DIALOGUE"); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 4) {
            frame_side(&l, dialogue_title(g), C_BOLD C_MAGENTA);
        } else if (r == 5) {
            frame_border(&l, '-', C_DIM);
        } else if (r == 8) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            int pad = 1 + (62 - (int)strlen(brief)) / 2;
            lpad(&l, pad);
            lcol(&l, C_WHITE); ltxt(&l, brief); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 10) {
            char page[32];
            snprintf(page, sizeof(page), "page %d/%d", g->dialogue_page + 1, pages);
            frame_side(&l, page, C_DIM);
        } else if (r == 13) {
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
            ltxt(&l, "  ");
            lcol(&l, C_YELLOW); ltxt(&l, status); lcol(&l, C_RESET);
            lpad(&l, 65);
            lcol(&l, C_DIM); ltxt(&l, "|"); lcol(&l, C_RESET);
        } else if (r == 16) {
            if (dialogue_can_accept(g))
                frame_side(&l, "1  accept / claim", C_BOLD C_GREEN);
            else
                frame_blank(&l);
        } else if (r == 19) {
            frame_side(&l, "Enter  next page     0  leave", C_DIM);
        } else if (r == 20) {
            frame_side(&l, "The dead are patient. You are not.", C_DIM);
        } else {
            frame_blank(&l);
        }
        out_line(&l, r == 22);
    }
    out_flush();
}

void render_game(const Game *g) {
    switch (g->mode) {
    case MODE_TITLE: render_title(); break;
    case MODE_HELP: render_help(); break;
    case MODE_SHOP: render_shop(g); break;
    case MODE_INV: render_inv(g); break;
    case MODE_DIALOGUE: render_dialogue(g); break;
    case MODE_COMBAT: render_combat(g); break;
    case MODE_DEAD: render_dead(g); break;
    case MODE_WIN: render_win(g); break;
    default: render_map(g); break;
    }
}

/* ------------------------------------------------------------------------- */
/*  Input handling                                                           */
/* ------------------------------------------------------------------------- */
