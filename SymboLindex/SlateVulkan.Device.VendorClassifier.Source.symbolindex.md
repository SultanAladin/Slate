//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Enumerated device scored into a capability set and a ranking.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/VendorClassifier/Source
%layer      SlateVulkan
%sources    1
%symbols    1
%annotated  0/1
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S VendorClassifier.cpp | 91 lines | 45684eda | 1 sym | Enumerated device scored into a capability set and a ranking.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SCORING
//------------------------------------------------------------------------------------------------------------------------

F Classify | VendorClassifier.cpp | 17-89 | - | - | ?
    in    Candidate            VkPhysicalDevice  [-]  ?
    in    PresentationSurface  VkSurfaceKHR      [-]  ?
    out   -                    ScoredCandidate   [-]  ?
    by    Api/CameraProjection.h, Api/SampleIntegrator.h, Api/TilingSpecification.h, Api/VectorInterchange.h, Api/VendorClassifier.h, Source/AnalyticProjection.cpp, (+8 more)
