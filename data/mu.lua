-- mu.lua — mustat block config.
-- Put this at ~/.config/mustat/mu.lua (or $XDG_CONFIG_HOME/mustat/mu.lua)
--
-- Each module{} call registers one block. `name` is its identity —
-- mustat uses it to tell blocks apart, so it must be unique across
-- the WHOLE file, not just within one `pos`. Multiple modules can
-- share the same pos freely, they just need different names (this is
-- what broke when two modules both used slot=1 in the old format —
-- slot is gone now, name is the only key).
--
-- `type` picks a fast built-in (clock/cpu/mem, computed in C) —
-- anything else runs `cmd` as a shell command every `interval`
-- seconds, so $(...) substitutions inside cmd are re-evaluated live,
-- e.g. the upower/battery example below.

module {
  name     = "clock",
  type     = "clock",
  pos      = "center",
  interval = 1,
}

module {
  name     = "date",
  type     = "script",
  cmd      = "date '+%Y-%m-%d'",
  pos      = "center",
  interval = 60,
}

module {
  name     = "battery",
  type     = "script",
  cmd      = [[upower -i "$(upower -e | grep BAT)" | awk '/state:/ {s=$2} /percentage:/ {print (s=="charging"?"C":s=="fully-charged"?"F":"B"),$2}']],
  pos      = "right",
  interval = 10,
}

module {
  name     = "volume",
  type     = "script",
  cmd      = "pactl get-sink-mute @DEFAULT_SINK@ | grep -q yes && echo ' muted' || pactl get-sink-volume @DEFAULT_SINK@ | awk '{print \" \"$5}'",
  pos      = "right",
  interval = 1,
}

module {
  name     = "cpu",
  type     = "cpu",
  pos      = "right",
  interval = 3,
}

module {
  name     = "mem",
  type     = "mem",
  pos      = "right",
  interval = 5,
}
