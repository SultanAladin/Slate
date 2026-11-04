//============================================================================================================================================
//                                                             ELEVATIONQUEUE.JS
//============================================================================================================================================
// 🧩 One transferable surface result per worker request; the UI retains only the latest pending revision.

import { ProjectSurface } from './SurfaceProjection.js';

self.onmessage = ({ data: Request }) =>
{
    try
    {
        const Started = performance.now();
        const Surface = ProjectSurface(Request.Document, Request.Extent, Request.Sampling);
        self.postMessage({ Revision: Request.Revision, Surface, Duration: performance.now() - Started },
            [Surface.Positions.buffer, Surface.Colours.buffer, Surface.Indices.buffer]);
    }
    catch (Failure)
    {
        self.postMessage({ Revision: Request.Revision, Refusal: Failure.message });
    }
};
