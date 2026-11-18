/*
 * npc.c - two hand written NPCs and their side quests.
 * Dialogue is a page table per quest state; no scripting language needed.
 */
#include "game.h"

#include <stdio.h>
#include <string.h>

static const char *const HERMIT_OFFER[] = {
    "You there. The moss has been listening to you.",
    "Five of the crypt's vermin. That is my price.",
    "Bring me their end and I will give you 60 gold and an old charm.",
    "Press 1 to accept, 0 to walk away.",
};

static const char *const HERMIT_ACTIVE[] = {
    "Still breathing? The moss is disappointed.",
    "Every beast you put down feeds the quiet.",
    "Five, I said. Press 1 when the count is done.",
};

static const char *const HERMIT_DONE[] = {
    "Good. The moss is quieter now.",
    "Take the gold. Take the charm. Do not come back as a ghost.",
};

static const char *const GHOST_OFFER[] = {
    "You wear a crown of dust, living thing.",
    "The lich below wears mine. It should not.",
    "End it and I will give you 150 gold and an Ember Ring.",
    "Press 1 to swear it, 0 to leave me to my cold.",
};

static const char *const GHOST_ACTIVE[] = {
    "The crowned one still turns in its sleep.",
    "Its fire is old. So is mine.",
    "Press 1 when the crown is ash.",
};

static const char *const GHOST_DONE[] = {
    "The crown is ash. I can feel the dark loosen.",
    "Keep the ring. It remembers a warmer hand.",
};

static const char *const *pages_for(const Game *g, int *count) {
    int npc = g->dialogue_npc;
    int q = npc;
    int st = g->quest_state[q];

    if (npc == 0) {
        if (st == 3) { *count = 2; return HERMIT_DONE; }
        if (st == 2) { *count = 3; return HERMIT_ACTIVE; }
        *count = 4; return HERMIT_OFFER;
    }
    if (st == 3) { *count = 2; return GHOST_DONE; }
    if (st == 2) { *count = 3; return GHOST_ACTIVE; }
    *count = 4; return GHOST_OFFER;
}

void quest_refresh(Game *g) {
    if (g->floor >= 1 && g->quest_state[0] == 0) g->quest_state[0] = 1;
    if (g->floor >= 5 && g->quest_state[1] == 0) g->quest_state[1] = 1;
}

const char *dialogue_title(const Game *g) {
    if (g->dialogue_npc < 0 || g->dialogue_npc >= NPC_COUNT) return "???";
    return NPCS[g->dialogue_npc].name;
}

int dialogue_pages(const Game *g) {
    int count = 0;
    pages_for(g, &count);
    return count;
}

const char *dialogue_line(const Game *g, int page) {
    int count = 0;
    const char *const *pages = pages_for(g, &count);
    if (page < 0) page = 0;
    if (page >= count) page = count - 1;
    return pages[page];
}

int dialogue_can_accept(const Game *g) {
    int q = g->dialogue_npc;
    if (q < 0 || q >= QUEST_COUNT) return 0;
    if (g->quest_state[q] == 1) return 1;
    if (g->quest_state[q] == 2) {
        if (q == 0) return g->quest_progress[0] >= 5;
        return g->quest_progress[1] >= 1;
    }
    return 0;
}

static void reward_quest(Game *g, int q) {
    g->quest_state[q] = 3;
    if (q == 0) {
        g->gold += 60;
        if (!grant_accessory(g, 0, 1)) g->gold += 80;
        log_add(g, "The Hermit pays 60 gold. The hunt is done.");
    } else {
        g->gold += 150;
        if (!grant_accessory(g, 2, 1)) g->gold += 80;
        log_add(g, "The Ghost pays 150 gold. The crown is ash.");
    }
}

void open_dialogue(Game *g, int npc_index) {
    if (npc_index < 0 || npc_index >= NPC_COUNT) return;
    g->mode = MODE_DIALOGUE;
    g->dialogue_npc = npc_index;
    g->dialogue_page = 0;
    g->talked_npc[npc_index] = 1;
    quest_refresh(g);
}

void handle_dialogue(Game *g, int key) {
    if (key == KEY_EOF || key == 3) {
        g->running = 0;
        return;
    }
    if (key == 27 || key == '0' || key == 'q' || key == 'Q') {
        g->mode = MODE_MAP;
        return;
    }
    if (key == '1' && dialogue_can_accept(g)) {
        int q = g->dialogue_npc;
        if (g->quest_state[q] == 1) {
            g->quest_state[q] = 2;
            g->quest_progress[q] = 0;
            log_add(g, "Quest accepted: %s.",
                    q == 0 ? "The Hermit's Pests" : "The Crown of Ash");
        } else {
            reward_quest(g, q);
        }
        g->dialogue_page = 0;
        return;
    }
    if (key == '\n' || key == ' ' || key == KEY_DOWN || key == KEY_RIGHT) {
        if (g->dialogue_page + 1 < dialogue_pages(g)) g->dialogue_page++;
        return;
    }
    if (key == KEY_UP || key == KEY_LEFT) {
        if (g->dialogue_page > 0) g->dialogue_page--;
    }
}

void quest_on_kill(Game *g, EnemyType type) {
    if (type == E_LICH) g->quest_progress[1] = 1;

    if (g->quest_state[0] == 2) {
        g->quest_progress[0]++;
        log_add(g, "The Hermit's tally: %d/5.", g->quest_progress[0]);
        if (g->quest_progress[0] == 5)
            log_add(g, "The Hermit's price is met. Return to him.");
    }
    if (type == E_LICH && g->quest_state[1] == 2)
        log_add(g, "The crowned lich falls. The Ghost is waiting.");
}
