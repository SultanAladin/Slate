//============================================================================================================================================
//                                                            REVISIONSEQUENCE.JS
//============================================================================================================================================
// 🧩 Byte-bounded reversible recipe snapshots with transactional import and deterministic undo/redo.

import { ValidateProject } from './ElevationProjection.js';

export class RevisionSequence
{
    constructor(Document, ByteLimit = 8 * 1048576)
    {
        this.Document = ValidateProject(Document);
        this.Before = [];
        this.After = [];
        this.ByteLimit = ByteLimit;
    }

    MeasureBytes()
    {
        return new TextEncoder().encode(JSON.stringify([this.Document, this.Before, this.After])).byteLength;
    }

    Replace(Document)
    {
        const Accepted = ValidateProject(Document);
        const Previous = JSON.stringify(this.Document);
        if (Previous === JSON.stringify(Accepted)) return false;
        this.Before.push(Previous);
        this.After.length = 0;
        this.Document = Accepted;
        while (this.Before.length > 24 || this.MeasureBytes() > this.ByteLimit)
        {
            if (!this.Before.length) break;
            this.Before.shift();
        }
        return true;
    }

    Rewind()
    {
        if (!this.Before.length) return false;
        this.After.push(JSON.stringify(this.Document));
        this.Document = JSON.parse(this.Before.pop());
        return true;
    }

    Advance()
    {
        if (!this.After.length) return false;
        this.Before.push(JSON.stringify(this.Document));
        this.Document = JSON.parse(this.After.pop());
        return true;
    }
}
