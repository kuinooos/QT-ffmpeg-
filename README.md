# FFmpeg Player GUI (Qt6 QML)

This folder contains a standalone Qt6 QML frontend implementation.
The backend is intentionally not implemented here.
You can keep using your FFmpeg pipeline and connect it through the bridge contract.

## Features in this GUI layer

- Apple-inspired modern desktop style
- Glass-like control panel with rounded corners
- Video area placeholder reserved for backend video surface
- Full control strip:
  - Open
  - Play or Pause
  - Stop
  - Previous or Next
  - Rewind 10s or Fast forward 10s
  - Seek timeline
  - Mute and volume
  - Playback mode cycling
  - Playback rate cycling
  - Fullscreen toggle
- Keyboard shortcuts:
  - Space: play or pause
  - Left or Right: rewind or fast forward
  - Up or Down: volume up or down
  - F: fullscreen toggle

## Build (Qt Creator + .pro)

Requirements:
- Qt 6.5.3+ (MinGW 64-bit)
- MinGW 11.2.0 64-bit compiler

Steps:
1. Open `ffmpeg_player_gui.pro` in Qt Creator.
2. Select kit: Desktop Qt 6.5.3 MinGW 64-bit.
3. Run qmake.
4. Build and Run.

Important:
- If you use a 64-bit compiler, FFmpeg libraries must be 64-bit (`../ffmpeg-x64`).
- The project will stop at qmake stage if it detects 64-bit compiler with `ffmpeg-x86`.

## Bridge contract

The frontend bridge class is in src/playerbridge.h.

Command signals for backend integration:
- commandOpenMedia(fileOrUrl)
- commandPlay()
- commandPause()
- commandStop()
- commandSeekTo(positionMs)
- commandSeekBy(deltaMs)
- commandPlayNext()
- commandPlayPrevious()
- commandSetVolume(volume)
- commandSetMuted(muted)
- commandSetPlaybackMode(mode)
- commandToggleFullscreen()

State and event outputs for GUI:
- playbackState
- positionMs
- durationMs
- bufferedRatio
- volume
- muted
- mediaTitle
- mediaArtist
- mediaPath
- hasMedia
- playbackMode
- playbackRate
- eventError(errorCode, message)
- eventInfo(message)

## Backend integration tip

Replace mock progress behavior in src/playerbridge.cpp with real backend callbacks:
- connect backend progress to setPositionMs()
- connect backend duration to setDurationMs()
- connect backend state to setPlaybackState()
- connect backend errors to eventError()

No FFmpeg decoding code is required or included in this GUI module.
