# EngineContent

The asset seat every Slate host reads at run time. `[link].carry` places this folder beside the
executable, so a host resolves its content from its own binary seat rather than from the repository —
the same arrangement `SlateAppearance.toml` already uses.

| Archive           | Holds                                                                    |
|-------------------|--------------------------------------------------------------------------|
| `FontArchives`    | Typefaces — `.otf`, `.ttf`, `.woff2`.                                      |
| `GraphicArchives` | Vector and raster graphics — `.svg` first, alongside `.png`, `.jpg`, `.webp`, `.tga`, `.exr`. |
| `MaterialArchives`| Material declarations and their bound texture sets — `.mat`.               |

Each archive keeps a `.gitkeep` so the arrangement survives a clone with no content in it.
