# Help-message background bounds

The upper-left HUD help box uses `CFont::PrintHelpString` under `FIX_BUGS`.
The disabled branch still calls the original `PrintString`.

The measuring pass runs the existing layout and glyph renderer with background
drawing disabled. It collects the nontransparent part of each actual font quad,
including native Latin fonts, Chinese atlas glyphs, controller-button icons,
wrapped lines and token formatting. The paragraph's complete ink bounds receive
the same four design-unit horizontal padding on both sides. Spaces and transparent
atlas bearings no longer make the right margin different from the left.

No alternate text layout, font size or menu/subtitle behavior is introduced.
The existing renderer still determines line breaks and glyph positions.
Measurement does not submit glyph geometry or draw controller icons.

The latest message bounds are cached by text and layout settings. Colour, fade
alpha and an unslanted pager's vertical movement do not trigger readbacks.
Each atlas is read at most once per measurement; temporary CPU atlas images are
freed immediately afterward. Font initialization, reload and shutdown invalidate
the cache. An unavailable atlas readback falls back to its quad bounds.

Validation: fixes-enabled and fixes-disabled x64 Release builds, plus
`tests/test_help_boxes.py`, which compiles
the production collector and help entry point against synthetic asymmetric
alpha atlases. Cases cover mixed text/tokens, multiline union, transparent
glyphs, fractional UV crop, equal horizontal padding, fade/pager caching,
font reload, scale changes and readback cleanup. These automated checks are
not a substitute for checking the final appearance in the game.
