# White Tea Service topology source

This archive holds the deterministic, editable OBJ topology sources for the default Slate proof scene.
`GenerateWhiteTeaService.py` is the source of the authored teacup, saucer, sugar bowl, milk jug, and the service teapot mesh. It emits named OBJ objects with source quadrilateral faces and per-corner UVs; the
future Codex loader must hand these face runs directly to `DecodedTopology` and let the established authoritative
Earcut path produce render triangles.

`WhiteDielectric.mtl` is only an interchange-side reference. The actual shared, revisioned material document will
be `WhiteDielectric.pigment`; the viewport's first pass remains fixed-white dielectric radiance.

`ServiceTeapot.obj` is the approved tea-service teapot replacement; this proof deliberately does not use or claim
to contain the Utah Teapot.
