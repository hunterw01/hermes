#define H2D_REPORT_WARN
#define H2D_REPORT_INFO
#define H2D_REPORT_VERBOSE
#define H2D_REPORT_FILE "application.log"
#include "hermes2d.h"

using namespace RefinementSelectors;

// This test makes sure that example 14-hcurl-adapt works correctly.

const int P_INIT = 2;                             // Initial polynomial degree. NOTE: The meaning is different from
                                                  // standard continuous elements in the space H1. Here, P_INIT refers
                                                  // to the maximum poly order of the tangential component, and polynomials
                                                  // of degree P_INIT + 1 are present in element interiors. P_INIT = 0
                                                  // is for Whitney elements.
const int INIT_REF_NUM = 1;                       // Number of initial uniform mesh refinements.
const double THRESHOLD = 0.3;                     // This is a quantitative parameter of the adapt(...) function and
                                                  // it has different meanings for various adaptive strategies (see below).
const int STRATEGY = 1;                           // Adaptive strategy:
                                                  // STRATEGY = 0 ... refine elements until sqrt(THRESHOLD) times total
                                                  //   error is processed. If more elements have similar errors, refine
                                                  //   all to keep the mesh symmetric.
                                                  // STRATEGY = 1 ... refine all elements whose error is larger
                                                  //   than THRESHOLD times maximum element error.
                                                  // STRATEGY = 2 ... refine all elements whose error is larger
                                                  //   than THRESHOLD.
                                                  // More adaptive strategies can be created in adapt_ortho_h1.cpp.
const CandList CAND_LIST = H2D_HP_ANISO;          // Predefined list of element refinement candidates. Possible values are
                                                  // H2D_P_ISO, H2D_P_ANISO, H2D_H_ISO, H2D_H_ANISO, H2D_HP_ISO,
                                                  // H2D_HP_ANISO_H, H2D_HP_ANISO_P, H2D_HP_ANISO.
                                                  // See User Documentation for details.
const int MESH_REGULARITY = -1;                   // Maximum allowed level of hanging nodes:
                                                  // MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
                                                  // MESH_REGULARITY = 1 ... at most one-level hanging nodes,
                                                  // MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
                                                  // Note that regular meshes are not supported, this is due to
                                                  // their notoriously bad performance.
const double CONV_EXP = 1.0;                      // Default value is 1.0. This parameter influences the selection of
                                                  // cancidates in hp-adaptivity. See get_optimal_refinement() for details.
const double ERR_STOP = 1.0;                      // Stopping criterion for adaptivity (rel. error tolerance between the
                                                  // reference mesh and coarse mesh solution in percent).
const int NDOF_STOP = 60000;                      // Adaptivity process stops when the number of degrees of freedom grows
                                                  // over this limit. This is to prevent h-adaptivity to go on forever.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_UMFPACK, SOLVER_PETSC,
                                                  // SOLVER_MUMPS, and more are coming.

// Problem parameters.
const double mu_r   = 1.0;
const double kappa  = 1.0;
const double lambda = 1.0;

// Bessel functions, exact solution, and weak forms.
#include "forms.cpp"

// Boundary condition types.
BCType bc_types(int marker)
{
  if (marker == 1 || marker == 6)
    return BC_ESSENTIAL; // perfect conductor
  else
    return BC_NATURAL;   // impedance
}

// Essential (Dirichlet) boundary condition values.
scalar essential_bc_values(int ess_bdy_marker, double x, double y)
{
  return 0;
}

int main(int argc, char* argv[])
{
  // Time measurement
  TimePeriod cpu_time;
  cpu_time.tick();

  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("lshape3q.mesh", &mesh);    // quadrilaterals
  //mloader.load("lshape3t.mesh", &mesh);  // triangles

  // Perform initial mesh refinemets.
  for (int i=0; i < INIT_REF_NUM; i++)  mesh.refine_all_elements();

  // Create an Hcurl space with default shapeset.
  HcurlSpace space(&mesh, bc_types, essential_bc_values, P_INIT);

  // Initialize the weak formulation.
  WeakForm wf;
  wf.add_matrix_form(callback(bilinear_form), HERMES_SYM);
  wf.add_matrix_form_surf(callback(bilinear_form_surf));
  wf.add_vector_form_surf(linear_form_surf, linear_form_surf_ord);

  // Initialize coarse and reference mesh solutions.
  Solution sln, ref_sln;

  // Initialize exact solution.
  ExactSolution sln_exact(&mesh, exact);

  // Initialize refinement selector.
  HcurlProjBasedSelector selector(CAND_LIST, CONV_EXP, H2DRS_DEFAULT_ORDER);
  
  // DOF and CPU convergence graphs.
  SimpleGraph graph_dof_est, graph_cpu_est, 
              graph_dof_exact, graph_cpu_exact;
  
  // Adaptivity loop:
  int as = 1; 
  bool done = false;
  do
  {
    info("---- Adaptivity step %d:", as);

    // Construct globally refined reference mesh and setup reference space.
    Space* ref_space = construct_refined_space(&space);

    // Assemble the reference problem.
    info("Solving on reference mesh.");
    bool is_linear = true;
    FeProblem* fep = new FeProblem(&wf, ref_space, is_linear);
    SparseMatrix* matrix = create_matrix(matrix_solver);
    Vector* rhs = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);
    fep->assemble(matrix, rhs);

    // Time measurement.
    cpu_time.tick();
    
    // Solve the linear system of the reference problem. If successful, obtain the solution.
    if(solver->solve()) Solution::vector_to_solution(solver->get_solution(), ref_space, &ref_sln);
    else error ("Matrix solver failed.\n");

    // Time measurement.
    cpu_time.tick();

    // Project the fine mesh solution onto the coarse mesh.
    info("Projecting reference solution on coarse mesh.");
    project_global(&space, &ref_sln, &sln, matrix_solver, HERMES_HCURL_NORM); 

    // Calculate element errors and total error estimate.
    info("Calculating error estimate and exact error."); 
    Adapt* adaptivity = new Adapt(&space, HERMES_HCURL_NORM);
    adaptivity->set_solutions(&sln, &ref_sln);
    double err_est_rel = adaptivity->calc_err_est(HERMES_TOTAL_ERROR_REL | HERMES_ELEMENT_ERROR_REL) * 100;

    // Calculate exact error,
    double err_exact_rel = adaptivity->calc_err_exact(HERMES_TOTAL_ERROR_REL, &sln_exact) * 100;

    // Report results.
    info("ndof_coarse: %d, ndof_fine: %d", 
      get_num_dofs(&space), get_num_dofs(ref_space));
    info("err_est_rel: %g%%, err_exact_rel: %g%%", err_est_rel, err_exact_rel);

    // Time measurement.
    cpu_time.tick();

    // Add entry to DOF and CPU convergence graphs.
    graph_dof_est.add_values(get_num_dofs(&space), err_est_rel);
    graph_dof_est.save("conv_dof_est.dat");
    graph_cpu_est.add_values(cpu_time.accumulated(), err_est_rel);
    graph_cpu_est.save("conv_cpu_est.dat");
    graph_dof_exact.add_values(get_num_dofs(&space), err_exact_rel);
    graph_dof_exact.save("conv_dof_exact.dat");
    graph_cpu_exact.add_values(cpu_time.accumulated(), err_exact_rel);
    graph_cpu_exact.save("conv_cpu_exact.dat");

    // If err_est_rel too large, adapt the mesh.
    if (err_est_rel < ERR_STOP) done = true;
    else 
    {
      info("Adapting coarse mesh.");
      done = adaptivity->adapt(&selector, THRESHOLD, STRATEGY, MESH_REGULARITY);
      
      // Increase the counter of performed adaptivity steps.
      if (done == false)  as++;
    }
    if (get_num_dofs(&space) >= NDOF_STOP) done = true;

    // Clean up.
    delete solver;
    delete matrix;
    delete rhs;
    delete adaptivity;
    if(done == false) delete ref_space->mesh;
    delete ref_space;
    delete fep;
    
  }
  while (done == false);
  
  verbose("Total running time: %g s", cpu_time.accumulated());

  int ndof = get_num_dofs(&space);

#define ERROR_SUCCESS                                0
#define ERROR_FAILURE                               -1
  printf("ndof allowed = %d\n", 1400);
  printf("ndof actual = %d\n", ndof);
  if (ndof < 1400) {      // ndofs was 1384 atthe time this test was created
    printf("Success!\n");
    return ERROR_SUCCESS;
  }
  else {
    printf("Failure!\n");
    return ERROR_FAILURE;
  }
}
