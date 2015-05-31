/*
Solver for a parabolic pde

Solves
M du/dt  +  A u = f
by an implicite Euler method

Please include this file to the src files given in netgen/ngsolve/Makefile
*/



#include <solve.hpp>
#include "../spacetime/spacetimeintegrators.hpp"

using namespace ngsolve;



/*
Every solver is a class derived from the class NumProc.
It collects objects (such as bilinear-forms, gridfunctions) and parameters.
*/
template <int D>
class NumProcSolveInstat : public NumProc
{
	protected:
	// // bilinear-form for matrix
	//BilinearForm * bftau;
	// // bilinear-form for the mass-matrix
	// BilinearForm * bfm;
	// linear-form providing the right hand side
    shared_ptr<LinearForm> lff; //initial condition
	// solution vector
    shared_ptr<GridFunction> gfu;
	// time step
	double dt;
	// total time
	double tend;
	// direct solver type
	string inversetype;
	string fesstr;
	
	bool userstepping;
    shared_ptr<PDE> sp_pde;
  
	public:
	/*
	In the constructor, the solver class gets the flags from the pde - input file.
	the PDE class apde constains all bilinear-forms, etc...
	*/
    NumProcSolveInstat (shared_ptr<PDE> apde, const Flags & flags) : NumProc (apde), sp_pde(apde)
	{
		// in the input-file, you specify the bilinear-forms for the stiffness and for the mass-term
		// like  "-bilinearforma=k". Default arguments are 'a' and 'm'

		// bfa = apde->GetBilinearForm (flags.GetStringFlag ("bilinearforma", "a"));
		// bfm = apde->GetBilinearForm (flags.GetStringFlag ("bilinearformm", "m"));
		lff = apde->GetLinearForm (flags.GetStringFlag ("linearform", "f"));
		gfu = apde->GetGridFunction (flags.GetStringFlag ("gridfunction", "u"));

		inversetype = flags.GetStringFlag ("solver", "pardiso");
		fesstr = flags.GetStringFlag ("fespace", "fes_st");

		userstepping = flags.GetDefineFlag ("userstepping");

		dt = flags.GetNumFlag ("dt", 0.001);
		tend = flags.GetNumFlag ("tend", 1);
	}


	/*
	Destructor does nothing
	*/
	virtual ~NumProcSolveInstat() 
	{ ; }


	/*
	creates an solver object
	*/
	static NumProc * Create (PDE & pde, const Flags & flags)
	{
		return new NumProcSolveInstat (pde, flags);
	}


	/*
	solve at one level
	*/
	virtual void Do(LocalHeap & lh)
	{
		cout << "solve solveinstat pde" << endl;

		// PointContainer<2+1> pc;
        // Array<Simplex<2+1> *> ret(0);
        // DecomposeIntoSimplices<2,3>(ET_TRIG,ET_SEGM,ret,pc,lh);

		// PointContainer<3+1> pc2;
        // Array<Simplex<3+1> *> ret2(0);
        // DecomposeIntoSimplices<3,4>(ET_TET,ET_SEGM,ret2,pc2,lh);

		// PointContainer<2> pc3;
        // Array<Simplex<2> *> ret3(0);
        // DecomposeIntoSimplices<2,2>(ET_TRIG,ET_POINT,ret3,pc3,lh);

		// PointContainer<3> pc4;
        // Array<Simplex<3> *> ret4(0);
        // DecomposeIntoSimplices<3,3>(ET_TET,ET_POINT,ret4,pc4,lh);

		shared_ptr<BilinearForm> bftau;
		Flags massflags;
		massflags.SetFlag ("fespace", fesstr.c_str());
		bftau = sp_pde->AddBilinearForm ("bftau", massflags);
		bftau -> SetUnusedDiag (0);

        auto one = make_shared<ConstantCoefficientFunction> (1);
		shared_ptr<BilinearFormIntegrator> bfidt = make_shared<ST_TimeDerivativeIntegrator<D> > (one);
		bftau -> AddIntegrator (bfidt);
		shared_ptr<BilinearFormIntegrator> bfitr = make_shared<SpaceTimeTimeTraceIntegrator<D,PAST> >(one);
		bftau -> AddIntegrator (bfitr);
		Array<shared_ptr<CoefficientFunction> > coef_ar(3);
		coef_ar[0] = make_shared<ConstantCoefficientFunction> (0);
		coef_ar[1] = make_shared<ConstantCoefficientFunction> (dt);
		coef_ar[2] = make_shared<ConstantCoefficientFunction> (1.0);

		shared_ptr<BilinearFormIntegrator> bfilap = make_shared<ST_LaplaceIntegrator<D> > (coef_ar);
		bftau -> AddIntegrator (bfilap);

		bftau -> Assemble(lh);

		// DifferentialOperator * traceop = new SpaceTimeTimeTraceIntegrator<D,FUTURE>(new ConstantCoefficientFunction(1.0));

		shared_ptr<GridFunctionCoefficientFunction> coef_u = make_shared<GridFunctionCoefficientFunction>(gfu); //, traceop);

		shared_ptr<LinearForm> lfrhs;
		Flags massflags2;
		massflags2.SetFlag ("fespace", fesstr.c_str());
		// lfrhs = pde.AddLinearForm ("lfrhs", massflags2);

		shared_ptr<FESpace> fes = gfu->GetFESpace();
		lfrhs = CreateLinearForm(fes,"lfrhs",massflags2);

		Array<shared_ptr<CoefficientFunction> > coef_lr(2);
		coef_lr[0] = make_shared<ConstantCoefficientFunction>(0);
		coef_lr[1] = coef_u;
		shared_ptr<LinearFormIntegrator> lfi_tr = make_shared<ST_TimeTraceSourceIntegrator<D> >(coef_lr);

		lfrhs -> AddIntegrator (lfi_tr);

		
		// reference to the matrices provided by the bi-forms.
		// will be of type SparseSymmetricMatrix<double> for scalar problems
		BaseMatrix & mata = bftau->GetMatrix();

		const BaseVector * vecf = &(lff->GetVector());

		BaseVector & vecu = gfu->GetVector();

		// create additional vectors:
		BaseVector & d = *vecu.CreateVector();
		BaseVector & w = *vecu.CreateVector();

		// set inversetype
		dynamic_cast<BaseSparseMatrix&> (mata) . SetInverseType (inversetype);
		// A sparse matrix can compute a sparse factorization. One has to cast to a sparse matrix:
		BaseMatrix & invmat = * dynamic_cast<BaseSparseMatrix&> (mata) . InverseMatrix(gfu->GetFESpace()->GetFreeDofs());
		// implicite Euler method
		double t;
		for (t = 0; t < tend; t += dt)
		{
			d = *vecf - mata * vecu;
			w = invmat * d;
			vecu += w;

			// update status text
			cout << "\rt = " << t;
	 		cout << flush;
			// update visualization
			Ng_Redraw ();
			if (userstepping)
				getchar();
			lfrhs -> Assemble(lh.Borrow());
			vecf = &(lfrhs -> GetVector());
		}
		cout << "\r               \rt = " << tend;
		cout << endl;
		// delete &d;
		// delete &w;
	}


	/*
	GetClassName
	*/
	virtual string GetClassName () const
	{
	  return "SolveInstat Solver";
	}


	/*
	PrintReport
	*/
	virtual void PrintReport (ostream & ost)
	{
	  ost << GetClassName() << endl
	<< "Linear-form     = " << lff->GetName() << endl
	<< "Gridfunction    = " << gfu->GetName() << endl
	<< "dt              = " << dt << endl
	<< "tend            = " << tend << endl;
	}


	/*
	PrintDoc
	*/
	static void PrintDoc (ostream & ost)
	{
		ost << 
			"\n\nNumproc SolveInstat:\n" \
			"------------------\n" \
			"Solves a solveinstat partial differential equation by an implicite Euler method\n\n" \
			"Required flags:\n" 
			"-linearform=<lfname>\n" \
			"    linear-form providing the right hand side\n" \
			"-gridfunction=<gfname>\n" \
			"    grid-function to store the solution vector\n" 
			"-dt=<value>\n"
			"    time step\n"
			"-tend=<value>\n"
			"    total time\n"
		<< endl;
	}
};


static RegisterNumProc<NumProcSolveInstat<2> > npst_solveinstat2d("st_solveinstat");
static RegisterNumProc<NumProcSolveInstat<3> > npst_solveinstat3d("st_solveinstat");
