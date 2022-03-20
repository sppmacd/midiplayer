# Markers

Markers may be used to indicate various events (like track start/end, important parts of a song etc.). They can be manually added using number keys. To enable markers, use `--markers [file]` command line option (they will be saved in `file`).

## Marker file
Marker file consists of lines, one per marker. The format is:

```
[marker_number]: [marker_time_in_ticks] 
```

Example:
```
1: 4422
2: 15961
3: 19417
```
