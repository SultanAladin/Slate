"""Boundary-first constrained cubic B-spline approximation of topological macros.
Shared master rails are solved ONCE. Opposite-side knot vectors are made
compatible by exact knot insertion, never by independently refitting edges.
Metrics are correspondence residuals to held-out L3 limit-position samples,
not a certified global Hausdorff bound or Class-A certificate.
"""
import json
from pathlib import Path
from collections import Counter,defaultdict
import numpy as np
from scipy.interpolate import BSpline, CubicSpline
from scipy.sparse import coo_matrix, csr_matrix, vstack, eye
from scipy.sparse.linalg import spsolve, lsmr
from macro_layout import Layout,edge

ROOT=Path(__file__).resolve().parents[1]

def knots(n):return np.r_[np.zeros(4),np.linspace(0,1,n-2)[1:-1],np.ones(4)]
def basis(t,k):return BSpline.design_matrix(np.clip(t,0,1),k,3).tocsr()
def evaluate(P,ku,kv,uv):
    return tensor(uv,ku,kv)@P.reshape(-1,3)
def tensor(uv,ku,kv):
    a=basis(uv[:,0],ku);b=basis(uv[:,1],kv)
    aa=a.indices.reshape(-1,4);bb=b.indices.reshape(-1,4)
    av=a.data.reshape(-1,4);bv=b.data.reshape(-1,4)
    cols=(aa[:,:,None]*(len(kv)-4)+bb[:,None,:]).reshape(-1)
    vals=(av[:,:,None]*bv[:,None,:]).reshape(-1)
    rows=np.repeat(np.arange(len(uv)),16)
    return coo_matrix((vals,(rows,cols)),shape=(len(uv),(len(ku)-4)*(len(kv)-4))).tocsr()

def project(P,ku,kv,xyz,uv,iterations=12):
    uv=uv.copy()
    su=BSpline(ku,np.eye(len(ku)-4),3);sv=BSpline(kv,np.eye(len(kv)-4),3)
    for _ in range(iterations):
        Bu=su(uv[:,0]);Bv=sv(uv[:,1]);Du=su(uv[:,0],nu=1);Dv=sv(uv[:,1],nu=1)
        def ev(A,B):return np.einsum('ni,nj,ijc->nc',A,B,P,optimize=True)
        pos=ev(Bu,Bv);a=ev(Du,Bv);b=ev(Bu,Dv);r=pos-xyz
        aa=np.sum(a*a,axis=1)+1e-16;bb=np.sum(b*b,axis=1)+1e-16;ab=np.sum(a*b,axis=1)
        ar=np.sum(a*r,axis=1);br=np.sum(b*r,axis=1);det=np.maximum(aa*bb-ab*ab,1e-24)
        d=np.c_[(bb*ar-ab*br)/det,(aa*br-ab*ar)/det]
        d=np.clip(d,-0.05,0.05)
        # Backtracking prevents jumps across a thin folded feature.
        old=np.sum(r*r,axis=1);best=uv.copy();besterr=old
        for scale in (1.,0.5,0.25,0.125):
            cand=np.clip(uv-scale*d,0,1)
            e=np.sum((evaluate(P,ku,kv,cand)-xyz)**2,axis=1)
            good=e<besterr;best[good]=cand[good];besterr[good]=e[good]
        move=np.max(np.abs(best-uv));uv=best
        if move<1e-8:break
    return uv

def insert_to(P,k,target):
    s=BSpline(k,P,3)
    counts=Counter(k)
    for t,n in sorted(Counter(target).items()):
        for _ in range(n-counts[t]):s=s.insert_knot(t)
    return s.c[:len(s.t)-4]
def union(*vectors):
    c=Counter()
    for v in vectors:c|=Counter(v)
    return np.array(sorted(c.elements()))
def reverse(c):return {'P':c['P'][::-1].copy(),'K':1-c['K'][::-1],'length':c['length']}
def join(curves):
    lengths=np.array([c['length'] for c in curves]);stops=np.r_[0,np.cumsum(lengths)/sum(lengths)];stops[-1]=1
    P=[];K=[0.]*4
    for i,(c,a,b) in enumerate(zip(curves,stops[:-1],stops[1:])):
        P.extend(c['P'] if i==0 else c['P'][1:])
        K.extend(a+(b-a)*c['K'][4:-4]);K.extend([b]*(4 if i==len(curves)-1 else 3))
    return {'P':np.array(P),'K':np.array(K),'length':sum(lengths)},stops

class Fitter:
    def __init__(self,L):
        self.L=L;self.rails={};self.sideinfo=[];self.patches=[]
        self.esamples={}
        for qi,q in enumerate(L.Q):
            s=L.S[qi]
            for (a,b),pts in zip(zip(q,np.roll(q,-1)),(s[:,0],s[4,:],s[::-1,4],s[0,::-1])):
                a,b=int(a),int(b);key=edge(a,b)
                canon=pts if a<b else pts[::-1]
                if key in self.esamples:
                    assert np.max(np.linalg.norm(self.esamples[key]-canon,axis=1))<1e-6
                self.esamples[key]=canon.copy()
        self.corners={v for sides in L.sides for s in sides for v in (s[0],s[-1])}

    def rail(self,vs):
        vs=tuple(vs);key=min(vs,vs[::-1]);rev=vs!=key
        if key not in self.rails:
            pts=[];vertex_offsets=[0]
            for a,b in zip(key[:-1],key[1:]):
                p=self.esamples[edge(a,b)]
                if a>b:p=p[::-1]
                if not pts:pts.extend(p)
                else:pts.extend(p[1:])
                vertex_offsets.append(len(pts)-1)
            pts=np.array(pts);d=np.r_[0,np.cumsum(np.linalg.norm(np.diff(pts,axis=0),axis=1))]
            length=d[-1]
            assert length>1e-9
            t=d/length
            keep=np.r_[True,np.diff(t)>1e-12];pts=pts[keep];tt=t[keep]
            k=knots(4)
            for attempt in range(36):
                n=len(k)-4;B=basis(tt,k);P=np.zeros((n,3));P[[0,-1]]=pts[[0,-1]]
                rhs=pts-B@P;A=B[:,1:-1].toarray()
                D=np.diff(np.eye(n),n=2,axis=0);weight=1e-10
                P[1:-1]=np.linalg.lstsq(np.vstack((A,weight**0.5*D[:,1:-1])),np.vstack((rhs,-weight**0.5*D@P)),rcond=1e-10)[0]
                err=np.linalg.norm(B@P-pts,axis=1)
                if err.max()<0.00015:break
                # Local knot at maximum-error parameter; simple multiplicity,
                # never C0 joins inside an otherwise smooth master rail.
                w=int(np.argmax(err));loc=float(tt[w])
                distinct=np.unique(k);j=np.clip(np.searchsorted(distinct,loc,side='right')-1,0,len(distinct)-2)
                loc=float((distinct[j]+distinct[j+1])/2)
                k=np.sort(np.r_[k,loc])
            r={'P':P,'K':k,'length':length,'vertices':list(key),'vt':t[vertex_offsets],
               'sample_max_mm':float(err.max()*1000)}
            self.rails[key]=r
        r=self.rails[key]
        return key,rev,(reverse(r) if rev else r)

    def side(self,vs):
        indices=[i for i,v in enumerate(vs) if v in self.corners]
        pieces=[];refs=[]
        for a,b in zip(indices[:-1],indices[1:]):
            key,rev,r=self.rail(vs[a:b+1]);pieces.append(r);refs.append((key,rev))
        curve,stops=join(pieces)
        params={}
        for (key,rev),a,b in zip(refs,stops[:-1],stops[1:]):
            r=self.rails[key];vv=r['vertices'];tt=r['vt']
            if rev:vv=vv[::-1];tt=1-tt[::-1]
            params.update({v:float(a+(b-a)*t) for v,t in zip(vv,tt)})
        return curve,params,refs,stops

    def parameterize(self,reg,sides):
        L=self.L;rs=set(reg);vids=sorted({int(v) for qi in reg for v in L.Q[qi]});index={v:i for i,v in enumerate(vids)}
        fixed={}
        for j,(_,p,_,_) in enumerate(sides):
            for v,t in p.items():
                fixed[v]=[(t,0),(1,t),(1-t,1),(0,1-t)][j]
        # Positive graph Laplacian of triangulated chart: square boundary fixed.
        nbs=defaultdict(set)
        for qi in reg:
            q=list(map(int,L.Q[qi]))
            for a,b in [(q[0],q[1]),(q[1],q[2]),(q[2],q[3]),(q[3],q[0]),(q[0],q[2])]:
                nbs[a].add(b);nbs[b].add(a)
        interior=[v for v in vids if v not in fixed];ii={v:i for i,v in enumerate(interior)}
        rows=[];cols=[];vals=[];rhs=np.zeros((len(interior),2))
        for v,i in ii.items():
            rows.append(i);cols.append(i);vals.append(len(nbs[v]))
            for w in nbs[v]:
                if w in ii:rows.append(i);cols.append(ii[w]);vals.append(-1)
                else:rhs[i]+=fixed[w]
        uv=np.zeros((len(vids),2))
        for v,p in fixed.items():uv[index[v]]=p
        if interior:
            A=coo_matrix((vals,(rows,cols)),shape=(len(ii),len(ii))).tocsc()
            sol=spsolve(A,rhs)
            for v,i in ii.items():uv[index[v]]=sol[i]
        q_uv=np.array([[uv[index[int(v)]] for v in L.Q[qi]] for qi in reg])
        # Verify both triangles of every L1 quad keep the oriented chart.
        def cross(a,b):return a[...,0]*b[...,1]-a[...,1]*b[...,0]
        jac1=cross(q_uv[:,1]-q_uv[:,0],q_uv[:,2]-q_uv[:,0])
        jac2=cross(q_uv[:,2]-q_uv[:,0],q_uv[:,3]-q_uv[:,0])
        if min(jac1.min(),jac2.min()) < -1e-10:raise ValueError('Folded harmonic chart')
        u,v=np.meshgrid(np.linspace(0,1,5),np.linspace(0,1,5),indexing='ij')
        weights=np.stack(((1-u)*(1-v),u*(1-v),u*v,(1-u)*v),axis=-1)
        sample_uv=np.einsum('ijk,nkd->nijd',weights,q_uv).reshape(-1,2)
        xyz=L.S[reg].reshape(-1,3)
        train=np.tile(((np.indices((5,5)).sum(axis=0)%2)==0).ravel(),len(reg))
        return sample_uv,xyz,train,float(min(jac1.min(),jac2.min()))

    def fit(self):
        for idx,(reg,sv) in enumerate(zip(self.L.regs,self.L.sides)):
            sides=[self.side(vs) for vs in sv]
            self.sideinfo.append(sides)
            # b,t run in +u, l,r run in +v. Boundary loop itself is CCW.
            bottom=sides[0][0];right=sides[1][0];top=reverse(sides[2][0]);left=reverse(sides[3][0])
            ku=union(bottom['K'],top['K']);kv=union(left['K'],right['K'])
            uv,xyz,train,jac=self.parameterize(reg,sides)
            # Adaptive uniform refinement of parameter axes, retaining exact rims.
            history=[]
            for attempt in range(4):
                nu,nv=len(ku)-4,len(kv)-4
                P=np.zeros((nu,nv,3))
                P[:,0]=insert_to(bottom['P'],bottom['K'],ku)
                P[:,-1]=insert_to(top['P'],top['K'],ku)
                P[0]=insert_to(left['P'],left['K'],kv)
                P[-1]=insert_to(right['P'],right['K'],kv)
                boundary_P=P.copy()
                for iteration in range(4):
                    B=tensor(uv[train],ku,kv)
                    inner=np.ones((nu,nv),bool);inner[[0,-1]]=False;inner[:,[0,-1]]=False
                    free=np.where(inner.ravel())[0]
                    A=B[:,free];rhs=xyz[train]-B@boundary_P.reshape(-1,3)
                    # A small ridge resolves underdetermined tiny charts without
                    # perturbing their exact, locked boundary poles.
                    # Thin-plate style pole-net fairing: eliminate unsupported oscillations.
                    from scipy.sparse import kron
                    Du=csr_matrix(np.diff(np.eye(nu),n=2,axis=0))
                    Dv=csr_matrix(np.diff(np.eye(nv),n=2,axis=0))
                    D=vstack((kron(Du,eye(nv)),kron(eye(nu),Dv))).tocsr()
                    DF=D[:,free];weight=1e-5
                    ata=(A.T@A+weight*(DF.T@DF)+eye(len(free))*1e-10).tocsc()
                    sol=spsolve(ata,A.T@rhs-weight*DF.T@(D@boundary_P.reshape(-1,3)))
                    P.reshape(-1,3)[free]=sol
                    uv=project(P,ku,kv,xyz,uv,iterations=8)
                err=np.linalg.norm(evaluate(P,ku,kv,uv)-xyz,axis=1)
                held=err[~train];rms=float(np.sqrt(np.mean(held**2)));mx=float(held.max())
                history.append({'poles':[nu,nv],'heldout_rms_mm':rms*1000,'heldout_max_mm':mx*1000})
                if rms<=0.0005 and mx<=0.005:break
                if attempt==3 or nu>55 or nv>55:break
                # Add SIMPLE internal knots, so added spans are C2, not C0.
                add=np.linspace(0,1,2**(attempt+2)+1)[1:-1]
                ku=union(ku,np.r_[np.zeros(4),add,np.ones(4)])
                kv=union(kv,np.r_[np.zeros(4),add,np.ones(4)])
            rec={'id':idx,'P':P,'ku':ku,'kv':kv,'uv':uv,'xyz':xyz,'train':train,'error':err,
                 'reg':np.array(reg),'sides':sv,'history':history,'min_chart_jacobian':jac}
            self.patches.append(rec)
            print(f' fit {idx+1}/{len(self.L.regs)}: {nu}x{nv} poles, held RMS {rms*1000:.3f} mm max {mx*1000:.3f} mm',flush=True)
        return self

if __name__=='__main__':
    import argparse,pickle
    p=argparse.ArgumentParser();p.add_argument('--part',default='all');a=p.parse_args()
    for name in ['Body_Main_Shell','Body_Front_Cowl','Body_Roof_Glass_Frame']:
        if a.part not in ('all',name):continue
        L=Layout(np.load(ROOT/'Source'/f'{name}_reference.npz')).construct()
        F=Fitter(L).fit()
        # Scratch cache is NOT a deliverable. Run exporters in the same env.
        out=ROOT/'CAD'/f'{name}_fit.pkl'
        with out.open('wb') as s:pickle.dump(F,s)
        print('wrote',out,flush=True)
