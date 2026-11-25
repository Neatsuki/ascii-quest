import re, subprocess, time, sys, collections, fcntl

_lock = open("/root/quest/drive.lock", "w")
try:
    fcntl.flock(_lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
except OSError:
    sys.exit(0)

SRC = "/mnt/d/arena/ascii-quest-src/data.c"
SESS = "quest"

def tmux(*a):
    return subprocess.run(["tmux", *a], capture_output=True, text=True).stdout

def send(k):
    with open("/root/quest/keys.log", "a") as _k:
        _k.write(repr(k) + "\n")
    tmux("send-keys", "-t", SESS, k)
    time.sleep(0.14)

def cap():
    return tmux("capture-pane", "-p", "-t", SESS)

t = open(SRC, encoding="utf-8").read()
blk = t[t.index("const char *const MAPS"):t.index("const char *const FLOOR_NAMES")]
rows = re.findall(r'^\s*"([^"]{31})",\s*$', blk, re.M)
MAPS = [rows[i*15:(i+1)*15] for i in range(len(rows)//15)]
GLYPHS = set(re.findall(r'\{\s*"[A-Za-z ]+",\s*\'(.)\',\s*\d+', t))
GLYPHS.discard("."); GLYPHS.discard("#")
GLYPHS -= set("NS>l")  # NPC / shop / stairs / lich-floor marker are not farm targets

def parse(pane):
    lines = pane.split("\n")
    floor = None
    for ln in lines:
        m = re.search(r"Floor (\d+)/\d+", ln)
        if m:
            floor = int(m.group(1)); break
    vis = [ln[1:32] for ln in lines if re.match(r'^\|.{31}\|', ln)][:15]
    hp = None; pot = 0; lvl = 0
    src = pane
    for ln in lines:
        if "YOU" in ln and "HP" in ln:
            src = ln; break
    m = re.search(r"HP \[[#\-]+\]\s*(\d+)/(\d+)", src)
    if m: hp = (int(m.group(1)), int(m.group(2)))
    m = re.search(r"P(\d+) B(\d+) E(\d+)", src)
    if m: pot = int(m.group(1))
    m = re.search(r"Lv (\d+)", pane)
    if m: lvl = int(m.group(1))
    m = re.search(r"Gold (\d+)", pane)
    gold = int(m.group(1)) if m else 0
    return floor, vis, hp, pot, lvl, gold

def locate(vis):
    if len(vis) < 15: return None
    for i, vr in enumerate(vis[:15]):
        j = vr.find("@")
        if j >= 0: return (j, i)
    return None

def bfs(grid, px, py, goals, blocked=frozenset()):
    H, W = len(grid), len(grid[0])
    prev = {}; seen = {(px, py)}
    q = collections.deque([(px, py)]); goal = None
    while q:
        x, y = q.popleft()
        if (x, y) in goals:
            goal = (x, y); break
        for dx, dy, k in ((0,-1,"w"), (0,1,"s"), (-1,0,"a"), (1,0,"d")):
            nx, ny = x+dx, y+dy
            if 0 <= nx < W and 0 <= ny < H and (nx, ny) not in seen and grid[ny][nx] != "#" and (nx, ny) not in blocked:
                seen.add((nx, ny)); prev[(nx, ny)] = ((x, y), k); q.append((nx, ny))
    if not goal: return ""
    path = []; cur = goal
    while cur != (px, py):
        p, k = prev[cur]; path.append(k); cur = p
    return "".join(reversed(path))

maxsteps = int(sys.argv[1]) if len(sys.argv) > 1 else 300
last_floor = None; targets = []; banned = set(); stall = 0; last_sig = None
for i in range(maxsteps):
    pane = cap()
    floor, vis, hp, pot, lvl, gold = parse(pane)
    with open("/root/quest/dbg.log", "a") as _f:
        _f.write("%d %r\n" % (i, pane.splitlines()[1][:56] if len(pane.splitlines()) > 1 else ""))
    if "YOU DIED" in pane or "GAME OVER" in pane:
        print("DEAD floor", floor, hp, flush=True); break
    if "[A]ttack" in pane:
        mb = re.search(r"P(\d+) B(\d+) E(\d+)", pane)
        elx = int(mb.group(3)) if mb else 0
        boss = ("ASCII King" in pane) or ("Dragon" in pane)
        if boss and elx > 0: send("e"); continue
        if hp and pot > 0 and hp[0]*100 < hp[1]*60: send("p")
        elif "no escape" in pane: send("a")
        elif hp and hp[0]*100 < hp[1]*40: send("f"); targets = []
        else: send("a")
        continue
    if "DIALOGUE" in pane:
        if "accept" in pane or "available" in pane or "claim" in pane:
            send("1")
        send("0"); continue
    if "SHOP" in pane or "PEDDLER" in pane:
        for _ in range(3): send("4")
        for _ in range(9): send("1")
        for _ in range(3): send("3")
        send("5")
        for _ in range(4):
            send("q"); time.sleep(0.15)
            p2 = cap()
            if "PEDDLER" not in p2 and "SHOP" not in p2:
                break
        continue
    if hp and pot > 0 and hp[0]*100 < hp[1]*65:
        send("p"); continue
    loc = locate(vis)
    if not loc:
        send("\x1b"); time.sleep(0.2); send("0"); continue
    px, py = loc
    if floor != last_floor:
        targets = [(x, y) for y in range(15) for x in range(31) if vis[y][x] in GLYPHS]
        if gold >= 40:
            targets = [(x, y) for y in range(15) for x in range(31) if vis[y][x] == "S"] + targets
        print(f"== floor {floor} at ({px},{py}) lv={lvl} hp={hp} targets={len(targets)} step {i}", flush=True)
        last_floor = floor
    if loc in targets: targets.remove(loc)
    mt = re.search(r"Turn (\d+)", pane)
    turn = int(mt.group(1)) if mt else -1
    sig = (turn, loc)
    if sig == last_sig:
        stall += 1
    else:
        stall = 0; last_sig = sig
    # grid comes from the RENDERED map (static MAPS can differ, e.g. floor 13)
    if len(vis) == 15:
        grid = [r.replace(" ", ".") for r in vis]
    else:
        grid = MAPS[floor-1] if (floor and 1 <= floor <= 14) else vis
    blocked = {(x, y) for y in range(15) for x in range(31) if vis[y][x] in "NS"} | banned
    stairs = set()
    if floor < 13:
        stairs = {(x, y) for y in range(15) for x in range(31) if vis[y][x] == ">"}
        if not stairs and floor and 1 <= floor <= 14:
            mm = MAPS[floor-1]
            stairs = {(x, y) for y in range(15) for x in range(31) if mm[y][x] == ">"}
    unk = {(x, y) for y in range(15) for x in range(31) if vis[y][x] == " "}
    shop_adj = {}
    if floor < 13 and gold >= 60 and pot < 7:
        for y in range(15):
            for x in range(31):
                if vis[y][x] == "S":
                    for dx, dy, k in ((0, -1, "w"), (0, 1, "s"), (-1, 0, "a"), (1, 0, "d")):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < 31 and 0 <= ny < 15 and vis[ny][nx] not in "#NS+ ":
                            shop_adj[(nx, ny)] = {"w": "s", "s": "w", "a": "d", "d": "a"}[k]
    if (px, py) in shop_adj:
        send(shop_adj[(px, py)]); stall = 0; continue
    goals = set(targets)
    if hp and pot == 0 and hp[0]*100 < hp[1]*55:
        goals = stairs
    elif not goals and floor >= 13:
        dz = {(x, y) for y in range(15) for x in range(31) if MAPS[floor-1][y][x] in "DX"}
        goals = (dz | unk) or stairs
    elif not goals and shop_adj:
        goals = set(shop_adj)
    elif not goals and unk:
        goals = unk
    elif not goals:
        goals = stairs
    path = bfs(grid, px, py, goals, blocked)
    if not path and targets:
        targets = []
        path = bfs(grid, px, py, stairs or unk, blocked)
    if not path and goals:
        path = bfs(grid, px, py, stairs, blocked) if stairs else ""
    if not path:
        # truly stuck: try every reachable goal kind, else shuffle toward any open tile
        moved = False
        for gk in (unk, stairs):
            if gk:
                alt = bfs(grid, px, py, gk, blocked)
                if alt:
                    path = alt; moved = True; break
        if not moved:
            print("STUCK", floor, px, py, len(unk), flush=True)
            for dx, dy, k in ((1, 0, "d"), (0, 1, "s"), (-1, 0, "a"), (0, -1, "w")):
                nx, ny = px + dx, py + dy
                if 0 <= nx < 31 and 0 <= ny < 15 and grid[ny][nx] not in "#" and (nx, ny) not in blocked:
                    send(k); stall = 0; moved = True; break
            if not moved:
                print("BOXED IN", floor, px, py, flush=True); break
            continue
    if stall > 6:
        k = path[0] if path else ""
        dx, dy = {"w": (0, -1), "s": (0, 1), "a": (-1, 0), "d": (1, 0)}.get(k, (0, 0))
        banned.add((px + dx, py + dy)); stall = 0
        print("STALL ban", (px + dx, py + dy), "floor", floor, flush=True)
        continue
    send(path[0])
    if i % 80 == 0:
        print(f"step {i} floor={floor} pos={loc} lv={lvl} hp={hp} pot={pot} left={len(targets)}", flush=True)
print("END", cap().split("\n")[1], flush=True)
