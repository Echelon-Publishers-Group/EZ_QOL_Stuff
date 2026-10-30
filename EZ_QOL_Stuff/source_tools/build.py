import bpy, bmesh, sys, json, math, os
from mathutils import Vector
sys.path.insert(0,os.environ.get('EZ_QOL_TOOLS','tools'))
from a3p3d import data_p3d as P
SRC=os.environ.get('EZ_QOL_SRC','work/EZ_QOL_Stuff'); OUT=os.environ.get('EZ_QOL_OUT','EZ_QOL_Stuff/data')
R=P.P3D_LOD_Resolution
def mesh_data(obj, ratio=1.0, xf=lambda v:v):
    o=obj.copy(); o.data=obj.data.copy(); bpy.context.collection.objects.link(o)
    if ratio<1.0:
        m=o.modifiers.new('d','DECIMATE'); m.ratio=ratio; m.use_collapse_triangulate=True
    dg=bpy.context.evaluated_depsgraph_get(); me=bpy.data.meshes.new_from_object(o.evaluated_get(dg))
    me.transform(obj.matrix_world)
    bm=bmesh.new(); bm.from_mesh(me); bmesh.ops.triangulate(bm,faces=bm.faces[:]); bm.to_mesh(me); bm.free()
    verts=[tuple(xf(Vector(v.co))) for v in me.vertices]
    uvl=me.uv_layers.active.data if me.uv_layers else None
    cn=me.corner_normals
    faces=[]
    for p in me.polygons:
        faces.append(([me.loops[l].vertex_index for l in p.loop_indices],[tuple(cn[l].vector) for l in p.loop_indices],[tuple(uvl[l].uv) if uvl else (0,0) for l in p.loop_indices]))
    bpy.data.objects.remove(o); bpy.data.meshes.remove(me)
    return verts,faces
class LB:
    def __init__(s,lod,res=0): s.L=P.P3D_LOD(); s.L.resolution.set(lod,res); s.sel={}; s.nidx={}; s.uvs=[]; s.mass=None; s.props=[]
    def add(s,verts,faces,names,tex='',mat=''):
        b=len(s.L.verts); s.L.verts+= [(*v,0) for v in verts]
        f0=len(s.L.faces)
        for vi,ns,uv in faces:
            ni=[]
            for n in ns:
                k=tuple(round(c,5) for c in n)
                if k not in s.nidx: s.nidx[k]=len(s.L.normals); s.L.normals.append(k)
                ni.append(s.nidx[k])
            s.L.faces.append([[b+i for i in vi],ni,[(u,1-v) for u,v in uv],tex,mat,0]); s.uvs+=uv
        for n in names: s.sel.setdefault(n,[set(),set()]); s.sel[n][0].update(range(b,len(s.L.verts))); s.sel[n][1].update(range(f0,len(s.L.faces)))
    def point(s,co,names):
        i=len(s.L.verts); s.L.verts.append((*co,0))
        for n in names: s.sel.setdefault(n,[set(),set()])[0].add(i)
    def finish(s):
        L=s.L; nv,nf=len(L.verts),len(L.faces)
        if L.faces:
            t=P.P3D_TAGG(); t.name='#UVSet#'; t.data=P.P3D_TAGG_DataUVSet(); t.data.uvs=s.uvs; L.taggs.append(t)
        if s.mass is not None:
            t=P.P3D_TAGG(); t.name='#Mass#'; t.data=P.P3D_TAGG_DataMass(); t.data.masses=[s.mass/nv]*nv; L.taggs.append(t)
        for k,v in s.props:
            t=P.P3D_TAGG(); t.name='#Property#'; t.data=P.P3D_TAGG_DataProperty(); t.data.key=k; t.data.value=v; L.taggs.append(t)
        for n,(vs,fs) in sorted(s.sel.items()):
            t=P.P3D_TAGG(); t.name=n; d=P.P3D_TAGG_DataSelection(); d.count_verts=nv; d.count_faces=nf
            d.weight_verts=[(i,1) for i in sorted(vs)]; d.weight_faces=[(i,1) for i in sorted(fs)]; t.data=d; L.taggs.append(t)
        return L
def box(mn,mx):
    v=[(x,y,z) for x in (mn[0],mx[0]) for y in (mn[1],mx[1]) for z in (mn[2],mx[2])]
    q=[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]
    return v,[(list(t),[(0,0,1)]*4,[(0,0)]*4) for t in q]
def disc(c,r,z0,z1,n=8):
    v=[]; 
    for z in (z0,z1):
        for i in range(n): a=2*math.pi*i/n; v.append((c[0]+r*math.cos(a),c[1]+r*math.sin(a),z))
    f=[(list(range(n-1,-1,-1))[:],),]
    faces=[([*range(n-1,-1,-1)],[(0,0,-1)]*n,[(0,0)]*n),([*range(n,2*n)],[(0,0,1)]*n,[(0,0)]*n)]
    for i in range(n): j=(i+1)%n; faces.append(([i,j,n+j,n+i],[(0,0,1)]*4,[(0,0)]*4))
    # split ngons into triangles fan (P3D max 4 sides)
    out=[]
    for vi,ns,uv in faces:
        if len(vi)<=4: out.append((vi,ns,uv))
        else:
            for k in range(1,len(vi)-1): out.append(([vi[0],vi[k],vi[k+1]],ns[:3],uv[:3]))
    return v,out
def bounds(vs):
    return [min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]

def build(name):
    bpy.ops.wm.open_mainfile(filepath=f'{SRC}/{name}.blend')
    mems={o.name:Vector(o.matrix_world.translation) for o in bpy.data.objects if o.type=='EMPTY'}
    meshes=[o for o in bpy.data.objects if o.type=='MESH']
    allv=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
    mn,mx=bounds(allv); ctr=Vector([(a+b)/2 for a,b in zip(mn,mx)])
    info={'name':name}
    if name=='ez_qol_drone':
        S=1.0
        mems['light_pos']=Vector((0.0,-0.2515,0.038))   # centre of the round lamp lens (emissive disc, faces -Y)
        mems['light_dir']=mems['light_pos']+Vector((0,-0.1,0))
        tex=r'EZ_QOL_Stuff\data\drone\textures\ez_qol_drone_co.paa'; mat=r'EZ_QOL_Stuff\data\drone\ez_qol_drone.rvmat'
        sels={'body':['body','camo'],'cargo':['cargo','camo']}
        for i in range(1,5): sels[f'rotor_{i}']=[f'rotor_{i}']
        ratios={1:(1,1),2:(0.45,0.3),4:(0.2,0.09)}  # (body/cargo, rotors)
        mass=6.0
    else:
        S=0.25/(mx[1]-mn[1])
        tex=r'EZ_QOL_Stuff\data\tablet\textures\ez_qol_tablet_co.paa'; mat=r'EZ_QOL_Stuff\data\tablet\ez_qol_tablet.rvmat'
        sels={'body':['body','camo'],'screen':['screen']}
        ratios={1:(1,1),2:(0.6,1),4:(0.3,1)}
        mass=1.0
    xf=lambda v:(v-ctr)*S
    info['scale']=S; info['offset']=list(-ctr)
    lods=[]
    for res,(rb,rr) in ratios.items():
        lb=LB(R.VISUAL,res)
        for o in meshes:
            ratio=rr if o.name.startswith('rotor') or o.name=='screen' else rb
            v,f=mesh_data(o,ratio,xf)
            t,m=(tex,mat)
            if o.name=='screen': t,m=r'EZ_QOL_Stuff\data\tablet\textures\ez_qol_tablet_screen_co.paa',r'EZ_QOL_Stuff\data\tablet\ez_qol_tablet_screen.rvmat'
            lb.add(v,f,sels[o.name],t,m)
        lods.append(lb.finish()); info[f'lod{res}_tris']=len(lb.L.faces)
    # simple convex components
    comps=[]
    if name=='ez_qol_drone':
        bodyv=[xf(o.matrix_world@v.co) for o in meshes if o.name=='body' for v in o.data.vertices]
        core=bounds([v for v in bodyv if abs(v.x)<0.15]); comps.append((box(*core),'body'))
        cv=[xf(o.matrix_world@v.co) for o in meshes if o.name=='cargo' for v in o.data.vertices]; comps.append((box(*bounds(cv)),'cargo'))
        for i in range(1,5):
            rv=[xf(o.matrix_world@v.co) for o in meshes if o.name==f'rotor_{i}' for v in o.data.vertices]; a,b=bounds(rv)
            comps.append((disc(((a[0]+b[0])/2,(a[1]+b[1])/2),(b[0]-a[0])/2,a[2],b[2]),f'rotor_{i}'))
    else:
        comps.append((box(*bounds([xf(v) for v in allv])),'body'))
    for kind in (R.VIEW_GEOMETRY,R.GEOMETRY,R.FIRE_GEOMETRY):
        lb=LB(kind)
        for n,((v,f),sel) in enumerate(comps,1): lb.add(v,f,[f'Component{n:02d}',sel])
        if kind==R.GEOMETRY: lb.mass=mass; lb.props=[('autocenter','0')]
        lods.append(lb.finish())
    # memory
    if name=='ez_qol_tablet':
        allx=[xf(v) for v in allv]; a,b=bounds(allx)
        mems={k:xf(v) for k,v in mems.items()}
        mems['boundingbox_min']=Vector(a); mems['boundingbox_max']=Vector(b); mems['ce_center']=Vector((0,0,0))
        sv=[xf(o.matrix_world@v.co) for o in meshes if o.name=='screen' for v in o.data.vertices]
        mems['screen_center']=sum(sv,Vector())/len(sv); mems['invview']=mems['screen_center']+Vector((0.35,0,0))
    else:
        mems={k:xf(v) for k,v in mems.items()}; mems['ce_center']=Vector((0,0,0))
    lb=LB(R.MEMORY)
    for k,v in sorted(mems.items()):
        names=[k]
        if '_axis_' in k: names.append(k.rsplit('_',1)[0])  # rotor_N_axis = begin+end
        lb.point(tuple(v),names)
    lods.append(lb.finish())
    info['memory']={k:[round(c,4) for c in v] for k,v in mems.items()}
    m=P.P3D_MLOD(); m.lods=lods
    sub='drone' if 'drone' in name else 'tablet'
    os.makedirs(f'{OUT}/{sub}',exist_ok=True); m.write_file(f'{OUT}/{sub}/{name}.p3d')
    json.dump(info,open(f'{OUT}/{sub}/{name}_memory_points.json','w'),indent=1)
    print('BUILT',name,json.dumps({k:v for k,v in info.items() if k!='memory'}))
build('ez_qol_drone'); build('ez_qol_tablet')
