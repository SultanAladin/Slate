"""Crease-preserving topological macro charts. No vehicle-specific stations.
Start with conforming separatrices; delete non-feature interfaces only when
the merged region remains a simple four-corner disk. T-junctions are allowed
in the *layout*: export must split the corresponding B-rep edge, not the face.
"""
from collections import defaultdict, deque, Counter
import numpy as np


def edge(a,b): return (min(a,b),max(a,b))

class Layout:
    def __init__(self, data):
        self.V=data['V'];self.Q=data['Q'];self.S=data['S']
        self.ef=defaultdict(list);self.vq=defaultdict(list);self.vn=defaultdict(set)
        for qi,q in enumerate(self.Q):
            for i in range(4):
                a,b=int(q[i]),int(q[(i+1)%4])
                self.ef[edge(a,b)].append(qi)
                self.vq[a].append(qi);self.vn[a].add(b);self.vn[b].add(a)
        crease={edge(*map(int,e)) for e,c in zip(data['E'],data['crease']) if c>=0.5}
        self.walls={e for e,fs in self.ef.items() if len(fs)!=2 or e in crease}
        self.features=crease & set(self.ef)
        self.cuts=set(self.walls)

    def sectors(self,v,reg,cuts):
        """Ordered sectors of faces around a vertex, separated by cuts."""
        unseen=set(self.vq[v]) & reg; out=[]
        while unseen:
            start=min(unseen);cur=start;backseen=set()
            while True:
                q=list(self.Q[cur]);i=q.index(v);e=edge(v,int(q[(i-1)%4]))
                nbs=[n for n in self.ef[e] if n!=cur and n in reg]
                if e in cuts or not nbs or nbs[0] in backseen:break
                backseen.add(cur);cur=nbs[0]
            seq=[];inter=[]
            while cur in unseen:
                unseen.remove(cur);seq.append(cur)
                q=list(self.Q[cur]);i=q.index(v);w=int(q[(i+1)%4]);e=edge(v,w)
                nbs=[n for n in self.ef[e] if n!=cur and n in reg]
                if e in cuts or not nbs or nbs[0] not in unseen:break
                inter.append(w);cur=nbs[0]
            out.append((seq,inter))
        return out

    def regions(self,cuts=None):
        cuts=self.cuts if cuts is None else cuts
        labels=np.full(len(self.Q),-1,dtype=int);regs=[]
        for seed in range(len(self.Q)):
            if labels[seed]>=0:continue
            idx=len(regs);labels[seed]=idx;todo=[seed];reg=[]
            while todo:
                qi=todo.pop();reg.append(qi);q=self.Q[qi]
                for i in range(4):
                    e=edge(int(q[i]),int(q[(i+1)%4]))
                    if e in cuts:continue
                    for nb in self.ef[e]:
                        if labels[nb]<0:labels[nb]=idx;todo.append(nb)
            regs.append(reg)
        return regs,labels

    def boundary(self,reg,cuts=None):
        cuts=self.cuts if cuts is None else cuts;rs=set(reg)
        nxt={};count=Counter();vs=set();es=set()
        for qi in reg:
            q=self.Q[qi]
            for i in range(4):
                a,b=int(q[i]),int(q[(i+1)%4]);e=edge(a,b)
                vs.add(a);es.add(e);count[a]+=1
                nbs=[f for f in self.ef[e] if f in rs]
                if len(nbs)==1:
                    if a in nxt:return None
                    nxt[a]=b
                elif e in cuts:return None # retained internal feature slit
        if len(vs)-len(es)+len(reg)!=1 or not nxt:return None
        corners=sorted(v for v in nxt if count[v]==1)
        if len(corners)!=4 or any(count[v]>2 for v in nxt):return None
        start=corners[0];loop=[start];v=nxt[start]
        while v!=start and len(loop)<=len(nxt):
            loop.append(v)
            if v not in nxt:return None
            v=nxt[v]
        if len(loop)!=len(nxt):return None
        ids=sorted(loop.index(v) for v in corners)
        sides=[]
        for j,i in enumerate(ids):
            end=ids[(j+1)%4]
            side=loop[i:end+1] if end>i else loop[i:]+loop[:end+1]
            sides.append(side)
        return sides

    def construct(self):
        allreg=set(range(len(self.Q)));queue=deque()
        for v in sorted(self.vq):
            walls=[w for w in self.vn[v] if edge(v,w) in self.walls]
            if not walls:
                if len(self.vq[v])!=4:
                    queue.extend((v,w) for w in sorted(self.vn[v]))
            else:
                for fs,inter in self.sectors(v,allreg,self.walls):
                    if len(fs)>2:
                        if len(fs)==4 and len(walls)==1:
                            queue.append((v,inter[1]))
                        else:
                            queue.extend((v,inter[i-1]) for i in range(1,len(fs),2) if i-1<len(inter))
        seen=set()
        while queue:
            a,b=queue.popleft();e=edge(a,b)
            if (a,b) in seen or e in self.walls:continue
            seen.add((a,b));self.cuts.add(e)
            if any(edge(b,w) in self.walls for w in self.vn[b]):continue
            if len(self.vq[b])!=4:continue
            near=set(self.ef[e])
            opp=[w for w in self.vn[b] if w!=a and not near.intersection(self.ef[edge(b,w)])]
            if len(opp)==1:queue.append((b,opp[0]))
        regs,labels=self.regions()
        bad=[r for r in regs if self.boundary(r) is None]
        if bad:
            # Conservative fallback: retain individual quad boundaries in invalid
            # regions. They will still participate in the same safe merge pass.
            for r in bad:
                for qi in r:
                    q=self.Q[qi]
                    self.cuts.update(edge(int(q[i]),int(q[(i+1)%4])) for i in range(4))
        initial=len(self.regions()[0])
        merges=0
        while True:
            regs,labels=self.regions();pairs=defaultdict(set)
            for e in self.cuts-self.walls:
                fs=self.ef[e]
                if len(fs)==2 and labels[fs[0]]!=labels[fs[1]]:
                    pairs[tuple(sorted((int(labels[fs[0]]),int(labels[fs[1]]))))].add(e)
            merged=False
            for (a,b),seams in sorted(pairs.items(),key=lambda kv:(min(len(regs[kv[0][0]]),len(regs[kv[0][1]])),kv[0])):
                if self.boundary(regs[a]+regs[b],self.cuts-seams) is not None:
                    self.cuts-=seams;merges+=1;merged=True;break
            if not merged:break
        self.regs=regs
        self.sides=[self.boundary(r) for r in regs]
        assert all(s is not None for s in self.sides)
        print(f'layout: {len(self.Q)} L1 quads -> {initial} charts -> {len(regs)} macro charts ({merges} merges)',flush=True)
        return self

if __name__=='__main__':
    from pathlib import Path
    root=Path(__file__).resolve().parents[1]
    for p in sorted((root/'Source').glob('*_reference.npz')):
        print(p.stem);L=Layout(np.load(p)).construct()
