# Finite grid fade proof

`editor-grid-fade.png` is recorded by `Tools/SceneDirectoryProof` from the real viewport `EditorPanel` footer and its real shared Grid settings controls.

The two new authored metre values are visible in the footer popup:

- **World extent** — finite radial boundary around world centre.
- **Camera fade** — camera-relative radius over which grid and axis coverage soften to zero.

The GPU implementation remains analytic in `WorkspaceOverlayFragment.slang`; it does not generate a large CPU line set. The two fades multiply, so a fragment must be inside both the finite world extent and the camera visibility radius.
