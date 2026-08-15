//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `78` — one manipulator that moves, rotates and scales everything movable, and the grips the artist grasps to do it.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/SpatialManipulator/Api
%layer      SlateDocument
%sources    1
%symbols    44
%annotated  25/44
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpatialManipulator.h | 411 lines | c13149b0 | 44 sym | `78` — one manipulator that moves, rotates and scales everything movable, and the grips the artist grasps to do it.

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE THREE AXES
//------------------------------------------------------------------------------------------------------------------------

E ManipulationAxis                   | SpatialManipulator.h | 32-39   | contract                      | -  | Which axis of the reference orientation a grip addresses. The screen entry is last because it addresses no basis axis at all — it is the camera plane, and `78` §3 lists it as a constraint rather than as a direction.
    has   AxisAlong   ManipulationAxis  [-]  ?
    has   AxisUp      ManipulationAxis  [-]  ?
    has   AxisAcross  ManipulationAxis  [-]  ?
    has   AxisScreen  ManipulationAxis  [-]  ?
    has   AxisCount   ManipulationAxis  [-]  ?
    by    Source/SpatialManipulator.cpp
    note  📝 The ordinal **is** the axis, so a grip's axis indexes a basis directly rather than being switched on.

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT A GRIP DOES
//------------------------------------------------------------------------------------------------------------------------

E ManipulationSubject                | SpatialManipulator.h | 53-60   | contract                      | -  | Which of the three transforms a grip edits, and along how many axes it edits it. alone, make every edit begin with a mode change the artist has to remember they are in; and the mode they are in is invisible the moment the manipulator leaves the display. once. The drag resolves against a plane rather than against a line, and `78` §2 fixes that plane at Open — two line solves would each re-derive their own and the two would disagree the moment the camera moved.
    has   Translate       ManipulationSubject  [-]  ?
    has   PlaneTranslate  ManipulationSubject  [-]  ?
    has   Scale           ManipulationSubject  [-]  ?
    has   Rotate          ManipulationSubject  [-]  ?
    has   SubjectCount    ManipulationSubject  [-]  ?
    by    Source/SpatialManipulator.cpp
    note  🔴 One manipulator addresses all three — `78` §5's first gate. Three separate manipulators, each shown
    note  📝 A plane translation is held apart from an axis translation rather than being two axis translations at

E ConstraintSubject                  | SpatialManipulator.h | 64-71   | contract                      | -  | What the drag resolves against, fixed at Open and never re-derived — `78` §3.
    has   AxisConstrained    ConstraintSubject  [-]  ?
    has   PlaneConstrained   ConstraintSubject  [-]  ?
    has   ScreenConstrained  ConstraintSubject  [-]  ?
    has   Unconstrained      ConstraintSubject  [-]  ?
    has   ConstraintCount    ConstraintSubject  [-]  ?
    by    Source/SpatialManipulator.cpp

E ReferenceOrientation               | SpatialManipulator.h | 78-85   | contract                      | -  | Whose axes the manipulator is drawn along and the drag is resolved in — `78` §3's second table. moves it off the surface it sits on; dragging along the surface reference slides it across the surface, which is what the gesture means and what the artist expects to have happened.
    has   DocumentAxes      ReferenceOrientation  [-]  ?
    has   OccupantAxes      ReferenceOrientation  [-]  ?
    has   SurfaceAxes       ReferenceOrientation  [-]  ?
    has   PlacementAxes     ReferenceOrientation  [-]  ?
    has   OrientationCount  ReferenceOrientation  [-]  ?
    note  🔴 The surface reference is what makes placement manipulation usable. Dragging a decal along document axes

E ManipulatedSubject                 | SpatialManipulator.h | 93-101  | contract                      | -  | What the manipulator is addressing — `78` §1's four targets. document space. The manipulator is drawn in document space and the drag is projected back through the attachment before it is applied. Applied in document space it would store an absolute transform and `00` §10.1 ②'s zero-cost rows would each become "re-resolve everything".
    has   Nothing        ManipulatedSubject  [-]  ?
    has   OneOccupant    ManipulatedSubject  [-]  ?
    has   ManyOccupants  ManipulatedSubject  [-]  ?
    has   OnePlacement   ManipulatedSubject  [-]  ?
    has   OneCamera      ManipulatedSubject  [-]  ?
    has   TargetCount    ManipulatedSubject  [-]  ?
    by    Source/SpatialManipulator.cpp
    note  🔴 A placement is manipulated in the space it is **stored** in, relative to its surface, and never in

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE GRIP
//------------------------------------------------------------------------------------------------------------------------

T ManipulationGrip                   | SpatialManipulator.h | 115-126 | nonallocating,nonthrowing     | -  | One grabbable part of the manipulator, in the manipulator's own space. when it is laid out. A manipulator that scaled with its target vanishes on a small occupant and fills the workspace on a large one, and in both cases the artist can no longer grasp the axis they want. screen-space intersection in `78` §4 actually tests. Testing the generated triangles instead would make a grip harder to hit exactly where it is thinnest, which is the tip of the cone the artist aims at.
    has   Edits          ManipulationSubject     [-]  ?
    has   Addressed      ManipulationAxis        [-]  ?
    has   GripColour     ColourSpecification     [-]  ?
    has   NearPosition   DocumentPosition        [-]  ?
    has   FarPosition    DocumentPosition        [-]  ?
    has   HalfExtent     double                  [-]  ?
    has   Generated      PrimitiveSpecification  [-]  ?
    has   GripPlacement  DecomposedTransform     [-]  ?
    has   GripDeclared   bool                    [-]  ?
    by    Source/SpatialManipulator.cpp
    note  🔴 A grip's geometry is declared in the manipulator's own space and scaled to a constant display extent
    note  📝 `Reach` and `HalfExtent` describe a capsule about the grip's own axis, and that capsule is what the

V GripAxisLength                     | SpatialManipulator.h | 132     | -                             | -  | ?

V GripTipReach                       | SpatialManipulator.h | 133     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripConeRadius                     | SpatialManipulator.h | 134     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripConeLength                     | SpatialManipulator.h | 135     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripScaleLength                    | SpatialManipulator.h | 136     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripScaleInboard                   | SpatialManipulator.h | 137     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripPlaneHalfExtent                | SpatialManipulator.h | 138     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripArcRadius                      | SpatialManipulator.h | 139     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripArcBand                        | SpatialManipulator.h | 140     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripArcSweep                       | SpatialManipulator.h | 141     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripRingRadius                     | SpatialManipulator.h | 142     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripRingBand                       | SpatialManipulator.h | 143     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V GripViewFraction                   | SpatialManipulator.h | 149     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V SnapTranslation                    | SpatialManipulator.h | 153     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V SnapScaleStep                      | SpatialManipulator.h | 154     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V SnapRotationStep                   | SpatialManipulator.h | 155     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

V ScaleFactorLeast                   | SpatialManipulator.h | 156     | -                             | -  | ?
    by    Source/SpatialManipulator.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE LAYOUT
//------------------------------------------------------------------------------------------------------------------------

T ManipulationLayout                 | SpatialManipulator.h | 167-262 | owning                        | -  | The manipulator's grips as they stand for one target, one reference orientation and one camera. those. A layout re-derived per pointer sample would move the grips under a drag that is already addressing one of them, which is the same defect `78` §2 refuses for the drag plane.
    has   GripCeiling      static constexpr std::uint32_t  [-]  ?
    has   Declared         std::vector<ManipulationGrip>   [-]  ?
    has   LaidOrigin       DocumentPosition                [-]  ?
    has   LaidOrientation  RotationQuaternion              [-]  ?
    has   LaidTarget       ManipulatedSubject              [-]  ?
    has   LaidUnitExtent   double                          [-]  ?
    has   LayoutDeclared   bool                            [-]  ?
    by    Source/SpatialManipulator.cpp
    note  🔴 Laid out afresh whenever the target, the orientation or the camera changes, and read unamended between

F ManipulationLayout::Layout         | SpatialManipulator.h | 188     | api,nonthrowing               | 🚩 | Lays the grips out about one origin, in one reference orientation, at a constant display extent. derivation is owed `46` §7 makes `Reconcile` the only writer of it, and grips laid out through last tick's projection sit where the artist was looking before they moved — which is exactly where they will click. because `46` has no scale to edit, and a caller deciding that would be a second place the rule lives.
    in    Origin       DocumentPosition         [mm]  where the manipulator sits, in document space
    in    Orientation  RotationQuaternion       [-]   the reference orientation's rotation, already resolved by the caller
    in    Camera       const CameraProjection&  [-]   the camera the extent is held constant against; its derivation must not be owed
    in    Addressing   ManipulatedSubject       [-]   which of `78` §1's targets is being manipulated
    out   -            Deliver                  [-]   refuses with ContentUnsupported for the closed target count and for a camera whose
    post  every grip this target offers is declared; the rest stand undeclared and are never intersected
    by    Api/DescriptorIndex.h, Source/DescriptorIndex.cpp, Source/ProgramIndex.cpp, Source/SpatialManipulator.cpp, Source/VisibilityRaster.cpp
    note  🔴 A camera owing a reconciliation refuses rather than laying out against the standing derivation.
    note  📝 Which grips a target offers is decided here and not by the caller. A camera offers no scale grip

F ManipulationLayout::Grasp          | SpatialManipulator.h | 210     | api,nonthrowing               | 🚩 | Which grip one pointer position grasps — `78` §4's own intersection, before `74` is consulted. grasps no grip, and when no layout stands `VisibilityIndex`, so `74` cannot pick it and `26` cannot outline it — asking `74` first would therefore return whatever stands behind the grip, and the artist would select through it. grips before the plane and rotation ones, so a pointer over the overlap of a cone and an arc grasps the cone — which is the smaller target and therefore the one that was aimed at.
    in    Camera         const CameraProjection&                                                           [-]   the camera the pointer was reported against
    in    PointerAlong   double                                                                            [px]  the display's first axis, zero at the left edge
    in    PointerAcross  double                                                                            [px]  its second, zero at the top edge
    in    DisplayAlong   std::uint32_t                                                                     [px]  the drawable extent the position was reported against
    in    DisplayAcross  std::uint32_t                                                                     [px]  ?
    in    `14`           §4.2, and it runs **before** `74` is consulted at all. The manipulator writes no  [-]   ?
    out   -              Deliver                                                                           [-]   the grip ordinal grasped; refuses with ContentUnsupported when the pointer
    by    Source/SpatialManipulator.cpp
    note  🔴 `78` §4: this is a **separate** screen-space test against the grips' own geometry, at precedence 2
    note  📝 The nearest grip along the ray wins, and ties go to the lower ordinal. The layout orders the axis

F ManipulationLayout::Resolve        | SpatialManipulator.h | 220     | api,nonthrowing               | ✔️ | One laid-out grip.
    in    GripOrdinal  std::uint32_t  [-]  ?
    out   -            Deliver        [-]  refuses with ContentUnsupported outside the laid-out count and for an undeclared grip
    by    Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DocumentSession.h, (+94 more)

F ManipulationLayout::Grips          | SpatialManipulator.h | 229     | api,nonallocating,nonthrowing | ✔️ | Every laid-out grip, for whoever records them. `OverlaySubject::Manipulator`. The manipulator contributes no recording of its own: a second one would be a second place `08`'s ordering is declared, and the two would order differently the first time either was amended.
    out   -  const std::vector<ManipulationGrip>&  [-]  ?
    by    Source/SpatialManipulator.cpp
    note  📝 Recorded in `08` §3 ⑪ — the depth-free overlay recording `80` already declares, at

F ManipulationLayout::Origin         | SpatialManipulator.h | 234     | api,nonallocating,nonthrowing | ✔️ | Where the manipulator sits and how it is turned, as the last layout placed it.
    out   -  DocumentPosition  [-]  ?
    by    Api/CurveSolver.h, Api/PointerIntersection.h, Api/ReflectanceIntegrator.h, Api/ReportSequence.h, Api/SpatialSubdivision.h, Api/VectorInterchange.h, (+30 more)

F ManipulationLayout::Orientation    | SpatialManipulator.h | 235     | -                             | -  | ?
    out   -  RotationQuaternion  [-]  ?
    by    Api/PartitionStructure.h, Api/PointerIntersection.h, Api/ReflectanceIntegrator.h, Api/SpatialSubdivision.h, Shared/PlanarClassifier.slang.h, Source/ConsoleHost.cpp, (+6 more)

F ManipulationLayout::UnitExtent     | SpatialManipulator.h | 242     | api,nonallocating,nonthrowing | ✔️ | The extent one manipulator unit spans in document space at the layout's distance. they were laid out at, and by nothing else.
    out   -  double  [-]  ?
    by    Source/SpatialManipulator.cpp
    note  📝 What makes the display extent constant. Read by the recording so the grips are drawn at the size

F ManipulationLayout::LayoutStanding | SpatialManipulator.h | 247     | api,nonallocating,nonthrowing | ✔️ | Whether a layout stands at all.
    out   -  bool  [-]  ?
    by    Source/SpatialManipulator.cpp

F ManipulationLayout::Reclaim        | SpatialManipulator.h | 252     | api,nonthrowing               | ✔️ | Discards the layout. The manipulator is not presented until one is laid out again.
    out   -  void  [-]  ?
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE DRAG
//------------------------------------------------------------------------------------------------------------------------

T ManipulationAmendment              | SpatialManipulator.h | 273-282 | nonallocating,nonthrowing     | -  | What one amendment of a drag has produced, in the space the target is stored in. transform. `78` §1's four targets each compose it differently — a placement composes it against its surface, an occupant against its attachment — and a finished transform would have had to choose one.
    has   Displacement   DocumentPosition     [-]  ?
    has   Turned         RotationQuaternion   [-]  ?
    has   ScaleAlong     double               [-]  ?
    has   ScaleUp        double               [-]  ?
    has   ScaleAcross    double               [-]  ?
    has   TurnedRadians  double               [-]  ?
    has   Edited         ManipulationSubject  [-]  ?
    by    Source/SpatialManipulator.cpp
    note  🔴 The amendment is reported as a **displacement, a factor and a rotation** rather than as a finished

T ManipulationSequence               | SpatialManipulator.h | 295-402 | owning                        | -  | One manipulation, following `10` §2.4's lifecycle exactly — `78` §2's correspondence. from the current pointer position it makes the manipulated object chase the cursor with increasing gain, which the artist reads as the manipulator being slippery rather than as the plane being wrong. `RevisionSequence` with positions the artist never meant to stop at, and undo then steps back one pixel at a time. Exactly `10` §2.4, unrelaxed, and the same rule `46`'s navigation and `72`'s positioning keep. on a grip continues to address that grip after the cursor leaves the workspace, and the capture that makes that true has exactly one owner.
    has   GraspedGrip      ManipulationGrip       [-]  ?
    has   Standing         ManipulationAmendment  [-]  ?
    has   FixedConstraint  ConstraintSubject      [-]  ?
    has   HeldCamera       CameraProjection       [-]  ?
    has   DragOrigin       DocumentPosition       [-]  ?
    has   AxisAlongSpan    double                 [-]  ?
    has   AxisUpSpan       double                 [-]  ?
    has   AxisAcrossSpan   double                 [-]  ?
    has   PlaneAlongSpan   double                 [-]  ?
    has   PlaneUpSpan      double                 [-]  ?
    has   PlaneAcrossSpan  double                 [-]  ?
    has   ReferenceAlong   double                 [-]  ?
    has   ReferenceUp      double                 [-]  ?
    has   ReferenceAcross  double                 [-]  ?
    has   OpenParameter    double                 [-]  ?
    has   OpenAngle        double                 [-]  ?
    has   OpenPlanePoint   DocumentPosition       [-]  ?
    has   UnitExtent       double                 [-]  ?
    has   OpenDeclared     bool                   [-]  ?
    by    Source/SpatialManipulator.cpp
    note  🔴 The drag resolves against a plane or axis **fixed at Open** and never re-derived per sample. Re-derived
    note  🔴 Nothing is recorded between Open and Seal — `78` §5. A transaction per pointer sample fills
    note  ⚠️ Pointer capture is held for the whole drag through `76` §3 and is **not** taken here. A drag that began

F ManipulationSequence::Open         | SpatialManipulator.h | 314     | api,nonthrowing               | 🚩 | Opens a manipulation against one grasped grip, fixing the axis or plane it resolves against. for an undeclared grip or a pointer that resolves no position on the fixed plane camera that is re-read each sample is a plane that moves whenever the artist orbits mid-drag.
    in    Grasped        const ManipulationGrip&    [-]   the grip the pointer grasped
    in    Laid           const ManipulationLayout&  [-]   the layout it was grasped from
    in    Camera         const CameraProjection&    [-]   the camera; read once here and never again during the drag
    in    PointerAlong   double                     [px]  where the pointer stood when the grip was grasped
    in    PointerAcross  double                     [px]  ?
    in    DisplayAlong   std::uint32_t              [px]  the drawable extent
    in    DisplayAcross  std::uint32_t              [px]  ?
    out   -              Deliver                    [-]   refuses with HostDenied when a drag is already open, and with ContentUnsupported
    post  🔴 the drag axis or plane is fixed; nothing is recorded until Seal
    by    Api/CameraProjection.h, Api/CommandSequence.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/HardwareMetrics.h, (+20 more)
    note  🔴 The camera is read **here and once**. `78` §2's rule is about the plane, and a plane fixed from a

F ManipulationSequence::Amend        | SpatialManipulator.h | 336     | api,nonthrowing               | 🚩 | Amends the open drag by one pointer position, against the axis or plane Open fixed. pointer that resolves no position against the fixed plane displacements accumulates their rounding too, and a snapped drag would then drift off its own increment over a long gesture — which reads as the snapping having been switched off.
    in    PointerAlong   double         [px]  where the pointer stands now
    in    PointerAcross  double         [px]  ?
    in    DisplayAlong   std::uint32_t  [px]  the drawable extent
    in    DisplayAcross  std::uint32_t  [px]  ?
    in    SnapDeclared   bool           [-]   true snaps the amendment to the declared increment
    out   -              Deliver        [-]   refuses with HostDenied when no drag is open, and with ContentUnsupported for a
    post  nothing is recorded; the amendment is readable and the target is unamended until Seal
    by    Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/IlluminantPopulation.h, Api/ImpressionSequence.h, (+26 more)
    note  📝 The pointer's position is taken rather than its displacement since the last sample. Accumulating

F ManipulationSequence::Abandon      | SpatialManipulator.h | 346     | api,nonallocating,nonthrowing | ✔️ | Ends the drag with no effect. The caller restores what stood at Open.
    out   -  Deliver  [-]  refuses with HostDenied when no drag is open
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/ImpressionSequence.h, Api/OcclusionScheduler.h, Api/RevisionSequence.h, Api/VisibilityRaster.h, (+10 more)

F ManipulationSequence::Seal         | SpatialManipulator.h | 353     | api,nonallocating,nonthrowing | ✔️ | Ends the drag, returning the amendment the caller commits as **one** transaction.
    out   -  Deliver  [-]  refuses with HostDenied when no drag is open
    post  🔴 exactly one transaction enters `RevisionSequence`, sealed by the caller — `78` §2 and §5
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/ImpressionSequence.h, Api/InterfaceExchange.h, (+19 more)

F ManipulationSequence::Amended      | SpatialManipulator.h | 358     | api,nonallocating,nonthrowing | ✔️ | The amendment as the drag stands, for the workspace to present while it is open.
    out   -  const ManipulationAmendment&  [-]  ?
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/ReportSequence.h, Api/TransmissionSequence.h, Source/BrushSpecification.cpp, (+16 more)

F ManipulationSequence::DragOpen     | SpatialManipulator.h | 363     | api,nonallocating,nonthrowing | ✔️ | Whether a drag is open.
    out   -  bool  [-]  ?
    by    Source/SpatialManipulator.cpp

F ManipulationSequence::Constrained  | SpatialManipulator.h | 368     | api,nonallocating,nonthrowing | ✔️ | What the open drag resolves against, as Open fixed it.
    out   -  ConstraintSubject  [-]  ?
    by    Source/SpatialManipulator.cpp

F ManipulationSequence::Grasped      | SpatialManipulator.h | 373     | api,nonallocating,nonthrowing | ✔️ | Which grip the open drag addresses.
    out   -  const ManipulationGrip&  [-]  ?
    by    Source/SpatialManipulator.cpp

F SLATE_DECLARES_PRECISION           | SpatialManipulator.h | 407     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
