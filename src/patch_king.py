import io
p = "/root/quest/drive5.py"
s = io.open(p, encoding="utf-8").read()

old_battle = '    if "[A]ttack" in pane:\n        if hp and pot > 0 and hp[0]*100 < hp[1]*60: send("p")'
new_battle = ('    if "[A]ttack" in pane:\n'
              '        mb = re.search(r"P(\\d+) B(\\d+) E(\\d+)", pane)\n'
              '        elx = int(mb.group(3)) if mb else 0\n'
              '        boss = ("ASCII King" in pane) or ("Dragon" in pane)\n'
              '        if boss and elx > 0: send("e"); continue\n'
              '        if hp and pot > 0 and hp[0]*100 < hp[1]*60: send("p")')
assert s.count(old_battle) == 1
s = s.replace(old_battle, new_battle)

old_shop = '        for _ in range(9): send("1")\n        send("5")'
new_shop = '        for _ in range(9): send("1")\n        for _ in range(3): send("3")\n        send("5")'
assert s.count(old_shop) == 1
s = s.replace(old_shop, new_shop)

io.open(p, "w", encoding="utf-8").write(s)
print("bot upgraded")
