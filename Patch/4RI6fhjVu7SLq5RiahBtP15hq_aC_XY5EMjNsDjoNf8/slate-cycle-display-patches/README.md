# Slate cycle-slot and display-count patch series

Base commit:

```text
b144533ddec7a7148b1bcf246bfc7e10900b193d
```

## Commit 1

```text
9917b441fa23ae9294282bf2fa89671ba0579f5e
Replace recording rotation vocabulary with cycle slots
```

Principal public changes:

```text
RecordingRotationDepth -> RecordingSlotCount
RotationSlot           -> CycleSlot
RotationOrdinal        -> RecordingOrdinal or SlotOrdinal, according to mechanism
PerRotation            -> PerSlot
RecordedRotation       -> RecordedSlot
```

Angular and quaternion uses of `Rotation` remain unchanged.

## Commit 2

```text
43f52976b2cd6e1ec80152d6d0854093ed2ee1ca
Separate display image counts from recording slots
```

Dear ImGui initialization now receives:

```text
MinImageCount <- DisplayScheduler::MinimumChainImageCount()
ImageCount    <- DisplayScheduler::ChainImageCount()
```

It no longer receives `RecordingSlotCount`. Chain reconstruction passes the reconstructed display counts through `ViewportSequence::Renegotiate`.

## Apply

From a clean branch containing `b144533`:

```bash
git switch harden/interface-record-consumption
git switch -c harden/cycle-display-count
git am 0001-Replace-recording-rotation-vocabulary-with-cycle-slo.patch
git am 0002-Separate-display-image-counts-from-recording-slots.patch
```

## Generated files

The portable commits deliberately exclude:

```text
*.symbolindex
SymboLindex/**
Engine/Engine-File-Structure.md
```

Your Release construction should regenerate those files. Add the resulting generated amendments to a separate commit after the two source commits, or amend them locally according to your normal process.

## Verification already completed

- Both patches apply cleanly to `b144533` with `git am`.
- Applying both patches reproduced the prepared Git tree exactly.
- No former scheduling identifiers remain in authoritative text.
- No interface path passes `RecordingSlotCount` as a display image count.
- Every new display count has a matching declaration, definition and call site.
- No whitespace errors or unintended byte-order-mark changes were found.

A complete MSVC/Vulkan build was not available in the Arena workspace. Run Slate's Release construction and exercise EditorHost under Vulkan validation, especially resize, minimize, restore, out-of-date and suboptimal chain paths.

## Patch hashes

```text
a0822de7bd7758d1b26a198e836f9fdd4bc0c2b9982fe17e78b9d7a0bfbf1c27  0001-Replace-recording-rotation-vocabulary-with-cycle-slo.patch
6e8511b47496612861e28f61c328e04ad59163e33ad35df02e9a5937d16fdb3d  0002-Separate-display-image-counts-from-recording-slots.patch
```
