# yuvplayer (xygnal fork)

> This repository is based on the previous windows repository https://github.com/Tee0125/yuvplayer. Thanks to the original author [Tee0125](https://github.com/Tee0125).<br>
> The new functions is implemented such as **.y4m support** and improved **YUV size detection**. 

## Fork Additions
- **Y4M** file support
- **YUV size detection** logic is improved
- **speed of play** is accelerated

## Build with MSBuild Tools (freeware ??)
1. Download `vs_BuildTools.exe` from https://visualstudio.microsoft.com/downloads/ (bottom of page)
2. Install with **Desktop development with C++** → select:
   - MSVC build tools for x64/x86 (latest)
   - C++ ATL for x64/x86 (latest)
   - C++ MFC for x64/x86 (latest)
3. Open **Developer Command Prompt for VS** from Start menu
4. Build:
   ```bat
   msbuild yuvplayer.vcxproj -p:Configuration=Debug
   :: or Release
   msbuild yuvplayer.vcxproj -p:Configuration=Release
   ```
5. Run `Debug\yuvplayer.exe` or `Release\yuvplayer.exe` and open/drag a `yuv`/`y4m`/`raw` file.

---

# yuvplayer (upstream)

Lightweight YUV player supporting various YUV formats.

### Supporting Format
- **Planar:** YUV420 (YV12), YUV422 (YV16), YUV444, RGB16, RGB24, RGB32
- **Interleaved:** NV12, NV21, UYVY, VYUY

### Features
- Zoom (4:1 ~ 1:4)
- Save frame as YUV/BMP

### Hotkeys
| Key | Action |
|-----|--------|
| → | Frame forward |
| ← | Frame backward |
| ↑ | Zoom in |
| ↓ | Zoom out |
| p / Space | Play/Pause |
| o | Open file |
| g | Go to frame |
| x | Exit |
| 2 | 2160p (UHD) |
| 1 | 1080p (FHD) |
| 7 | 720p (HD) |
| 5 | 540p |
| 3 | 360p |
