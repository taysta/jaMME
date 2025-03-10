## jaMME-jaPRO:
* Get the latest release from the [Releases](https://github.com/taysta/jaMME/releases) page
* Put ffmpeg.exe in the same folder as jaMME.exe to enable pipe output to ffmpeg using the provided scripts.
* Put japro cgame and ui libraries in the japro folder for extended jaPRO support.
* Use `+set demoSeekPreRecord "00:00"` after `+set seek 0` (as a fallback) for race demos with pre-recording to skip to the start time, you can offset this value. (eg `"-00:01"` to seek to 1 second before the race start)
* Use the `mme_autoSeekPreRecord` cvar to set a default value that will always be applied as if the `demoSeekPreRecord` command was called at the load of a demo. (eg `"0"` = disabled, `"00:01"` to seek to 1 second after race start, `"1"` to seek to 1 minute after the race start)

Jedi Academy Movie Maker's Edition
==================================
[![build](https://github.com/taysta/jaMME/actions/workflows/build.yml/badge.svg)](https://github.com/entdark/jaMME/actions/workflows/build.yml)

Jedi Academy Movie Maker's Edition (jaMME) is an engine modification of Jedi Academy for moviemaking. It's a port of q3mme with most of its features and some new ones. The modification is based on very early (May 2013th) version of OpenJK. Original source code belongs to Raven Software.

# Features #
* demo playback control (pause, rewind)
* free camera mode
* chase camera mode
* time speed animation
* capturing motion blur
* capturing output in stereo 3D
* capturing output for Virtual Reality
* different output types: mp4, avi, png, jpg, tga
* playing music on background to synchronize it with editing
* saving depth of field mask
* overriding players information: name, saber colours, hilts, team, model
* realistic first person view with visible body (trueview)
* recording audio to wav
* replacing world textures with your own
* replacing skybox with one solid colour (chroma key)
* capturing in any resolution
* off-screen capturing
* capturing a list of demos
* supporting mods: base (basejka, base_enhanced), ja+ (ja++), lugormod, makermod, PRMod

# Contributors #
* ent
* Scooper
* redsaurus
* teh
* Alpha/Avygeil
* tayst

# Installation #
Extract the archive to "GameData" folder.

# Dependencies #
* zlib for pk3 support
* libmad for mp3 support
* libogg for ogg vorbis support
* libvorbis for ogg vorbis support
* libflac for flac support

# Credits #
* q3mme crew and their q3mme mod
* Raz0r and his JA++ code for JA+ and JA++ compatibiliy, also he helped a lot
* teh and his pugmod that was a good starting point, also some jaMME features are taken from pugmod; and his jamme port for linux
* Sil and his features from SMod
* CaNaBiS and his help in explaining of how q3mme works
* Razorace and his trueview feature (taken from JA++)
* Grant R. Griffin and his "FIR filter algorithms for use in C"
* OpenJK contributors and their OpenJK project
* redsaurus for Mac support
* Beloko Games and some of their codes for Android support
* tayst and his CI+CD
