import re, sys, collections
t = open("/mnt/d/arena/ascii-quest-src/data.c", encoding="utf-8").read()
blk = t[t.index("const char *const MAPS"):t.index("const char *const FLOOR_NAMES")]
rows = re.findall(r'^\s*"([^"]{31})",\s*$', blk, re.M)
MAPS = [rows[i*15:(i+1)*15] for i in range(len(rows)//15)]
f, x0, y0 = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
g = MAPS[f-1]
goals = {(x, y) for y in range(15) for x in range(31) if g[y][x] == '>'}
prev = {}; seen = {(x0, y0)}; q = collections.deque([(x0, y0)]); goal = None
while q:
    x, y = q.popleft()
    if (x, y) in goals: goal = (x, y); break
    for dx, dy, k in ((0,-1,"w"), (0,1,"s"), (-1,0,"a"), (1,0,"d")):
        nx, ny = x+dx, y+dy
        if 0 <= nx < 31 and 0 <= ny < 15 and (nx,ny) not in seen and g[ny][nx] not in "#DX":
            seen.add((nx,ny)); prev[(nx,ny)] = ((x,y), k); q.append((nx,ny))
if not goal:
    print("NO PATH"); sys.exit(1)
path = []; cur = goal
while cur != (x0, y0):
    p, k = prev[cur]; path.append(k); cur = p
print("".join(reversed(path)), goal)
