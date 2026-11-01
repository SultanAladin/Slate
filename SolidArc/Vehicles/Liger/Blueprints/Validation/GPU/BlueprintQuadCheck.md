# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1800 x 1010 · 14 frames · 4.66377 s · slowest frame 2991.11 ms
- Script: /home/user/Slate/SolidArc/Vehicles/Liger/Tools/BlueprintQuadCheck.txt
- Expectations: 6 / 6 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 2 | `steps == 0` | 0 | 🟢 |
| 3 | `refused == 0` | 0 | 🟢 |
| 4 | `image-planes == 6` | 6 | 🟢 |
| 5 | `image-planes-drawn == 3` | 3 | 🟢 |
| 6 | `split-panes == 4` | 4 | 🟢 |
| 10 | `camera-distance > 1` | 8.690360944 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| Liger_Quad | 7 | `33d0df8837417cd0` |
| Liger_Quad_Framed | 14 | `c1d5e23118426efc` |

## Journal

