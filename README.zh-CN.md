# ASCII QUEST — Crypt of the ASCII King

> [GitHub 仓库](https://github.com/Neatsuki/ascii-quest) · [English README](README.md)

一个用 **x86-32 NASM + C** 写的简易 ASCII 终端 RPG，运行于 **Linux**。
没有 ncurses，没有游戏引擎：画面全部由 ANSI 转义序列和字符拼出来，
底层 I/O 与随机数直接走 NASM 的 `int 0x80` 系统调用。

代码按职责拆成 **10 个 C 模块 + 1 个共享头文件 + 1 个 NASM 文件**，
不是单文件工程；地图、怪物、词缀等静态表集中在 `src/data.c`。

```
+================================================================+
|  ASCII QUEST  Crypt of the ASCII King      Floor 7/13 | Turn 0 |
+================================================================+
+-------------------------------+   +----------------------------+
|###                            |   | @ HERO                Lv 1 |
|#@....#                        |   | HP [############] 24/24    |
|#.....#                        |   | ATK 5 (5+0) DEF 1          |
| ..$..                         |   | XP 0/10   Gold 0           |
| .....                         |   | Items P2 B1 E0             |
| ....                          |   | W: Rusty Dagger            |
| ..                            |   | Floor 7: Frozen Crypt      |
|                               |   | Turns 0                    |
|                               |   | Charm: none                |
|                               |   | LEGEND                     |
|                               |   | @ you   ! potion  $ gold   |
|                               |   | > stairs  # wall  + door   |
|                               |   | S shop   N talk            |
|                               |   | move: WASD/arrows  ? help  |
|                               |   | P/B/E items   I bag        |
+-------------------------------+   +----------------------------+
  > The gate seals behind you. Only the stairs lead on.
  * The crypt waits.
  WASD/arrows | bump fight | P potion | I bag | ? help | Q quit
```

走进 `S` 就会打开商店。消耗品、下一把武器和饰品都在这一个柜台：

```
+================================================================+
|  THE PEDDLER                               Floor 1/13 | Turn 0 |
+================================================================+
|                                                                |
|  "Steel or a swig? Both keep you breathing."                   |
|                                                                |
|  Gold 500             HP 24/24                P2 B1 E0         |
|  W: Rusty Dagger                                               |
|                                                                |
+----------------------------------------------------------------+
|  FOR SALE                                                      |
|  [1]  Potion x2               12 gold    heals 12 HP           |
|  [2]  Bomb x1                 18 gold    25 damage in battle   |
|  [3]  Elixir x0               30 gold    heal 20, +4 ATK       |
|  [4]  Short Sword             15 gold    ATK +2                |
|  [5]  Iron Charm              80 gold    DEF +2                |
+----------------------------------------------------------------+
|  > Steel or a swig? Both keep you breathing.                   |
|  1-3 supplies  4 weapon  5 charm  0 leave                      |
|  [0]  Leave the counter                                        |
|  The peddler watches your purse, not your face.                |
|                                                                |
+================================================================+
```

按 `I` 打开背包。六把武器是收集品，买过的都能随时换；武器可能带随机词缀，
饰品占一个独立槽位：

```
+================================================================+
|  BACKPACK                                  Floor 1/13 | Turn 0 |
+================================================================+
|  EQUIPMENT                                                     |
|  [1] > Rusty Dagger           +0 ATK [plain]                   |
|  [2]   Short Sword            locked                           |
|  [3]   War Axe                locked                           |
|  [4]   Rune Blade             locked                           |
|  [5]   Silver Blade           locked                           |
|  [6]   Dragonbone Blade       locked                           |
|  CHARM  none  (A cycles owned)                                 |
|  SUPPLIES                                                      |
|  [P]  Potion  x2            heals 12 HP                        |
|  [B]  Bomb    x1            25 damage in battle                |
|  [E]  Elixir  x0            heal 20, +4 ATK                    |
|  Gold 0                                                        |
|  > Leather, steel, and something that ticks.                   |
|                                                                |
|  1-6 equip  P potion  B bomb  E elixir  A charm  I close       |
|                                                                |
|  The pack remembers every bargain.                             |
|                                                                |
+================================================================+
```

地图上的 `N` 是 NPC。走进它不会开战，而是进入对话；`1` 接受或交付支线：

```
+================================================================+
|  DIALOGUE                                                      |
+================================================================+
|                                                                |
|                           The Hermit                           |
+----------------------------------------------------------------+
|                                                                |
|                                                                |
|        You there. The moss has been listening to you.          |
|                                                                |
|                            page 1/4                            |
|                                                                |
|                                                                |
|  Quest available: The Hermit's Pests                           |
|                                                                |
|                                                                |
|                       1  accept / claim                        |
|                                                                |
|                                                                |
|                 Enter  next page     0  leave                  |
|               The dead are patient. You are not.               |
|                                                                |
+================================================================+
```

加 `--random` 后每层都会重新生成房间与走廊，地图名显示为 `Shifting Halls`：

```
+================================================================+
|  ASCII QUEST  Crypt of the ASCII King      Floor 1/13 | Turn 0 |
+================================================================+
+-------------------------------+   +----------------------------+
|                               |   | @ HERO                Lv 1 |
|                               |   | HP [############] 24/24    |
|                               |   | ATK 5 (5+0) DEF 1          |
|               #########       |   | XP 0/10   Gold 0           |
|               #.......#       |   | Items P2 B1 E0             |
|               #.......#       |   | W: Rusty Dagger            |
|              .....@...#       |   | Floor 1: Shifting Halls    |
|               #.......#       |   | Turns 0                    |
|                #######        |   | Charm: none                |
|                               |   | LEGEND                     |
|                               |   | @ you   ! potion  $ gold   |
|                               |   | > stairs  # wall  + door   |
|                               |   | S shop   N talk            |
|                               |   | move: WASD/arrows  ? help  |
|                               |   | P/B/E items   I bag        |
+-------------------------------+   +----------------------------+
  > The gate seals behind you. Only the stairs lead on.
  * The halls shift each time you descend.
  WASD/arrows | bump fight | P potion | I bag | ? help | Q quit
```

## 玩法

从地牢第一层一路向下，穿过 **十三层地图**，在底层击败恶龙。

地图带战争迷雾和视线遮挡，没走过的地方是黑的。

- 走进怪物即开战（bump-to-fight），战斗是回合制
- `A` 攻击、`P` 喝药、`B` 扔炸弹、`E` 喝药剂、`F` 逃跑（对恶龙无效）
- 捡 `!` 药水与 `$` 金币；`>` 是下一层楼梯
- 按 `I` 打开背包：`1`-`6` 换武器，`P`/`B`/`E` 使用消耗品，`A` 切换饰品
- 走进 `S` 商人购物：消耗品、下一把武器或饰品
- 走进 `N` NPC 对话，可以接支线；任务进度会记录在对话页和日志里
- 击杀获得经验，升级会提升最大生命、攻击和防御并回满血
- `F5` 保存、`F9` 读取（启动时给了 `--save PATH`，或 `--load` 过存档）
- 每下一层会自动存档到当前存档路径
- 第 6 层有巫妖把守，第 10-12 层是新增区域，第 13 层是恶龙；恶龙每第三回合喷火，火焰伤害无视防御
- 蜘蛛、巫妖的毒会持续 3 回合，每回合掉 1 HP；巫妖、食尸鬼会吸血
- 死亡后按 `R` 重来，按 `Q` 退出

### 十三层地图

| 层 | 名称 | 说明 |
|----|------|------|
| 1 | The Mossy Halls | 起点，史莱姆与哥布林，商店 |
| 2 | The Sunken Cells | 蝙蝠与骷髅；隐士在此 |
| 3 | The Bone Gallery | 骷髅群，商店 |
| 4 | The Ember Depths | 熔岩与骷髅，商店 |
| 5 | The Obsidian Vault | 黑曜石大厅，商店 |
| 6 | The Ashen Gate | 巫妖 mini-boss；灰烬幽灵在此 |
| 7 | Frozen Crypt | 老鼠、蜘蛛、食尸鬼、幽灵，商店 |
| 8 | Whisper Warrens | 更密集的怪物，商店 |
| 9 | Clockwork Sanctum | 魔像与小鬼，商店 |
| 10 | The Molten Forge | 熔炉工坊，小鬼/邪教徒/魔像，商店 |
| 11 | The Starless Span | 无星之桥，幽灵/邪教徒/食尸鬼 |
| 12 | The Crimson Gate | 猩红之门，邪教徒与幼龙，商店 |
| 13 | The Dragon's Maw | 恶龙决战 |

### 敌人

| 字符 | 敌人 | HP | 攻击 | 防御 | 经验 | 金币 | 特性 |
|------|------|----|------|------|------|------|------|
| `s` | Slime | 6 | 2 | 0 | 3 | 2-5 | — |
| `g` | Goblin | 10 | 4 | 1 | 6 | 5-9 | — |
| `b` | Bat | 8 | 3 | 0 | 5 | 3-7 | 20% 闪避 |
| `k` | Skeleton | 14 | 6 | 2 | 10 | 8-14 | — |
| `r` | Rat | 5 | 2 | 0 | 3 | 1-3 | 30% 闪避 |
| `x` | Spider | 9 | 3 | 0 | 8 | 3-6 | 35% 使玩家中毒 |
| `i` | Imp | 12 | 5 | 1 | 10 | 5-9 | 30% 火球 |
| `z` | Ghoul | 20 | 6 | 1 | 14 | 6-10 | 每次攻击吸血 3 |
| `w` | Wraith | 16 | 8 | 0 | 18 | 8-12 | 25% 闪避，无视 60% 防御 |
| `o` | Golem | 34 | 7 | 7 | 30 | 12-18 | 35% 发呆一回合 |
| `L` | Lich | 90 | 12 | 4 | 70 | 40-60 | 吸血 2、25% 中毒、无视 40% 防御 |
| `c` | Ember Cultist | 22 | 9 | 3 | 24 | 10-16 | 30% 火球 |
| `W` | Wyrmling | 55 | 14 | 6 | 50 | 30-45 | 10% 闪避，35% 火球 |
| `D` | Dragon | 340 | 20 | 9 | 450 | 300-450 | 每第 3 回合喷火 16-20，无视防御 |

### 物品与背包

初始背包是药水 x2、炸弹 x1、药剂 x0。消耗品的数量和效果都可以在背包里查看，
但炸弹和药剂只能在战斗中使用：

| 物品 | 按键 | 效果 | 价格 | 上限 |
|------|------|------|------|------|
| Potion | `P` | 回复 12 HP（地图或战斗） | 12 金币 | 9 |
| Bomb | `B` | 造成 25-32 伤害，无视防御 | 18 金币 | 5 |
| Elixir | `E` | 回复 20 HP，本场战斗攻击 +4 | 30 金币 | 3 |

按 `I` 打开背包后，`1`-`6` 装备已拥有的武器，`P`/`B`/`E` 使用物品，
`A` 在已拥有的饰品间切换。药剂的攻击加成只持续当前战斗，战斗结束或重新开战就会清除。
中毒时战斗状态栏会显示红色的 `PSN3`、`PSN2`……倒计时。

### 装备词缀

每买一把新武器，会随机附带一个词缀；在背包里可以看到 `[Vampiric]` 这样的标记。
词缀直接参与 `player_atk()` / `player_def()` / `player_crit()` / `player_lifesteal()` 的计算：

| 词缀 | 效果 |
|------|------|
| Keen | 攻击 +2 |
| Vampiric | 每次命中回复 2 HP |
| Bulwark | 防御 +2 |
| Cruel | 攻击 +1，暴击率 +10% |
| Runic | 攻击 +3 |

### 饰品

饰品占一个独立槽位，商店按固定顺序出售，买到的会立即装备；重复购买会跳过已拥有的。
背包里按 `A` 循环切换：

| 饰品 | 效果 | 价格 |
|------|------|------|
| Iron Charm | 防御 +2 | 80 金币 |
| Bloodstone | 每次命中回复 2 HP | 100 金币 |
| Ember Ring | 攻击 +3 | 120 金币 |
| Lucky Coin | 攻击 +1、防御 +1 | 90 金币 |

### NPC 与支线

| NPC | 位置 | 任务 | 奖励 |
|-----|------|------|------|
| The Hermit | 第 2 层 (10,10) | 击杀 5 只怪物 | 60 金币 + Iron Charm |
| Ashen Ghost | 第 6 层 (9,3) | 击杀巫妖 | 150 金币 + Ember Ring |

对话页里 `Enter` / `Space` / 方向键翻页，`1` 接受或交付，`0` / `Esc` 离开。
如果奖励饰品已经拥有，会改为额外 80 金币。任务进度在对话页和日志里都能看到。

### 商店与武器

商人出现在第 1、3、4、5、7、8、9、10、12 层，走进 `S` 打开商店。`1`-`3` 买消耗品，
`4` 买下一把武器，`5` 买饰品，`0` 离开：

| 商品 | 效果 | 价格 |
|------|------|------|
| Potion | 药水 +1 | 12 金币 |
| Bomb | 炸弹 +1 | 18 金币 |
| Elixir | 药剂 +1 | 30 金币 |
| Short Sword | 攻击 +2 | 15 金币 |
| War Axe | 攻击 +5 | 45 金币 |
| Rune Blade | 攻击 +9 | 90 金币 |
| Silver Blade | 攻击 +13 | 150 金币 |
| Dragonbone Blade | 攻击 +18 | 260 金币 |

武器只能按顺序升级，商店每次只卖 `best_weapon + 1`，买完自动装备并随机附上一个词缀。
买过的武器不会消失，可以在背包里自由切换。面板里的 `ATK 23 (5+18)`
表示基础攻击 5、武器加成 18；`DEF` 同理，会算上词缀和饰品。

## 存档与随机地牢

### 存档

```bash
./rpg --save /tmp/aq.sav          # 启动并指定存档
./rpg --load /tmp/aq.sav          # 直接从存档进入地图
./rpg --load /tmp/aq.sav --save /tmp/aq.sav   # 读档，并继续写回同一文件
```

- 游戏内 `F5` 保存，`F9` 读取；只有启动时给了 `--save` 或 `--load` 才可用
- 每次下楼会自动保存到当前存档路径
- 存档是版本化二进制格式：magic `AQSV`、`SAVE_VERSION 2`，逐字段显式读写，
  不做 `fwrite(&game, sizeof game)`；读档会校验 magic、版本、地图尺寸、敌人/物品数量
- v2 起地图扩展到 13 层，旧 v1 存档会被安全拒绝，需要重新开始
- 保存内容包括地图、视野、玩家属性、背包、武器词缀、饰品、任务进度、敌人、物品、
  日志、随机数状态；随机地牢的地图也原样存入，所以读档后不会重新生成

### 随机地牢

```bash
./rpg --random
./rpg --random --floor 6
```

`--random` 会让每一层在进入时生成 5-7 个不重叠房间，用 L 形走廊按顺序连接，
因此整层一定连通；玩家放在第一间房，楼梯放在最后一间房。商店、NPC、巫妖和恶龙
按层数放置。地图仍显示 `Floor N/13`，名字是 `Shifting Halls`。

## 构建

需要 32 位 x86 Linux 工具链。Debian / Ubuntu / WSL：

```bash
sudo apt install nasm gcc-multilib libc6-dev-i386
make
./rpg
```

验证产物确实是 32 位 ELF：

```bash
file rpg
# rpg: ELF 32-bit LSB executable, Intel i386 ...
```

### 命令行选项

```text
./rpg [options]

  --seed N      固定随机种子（同一种子地图与敌人行为可复现）
  --floor N     直接从第 1..13 层开始（开发/测试用）
  --random      每层程序化生成
  --save PATH   F5 与自动存档使用的文件
  --load PATH   读取存档并从地图开始
  --help        显示帮助
```

示例：

```bash
./rpg --seed 42
./rpg --floor 13
./rpg --random --seed 7
./rpg --save /tmp/aq.sav
```

终端至少需要 **70x24**，推荐 100x30 或更大。Windows 下建议在
Windows Terminal 的 WSL 标签页里运行，字体使用等宽字体。

## 操作

| 按键 | 作用 |
|------|------|
| `WASD` / 方向键 | 移动 |
| 走向怪物 | 开战 |
| 走向 `S` 商人 | 打开商店 |
| 走向 `N` NPC | 打开对话 |
| `1` / `2` / `3` / `4` / `5` / `0` | 商店内：买药水 / 炸弹 / 药剂 / 下一把武器 / 饰品 / 离开 |
| `I` | 打开背包；背包内 `1`-`6` 装备武器，`P`/`B`/`E` 使用物品，`A` 切饰品 |
| `A` / `Enter` | 战斗中攻击 |
| `P` | 喝药水（地图或战斗中，回复 12 HP） |
| `B` | 战斗中扔炸弹（25-32 伤害，无视防御） |
| `E` | 战斗中喝药剂（回复 20 HP，本场战斗攻击 +4） |
| `F` | 战斗中逃跑（55%，恶龙不可逃） |
| `F5` / `F9` | 保存 / 读取当前存档 |
| `Space` | 原地等待一回合 |
| `?` | 帮助 |
| `Q` 然后 `Y` | 退出 |
| `R` | 死亡/胜利后重新开始 |

对话中：`Enter` / `Space` 翻页，`1` 接受或交付，`0` / `Esc` 离开。

## 项目结构

```
.
├── Makefile
├── README.md
├── rpg                 # 构建产物（32 位 ELF）
├── src
│   ├── game.h          # 共享类型、常量、函数原型
│   ├── data.c          # 怪物/武器/词缀/饰品/NPC/十三层地图
│   ├── ui.c            # 终端、输出缓冲、按键解析
│   ├── render.c        # 地图/战斗/商店/背包/对话/标题
│   ├── world.c         # 地形、视野、移动、拾取、上下楼
│   ├── dungeon.c       # --random 的程序化楼层
│   ├── combat.c        # 回合制战斗、特性、中毒、升级
│   ├── inventory.c     # 消耗品、武器、词缀、饰品、商店
│   ├── npc.c           # 对话页、任务状态与奖励
│   ├── save.c          # 版本化二进制存档
│   ├── main.c          # 命令行、主循环、各模式按键
│   └── asmfuncs.asm    # NASM：write/read/PRNG/memset32/PRNG 状态
└── tests
    └── smoke_test.py   # 伪终端冒烟测试
```

### 模块职责

| 文件 | 职责 |
|------|------|
| `game.h` | 唯一的公共头文件；`Game` 是完整游戏状态 |
| `data.c` | 只放静态数据，不依赖渲染与输入 |
| `ui.c` | 输出缓冲与终端 raw 模式；`read_key()` 解析方向键/F5/F9 |
| `render.c` | 只读 `Game` 并画屏，不修改游戏状态 |
| `world.c` | 地图加载、视野、移动、拾取、下楼、随机数入口 |
| `dungeon.c` | 房间/走廊生成、按层放怪与 Boss |
| `combat.c` | 玩家/敌人回合、特性、毒、掉落与升级 |
| `inventory.c` | 所有装备数值与商店交易 |
| `npc.c` | NPC 对话页与任务状态机 |
| `save.c` | 显式字段的存档读写与校验 |
| `main.c` | 参数解析、事件循环、按模式分发按键 |
| `asmfuncs.asm` | `int 0x80` 的 write/read 与 xorshift32 |

### C 与汇编的分工

`src/asmfuncs.asm` 提供 7 个 cdecl 函数，系统调用部分全部通过 32 位 `int 0x80`：

| 函数 | 系统调用 / 实现 |
|------|------------------|
| `asm_write(fd, buf, count)` | `eax=4` |
| `asm_read(fd, buf, count)` | `eax=3` |
| `asm_random()` | xorshift32，返回 32 位无符号数 |
| `asm_srandom(seed)` | 设置种子，0 会被替换为非零值 |
| `asm_fill32(dst, value, count)` | `rep stosd`，用于清空地图/视野数组 |
| `asm_get_random_state()` | 读取 xorshift32 状态，存档用 |
| `asm_set_random_state(state)` | 恢复 xorshift32 状态，读档用 |

C 侧负责游戏逻辑和画面拼装；需要无缓冲、低延迟的终端 I/O 时直接调用
汇编函数，避免 stdio 缓冲干扰逐帧渲染。链接仍然是动态链接 libc 的
普通 32 位程序，构建参数见 `Makefile`：

```make
CFLAGS  := -m32 -march=i386 -std=gnu11 -O2 -Wall -Wextra \
           -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables
LDFLAGS := -m32 -no-pie -Wl,-z,noexecstack
```

## 测试

```bash
make test
```

`tests/smoke_test.py` 用 Python 标准库的 `pty` 启动真实游戏进程，
设置窗口大小后模拟按键，共 **73 项检查**，覆盖：

- `--help` 与非法参数
- 标题页 → 地图
- 状态面板、玩家与地图边框
- 移动后重绘
- 帮助页打开/关闭
- 背包打开、词缀/饰品栏、武器锁定提示、地图上禁用炸弹
- 退出确认与取消
- 十三层地图逐层加载与退出，`Floor N/13` 标题
- 第 13 层（恶龙层）开发入口
- `--random` 随机层加载、标题与玩家
- `F5` 保存、`--load` 读档、`F9` 原地读档与回合恢复
- 第 2 层 NPC 对话、任务页与接受任务
- 正常退出码、终端与光标恢复

测试不依赖 `pyte` 等第三方库，内部带一个只处理本游戏所用
ANSI 序列的迷你屏幕模拟器。

## 实现小记

- 地图 31x15，十三层手工地图全部在 `data.c`；`--random` 时由 `dungeon.c` 生成
- 视野半径约 5 格，用 Bresenham 直线做视线遮挡；走过的格子保留记忆
- 敌人会在 6 格内以 65% 概率追击，恶龙守门不追
- 基础伤害 = `攻击 - 防御 + rand(0..2)`，12% 暴击
- 怪物特性集中在 `ENEMY_TRAITS[]`：闪避、吸血、中毒、火球、发呆、破甲
- 中毒 `poison_turns = 3`，每次玩家行动前结算 1 点伤害并显示 `PSN`
- 经验阈值：`10 + (等级 - 1) * 15`，可以连续升多级
- 消耗品共用 `supplies[]`，上限 9/5/3；炸弹伤害 `25 + rand(8)` 且无视防御
- 武器用位图记录已拥有，背包切换只改 `weapon`；商店永远卖 `best_weapon + 1`
- 每把新武器在购买时随机滚一个词缀；词缀/饰品统一在 `world.c` 的
  `player_atk/def/crit/lifesteal` 里结算，战斗和面板共用同一套数值
- 药剂 buff 只持续当前战斗，`start_combat` 会清零
- 存档不用结构体整体 dump：`save.c` 显式写每个字段，版本变更时只需改
  `SAVE_VERSION` 与字段列表；随机地牢的地图也写进存档
- 渲染每帧先写进 64 KB 输出缓冲，再一次性 `write`，避免闪烁
- 正常退出与 `atexit` 都会恢复 termios 并离开备用屏幕；`Q` 走的就是这条路径

## 许可

MIT，详见 [LICENSE](LICENSE)。
