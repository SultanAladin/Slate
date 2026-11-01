"""Export real cubic NURBS B-rep sheets as AP214 STEP and native SolidArc.
Shared EDGE_CURVE instances implement T-junctions without splitting a surface
into extra faces. Sheets remain sheets; no invented underside or thickness.
"""
from pathlib import Path
from collections import Counter,defaultdict
import json,pickle,sys
import numpy as np
from fit_macros import Fitter,evaluate,ROOT
from scipy.interpolate import BSpline
# Caches built by running fit_macros.py have a __main__.Fitter qualifier.
sys.modules['__main__'].Fitter=Fitter

class Writer:
    def __init__(self):self.lines=[];self.vmap={};self.emap={}
    def add(self,s):
        r=f'#{len(self.lines)+1}';self.lines.append(f'{r}={s};');return r
    def num(self,x):
        if abs(x)<1e-14:x=0.
        t=format(float(x),'.16g');return t if '.' in t or 'e' in t else t+'.'
    def point(self,p):
        p=p.copy()
        if abs(p[1])<1e-7:p[1]=0.
        return self.add("CARTESIAN_POINT('',("+','.join(self.num(x*1000) for x in p)+'))')
    def vertex(self,p):
        p=p.copy()
        if abs(p[1])<1e-7:p[1]=0.
        key=(getattr(self,'vertex_namespace',0),tuple(np.round(p,9)))
        if key not in self.vmap:self.vmap[key]=self.add(f"VERTEX_POINT('',{self.point(p)})")
        return self.vmap[key]
    def knot(self,k):
        c=Counter(k)
        return '('+','.join(str(c[t]) for t in sorted(c))+')','('+','.join(self.num(t) for t in sorted(c))+')'
    def curve(self,P,K):
        refs=','.join(self.point(p) for p in P);m,k=self.knot(K)
        return self.add(f"B_SPLINE_CURVE_WITH_KNOTS('',3,({refs}),.UNSPECIFIED.,.F.,.F.,{m},{k},.UNSPECIFIED.)")
    def surface(self,P,ku,kv):
        rows=','.join('('+','.join(self.point(p) for p in row)+')' for row in P)
        mu,u=self.knot(ku);mv,v=self.knot(kv)
        return self.add(f"B_SPLINE_SURFACE_WITH_KNOTS('',3,3,({rows}),.UNSPECIFIED.,.F.,.F.,.F.,{mu},{mv},{u},{v},.UNSPECIFIED.)")
    def context(self,name):
        app=self.add("APPLICATION_CONTEXT('automotive_design')")
        self.add(f"APPLICATION_PROTOCOL_DEFINITION('international standard','automotive_design',2000,{app})")
        pc=self.add(f"PRODUCT_CONTEXT('',{app},'mechanical')")
        prod=self.add(f"PRODUCT('{name}','{name}','',({pc}))")
        self.add(f"PRODUCT_RELATED_PRODUCT_CATEGORY('part',$,({prod}))")
        form=self.add(f"PRODUCT_DEFINITION_FORMATION('','',{prod})")
        dc=self.add(f"PRODUCT_DEFINITION_CONTEXT('part definition',{app},'design')")
        pd=self.add(f"PRODUCT_DEFINITION('design','',{form},{dc})")
        shape=self.add(f"PRODUCT_DEFINITION_SHAPE('',{pd})")
        lu=self.add('( LENGTH_UNIT() NAMED_UNIT(*) SI_UNIT(.MILLI.,.METRE.) )')
        au=self.add('( NAMED_UNIT(*) PLANE_ANGLE_UNIT() SI_UNIT($,.RADIAN.) )')
        su=self.add('( NAMED_UNIT(*) SI_UNIT($,.STERADIAN.) SOLID_ANGLE_UNIT() )')
        unc=self.add(f"UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(0.00001),{lu},'distance_accuracy_value','confusion accuracy')")
        ctx=self.add(f"( GEOMETRIC_REPRESENTATION_CONTEXT(3) GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT(({unc})) GLOBAL_UNIT_ASSIGNED_CONTEXT(({lu},{au},{su})) REPRESENTATION_CONTEXT('Context3D','3D Context with UNIT and UNCERTAINTY') )")
        return shape,ctx
    def part(self,name,F,fit_statistics=True):
        faces=[];edgeuses=Counter();face_edges=[];gaps=[]
        owners_half=defaultdict(list)
        for p,ss in zip(F.patches,F.sideinfo):
            for _,_,refs,_ in ss:
                for key,rev in refs:owners_half[key].append(p['id'])
        # A source edge with >2 incident sheets is not a manifold seam.
        # Retain two incidences; detach each additional branch with its own
        # coincident edge. Record this explicitly instead of hiding the defect.
        detached={(key,pid) for key,ids in owners_half.items() for pid in ids[2:]}
        detached_faces={pid for key,pid in detached}
        for mirrored in (False,True):
            sign=np.array([1.,-1. if mirrored else 1.,1.])
            for patch,ss in zip(F.patches,F.sideinfo):
                self.vertex_namespace=(name,patch['id']) if patch['id'] in detached_faces else (name,'main')
                P=patch['P']*sign;surf=self.surface(P,patch['ku'],patch['kv'])
                uses=[];uedges=[]
                for j,(_,params,refs,stops) in enumerate(ss):
                    for (key,rev),a,b in zip(refs,stops[:-1],stops[1:]):
                        r=F.rails[key];rp=r['P']*sign;K=r['K']
                        # Merge true symmetry rails, not coincident unrelated trim lines.
                        symmetric=np.max(np.abs(rp[:,1]))<1e-7
                        ek=(name,key,False if symmetric else mirrored,patch['id'] if patch['id'] in detached_faces else -1)
                        if ek not in self.emap:
                            vv0=self.vertex(rp[0]);vv1=self.vertex(rp[-1])
                            self.emap[ek]=self.add(f"EDGE_CURVE('',{vv0},{vv1},{self.curve(rp,K)},.T.)")
                        e=self.emap[ek];sense=not rev
                        uses.append((e,sense));uedges.append(e);edgeuses[e]+=1
                        t=np.linspace(0,1,65);side_t=a+(b-a)*t
                        chart=[np.c_[side_t,np.zeros_like(t)],np.c_[np.ones_like(t),side_t],np.c_[1-side_t,np.ones_like(t)],np.c_[np.zeros_like(t),1-side_t]][j]
                        rail=BSpline(K,rp[::-1] if rev else rp,3) # reversing knots too below
                        if rev:rail=BSpline(1-K[::-1],rp[::-1],3)
                        diff=evaluate(P,patch['ku'],patch['kv'],chart)-rail(t)
                        gaps.append(float(np.max(np.linalg.norm(diff,axis=1))*1000))
                if mirrored:uses=[(e,not s) for e,s in reversed(uses)]
                o=[self.add(f"ORIENTED_EDGE('',*,*,{e},{'.T.' if s else '.F.'})") for e,s in uses]
                loop=self.add("EDGE_LOOP('',("+','.join(o)+'))')
                bound=self.add(f"FACE_OUTER_BOUND('',{loop},.T.)")
                face=self.add(f"ADVANCED_FACE('{name}_{'L' if mirrored else 'R'}_{patch['id']:03d}',({bound}),{surf},{'.F.' if mirrored else '.T.'})")
                faces.append(face);face_edges.append(uedges)
        assert max(gaps)<1e-5, f'Boundary mismatch {max(gaps)} mm'
        # Connected components become separate OPEN_SHELLs within a part.
        owners=defaultdict(list)
        for i,es in enumerate(face_edges):
            for e in es:owners[e].append(i)
        unseen=set(range(len(faces)));shells=[]
        while unseen:
            seed=min(unseen);unseen.remove(seed);todo=[seed];comp=[]
            while todo:
                i=todo.pop();comp.append(faces[i])
                for e in face_edges[i]:
                    for nb in owners[e]:
                        if nb in unseen:unseen.remove(nb);todo.append(nb)
            shells.append(self.add("OPEN_SHELL('',("+','.join(comp)+'))'))
        model=self.add(f"SHELL_BASED_SURFACE_MODEL('{name}',({','.join(shells)}))")
        if not fit_statistics:
            return model, {'name':name,'faces':len(faces),
                'control_poles':sum(p['P'].shape[0]*p['P'].shape[1]*2 for p in F.patches),
                'master_rails_half':len(F.rails),'sewn_edges':sum(v==2 for v in edgeuses.values()),
                'open_edges':sum(v==1 for v in edgeuses.values()),
                'nonmanifold_edges':sum(v>2 for v in edgeuses.values()),'open_shells':len(shells),
                'source_nonmanifold_branches_detached':len(detached)*2,'boundary_max_mm':max(gaps)}
        held=np.concatenate([p['error'][~p['train']] for p in F.patches])*1000
        poles=sum(p['P'].shape[0]*p['P'].shape[1]*2 for p in F.patches)
        stats={'name':name,'faces':len(faces),'control_poles':poles,'master_rails_half':len(F.rails),
               'sewn_edges':sum(v==2 for v in edgeuses.values()),'open_edges':sum(v==1 for v in edgeuses.values()),
               'nonmanifold_edges':sum(v>2 for v in edgeuses.values()),'open_shells':len(shells),
               'source_nonmanifold_branches_detached':len(detached)*2,
               'boundary_max_mm':max(gaps),'heldout_samples_half':len(held),
               'heldout_rms_mm':float(np.sqrt(np.mean(held**2))),'heldout_p99_mm':float(np.percentile(held,99)),
               'heldout_max_mm':float(held.max()),
               'patches_over_0_5mm_rms':sum(p['history'][-1]['heldout_rms_mm']>0.5 for p in F.patches),
               'patches':[{'id':p['id'],'poles':list(p['P'].shape[:2]),'min_chart_jacobian':p['min_chart_jacobian'],**p['history'][-1]} for p in F.patches]}
        return model,stats
    def finish(self,name,shape,ctx,models):
        rep=self.add(f"MANIFOLD_SURFACE_SHAPE_REPRESENTATION('{name}',({','.join(models)}),{ctx})")
        self.add(f'SHAPE_DEFINITION_REPRESENTATION({shape},{rep})')
        return f"ISO-10303-21;\nHEADER;\nFILE_DESCRIPTION(('Liger macro NURBS open sheets'),'2;1');\nFILE_NAME('{name}.step','2026-11-01T00:00:00',(''),(''),'Slate','SolidArc','');\nFILE_SCHEMA(('AUTOMOTIVE_DESIGN {{ 1 0 10303 214 1 1 1 1 }}'));\nENDSEC;\nDATA;\n"+'\n'.join(self.lines)+'\nENDSEC;\nEND-ISO-10303-21;\n'

def main():
    names=['Body_Main_Shell','Body_Front_Cowl','Body_Roof_Glass_Frame']
    allstats=[];steps=[];full=Writer();shape,ctx=full.context('Liger_Macro_CAD');models=[]
    for i,name in enumerate(names):
        with (ROOT/'CAD'/f'{name}_fit.pkl').open('rb') as s:F=pickle.load(s)
        w=Writer();sh,cx=w.context(name);m,stats=w.part(name,F);text=w.finish(name,sh,cx,[m])
        (ROOT/'CAD'/f'{name}.step').write_text(text)
        model,_=full.part(name,F);models.append(model);allstats.append(stats)
        steps.append(f'[[Steps]]\nKind = 18\nLabel = "LigerPart{i+1}"\nCaption = "{name}"\nExchangeUnit = 1000.0\nExchange = """\n{text}"""\n')
        print(name,json.dumps({k:v for k,v in stats.items() if k!='patches'}),flush=True)
    (ROOT/'CAD/Liger_Macro_CAD.step').write_text(full.finish('Liger_Macro_CAD',shape,ctx,models))
    doc='Format = "SolidArc"\nFormatVersion = 1\nTitle = "Liger — crease-preserving macro CAD (review build)"\nDescription = "Three open NURBS sheet parts; see validation report for fitting deviations and exclusions."\n\n'+'\n'.join(steps)
    (ROOT/'CAD/Liger_Macro_CAD.solidarc').write_text(doc)
    (ROOT/'CAD/metrics.json').write_text(json.dumps({'parts':allstats},indent=2)+'\n')
if __name__=='__main__':main()
