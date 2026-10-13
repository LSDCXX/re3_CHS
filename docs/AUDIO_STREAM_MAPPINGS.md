# Audio Samples filename mapping (OpenAL)

The OpenAL audio backend reads audio/audio-samples.ini at initialization.
Its [Samples] section is an ordered list of bare relative filenames, compatible
with CookiePLMonster's Audio Samples configuration. No ASI is required.

Stream indices retain their original meaning. Missing, unsafe or overlong paths
leave that stream's compiled-in filename unchanged, without shifting later
entries. Removing the configuration restores the defaults on the next audio
initialization. Mapped files take priority over optional PS2 stream paths.
With any active mappings, the unversioned audio/sound.cache is neither read nor
written, so old MP3/Opus durations cannot be reused for replacement WAV streams.

This supports the current x64 OpenAL build and is independent of FIX_BUGS.
It does not add new decoders or change SFX banks. The supplied Xbox Audio III
archive contains 16-bit PCM WAV streams supported by the existing decoder.

Installation: back up the game's audio folder, then extract the archive's audio
folder into the game directory. Keep audio/audio-samples.ini alongside the WAV
files. Do not install audio_samples.asi. Radio and SFX packages are separate.
This source change does not automatically install or redistribute the assets.

Tests compile the actual parser, covering ordered entries, CRCRLF, UTF-8 BOM,
comments, section boundaries, invalid/missing filenames and reinitialization.
The user's original archive config is additionally checked when available.
