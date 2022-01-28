# Config File Syntax

Config is loaded from `config.cfg` file in current working directory.

Config file is a plain text format which looks like this:

```
# comment
color [channel=0][black=2n+1] 255 0 0
color [channel=0] 33 100 99
max_events_per_track 4096
particle_count 10
background_color 7 15 14
display_font "/usr/share/fonts/quicksand/Quicksand-Regular.ttf"
label_font_size 60
label_fade_time 40
```

## Options

This list may not be up to date. The up-to-date list can be found by running `midi --config-help`.

### `background_color <color(rgb)>`
Background color

### `background_image <path(string)>`
Path to background image

### `color <selectors(Selector)...> <color(rgb[a])`
Key tile color

### `default_color <color(rgb[a])`
Default key tile color

### `display_font <path(string)>`
Font used for displaying e.g. labels

### `label_fade_time <time(int)>`
Label fade time (in frames)

### `label_font_size <size(int)>`
Font size for labels (in pt)

### `max_events_per_track <count(int)>`
Maximum event count that are stored in track. Applicable only for realtime mode. Doesn't affect events that are saved to MIDI file; is used just for performance. Too small number may break display when there is a long key press along with many short key presses. It's recommended to not touch this value unless you know what you are doing.

### `overlay_color <color(rgb[a])`
Overlay (fade out) color

### `particle_count <count(int)>`
Particle count (per tick)

### `particle_glow_size <radius(float)>`
Particle glow size (in keys)

### `particle_radius <radius(float)>`
Particle radius (in keys)

### `play_scale <value(float)>`
Y scale (tile falling speed) for play mode

### `real_time_scale <value(float)>`
Y scale (tile falling speed) for realtime mode

## Argument Types

### `int`
An integer number in range -2^31 - 2^31-1. This range may be limited by specific options.

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
* `black_key` - Index of black key, A♯0 key is `0`
