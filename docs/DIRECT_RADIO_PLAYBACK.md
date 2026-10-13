# Direct radio playback (OpenAL)

Radio stations and the user-music stream now play through one nonspatialized
stereo source. Decoded planar PCM is interleaved without changing samples,
sample rate, pitch or channel order. Mono music is duplicated into both stereo
channels. The second source is not queued or started. Radio panning no longer
places two mono sources around the listener; optional OpenAL spatialization is
disabled and direct channel routing enabled when supported.
When per-source resampler selection is available, radio uses the highest listed
non-fast Sinc option; ordinary streams restore the device's default resampler.

The paired buffer scheduler retains its bookkeeping, but services and unqueues
only the active source for radio. Position offsets account for stereo frames.
Setup restores ordinary source properties for nonradio streams, including after
switching streams or reopening the audio provider. Ambience, police audio,
dialogue, cutscenes and SFX retain their existing playback paths.

This is independent of FIX_BUGS. Volume, fades and dialogue ducking are retained.
OpenAL/device sample-rate conversion and the system output mixer still apply;
there is no promise of identical perceived loudness or player-specific EQ,
normalization, enhancements or dynamics processing. No global device effects
are disabled for game sound effects.

Automated tests compile the production upload/queue/play functions and verify
exact stereo sample order, unchanged source rate, mono duplication, single-source
radio playback, and unchanged paired-source dialogue. In-game listening and
real-device pause/seek/loop/provider-reset behaviour still need confirmation.
