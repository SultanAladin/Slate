"""Small deterministic regression checks; run python Tools/test_pipeline.py."""
import unittest
from collections import defaultdict
import numpy as np
from macro_layout import Layout,edge
from fit_macros import Fitter,basis,insert_to,union,evaluate
from export_cad import Writer


def grid(nx=4,ny=3,crease=False):
    V=np.array([[i/nx,j/ny,0.] for i in range(nx+1) for j in range(ny+1)])
    Q=[];S=[];edges=set()
    for i in range(nx):
        for j in range(ny):
            a=i*(ny+1)+j;b=(i+1)*(ny+1)+j
            q=[a,b,b+1,a+1];Q.append(q)
            for u,v in zip(q,q[1:]+q[:1]):edges.add(edge(u,v))
            sample=np.array([[(1-u)*(1-v)*V[a]+u*(1-v)*V[b]+u*v*V[b+1]+(1-u)*v*V[a+1] for v in np.linspace(0,1,5)] for u in np.linspace(0,1,5)])
            S.append(sample)
    E=sorted(edges);c=np.array([1. if crease and abs(V[a,0]-.5)<1e-9 and abs(V[b,0]-.5)<1e-9 else 0. for a,b in E])
    return {'V':V,'Q':np.array(Q),'S':np.array(S),'E':np.array(E),'crease':c}

class PipelineTests(unittest.TestCase):
    def test_grid_merges(self):
        L=Layout(grid()).construct();self.assertEqual(len(L.regs),1)
        self.assertEqual(sum(map(len,L.regs)),len(L.Q))
    def test_crease_is_preserved(self):
        L=Layout(grid(crease=True)).construct();self.assertEqual(len(L.regs),2)
        self.assertTrue(L.features.issubset(L.cuts))
    def test_planar_fit(self):
        F=Fitter(Layout(grid()).construct()).fit()
        self.assertEqual(len(F.patches),1)
        self.assertLess(F.patches[0]['error'].max(),1e-5)
        w=Writer();model,stats=w.part('TestPlane',F)
        self.assertEqual(stats['nonmanifold_edges'],0)
        self.assertLess(stats['boundary_max_mm'],1e-7)
    def test_knot_insertion_preserves_curve(self):
        k=np.r_[np.zeros(4),np.ones(4)];P=np.array([[0,0,0],[1,.1,0],[2,.1,.1],[3,0,0.]])
        target=np.sort(np.r_[k,.25,.5,.75]);Q=insert_to(P,k,target);t=np.linspace(0,1,101)
        self.assertLess(np.max(np.linalg.norm(basis(t,k)@P-basis(t,target)@Q,axis=1)),1e-12)
    def test_shared_rail_export(self):
        F=Fitter(Layout(grid(crease=True)).construct()).fit()
        w=Writer();_,stats=w.part('TestCrease',F)
        self.assertLess(stats['boundary_max_mm'],1e-7)
        self.assertEqual(stats['nonmanifold_edges'],0)
        self.assertGreater(stats['sewn_edges'],0)

if __name__=='__main__':unittest.main(verbosity=2)
