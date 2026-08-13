//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Where the viewer is and how a document position becomes a display position — one answer, read by twelve documents.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/CameraProjection/Api
%layer      SlateDocument
%sources    1
%symbols    39
%annotated  29/39
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S CameraProjection.h | 359 lines | 5204e08c | 39 sym | Where the viewer is and how a document position becomes a display position — one answer, read by twelve documents.

//------------------------------------------------------------------------------------------------------------------------
//                                                    TWO PROJECTIONS
//------------------------------------------------------------------------------------------------------------------------

E ProjectionSubject                      | CameraProjection.h | 29-34   | contract                      | -  | Which of `46` §3's two projections a camera declares. and changes what occludes what, zoom amends the extent parameter below and does not. An application that binds one control to whichever is convenient produces an artist who cannot say why their composition changed.
    has   Perspective      ProjectionSubject  [-]  ?
    has   Parallel         ProjectionSubject  [-]  ?
    has   ProjectionCount  ProjectionSubject  [-]  ?
    by    Source/CameraProjection.cpp, Source/OcclusionProjection.cpp, Source/PointerIntersection.cpp, Source/SpatialManipulator.cpp
    note  ⚠️ Dolly and zoom are different edits and the enumeration keeps them apart: dolly amends the placement

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE CLIPPING INTERVAL
//------------------------------------------------------------------------------------------------------------------------

T ClippingInterval                       | CameraProjection.h | 47-58   | nonallocating,nonthrowing     | -  | The interval a camera resolves depth across. frustum with no interior, and the symptom is an empty workspace with no error anywhere — which is why `IntervalValid` is asked before the projection is derived rather than after it produces nothing. the host toolchain still defines and a member of either spelling does not survive preprocessing.
    has   Nearest   double  [-]  ?
    has   Furthest  double  [-]  ?
    note  🔴 One declaration, not two loose numbers. A nearest value that has crossed above the furthest is a
    note  ⚠️ Spelled `Nearest` and `Furthest` rather than the obvious pair, because `near` and `far` are macros

F ClippingInterval::IntervalValid        | CameraProjection.h | 54-57   | -                             | ✔️ | Whether the interval has an interior at all.
    out   -  constexpr bool  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/OcclusionProjection.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE CAMERA
//------------------------------------------------------------------------------------------------------------------------

T CameraSpecification                    | CameraProjection.h | 72-81   | nonallocating,nonthrowing     | -  | Everything one camera declares — `46` §2's table as storage. travels with the work. The display space is not, and `36` §2 rules that separately. `00` §10 conflict 33 records the two answers this used to have. derived from whatever the ray strikes moves when the artist orbits past a gap, and the object they were inspecting leaves the display.
    has   Placement         DecomposedTransform  [-]  ?
    has   Projected         ProjectionSubject    [-]  ?
    has   ExtentParameter   double               [-]  ?
    has   Clipping          ClippingInterval     [-]  ?
    has   SensorProportion  double               [-]  ?
    has   Exposure          double               [-]  ?
    has   FocusPosition     DocumentPosition     [-]  ?
    by    Api/OcclusionProjection.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/OcclusionProjection.cpp, Source/PointerIntersection.cpp, Source/SpatialManipulator.cpp
    note  🔴 Exposure is held **here**, in the document, because it is an authored decision about the image and
    note  🔴 The focus position is declared and never inferred from what the view happens to meet. An orbit centre

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE VIEW PROJECTION
//------------------------------------------------------------------------------------------------------------------------

T ViewProjection                         | CameraProjection.h | 92-98   | nonallocating,nonthrowing     | -  | Document space to view-relative space, per `02` §3.2 — the rebasing origin and the matrices around it. `Rebase` before anything narrows, which is the whole of `02` §3.2's contract. A view matrix carrying the camera's document position would narrow a billion-millimetre coordinate and lose the millimetre.
    has   ViewOrigin    DocumentPosition    [-]  ?
    has   ViewRotation  ProjectedTransform  [-]  ?
    has   Projected     ProjectedTransform  [-]  ?
    has   Composed      ProjectedTransform  [-]  ?
    by    Api/OcclusionProjection.h, Api/PartitionClassifier.h, Api/VisibilityRaster.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/OcclusionProjection.cpp, (+3 more)
    note  🔴 The translation is **not** in `ViewRotation`. It is the rebasing subtraction, performed at 64-bit by

F Derive                                 | CameraProjection.h | 110     | api,nonallocating,nonthrowing | ✔️ | Derives the view projection of one declared camera. proportion, or an extent parameter with no interior constants live in `Contract/` because `16` compares against them, `30` marches against them and `80` depth-tests against them; one document reversing its own comparison in isolation produces geometry that vanishes rather than geometry that sorts wrongly.
    in    Declaring  const CameraSpecification&  [-]  the camera
    out   -          Outcome                     [-]  refuses with ContentUnsupported for an invalid clipping interval, a non-positive sensor
    by    Api/AttachmentIndex.h, Api/ChartPartition.h, Api/IlluminantPopulation.h, Api/OcclusionProjection.h, Api/OcclusionScheduler.h, Api/QuadratureIntegrator.h, (+10 more)
    note  📐 Depth is reversed — `NearPlaneDepth` at the nearest plane, `FarPlaneDepth` at the furthest. The

F SLATE_DECLARES_PRECISION               | CameraProjection.h | 111     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/ChannelPanel.h, Api/ChartPartition.h, (+50 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE FRUSTUM
//------------------------------------------------------------------------------------------------------------------------

T FrustumPlane                           | CameraProjection.h | 119-125 | nonallocating,nonthrowing     | -  | One bounding plane, in view-relative space, with the interior on the non-negative side.
    has   NormalX   double  [-]  ?
    has   NormalY   double  [-]  ?
    has   NormalZ   double  [-]  ?
    has   Constant  double  [-]  ?
    by    Source/CameraProjection.cpp

T FrustumSpace                           | CameraProjection.h | 135-176 | owning                        | -  | The six planes `16` culls against, derived from the composed projection. inward-rounded plane culls geometry the camera can see, and the artist meets it as a surface that disappears along one edge of the display. linear form in the rebased position, and the four coefficients of that form are the plane. Deriving the planes from the eight resolved corners instead is the same answer computed less exactly and more slowly.
    has   PlaneCount      static constexpr std::uint32_t  [-]  ?
    has   Planes          FrustumPlane[PlaneCount]        [-]  ?
    has   RebasingOrigin  DocumentPosition                [-]  ?
    has   PlanesDerived   bool                            [-]  ?
    by    Api/OcclusionProjection.h, Api/PartitionClassifier.h, Source/CameraProjection.cpp, Source/PartitionClassifier.cpp, Source/PointerIntersection.cpp
    note  🔴 Planes are pushed **outward** by `FrustumOutwardMargin`, matching `38` §6 and `40` §6. An
    note  📐 Extraction is by the standard row-sum identity over the composed matrix: a clip-space inequality is a

F FrustumSpace::Construct                | CameraProjection.h | 146     | api,nonallocating,nonthrowing | ✔️ | Derives all six planes from a view projection.
    in    Projected  const ViewProjection&  [-]  as `Derive` produced it
    out   -          void                   [-]  ?
    post  every plane is unit-normalised and pushed outward
    by    Api/AnalyticProjection.h, Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CommandSequence.h, Api/CycleScheduler.h, (+62 more)

F FrustumSpace::Classify                 | CameraProjection.h | 159     | api,nonallocating,nonthrowing | ✔️ | Classifies one document-space extent against the frustum. millimetres from the document origin classifies against the camera rather than against the origin. corner furthest along the normal decides exclusion and the corner least along it decides straddling; the remaining six carry no additional fact.
    in    Least     DocumentPosition  [mm]  the extent's lower corner, in document space
    in    Greatest  DocumentPosition  [mm]  its upper corner
    out   -         Overlap           [-]   +1 wholly inside, 0 straddling a plane, −1 wholly outside
    by    Api/SampleIntegrator.h, Api/TilingSpecification.h, Api/VectorInterchange.h, Api/VendorClassifier.h, Source/AnalyticProjection.cpp, Source/CameraProjection.cpp, (+8 more)
    note  🔴 The extent is rebased at 64-bit before it is narrowed — `02` §3.2 — so an extent a billion
    note  📐 Answered by the two extremal corners along each plane's normal rather than by all eight. The

F FrustumSpace::Contains                 | CameraProjection.h | 164     | api,nonallocating,nonthrowing | ✔️ | Whether one document-space position lies inside every plane.
    in    Subject  DocumentPosition  [-]  ?
    out   -        bool              [-]  ?
    by    Source/CameraProjection.cpp

F FrustumSpace::Plane                    | CameraProjection.h | 169     | api,nonallocating,nonthrowing | ✔️ | One derived plane, for whoever presents the frustum.
    in    PlaneOrdinal  std::uint32_t        [-]  ?
    out   -             const FrustumPlane&  [-]  ?
    by    Api/PrimitiveStructure.h, Source/CameraProjection.cpp, Source/PrimitiveStructure.cpp, Source/SpatialManipulator.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       NAVIGATION
//------------------------------------------------------------------------------------------------------------------------

E NavigationSubject                      | CameraProjection.h | 184-191 | contract                      | -  | Which navigation gesture is open.
    has   Orbit            NavigationSubject  [-]  ?
    has   Pan              NavigationSubject  [-]  ?
    has   Dolly            NavigationSubject  [-]  ?
    has   Zoom             NavigationSubject  [-]  ?
    has   NavigationCount  NavigationSubject  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp

V OrbitRadiansPerPixel                   | CameraProjection.h | 197     | -                             | -  | ?
    by    Source/CameraProjection.cpp

V PanFractionPerPixel                    | CameraProjection.h | 198     | -                             | -  | ?
    by    Source/CameraProjection.cpp

V DollyFractionPerPixel                  | CameraProjection.h | 199     | -                             | -  | ?
    by    Source/CameraProjection.cpp

V ZoomFractionPerPixel                   | CameraProjection.h | 200     | -                             | -  | ?
    by    Source/CameraProjection.cpp

T NavigationSequence                     | CameraProjection.h | 209-255 | owning                        | -  | One navigation gesture, following `10` §2.4's lifecycle exactly. sample would fill `RevisionSequence` with a thousand states, and `84` would present a scrub bar that is almost entirely camera motion. presented from it, and the caller commits the sealed specification as its own transaction.
    has   PriorCamera    CameraSpecification  [-]  ?
    has   AmendedCamera  CameraSpecification  [-]  ?
    has   Declaring      NavigationSubject    [-]  ?
    has   OpenDeclared   bool                 [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp
    note  🔴 A gesture Seals **one** transaction. `10` §2.4 is not relaxed here: an orbit recorded per pointer
    note  ⚠️ Nothing is recorded between Open and Seal. The amended camera is readable so the workspace can be

F NavigationSequence::Open               | CameraProjection.h | 217     | api,nonallocating,nonthrowing | ✔️ | Opens a gesture against a standing camera, holding its prior specification.
    in    Declaring  NavigationSubject           [-]  ?
    in    Standing   const CameraSpecification&  [-]  ?
    out   -          Outcome                     [-]  refuses with HostDenied when a gesture is already open
    by    Api/CommandSequence.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/HardwareMetrics.h, Api/ImpressionSequence.h, (+20 more)

F NavigationSequence::Amend              | CameraProjection.h | 225     | api,nonallocating,nonthrowing | ✔️ | Amends the open gesture by one pointer displacement.
    in    DisplacementAlong   double   [px]  horizontal displacement since the last amendment
    in    DisplacementAcross  double   [px]  vertical displacement since the last amendment
    out   -                   Outcome  [-]   refuses with HostDenied when no gesture is open
    by    Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/IlluminantPopulation.h, Api/ImpressionSequence.h, Api/MaterialSpecification.h, (+26 more)

F NavigationSequence::Abandon            | CameraProjection.h | 231     | api,nonallocating,nonthrowing | ✔️ | Ends the gesture with no effect, returning the prior specification.
    out   -  Outcome  [-]  refuses with HostDenied when no gesture is open
    by    Api/DecalProjection.h, Api/ImpressionSequence.h, Api/OcclusionScheduler.h, Api/RevisionSequence.h, Api/SpatialManipulator.h, Api/VisibilityRaster.h, (+10 more)

F NavigationSequence::Seal               | CameraProjection.h | 237     | api,nonallocating,nonthrowing | ✔️ | Ends the gesture, returning the specification the caller commits as one transaction.
    out   -  Outcome  [-]  refuses with HostDenied when no gesture is open
    by    Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/ImpressionSequence.h, Api/InterfaceExchange.h, Api/RevisionSequence.h, (+19 more)

F NavigationSequence::Amended            | CameraProjection.h | 242     | api,nonallocating,nonthrowing | ✔️ | The camera as the gesture has amended it, for presentation while it is open.
    out   -  const CameraSpecification&  [-]  ?
    by    Api/DecalProjection.h, Api/DescriptorIndex.h, Api/ReportSequence.h, Api/SpatialManipulator.h, Api/TransmissionSequence.h, Source/BrushSpecification.cpp, (+16 more)

F NavigationSequence::GestureOpen        | CameraProjection.h | 247     | api,nonallocating,nonthrowing | ✔️ | Whether a gesture is open.
    out   -  bool  [-]  ?
    by    Api/DecalProjection.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/DecalProjection.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                        FRAMING
//------------------------------------------------------------------------------------------------------------------------

F Frame                                  | CameraProjection.h | 275     | api,nonallocating,nonthrowing | ✔️ | Produces a placement containing one document-space extent. because framing that also changed the field would be framing that changed the composition. parameter is what decides containment there, and this routine is forbidden to touch it. `46` §5 records that as the declared behaviour rather than an omission. the extent is contained on both axes rather than on whichever happens to be wider.
    in    Standing  const CameraSpecification&  [-]   the camera whose projection and rotation are kept
    in    Least     DocumentPosition            [mm]  the extent's lower corner
    in    Greatest  DocumentPosition            [mm]  its upper corner
    out   -         Outcome                     [-]   refuses with ContentUnsupported for an inverted extent or an invalid projection
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/ControlText.cpp
    note  🔴 The **placement only** is produced. The projection's extent parameter is left as the artist set it,
    note  ⚠️ For a parallel projection the placement is centred and nothing else can be done — the extent
    note  📐 The distance is solved against the extent's bounding sphere and the lesser of the two half-angles, so

F SLATE_DECLARES_PRECISION               | CameraProjection.h | 278     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/ChannelPanel.h, Api/ChartPartition.h, (+50 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CAMERA
//------------------------------------------------------------------------------------------------------------------------

T CameraProjection                       | CameraProjection.h | 294-351 | owning                        | -  | One camera occupant — its specification, and the two derivations reconciled from it. attaches through `AttachmentFollows` and is manipulated by `78`. A camera held outside the population is one the artist cannot select, name, group, attach or undo. illuminants do not; its presence in the workspace is an `80` overlay at `08` §3 ⑩. consumer reading the frustum without it reads the frustum of the camera as it stood last tick, which is a cull against a view the artist has already left.
    has   Specification   CameraSpecification  [-]  ?
    has   DerivedView     ViewProjection       [-]  ?
    has   DerivedFrustum  FrustumSpace         [-]  ?
    has   CameraOccupant  OccupantIdentity     [-]  ?
    has   ReconcileOwed   bool                 [-]  ?
    by    Api/PointerIntersection.h, Api/SpatialManipulator.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/PointerIntersection.cpp, Source/SpatialManipulator.cpp
    note  🔴 A camera is an **occupant of the document population**. It enrols in `12`, appears in the outliner,
    note  🔴 Being an occupant does not make a camera shaded. It writes no `VisibilityIndex`, exactly as `44`'s
    note  ⚠️ `Reconcile` is the only writer of the two derivations, and it is owed after every amendment. A

F CameraProjection::Declare              | CameraProjection.h | 302     | api,nonallocating,nonthrowing | ✔️ | Declares which occupant this camera is, and its initial specification.
    in    Subject    OccupantIdentity            [-]  ?
    in    Declaring  const CameraSpecification&  [-]  ?
    out   -          Outcome                     [-]  refuses with IdentityStale for an undeclared identity
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DiagnosticExtension.h, Api/DisplayProjection.h, (+65 more)

F CameraProjection::Amend                | CameraProjection.h | 308     | api,nonallocating,nonthrowing | ✔️ | Amends the specification, leaving the derivations owed.
    in    Amending  const CameraSpecification&  [-]  ?
    out   -         Outcome                     [-]  refuses with IdentityStale before Declare has delivered
    by    Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/IlluminantPopulation.h, Api/ImpressionSequence.h, Api/MaterialSpecification.h, (+26 more)

F CameraProjection::DeclareDisplayExtent | CameraProjection.h | 316     | api,nonallocating,nonthrowing | ✔️ | Declares the display's drawable extent, from which the sensor proportion follows. carrying its author's window proportion opens framed for their monitor and not for the artist's.
    in    Width   std::uint32_t  [-]  ?
    in    Height  std::uint32_t  [-]  ?
    out   -       Outcome        [-]  refuses with ContentUnsupported for a zero extent on either axis
    by    Source/CameraProjection.cpp
    note  ⚠️ The proportion is derived from the display and is not stored as an authored property. A document

F CameraProjection::Reconcile            | CameraProjection.h | 323     | api,nonthrowing               | ✔️ | Re-derives the view projection and the frustum.
    out   -  Outcome  [-]  carries `Derive`'s refusal when the specification cannot be projected
    post  the frustum and the view projection describe the current specification
    by    Api/OutlinerSequence.h, Api/SurfaceTileSpace.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/OutlinerSequence.cpp, Source/SurfaceTileSpace.cpp

F CameraProjection::Declared             | CameraProjection.h | 325     | -                             | -  | ?
    out   -  const CameraSpecification&  [-]  ?
    by    Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DiagnosticPanel.h, (+82 more)

F CameraProjection::Projected            | CameraProjection.h | 326     | -                             | -  | ?
    out   -  const ViewProjection&  [-]  ?
    by    Api/OcclusionProjection.h, Api/PointerIntersection.h, Api/SpectralProjection.h, Source/CameraProjection.cpp, Source/ColourProjection.cpp, Source/ConsoleHost.cpp, (+6 more)

F CameraProjection::Frustum              | CameraProjection.h | 327     | -                             | -  | ?
    out   -  const FrustumSpace&  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp

F CameraProjection::Occupant             | CameraProjection.h | 332     | api,nonallocating,nonthrowing | ✔️ | Which occupant this camera is.
    out   -  OccupantIdentity  [-]  ?
    by    Api/DecalProjection.h, Api/MaterialSpecification.h, Api/PartitionStructure.h, Api/PointerIntersection.h, Api/PropertySpecification.h, Api/RowSequence.h, (+17 more)

F CameraProjection::Exposure             | CameraProjection.h | 337     | api,nonallocating,nonthrowing | ✔️ | The authored exposure `66` §2 reads.
    out   -  double  [-]  ?
    by    Api/DisplayProjection.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/DisplayProjection.cpp

F CameraProjection::DerivationOwed       | CameraProjection.h | 342     | api,nonallocating,nonthrowing | ✔️ | Whether an amendment has been made that `Reconcile` has not yet absorbed.
    out   -  bool  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/PointerIntersection.cpp, Source/SpatialManipulator.cpp

F SLATE_DECLARES_PRECISION               | CameraProjection.h | 357     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Exact    PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/ChannelPanel.h, Api/ChartPartition.h, (+50 more)
