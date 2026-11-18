/*
 * inventory.c - consumables, weapons, affixes, accessories and the shop.
 * All gear math lives here so combat and rendering stay independent.
 */
#include "game.h"

#include <stdio.h>

void open_shop(Game *g) {
    g->mode = MODE_SHOP;
    snprintf(g->shop_msg, sizeof(g->shop_msg),
             "Steel or a swig? Both keep you breathing.");
}

void shop_buy_supply(Game *g, SupplyType type) {
    const SupplyDef *s = &SUPPLY_DEFS[type];

    if (g->supplies[type] >= s->cap) {
        snprintf(g->shop_msg, sizeof(g->shop_msg),
                 "Your pack cannot hold another %s.", s->name);
        return;
    }
    if (g->gold < s->price) {
        snprintf(g->shop_msg, sizeof(g->shop_msg),
                 "The %s costs %d gold. You have %d.",
                 s->name, s->price, g->gold);
        return;
    }

    g->gold -= s->price;
    g->supplies[type]++;
    snprintf(g->shop_msg, sizeof(g->shop_msg),
             "%s added to your pack. (%d gold left)", s->name, g->gold);
}

void shop_buy_weapon(Game *g) {
    int next = g->best_weapon + 1;

    if (next >= WEAPON_COUNT) {
        snprintf(g->shop_msg, sizeof(g->shop_msg),
                 "You already carry the %s. Nothing sharper here.",
                 WEAPONS[g->best_weapon].name);
        return;
    }
    if (g->gold < WEAPONS[next].price) {
        snprintf(g->shop_msg, sizeof(g->shop_msg),
                 "The %s costs %d gold. You have %d.",
                 WEAPONS[next].name, WEAPONS[next].price, g->gold);
        return;
    }

    g->gold -= WEAPONS[next].price;
    g->best_weapon = next;
    g->weapons_owned |= 1u << next;
    g->weapon = next;
    if (g->weapon_affix[next] < 0)
        g->weapon_affix[next] = rand_range(AFFIX_COUNT);
    int aff = g->weapon_affix[next];
    snprintf(g->shop_msg, sizeof(g->shop_msg),
             "The %s is yours. %s, ATK %d. (%d gold)",
             WEAPONS[next].name, AFFIXES[aff].name,
             player_atk(g), g->gold);
}

void shop_buy_accessory(Game *g) {
    int next = -1;
    for (int i = 0; i < ACCESSORY_COUNT; i++) {
        if (!(g->accessories_owned & (1u << i))) { next = i; break; }
    }
    if (next < 0) {
        snprintf(g->shop_msg, sizeof(g->shop_msg),
                 "You own every charm I have. Wear them in health.");
        return;
    }
    if (g->gold < ACCESSORIES[next].price) {
        snprintf(g->shop_msg, sizeof(g->shop_msg),
                 "The %s costs %d gold. You have %d.",
                 ACCESSORIES[next].name, ACCESSORIES[next].price, g->gold);
        return;
    }
    g->gold -= ACCESSORIES[next].price;
    g->accessories_owned |= 1u << next;
    g->accessory = next;
    snprintf(g->shop_msg, sizeof(g->shop_msg),
             "The %s hums on your belt. %s. (%d gold left)",
             ACCESSORIES[next].name, ACCESSORIES[next].desc, g->gold);
}

int grant_accessory(Game *g, int index, int announce) {
    if (index < 0 || index >= ACCESSORY_COUNT) return 0;
    if (g->accessories_owned & (1u << index)) return 0;
    g->accessories_owned |= 1u << index;
    if (g->accessory < 0) g->accessory = index;
    if (announce) {
        log_add(g, "You gain the %s. %s.",
                ACCESSORIES[index].name, ACCESSORIES[index].desc);
    }
    return 1;
}

void cycle_accessory(Game *g) {
    if (!g->accessories_owned) {
        snprintf(g->inv_msg, sizeof(g->inv_msg),
                 "You own no charms. The peddler sells four.");
        return;
    }
    int start = g->accessory;
    for (int step = 1; step <= ACCESSORY_COUNT; step++) {
        int idx = (start + step) % ACCESSORY_COUNT;
        if (idx < 0) idx += ACCESSORY_COUNT;
        if (g->accessories_owned & (1u << idx)) {
            g->accessory = idx;
            snprintf(g->inv_msg, sizeof(g->inv_msg),
                     "You wear the %s. %s.",
                     ACCESSORIES[idx].name, ACCESSORIES[idx].desc);
            return;
        }
    }
    g->accessory = -1;
    snprintf(g->inv_msg, sizeof(g->inv_msg), "You put the charm away.");
}

void equip_weapon(Game *g, int index) {
    if (index < 0 || index >= WEAPON_COUNT ||
        !(g->weapons_owned & (1u << index))) {
        snprintf(g->inv_msg, sizeof(g->inv_msg),
                 "You do not own that weapon yet.");
        return;
    }
    g->weapon = index;
    int aff = g->weapon_affix[index];
    if (aff >= 0)
        snprintf(g->inv_msg, sizeof(g->inv_msg),
                 "You equip the %s (%s). ATK %d.",
                 WEAPONS[index].name, AFFIXES[aff].name, player_atk(g));
    else
        snprintf(g->inv_msg, sizeof(g->inv_msg),
                 "You equip the %s. ATK %d.",
                 WEAPONS[index].name, player_atk(g));
}

void open_inventory(Game *g) {
    g->mode = MODE_INV;
    snprintf(g->inv_msg, sizeof(g->inv_msg),
             "Leather, steel, and something that ticks.");
}

void inv_use_supply(Game *g, SupplyType type) {
    if (type == SUP_POTION) {
        int before = g->hp;
        map_drink_potion(g);
        if (g->hp > before)
            snprintf(g->inv_msg, sizeof(g->inv_msg),
                     "You drink a potion and recover %d HP.", g->hp - before);
        else if (g->supplies[SUP_POTION] <= 0)
            snprintf(g->inv_msg, sizeof(g->inv_msg),
                     "You have no potions left.");
        else
            snprintf(g->inv_msg, sizeof(g->inv_msg),
                     "You are already at full health.");
    } else if (type == SUP_BOMB) {
        snprintf(g->inv_msg, sizeof(g->inv_msg),
                 "Bombs are only useful in a fight.");
    } else {
        snprintf(g->inv_msg, sizeof(g->inv_msg),
                 "Elixirs only work in a fight.");
    }
}
