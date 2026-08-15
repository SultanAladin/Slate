//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The nine-lobe fit, and the normalisation derived from it rather than beside it.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/SpectralProjection/Source
%layer      SlateMath
%sources    1
%symbols    3
%annotated  0/3
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpectralProjection.cpp | 85 lines | 9e32ebe8 | 3 sym | The nine-lobe fit, and the normalisation derived from it rather than beside it.

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE LOBE
//------------------------------------------------------------------------------------------------------------------------

F Lobe                   | SpectralProjection.cpp | 23-33 | - | - | ?
    in    Wavelength  double  [-]  ?
    in    Centre      double  [-]  ?
    in    LowerWidth  double  [-]  ?
    in    UpperWidth  double  [-]  ?
    out   -           double  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                             THE COLOUR-MATCHING FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

F ProjectWavelength      | SpectralProjection.cpp | 41-64 | - | - | ?
    in    Wavelength  double                 [-]  ?
    out   -           TristimulusCoordinate  [-]  ?
    by    Api/SpectralProjection.h, Source/AtmosphereIntegrator.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE NORMALISATION
//------------------------------------------------------------------------------------------------------------------------

F LuminanceNormalisation | SpectralProjection.cpp | 70-83 | - | - | ?
    in    Rule  const QuadratureRule&  [-]  ?
    out   -     Deliver<double>        [-]  ?
    by    Api/SpectralProjection.h, Source/AtmosphereIntegrator.cpp
