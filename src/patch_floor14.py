import io

def patch(path, pairs):
    s = io.open(path, encoding="utf-8").read()
    for old, new in pairs:
        assert s.count(old) == 1, (path, old, s.count(old))
        s = s.replace(old, new)
    io.open(path, "w", encoding="utf-8").write(s)

patch("/root/quest/dash.py", [
    (r")/13 \| Turn", r")/\d+ \| Turn"),
    ("%(floor)s/13", "%(floor)s/14"),
])
patch("/root/quest/supervise.py", [
    (r"Floor (\d+)/13", r"Floor (\d+)/\d+"),
])
print("patched")
