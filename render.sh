build/midi -o play "$1" | ffmpeg -f rawvideo -pix_fmt rgba -s:v 1920x1080 -r 60  -i pipe: -c:v hevc_nvenc output.mp4
