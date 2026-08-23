//============================================================================================================================================
//                                                         SLIDINGPAGES.CPP
//============================================================================================================================================
// 🧩 Compile-time geometry proofs for the shared horizontal page placement.

#include "SlateUI/Interface/SlidingPages/Api/SlidingPages.h"

namespace Slate
{
namespace
{

constexpr PlaneExtent ProofViewport = Spanning(10.0f, 20.0f, 100.0f, 40.0f);
constexpr SlidingPagePlacement ForwardStart = SlidingPages::Place(ProofViewport, 0.0f, true);
constexpr SlidingPagePlacement ForwardHalf  = SlidingPages::Place(ProofViewport, 0.5f, true);
constexpr SlidingPagePlacement ForwardEnd   = SlidingPages::Place(ProofViewport, 1.0f, true);
constexpr SlidingPagePlacement BackwardHalf = SlidingPages::Place(ProofViewport, 0.5f, false);
constexpr SlidingPagePlacement Clamped      = SlidingPages::Place(ProofViewport, 2.0f, true);

static_assert(ForwardStart.Departing.MinimumX == 10.0f && ForwardStart.Incoming.MinimumX == 110.0f);
static_assert(ForwardHalf.Departing.MinimumX == -40.0f && ForwardHalf.Incoming.MinimumX == 60.0f);
static_assert(ForwardEnd.Departing.MinimumX == -90.0f && ForwardEnd.Incoming.MinimumX == 10.0f);
static_assert(BackwardHalf.Departing.MinimumX == 60.0f && BackwardHalf.Incoming.MinimumX == -40.0f);
static_assert(Clamped.Incoming.MinimumX == 10.0f && !Clamped.Travelling);
static_assert(ForwardHalf.Departing.Width() == ProofViewport.Width());
static_assert(ForwardHalf.Incoming.Height() == ProofViewport.Height());

}   // namespace
}   // namespace Slate
