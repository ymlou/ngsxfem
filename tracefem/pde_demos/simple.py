# solve the Laplace Beltrami equation on a sphere
# with Dirichlet boundary condition u = 0

from math import pi

# ngsolve stuff
from ngsolve import *

# xfem and trace fem stuff
import libngsxfem_py.xfem as xfem                                 

# geometry stuff
# unit square [-2,2]^2 (boundary index 1 everywhere)
def MakeSquare():
    from netgen.geom2d import SplineGeometry
    square = SplineGeometry()
    square.AddRectangle([-2,-2],[2,2],bc=1)
    return square;

# unit cube [-2,2]^3 (boundary index 1 everywhere)
def MakeCube():
    from netgen.csg import CSGeometry, OrthoBrick, Pnt
    cube = CSGeometry()
    cube.Add (OrthoBrick(Pnt(-2,-2,-2), Pnt(2,2,2)))
    return cube;

# meshing
from netgen.meshing import MeshingParameters
ngsglobals.msg_level = 1
mesh = Mesh (MakeCube().GenerateMesh(maxh=1.0, quad_dominated=False))

# The mesh deformation calculator
from lsetcurv import *
order = 2
lsetmeshadap = LevelSetMeshAdaptation(mesh, order=order, threshold=0.2, discontinuous_qn=True)

# The level set
levelset = sqrt(x*x+y*y+z*z) - 1

# the solution
sol = sin(pi*z)

Vh = H1(mesh, order=order, dirichlet=[1])
Vh_tr = FESpace ("xfespace", mesh=mesh, order=order, flags = {"trace" : True, "dgjumps" : False, "ref_space" : 0})
CastToXFESpace(Vh_tr).SetBaseFESpace(Vh)
CastToXFESpace(Vh_tr).SetLevelSet(levelset)

a = BilinearForm(Vh_tr, symmetric = True, flags = { })
a += BFI (name = "tracemass", coef = 1.0)
a += BFI (name = "tracelaplacebeltrami", coef = 1.0)

f = LinearForm(Vh_tr)
f += LFI (name = "tracesource", coef = (sin(pi*z)*(pi*pi*(1-z*z)+1)+cos(pi*z)*2*pi*z) )

c = Preconditioner(a, type="local", flags= { "test" : True })
#c = Preconditioner(a, type="direct", flags= { "inverse" : "pardiso", "test" : True })

u = GridFunction(Vh_tr)

def SolveProblem():

    # Calculation of the deformation:
    deformation = lsetmeshadap.CalcDeformation(levelset)
    # print ("Computed mesh transformation.")
    lset_ho = lsetmeshadap.lset_ho
    # lsetmeshadap.lset_p1
    print ("Max. dist of discr. intf. to exact level set fct:", lsetmeshadap.CalcMaxDistance(levelset))
    print ("Max. dist of discr. intf. to discr. level set f.:", lsetmeshadap.CalcMaxDistance(lset_ho))
    # Applying the mesh deformation
    mesh.SetDeformation(deformation)

    Vh.Update()
    Vh_tr.Update()
    u.Update()
    a.Assemble();
    f.Assemble();
    c.Update();
    solvea = CGSolver( mat=a.mat, pre=c.mat, complex=False, printrates=False, precision=1e-8, maxsteps=2000)
    # # the boundary value problem to be solved on each level
    u.vec.data = solvea * f.vec;
    print ("Number of iterations:", solvea.GetSteps())

def PostProcess():
    CalcTraceDiff( u, sol, intorder=2*order);
    mesh.UnsetDeformation()
    RefineAtLevelSet(gf=lsetmeshadap.lset_p1)

    
SolveProblem()
PostProcess()
for i in range(4):
    mesh.Refine()
    SolveProblem()
    PostProcess()