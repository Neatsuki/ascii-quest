# 第二期 · 横屏 53 秒｜《一个跑不起来的 32 位游戏》

> 定位：技术叙事款（B 站/YouTube 横屏）。真实录屏 + 极简卡片，讲「救活 → 逆向 → 机器人通关 → 魔改 → 再通关」。
> 成片：`D:\arena\video\ascii-quest.mp4`（1920×1080 / 30fps / 53s / 1.7 MB）
> 封面：`D:\arena\video\cover-rpg.png`
> 素材：`clip_gameplay2.mp4`（真实闯关）、`clip_code2.mp4`（diff / gcc / 机器人日志 / stats.csv）

---

## 一、成片时间轴

| 时间 | 内容 | 类型 |
|---|---|---|
| 0–3.5s | 「一个跑不起来的 32 位游戏 / 它躺在 Docker 里，连 --help 都 segfault」 | 卡片 |
| 3.5–7.5s | 「第一步：从容器里捞出来 / docker cp → WSL Ubuntu → 直接跑通」 | 卡片 |
| 7.5–16.5s | 真实画面：ASCII QUEST 地牢，「You defeat the Slime! +3 XP」 | 录屏 |
| 16.5–20.5s | 「第二步：逆向存档格式 / magic 0x41515356 AQSV、偏移 20 是楼层」 | 卡片 |
| 20.5–28.5s | 真实画面：`diff -u src_backup/data.c src/data.c`，+E_KING 结构体、ASCII 王冠 | 录屏 |
| 28.5–34.5s | 真实画面：`gcc -m32 … && nasm …` 重新编译 | 录屏 |
| 34.5–43s | 真实画面：`tail drive_dlc.log`，机器人一步步 floor 1→5 | 录屏 |
| 43–46.5s | 「第三小时：机器人通关 / Lv15 · 913 金 · 1518 回合」 | 卡片 |
| 46.5–50.5s | 「然后我改了游戏 / FLOORS 13→14，新 Boss The ASCII King」 | 卡片 |
| 50.5–54s | 「再通关一次 / Lv18 · 1101 金 · 2360 回合」 | 卡片 |
| 54–57.5s | 「源码、存档、机器人都在仓库里」 | 卡片 |

> 卡片都做了极慢推近（Ken Burns），不是死画面。

---

## 二、口播稿（约 320 字，语速 4.5 字/秒 ≈ 70 秒，可裁）

> 桌面上躺着一个 32 位的终端游戏，13 层地牢、一条龙，听着挺完整。
> 问题是它在一个 Docker 容器里，连 `--help` 都 segfault。
> （画外音停顿）
> 我把它 `docker cp` 出来，丢进 WSL 的 Ubuntu——跑得好好的。**问题出在环境，不在程序。**
> 接下来是逆向：存档是个二进制，magic 是 `AQSV`，偏移 20 的字段是楼层——而且是 0 起算的，所以它写 1 的时候，界面显示第 2 层。这个坑让我对着错的地图找了三分钟的路。
> 有了格式，就能写机器人：读屏幕、BFS 找最近的怪、血少了喝药、怪打不过就跑。第三个小时，它自己把龙杀了——Lv15、913 金、1518 回合。
> 然后我做了件更过分的事：把源码里 `FLOORS 13` 改成 14，加了个新 Boss「The ASCII King」，460 血、每三回合放一次雷、不能跑。
> 半小时后，机器人把国王也杀了——Lv18、1101 金、2360 回合。
> 源码、存档、机器人脚本都在仓库里。想复现，或者改成你自己的版本，评论区见。

---

## 三、发布文案

**标题**
> 我把 Docker 里一个跑不起来的 32 位游戏救活了，还给它加了个 Boss

**备选**
- 一晚上：逆向 → 写机器人通关 → 改游戏 → 再通关
- 32 位终端 RPG：从 segfault 到「THE KING IS SLAIN」

**简介**
> 全程在 WSL Ubuntu 里做：docker cp 救出二进制 → 逆向存档格式（magic AQSV）→ 写自动驾驶机器人（BFS + 战斗策略）→ 通关 → 改源码加第 14 层和新 Boss → 再通关。
> 工具链：gcc -m32 / nasm / python3 / tmux。视频里的代码、日志、存档都是真的。

**标签**
`#逆向 #游戏开发 #独立游戏 #终端 #Linux #WSL #自动化 #程序员`

---

## 四、如果再精修（给你在剪映里做的事）

1. **配 BGM**：B 站区搜「科技感 / 悬疑推近」，音量压到 −22 dB，口播之上。
2. **字幕**：剪映「识别字幕」一键，再把`.md`里口播稿贴进去校对。
3. **动效**：卡片段加「缩放 + 闪白转场」，录屏段加「鼠标高亮」。
4. **节奏**：53 秒偏短，可把开头的救援过程补一段（docker ps / segfault 复现），拉到 90 秒，B 站完播率更好。

---

## 五、复现命令（简介里可以放）

```bash
docker cp ascii-quest:/ascii-quest/rpg .
wsl -d Ubuntu -- bash -c 'mkdir -p ~/quest && mv rpg ~/quest && cd ~/quest && chmod +x rpg && ./rpg --help'
# 逆向：save.c 里 magic 0x41515356 / version 2 / 偏移 20 = floor
# 机器人：python3 drive5.py 2500
# 改游戏：patch src/game.h FLOORS 13→14，data.c 加 [E_KING]
cd ~/quest/src && gcc -m32 -O2 -w -c *.c && nasm -f elf32 asmfuncs.asm -o asmfuncs.o && gcc -m32 -no-pie -o rpg *.o
```

---

## 六、配音 + BGM（已做，2026-11-25 晚）

- **配音**：edge-tts `zh-CN-YunxiNeural`，语速 +12%，逐句合成后按句长卡点拼视频 —— 每个镜头的长度 = 那句台词的音频长度 + 0.42s，所以话音落画面也切。
- **BGM**：程序生成的极简低频垫（55Hz + 82.5Hz 纯五度 + 少量噪声，lowpass 200Hz），音量 0.18，淡入 2s / 淡出 3s。
- **成片**：`D:\arena\video\ascii-quest-final.mp4`（1920×1080 / 75.2s / 3.1 MB / AAC 单声道 / 平均 −23.7 dB，峰值 −3.0 dB）
- **构建脚本**：`make_final.py`（改台词改 `SEGS` 里的字符串，重跑 3 分钟出新片）

### 没做到的

原本想补拍「Docker 里 segfault」的真实画面，但 docker CLI 走 Kiri 的 pipe（`npipe:////./pipe/kiri`），Kiri 现在没跑，daemon 连不上。可选替代：
1. 启动 Kiri → 重启容器 → 真录（最真实）；
2. 用 WSL 的 Alpine（`kiri` 发行版，没有 32 位 loader）跑 `./rpg`，录「跑不起来」的报错，字幕注明是同一类问题。