# TOML Schema Proposal (Draft)

This is a draft schema for the planned TOML cutover of gameplay data only.
It mirrors all fields currently written by the ASCII formats. Values should
be written in full (including defaults) to preserve fidelity and avoid
behavior changes.

All filenames change to `.toml`. Index files become TOML too.

## Conventions

- Strings mirror existing text fields; preserve current escaping rules for
  embedded newlines where applicable.
- Flags and bitvectors are stored as integer arrays unless otherwise noted.
- VNUM and RNUM are stored as integers.
- Timestamps are Unix epoch seconds.

## Accounts

Path: `lib/acctfiles/<A-E|F-J|K-O|P-T|U-Z|ZZZ>/<name>.toml`

```toml
name = "AccountName"
password = "hashed"
email = "user@example.com" # optional
current_pc = "CharName"    # optional
pcs = ["CharOne", "CharTwo"]
```

## Player Index

Path: `lib/plrfiles/index.toml` (and `index.mini.toml` if mini mode persists)

```toml
[[player]]
id = 12345
name = "charname"
level = 10
flags = 0
last = 1700000000
```

## Player File

Path: `lib/plrfiles/<A-E|F-J|K-O|P-T|U-Z|ZZZ>/<name>.toml`

```toml
name = "CharName"
short_desc = "short desc"        # optional
password = "hashed"
account = "AccountName"          # optional
description = "long desc"        # optional
background = "long background"   # optional
poofin = "text"                  # optional
poofout = "text"                 # optional
sex = 0
class = 0
species = 0
level = 1
id = 12345
birth = 1700000000
age = 18
age_year = 205
played = 1234
logon = 1700000000
last_motd = 0
last_news = 0
reroll_used = 0
reroll_expires = 0
reroll_old_abils = [0, 0, 0, 0, 0, 0] # str int wis dex con cha
host = "example.net"             # optional
height = 180
weight = 180
act_flags = [0, 0, 0, 0]
aff_flags = [0, 0, 0, 0]
pref_flags = [0, 0, 0, 0]
saving_throws = [0, 0, 0, 0, 0]   # thr1..thr5
wimp = 0
freeze = 0
invis = 0
load_room = 0
bad_passwords = 0
conditions = { hunger = 0, thirst = 0, drunk = 0 }
hmv = { hit = 0, max_hit = 0, mana = 0, max_mana = 0, stamina = 0, max_stamina = 0 }
abilities = { str = 0, int = 0, wis = 0, dex = 0, con = 0, cha = 0 }
ac = 0
coins = 0
bank_coins = 0
exp = 0
olc_zone = 0
page_length = 0
screen_width = 0
quest_points = 0
quest_counter = 0
current_quest = 0
completed_quests = [101, 102]
triggers = [2001, 2002]
skill_gain_next = [0, 0, 0]      # length MAX_SKILLS; index 1..MAX_SKILLS

[[skill]]
id = 1
level = 50

[[affect]]
spell = 0
duration = 0
modifier = 0
location = 0
bitvector = [0, 0, 0, 0]

[[alias]]
alias = "look"
replacement = "l"
type = 0

[[var]]
name = "quest_state"
context = 0
value = "value"
```

## Player Objects (Crash/Forced/Logout/etc.)

Path: `lib/plrobjs/<A-E|F-J|K-O|P-T|U-Z|ZZZ>/<name>.toml`

```toml
[header]
save_code = 0
timed = 0
net_cost = 0
coins = 0
account = 0
item_count = 0

[[object]]
vnum = 1234
locate = 0
nest = 0
values = [0, 0, 0, 0, 0, 0]      # NUM_OBJ_VAL_POSITIONS
extra_flags = [0, 0, 0, 0]
wear_flags = [0, 0, 0, 0]
name = "obj name"                # optional
short = "short desc"             # optional
description = "long desc"        # optional
main_description = "extra desc"  # optional
type = 0
weight = 0
cost = 0
```

## Player Vars (standalone file)

Path: `lib/plrvars/<A-E|F-J|K-O|P-T|U-Z|ZZZ>/<name>.toml`

```toml
[[var]]
name = "varname"
context = 0
value = "value"
```

## Houses (objects only)

Path: `lib/house/<vnum>.toml`

```toml
[[object]]
vnum = 1234
locate = 0
nest = 0
values = [0, 0, 0, 0, 0, 0]
extra_flags = [0, 0, 0, 0]
wear_flags = [0, 0, 0, 0]
name = "obj name"
short = "short desc"
description = "long desc"
main_description = "extra desc"
type = 0
weight = 0
cost = 0
```

## Room Save

Path: `lib/world/rsv/<zone>.toml`

```toml
[[room]]
vnum = 1000
saved_at = 1700000000

[[room.object]]
vnum = 1234
timer = 0
weight = 0
cost = 0
cost_per_day = 0
extra_flags = [0, 0, 0, 0]
wear_flags = [0, 0, 0, 0]
values = [0, 0, 0, 0, 0, 0]
contents = []                    # recursive list of objects with same shape

[[room.mob]]
vnum = 2000

[[room.mob.equipment]]
wear_pos = 5
vnum = 1234
contents = []                    # recursive list of objects by vnum only

[[room.mob.inventory]]
vnum = 1234
contents = []
```

## World Index Files

Paths:
- `lib/world/wld/index.toml`
- `lib/world/mob/index.toml`
- `lib/world/obj/index.toml`
- `lib/world/zon/index.toml`
- `lib/world/shp/index.toml`
- `lib/world/trg/index.toml`
- `lib/world/qst/index.toml`

```toml
files = ["0.toml", "1.toml"]
```

## Rooms

Path: `lib/world/wld/<zone>.toml`

```toml
[[room]]
vnum = 1000
name = "Room name"
description = "Room description"
flags = [0, 0, 0, 0]
sector = 0

[[room.exit]]
dir = 0
description = "exit desc"
keyword = "door"
exit_info = 0
key = 0
to_room = 1001

[[room.extra_desc]]
keyword = "sign"
description = "text"

[[room.forage]]
obj_vnum = 1234
dc = 10

room.triggers = [3000, 3001]
```

## Mobiles

Path: `lib/world/mob/<zone>.toml`

```toml
[[mob]]
vnum = 2000
name = "mob name"
keywords = "keywords"
short = "short"
long = "long"
description = "desc"
background = "background"        # optional
flags = [0, 0, 0, 0]
aff_flags = [0, 0, 0, 0]
alignment = 0
mob_type = "simple"              # "simple" or "enhanced"

[mob.simple]
level = 1
hit_dice = 1
mana_dice = 1
stamina_dice = 1
pos = 0
default_pos = 0
sex = 0

[mob.enhanced]
pos = 0
default_pos = 0
sex = 0
abilities = { str = 11, dex = 11, con = 11, int = 11, wis = 11, cha = 11 }
class = 0
species = 0
age = 0
saving_throws = { str = 0, dex = 0, con = 0, int = 0, wis = 0, cha = 0 }
skills = [{ id = 1, level = 50 }]
attack_type = 0

[[mob.loadout]]
wear_pos = 0
vnum = 1234
quantity = 1

[[mob.skin_yield]]
obj_vnum = 1234
dc = 10

mob.triggers = [3000, 3001]
```

## Objects

Path: `lib/world/obj/<zone>.toml`

```toml
[[object]]
vnum = 3000
name = "obj name"
short = "short"
description = "desc"
main_description = "long"
type = 0
extra_flags = [0, 0, 0, 0]
wear_flags = [0, 0, 0, 0]
affect_flags = [0, 0, 0, 0]
values = [0, 0, 0, 0]
weight = 0
cost = 0
level = 0
timer = 0

[[object.extra_desc]]
keyword = "inscription"
description = "text"

[[object.affect]]
location = 0
modifier = 0

object.triggers = [3000, 3001]
```

## Zones

Path: `lib/world/zon/<zone>.toml`

```toml
[[zone]]
vnum = 1
builders = "None."
name = "Zone name"
bot = 100
top = 199
lifespan = 30
reset_mode = 2
flags = [0, 0, 0, 0]
min_level = -1
max_level = -1

[[zone.command]]
command = "M"
if_flag = 0
arg1 = 0
arg2 = 0
arg3 = 0
sarg1 = ""                       # for V command
sarg2 = ""                       # for V command
line = 0
```

## Shops

Path: `lib/world/shp/<zone>.toml`

```toml
[[shop]]
vnum = 4000
products = [3000, 3001]
buy_profit = 1.0
sell_profit = 1.0
trade_with = 0
broke_temper = 0
bitvector = 0
keeper = 2000
rooms = [1000]
open1 = 0
close1 = 28
open2 = 0
close2 = 0
bank = 0
sort = 0

[[shop.buy_type]]
type = 0
keyword = ""                     # optional

[shop.messages]
no_such_item1 = ""
no_such_item2 = ""
do_not_buy = ""
missing_cash1 = ""
missing_cash2 = ""
message_buy = ""
message_sell = ""
```

## Triggers

Path: `lib/world/trg/<zone>.toml`

```toml
[[trigger]]
vnum = 5000
name = "trigger name"
attach_type = 0
flags = 0
narg = 0
arglist = ""
commands = ["say hello", "wait 2", "say bye"]
```

## Quests

Path: `lib/world/qst/<zone>.toml`

```toml
[[quest]]
vnum = 6000
name = "Quest name"
description = "desc"
info = "info"
done = "done"
quit = "quit"
type = 0
quest_master = 2000
flags = 0
target = 0
prev_quest = 0
next_quest = 0
prereq = 0
values = [0, 0, 0, 0, 0, 0, 0]
rewards = { coins = 0, exp = 0, obj_vnum = 0 }
```
