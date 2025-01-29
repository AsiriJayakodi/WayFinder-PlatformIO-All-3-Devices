# WayFinder-PlatformIO-All-3-Devices

# Build and Upload for Each Board

To compile and upload for a specific board, use:
```sh
pio run -e user
pio run -e inter
pio run -e base
```
For uploading:
```sh
pio run -e user -t upload
pio run -e inter -t upload
pio run -e base -t upload
```

This setup keeps everything in one project while managing different firmware for each board. Let me know if you need refinements! 🚀