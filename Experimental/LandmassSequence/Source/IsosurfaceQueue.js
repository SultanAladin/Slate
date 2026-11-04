//============================================================================================================================================
//                                                             ISOSURFACEQUEUE.JS
//============================================================================================================================================
// 🧩 One bounded 3D extraction request and transferable result; no planet-sized voxel allocation.

import { ProjectSurface } from './SurfaceProjection.js';
self.onmessage = ({ data: Request }) =>
{
    try
    {
        const Started = performance.now();
        const Surface = ProjectSurface(Request.Document, Request.Extent, Request.Sampling);
        self.postMessage({ Revision: Request.Revision, Extent: Request.Extent, Surface, Duration: performance.now() - Started }, [Surface.Positions.buffer, Surface.Colours.buffer]);
    }
    catch (Failure)
    {
        self.postMessage({ Revision: Request.Revision, Refusal: Failure.message });
    }
};
