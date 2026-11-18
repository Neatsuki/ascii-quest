/*
 * combat.c - turn based fights, monster traits, poison and rewards.
 * The rules are deliberately small and data driven through ENEMY_TRAITS[].
 */
#include "game.h"

#include <stdio.h>
#include <string.h>

static const char *enemy_article(const EnemyDef *d) {
    char c = d->name[0];
    if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' ||
        c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
        return "An";
    return "A";
}

void level_up(Game *g) {
    while (g->xp >= g->xp_next) {
        g->xp -= g->xp_next;
        g->level++;
        g->max_hp += 6;
        g->atk += 2;
        g->def += 1;
        g->hp = g->max_hp;
        g->xp_next = 10 + (g->level - 1) * 15;
        log_add(g, "LEVEL UP! You are now level %d.", g->level);
    }
}

void combat_enemy_turn(Game *g, Enemy *e) {
    const EnemyDef *d = enemy_def(e->type);
    const EnemyTrait *tr = enemy_trait(e->type);
    int dmg = 0;

    if (e->type == E_DRAGON && (g->combat_turn % 3) == 0) {
        dmg = 16 + rand_range(5);
        g->hp -= dmg;
        log_add(g, "The dragon breathes fire! %d damage!", dmg);
    } else if (tr->slow > 0 && rand_range(100) < tr->slow) {
        log_add(g, "The %s grinds to a halt.", d->name);
        return;
    } else if (tr->fire > 0 && rand_range(100) < tr->fire) {
        dmg = imax(1, d->atk * 2 - player_def(g) / 2 + rand_range(3));
        g->hp -= dmg;
        log_add(g, "The %s hurls a firebolt! %d damage!", d->name, dmg);
    } else {
        int def = player_def(g) - (player_def(g) * tr->pierce) / 100;
        dmg = imax(1, d->atk - def + rand_range(2));
        g->hp -= dmg;
        log_add(g, "The %s hits you for %d damage.", d->name, dmg);
    }

    if (g->hp <= 0) {
        g->hp = 0;
        g->mode = MODE_DEAD;
        return;
    }

    if (tr->drain > 0) {
        int before = e->hp;
        e->hp = imin(e->max_hp, e->hp + tr->drain);
        if (e->hp > before)
            log_add(g, "The %s drains %d HP from you.", d->name, e->hp - before);
    }

    if (tr->poison > 0 && g->poison_turns == 0 &&
        rand_range(100) < tr->poison) {
        g->poison_turns = 3;
        log_add(g, "The %s poisons you! (-1 HP for 3 turns)", d->name);
    }
}

static int combat_poison_tick(Game *g) {
    if (g->poison_turns <= 0) return 1;

    g->poison_turns--;
    g->hp -= 1;
    log_add(g, "Poison burns you for 1 damage. (%d turns left)",
            g->poison_turns);
    if (g->hp <= 0) {
        g->hp = 0;
        g->mode = MODE_DEAD;
        return 0;
    }
    return 1;
}

void combat_victory(Game *g, Enemy *e) {
    const EnemyDef *d = enemy_def(e->type);
    int gold = d->gold_min + rand_range(d->gold_max - d->gold_min + 1);

    e->alive = 0;
    g->xp += d->xp;
    g->gold += gold;
    g->poison_turns = 0;
    g->combat_buff = 0;
    g->mode = MODE_MAP;

    log_add(g, "You defeat the %s! +%d XP, +%d gold.", d->name, d->xp, gold);
    quest_on_kill(g, e->type);
    level_up(g);
    if (e->type == E_DRAGON) {
        g->mode = MODE_WIN;
        return;
    }
    update_visibility(g);
}

void start_combat(Game *g, int enemy_index, int ambush) {
    Enemy *e = &g->enemies[enemy_index];
    const EnemyDef *d = enemy_def(e->type);

    g->mode = MODE_COMBAT;
    g->combat_enemy = enemy_index;
    g->combat_turn = 0;
    g->combat_buff = 0;
    g->poison_turns = 0;
    g->log_count = 0;
    if (ambush)
        log_add(g, "%s %s lunges out of the dark!", enemy_article(d), d->name);
    else
        log_add(g, "%s %s blocks your path!", enemy_article(d), d->name);
}

void combat_player_attack(Game *g) {
    Enemy *e = &g->enemies[g->combat_enemy];
    const EnemyDef *d = enemy_def(e->type);
    const EnemyTrait *tr = enemy_trait(e->type);
    int dmg;
    int crit = 0;

    if (!combat_poison_tick(g)) return;

    g->combat_turn++;

    if (tr->dodge > 0 && rand_range(100) < tr->dodge) {
        log_add(g, "The %s slips aside. Your blow misses!", d->name);
    } else {
        dmg = imax(1, combat_atk(g) - d->def + rand_range(3));
        if (rand_range(100) < player_crit(g)) {
            dmg *= 2;
            crit = 1;
        }
        e->hp -= dmg;
        if (e->hp < 0) e->hp = 0;
        if (crit)
            log_add(g, "CRITICAL! You strike the %s for %d damage.", d->name, dmg);
        else
            log_add(g, "You hit the %s for %d damage.", d->name, dmg);

        int steal = player_lifesteal(g);
        if (steal > 0 && g->hp < g->max_hp) {
            int healed = imin(steal, g->max_hp - g->hp);
            g->hp += healed;
            log_add(g, "Your weapon drinks %d HP.", healed);
        }
    }

    if (e->hp <= 0) {
        combat_victory(g, e);
        return;
    }
    combat_enemy_turn(g, e);
}

void combat_use_supply(Game *g, SupplyType type) {
    Enemy *e = &g->enemies[g->combat_enemy];
    int healed;

    if (g->supplies[type] <= 0) {
        if (type == SUP_POTION)
            log_add(g, "You have no potions left!");
        else if (type == SUP_BOMB)
            log_add(g, "You have no bombs left!");
        else
            log_add(g, "You have no elixirs left!");
        return;
    }

    if (!combat_poison_tick(g)) return;

    if (type == SUP_POTION) {
        if (g->hp >= g->max_hp) {
            log_add(g, "You are already at full health.");
            return;
        }
        g->supplies[SUP_POTION]--;
        healed = imin(12, g->max_hp - g->hp);
        g->hp += healed;
        g->combat_turn++;
        log_add(g, "You drink a potion and recover %d HP.", healed);
    } else if (type == SUP_BOMB) {
        int dmg = 25 + rand_range(8);
        g->supplies[SUP_BOMB]--;
        e->hp -= dmg;
        if (e->hp < 0) e->hp = 0;
        g->combat_turn++;
        log_add(g, "You hurl a bomb: %d damage!", dmg);
        if (e->hp <= 0) {
            combat_victory(g, e);
            return;
        }
    } else {
        if (g->combat_buff > 0) {
            log_add(g, "The elixir's fire is already in your blood.");
            return;
        }
        g->supplies[SUP_ELIXIR]--;
        healed = imin(20, g->max_hp - g->hp);
        g->hp += healed;
        g->combat_buff = 4;
        g->combat_turn++;
        if (healed > 0)
            log_add(g, "You drain an elixir: +4 ATK, %d HP.", healed);
        else
            log_add(g, "You drain an elixir: +4 ATK.");
    }

    combat_enemy_turn(g, e);
}

void combat_flee(Game *g) {
    Enemy *e = &g->enemies[g->combat_enemy];
    const EnemyDef *d = enemy_def(e->type);

    if (e->type == E_DRAGON) {
        log_add(g, "The dragon fills the gate. There is no escape.");
        return;
    }

    g->combat_turn++;
    if (rand_range(100) < 55) {
        g->mode = MODE_MAP;
        g->poison_turns = 0;
        g->combat_buff = 0;
        log_add(g, "You slip away into the corridor.");
        update_visibility(g);
    } else {
        log_add(g, "You stumble! The %s attacks.", d->name);
        combat_enemy_turn(g, e);
    }
}
