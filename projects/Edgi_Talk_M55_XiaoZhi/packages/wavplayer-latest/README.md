# wavplayer

[中文页](README_ZH.md) | English

## 1. Introduction

**wavplayer** is a simple wav format music player that provides functions for playing and recording wav files, supporting functions such as play, stop, pause, resume, and volume adjustment.

### 1.1. File structure

| Folder | Description |
| ---- | ---- |
| src | Core source code, which mainly implements wav playback and recording, and export Finsh command line |
| inc | Header file directory |

### 1.2 License

The wavplayer package complies with the Apache 2.0 license, see the `LICENSE` file for details.

### 1.3 Dependency

- RT-Thread 4.0+
- RT-Thread Audio driver framework
- optparse command line parameter parsing package

### 1.4 Configuration Macro Description

```shell
 --- WavPlayer: Minimal music player for wav file play and record.
 [*] Enable support for play
 (sound0) The play device name
 [*] Enable support for record
 (sound0) The record device name
       Version (v1.0.0) --->
```

**Enable support for play**: enable wav play function
**The play device name**: Specify the sound card device used for playback, default `sound0`
**Enable support for record**: enable wav recording function
**The record device name**: Specify the sound card device used for recording, the default is the same as the playback, use `sound0`.

## 2. Use

Common functions of wavplayer have been exported to Finsh command line for developers to test and use. Commands are mainly divided into two categories: playback and recording, which provide different functions.

**The functions provided by the play command are as follows**

```shell
msh />wavplay -help
usage: wavplay [option] [target] ...

usage options:
  -h, --help Print defined help message.
  -s URI, --start=URI Play wav music with URI(local files).
  -t, --stop Stop playing music.
  -p, --pause Pause the music.
  -r, --resume Resume the music.
  -v lvl, --volume=lvl Change the volume(0~99).
  -d, --dump Dump play relevant information.
```

**The functions provided by the recording command are as follows**

```shell
msh />wavrecord -h
usage: wavrecord [option] [target] ...

usage options:
  -h, --help Print defined help message.
  -s file --start=file <samplerate> <channels> <samplebits>
                                        record wav music to filesystem.
  -t, --stop Stop record.
```

### 2.1 Play function

- Start playing

```shell
msh />
msh />wavplay -s song_44.wav
Information:
sampletate 44100
channels 2
sample bits width 16
[I/WAV_PLAYER] play start, uri=song_44.wav
```

- Stop play

```shell
msh />wavplay -t
[I/WAV_PLAYER] play end
```

- Pause playback

```shell
msh />
msh />wavplay -p
msh />
```

- Resume playback

```shell
msh />
msh />wavplay -r
msh />
```

- Set volume

```shell
msh />
msh />wavplay -v 88
msh />
```

### 2.2 Recording function

- start recording

```shell
msh />wavrecord -s test.wav
Information:
sampletate 8000
channels 2
```

- Stop recording

```shell
msh />
msh />wavrecord -t
msh />
```

## 3. Matters needing attention

- Only supports audio with 16bit sampling bits

## 4. Contact

- Maintenance: Zero-Free
- Homepage: https://github.com/RT-Thread-packages/wavplayer
