# Config File Syntax

Config is loaded from `config.cfg` file in current working directory.

Config file is a plain text format which looks like this:

```ini
background_image "test.jpg"
display_font "res/dejavu-sans-mono.ttf"

# comment
on (time=5s) set {
    on (time=5s) set(transition=0.1s) {
        background_color 255 255 255
    }
}

on (time=10s) set(transition=0.1s) {
    color [channel=0] 255 0 0
}

on (time=15s) set(transition=0.1s) {
    color [channel=0][white_key=2n+1] 0 0 255 # other comment
}

on (time=0.5s) set {
    every (2s) set(transition=0.1s) {
        background_color 0 20 0
        on (time=1s) set(transition=0.2s) {
            background_color 20 0 0
        }
    }
}

every(5s) add_event Text(text="foo")
```

## Properties

This list may not be up to date. The up-to-date list can be found by running `midi --config-help`.

### `animate_color <selectors: SelectorList> <color: Color<RGBA>>`
Apply transition of tile color. Applies only to tiles matching `selector`. Some attributes (marked as non-animatable attributes) don't work with this property; use `color` instead. Is always overridden by `color`.

### `background_color <color: Color<RGB>>`
Background color. The plain-color background is always covered by background image if it is specified.

### `background_image <path: string>`
Path to background image.

### `color <selectors: SelectorList> <color: Color<RGBA>>`
Tile color, applied only to tiles matching `selector`. Is not affected by transitions. This overrides `animate_color`.

### `default_color <color: Color<RGBA>>`
Default key tile color. Used when no selector specified in `color` matches a tile.

### `display_font <path: string>`
Font used for displaying e.g. labels.

### `label_fade_time <time: int(range 1-1000)>`
Label fade time (in frames).

### `label_font_size <time: int(range 1-1000)>`
Font size for labels (in pt).

### `max_events_per_track <count: int>`
Maximum event count that are stored in track. Applicable only for realtime mode. Doesn't affect events that are saved to MIDI file; is used just for performance. Too small number may break display when there is a long key press along with many short key presses. It's recommended not to touch this value unless you know what you are doing.

### `overlay_color <color: Color<RGBA>>`
Overlay (fade out) color.

### `particle_count <count: int>`
Count of particles that are spawned every tick.

### `particle_glow_size <radius: float>`
Particle glow size (in keys).

### `particle_gravity <value: float>`
How fast particles fall

### `particle_max_wind <value: float>`
Max force that can be applied to particle due to wind

### `particle_radius <radius: float>`
Particle radius (in keys).

### `particle_temperature_decay <value: float>`
How much temperature is left every tick

### `particle_temperature_multiplier <value: float>`
How fast particles go up due to temperature

### `particle_x_drag <value: float>`
How much particles are slowed down in X axis

### `scale <value: float>`
Y scale (tile falling speed).

## Argument Types

### `int`
An integer number in range -2^31 - 2^31-1. This range may be limited by specific properties.

Examples: `1235`, `-54`

### `float`
A float (real) number.

Examples: `54.3`, `-43.2`

### `string`
A text. It must be quoted if contains spaces.

Examples: `test`, `"some text with spaces"`, `'some text with spaces'`.

Escaping is not supported for now.

### `rgb`
A RGB color (without opacity). Stored as integers in range 0 - 255.

Examples: `255 0 0` (red), `0 0 255` (blue)

### `rgb[a]`
A RGBA color (with opacity). The opacity (alpha) argument is optional. All arguments are integers in range 0 - 255.

Examples: `0 0 0 128` (half-transparent black), `255 0 0` (opaque red)

### `Selector...`
A list of selectors. They are described in detail below.

### `time`
A time value with unit. Unit can be `s` (seconds), `t` (MIDI ticks) or `f` (render frames).

Some examples:
```ini
1s      # 1 second
1t      # 1 MIDI tick
1.5s    # 1.5 second
2f      # 2 frames
```

Examples: `[channel=0][black=2n+1]`

## Selectors
Selectors can be used to select (as the name suggest) events / key tiles that should be affected by a specific property. The only command that supports selectors is `color` at the moment.

Selector is written like CSS attribute selector, that is:

```css
[attribute=value]
```

Value may be:
* an integer
* a float
* an inclusive range (e.g `12-24` - all values bigger or equal `12` and less or equal `24`)
* a linear equation (e.g. `2n+1` - select every second value, starting from 1)

Available selector attributes:
* `channel` - A MIDI channel that event is stored on
* `note` - Note value, as stored in MIDI
* `white_key` - Index of white key (A0 key is `0`, B0 is `1` etc.). `-1` if key is black.
* `black_key` - Index of black key, (A♯0 key is `0`, C♯1 is `1` etc.). `-1` if key is white.
* `time` (non-animatable) - A MIDI tick the event is played.

## `on` Statement
You can execute an action only when a condition is met, e.g. with some delay. This is possible with `on` statement, which looks like this:

```
on(condition=value) action
```

The `on` statement is checked on every tick.

See detailed description of [Actions](#actions) and [conditions](#conditions).

### Examples

Set `background_color` after 1 second.
```ini
on(time=1s) set {
    background_color 255 255 255
}
```

## Conditions

### `time`=time

True if the current time is equal to `time`. More specifically, the current tick must contain time point specified by `time`.

### `startup`
True only at the first tick (run only once at startup).

Example:
```ini
on(startup) set {
    statements...
}
```

is functionally equivalent to just
```ini
statements...
```

### `mode`=mode
True if `mode` is equal to mode specified in command line and only at the first tick. Valid values are `realtime` or `play`.

## Actions
Many statements takes actions as arguments, for example, the `on` statement.

### `set`
Executes a set of statements. Can be used to set properties or nest other statements. It can also be used to specify transition for properties.

Syntax:
```
set (property1=value1,property2=value2,...) {
    <statements...>
}
```

Properties may be omitted (along with parentheses).

Available properties:

* `transition`: Specifies transition time.
* `function`: Specifies transition timing function (`constant0`, `constant1`, `linear` or `ease-in-out-quad`).

#### Examples

Just set `background_color`:
```ini
[...] set {
    background_color 255 255 255
}
```

Set `background_color` with 1 second transition and ease-in-out-quad timing function:
```ini
[...] set(transition=1s,function="ease-in-out-quad") {
    background_color 255 255 255
}
```

Set a next `on` statement to be executed 2 seconds after the `set` statement:
```ini
[...] set {
    on(time=1s) set {
        background_color 255 255 255
    }
}
```

The timings are stacked, so that you can do several actions with a constant delay:

```ini
on(time=10s) set {
    on(time=1s) set(transition=1s) {
        background_color 255 0 0
    }
    on(time=2s) set(transition=1s) {
        background_color 0 255 0
    }
    on(time=3s) set(transition=1s) {
        background_color 0 0 255
    }
}
```

These all `on` statements will run after 10 seconds, so the first will run at 11s, second at 12s, and third at 13s.

### `add_event`
Adds a MIDI event to track 0 of the output. The event is also rendered as a tile.

Syntax:
```ini
add_event <Type>(property1=value1,property2=value2,...)
```

Types:
* `Text` - Display a label (and also add TextEvent to MIDI)
    * `text`: A text to be displayed
    * `type`: Only `track_name` is supported and is the default.

#### Examples

Add a `foo` label as track name:
```ini
add_event Text(text="foo")
```

## `if` statement
The `if` statement works (and looks) similarly to `on` statement, except it is checked only once, when statement is run.

This won't work as expected if the condition is `time` because of that

### Examples

Do something only if mode is realtime:
```ini
if (mode=realtime) set {
    # These statements are executed only once, even though
    # the mode is "realtime" all the time.
    scale 1.0
    background_image "test2.png"
}
```

Invalid use of `if` with `time` condition:
```ini
if (time=5s) set {
    # This won't ever run because it is checked only once at startup.
    background_image "test2.png"
}
```

## `every` statement
The `every` statement can be used to execute action in intervals.

Syntax:
```ini
every(<time>) <action>
```

### Examples

Add a label every 5 seconds:
```ini
every(5s) add_event Text(text="foo")
```

Set `background_color` to `rgb(0, 20, 0)` every 2 seconds. Also, change it to `rgb(20, 0, 0)` after 1s in every iteration:
```ini
every(2s) set(transition=0.1s) {
    background_color 0 20 0
    on(time=1s) set(transition=0.2s) {
        background_color 20 0 0
    }
}
```
