# Layer Stack inline-card proof

These 1280 × 900 PNGs are rasters of the real editor workspace and the real `TexturePaintPanel` / `SceneDirectoryPanel`. The proof harness records through `RecordingSurface` and CPU-rasterizes the resulting ImGui draw list; these are not mock-ups.

- [`editor-layerstack-card.png`](editor-layerstack-card.png) — the **Levels** entry's trailing V opened its card beneath the row. The card is intentionally limited to Info, Height → Normal, Effects, Colour Blending, and Channel Blending.
- [`editor-overview.png`](editor-overview.png) — the Directory destination with its footer spanning the complete Directory page, including beneath the details side.

The layer-card scenario drives the actual interactions and asserts both paths remain independent:

1. Press the trailing disclosure V and assert the inline card opens while `StackPage == 0`.
2. Close it, double-contact the row body, and assert the carousel travels to the properties page.
3. Press Tab to return, reopen the V-card, and capture the raster.

```text
[assert] V-card and double-contact carousel remain independent
```

SHA-256:

```text
c44fe42d49819c469297824306fbe3bcd8cd0a67492ce537309c7b948766edd5  editor-layerstack-card.png
7eb404fceb383f7d329c36fbe27d14751fc61193f67b213dc2db138dccb5f9ce  editor-overview.png
```
