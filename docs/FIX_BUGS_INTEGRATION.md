# Native fix integration and code conventions

The audited GTA III fixes now retain their pre-fix alternatives under the existing
`FIX_BUGS` switch. The default configuration continues enabling the fixes.

The follow-up covers briefs position/spacing, fade camera pitch, radar disc tint,
the 500 ms siren tap window and Japanese text-box padding. Existing protected
SilentPatch health, glass, NPC weapon, roadblock, extra, audio and vehicle-light
fixes keep their guards. MixSets distance tuning, Chinese rendering, menu maps
and Classic Axis remain separate features.

Touched code follows the repository's formatting configuration. Native pixel
buffers use `uint8`/`uint32`, packed alpha shifts use unsigned arithmetic, and
distance constants no longer require `constexpr`. PNG bytes are unchanged.

For a Release build that exercises the original alternatives, define
`RE3_DISABLE_FIX_BUGS`. This suppresses the normal `FIX_BUGS` definition and the
two radio UI options that upstream explicitly requires to depend on it.
Required `FIX_BUGS_64` compatibility remains enabled for x64.

Validation: Windows x64 D3D9/OpenAL Release compile/link with fixes enabled and
disabled; existing controller-input regression tests; changed-code whitespace
checks with CRLF recognized as a line ending. The deliverable is the default
fixes-enabled executable. No new full in-game regression pass is claimed.
