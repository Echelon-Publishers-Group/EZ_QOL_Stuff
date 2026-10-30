import sys,glob,os
sys.path.insert(0,os.environ.get('EZ_QOL_TOOLS','tools'))
from a3p3d import data_p3d as P
import numpy as np
for f in sorted(glob.glob(os.environ.get('EZ_QOL_OUT','EZ_QOL_Stuff/data')+'/*/*.p3d')):
    m=P.P3D_MLOD.read_file(f); print('\n==',f.split('/')[-1],'MLOD v',m.version,'LODs',len(m.lods))
    for L in m.lods:
        lod,res=L.resolution.get(); V=np.array([v[:3] for v in L.verts])
        sels=[t.name for t in L.taggs if t.is_selection()]; other=[t.name for t in L.taggs if not t.is_selection()]
        nm={0:f'LOD {res}',6:'Geometry',9:'Memory',14:'ViewGeometry',15:'FireGeometry'}[lod]
        print(f' [{nm}] res={float(L.resolution):g} verts={len(V)} faces={len(L.faces)} size(xyz,m)={np.round(V.max(0)-V.min(0),3).tolist()} min={np.round(V.min(0),3).tolist()} max={np.round(V.max(0),3).tolist()}')
        print('   selections:',sels,' taggs:',other)
        for t in L.taggs:
            if t.name=='#Mass#': print('   mass total %.2f kg'%sum(t.data.masses))
            if t.name=='#Property#': print('   property',t.data.key,'=',t.data.value)
        if lod==9:
            for t in L.taggs:
                if t.is_selection(): print('    ',t.name,[np.round(V[i],4).tolist() for i,_ in t.data.weight_verts])
        if lod in (6,14,15):
            bad=0
            for t in L.taggs:
                if t.name.startswith('Component'):
                    idx=[i for i,_ in t.data.weight_verts]; s=set(idx); C=V[idx]
                    for fc in L.faces:
                        if fc[0][0] in s:
                            p=V[fc[0]]; n=np.cross(p[1]-p[0],p[2]-p[0]); d=(C-p[0])@n
                            if d.max()>1e-7: bad+=1
            print('   convex/outward-winding violations:',bad)
