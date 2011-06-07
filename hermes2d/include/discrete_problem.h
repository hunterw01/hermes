/// This file is part of Hermes2D.
///
/// Hermes2D is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 2 of the License, or
/// (at your option) any later version.
///
/// Hermes2D is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with Hermes2D. If not, see <http:///www.gnu.org/licenses/>.

#ifndef __H2D_DISCRETE_PROBLEM_H
#define __H2D_DISCRETE_PROBLEM_H

#include "matrix.h"
#include "../../hermes_common/include/hermes_common.h"
#include "tables.h"
#include "adapt/adapt.h"
#include "graph.h"
#include "form/forms.h"
#include "weakform/weakform.h"
#include "function/function.h"
#include "neighbor.h"
#include "refinement_selectors/selector.h"
#include "adapt/kelly_type_adapt.h"
#include <map>

class PrecalcShapeset;

/// Multimesh neighbors traversal class.
class NeighborNode
{
public:
  NeighborNode(NeighborNode* parent, unsigned int transformation);
  ~NeighborNode();
  void set_left_son(NeighborNode* left_son);
  void set_right_son(NeighborNode* right_son);
  void set_transformation(unsigned int transformation);
  NeighborNode* get_left_son();
  NeighborNode* get_right_son();
  unsigned int get_transformation();
private:
  NeighborNode* parent;
  NeighborNode* left_son;
  NeighborNode* right_son;
  unsigned int transformation;
};

/// Discrete problem class.
///
/// This class does assembling into external matrix / vactor structures.
///
template<typename Scalar>
class HERMES_API DiscreteProblem : public DiscreteProblemInterface<Scalar>
{
public:
  /// Constructors.
  DiscreteProblem(WeakForm<Scalar>* wf, Hermes::vector<Space<Scalar>*> spaces);
  DiscreteProblem(WeakForm<Scalar>* wf, Space<Scalar>* space);

  /// Non-parameterized constructor (currently used only in KellyTypeAdapt to gain access to NeighborSearch methods).
  DiscreteProblem() : wf(NULL), pss(NULL) {num_user_pss = 0; sp_seq = NULL;}

  /// Init function. Common code for the constructors.
  void init();

  /// Destuctor.
  virtual ~DiscreteProblem();
  void free();

  /// GET functions.
  /// Get pointer to n-th space.
  Space<Scalar>* get_space(int n) { return this->spaces[n]; }

  /// Get whether the DiscreteProblem is linear.
  bool get_is_linear() { return is_linear;};

  /// Get the weak forms.
  WeakForm<Scalar>* get_weak_formulation() { return this->wf;};

  /// Get all spaces as a Hermes::vector.
  Hermes::vector<Space<Scalar>*> get_spaces() {return this->spaces;}

  /// This is different from H3D.
  PrecalcShapeset* get_pss(int n) {  return this->pss[n];  }

  /// Get the number of unknowns.
  int get_num_dofs();

  /// Get info about presence of a matrix.
  bool is_matrix_free() { return wf->is_matrix_free(); }


  /// Preassembling.
  /// Precalculate matrix sparse structure.
  /// If force_diagonal_block == true, then (zero) matrix
  /// antries are created in diagonal blocks even if corresponding matrix weak
  /// forms do not exist. This is useful if the matrix is later to be merged with
  /// a matrix that has nonzeros in these blocks. The Table serves for optional
  /// weighting of matrix blocks in systems.
  void create_sparse_structure(SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs = NULL,
    bool force_diagonal_blocks = false, Table* block_weights = NULL);

  /// Assembling utilities.
  /// Check whether it is sane to assemble.
  /// Throws errors if not.
  void assemble_sanity_checks(Table* block_weights);

  /// Converts coeff_vec to u_ext.
  /// If the supplied coeff_vec is NULL, it supplies the appropriate number of NULL pointers.
  void convert_coeff_vec(Scalar* coeff_vec, Hermes::vector<Solution<Scalar>*> & u_ext, bool add_dir_lift);

  /// Initializes psss.
  void initialize_psss(Hermes::vector<PrecalcShapeset*>& spss);

  /// Initializes refmaps.
  void initialize_refmaps(Hermes::vector<RefMap*>& refmap);

  /// Initialize a state, returns a non-NULL Element.
  Element* init_state(Stage<Scalar>& stage, Hermes::vector<PrecalcShapeset*>& spss, 
    Hermes::vector<RefMap*>& refmap, Element** e, Hermes::vector<bool>& isempty, Hermes::vector<AsmList<Scalar>*>& al);


  /// Assembling.
  /// General assembling procedure for nonlinear problems. coeff_vec is the
  /// previous Newton vector. If force_diagonal_block == true, then (zero) matrix
  /// antries are created in diagonal blocks even if corresponding matrix weak
  /// forms do not exist. This is useful if the matrix is later to be merged with
  /// a matrix that has nonzeros in these blocks. The Table serves for optional
  /// weighting of matrix blocks in systems. The parameter add_dir_lift decides 
  /// whether Dirichlet lift will be added while coeff_vec is converted into 
  /// Solutions.
  void assemble(Scalar* coeff_vec, SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs = NULL,
    bool force_diagonal_blocks = false, bool add_dir_lift = true, Table* block_weights = NULL);

  /// Light version passing NULL for the coefficient vector. External solutions 
  /// are initialized with zeros.
  void assemble(SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs = NULL, bool force_diagonal_blocks = false, 
    Table* block_weights = NULL);

  /// Assemble one stage.
  void assemble_one_stage(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext);

  /// Assemble one state.
  void assemble_one_state(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, Element** e, 
    bool* bnd, SurfPos* surf_pos, Element* trav_base);

  /// Assemble volume matrix forms.
  void assemble_volume_matrix_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al);
  void assemble_multicomponent_volume_matrix_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al);

  /// Assemble volume vector forms.
  void assemble_volume_vector_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al);
  void assemble_multicomponent_volume_vector_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al);

  /// Assemble surface and DG forms.
  void assemble_surface_integrals(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);

  /// Assemble surface matrix forms.
  void assemble_surface_matrix_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base);
  void assemble_multicomponent_surface_matrix_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base);

  /// Assemble surface vector forms.
  void assemble_surface_vector_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base);
  void assemble_multicomponent_surface_vector_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base);

  /// Assemble DG forms.
  void assemble_DG_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);

  /// Assemble one DG neighbor.
  void assemble_DG_one_neighbor(bool edge_processed, unsigned int neighbor_i, Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
       Hermes::vector<PrecalcShapeset *>& spss, Hermes::vector<RefMap *>& refmap, std::map<unsigned int, PrecalcShapeset *> npss,
       std::map<unsigned int, PrecalcShapeset *> nspss, std::map<unsigned int, RefMap *> nrefmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext, 
        Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);

  /// Assemble DG matrix forms.
  void assemble_DG_matrix_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, std::map<unsigned int, PrecalcShapeset*> npss,
    std::map<unsigned int, PrecalcShapeset*> nspss, std::map<unsigned int, RefMap*> nrefmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);
  void assemble_multicomponent_DG_matrix_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, std::map<unsigned int, PrecalcShapeset*> npss,
    std::map<unsigned int, PrecalcShapeset*> nspss, std::map<unsigned int, RefMap*> nrefmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);

  /// Assemble DG vector forms.
  void assemble_DG_vector_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);
  void assemble_multicomponent_DG_vector_forms(Stage<Scalar>& stage, 
    SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
    Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext, 
    Hermes::vector<bool>& isempty, int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat, 
    int isurf, Element** e, Element* trav_base, Element* rep_element);

  void invalidate_matrix() { have_matrix = false; }

  void set_fvm() {this->is_fvm = true;}

protected:
  DiscontinuousFunc<Ord>* init_ext_fn_ord(NeighborSearch<Scalar>* ns, MeshFunction<Scalar>* fu);

  // Matrix<Scalar> volume forms.

  // Main function for the evaluation of weak forms. 
  // Evaluates weak form on element given by the RefMap, 
  // using either non-adaptive or adaptive numerical quadrature.
  Scalar eval_form(MatrixFormVol<Scalar>* mfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv);
  void eval_form(MultiComponentMatrixFormVol<Scalar>* mfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv, Hermes::vector<Scalar>& result);

  int calc_order_matrix_form_vol(MatrixFormVol<Scalar>* mfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv);
  int calc_order_matrix_form_vol(MultiComponentMatrixFormVol<Scalar>* mfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv);

  // Elementary function used in eval_form() in adaptive mode.
  Scalar eval_form_subelement(int order, MatrixFormVol<Scalar>* mfv, 
    Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, 
    RefMap* ru, RefMap* rv);

  // Evaluates weak form on element given by the RefMap, using adaptive 
  // numerical quadrature.
  Scalar eval_form_adaptive(int order_init, Scalar result_init,
    MatrixFormVol<Scalar>* mfv, 
    Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, 
    RefMap* ru, RefMap* rv);

  // Vector<Scalar> volume forms. The functions provide the same functionality as the
  // parallel ones for matrix volume forms.

  Scalar eval_form(VectorFormVol<Scalar>* vfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv);
  void eval_form(MultiComponentVectorFormVol<Scalar>* vfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, Hermes::vector<Scalar>& result);

  int calc_order_vector_form_vol(VectorFormVol<Scalar>* mfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv);
  int calc_order_vector_form_vol(MultiComponentVectorFormVol<Scalar>* mfv, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv);

  Scalar eval_form_subelement(int order, VectorFormVol<Scalar>* vfv, 
    Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv);

  Scalar eval_form_adaptive(int order_init, Scalar result_init,
    VectorFormVol<Scalar>* vfv, 
    Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv);

  // Matrix<Scalar> surface forms. The functions provide the same functionality as the
  // parallel ones for matrix volume forms.

  Scalar eval_form(MatrixFormSurf<Scalar>* mfs, 
    Hermes::vector<Solution<Scalar>*> u_ext, 
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv, SurfPos* surf_pos);
  void eval_form(MultiComponentMatrixFormSurf<Scalar>* mfs, 
    Hermes::vector<Solution<Scalar>*> u_ext, 
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv, SurfPos* surf_pos, Hermes::vector<Scalar>& result);

  int calc_order_matrix_form_surf(MatrixFormSurf<Scalar>* mfs, 
    Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, 
    RefMap* ru, RefMap* rv, SurfPos* surf_pos);
  int calc_order_matrix_form_surf(MultiComponentMatrixFormSurf<Scalar>* mfs, 
    Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, 
    RefMap* ru, RefMap* rv, SurfPos* surf_pos);

  Scalar eval_form_subelement(int order, MatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, RefMap* rv, SurfPos* surf_pos);

  Scalar eval_form_adaptive(int order_init, Scalar result_init,
    MatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, 
    RefMap* rv, SurfPos* surf_pos);

  // Vector<Scalar> surface forms. The functions provide the same functionality as the
  // parallel ones for matrix volume forms.

  Scalar eval_form(VectorFormSurf<Scalar>* vfs, 
    Hermes::vector<Solution<Scalar>*> u_ext, 
    PrecalcShapeset* fv, RefMap* rv, SurfPos* surf_pos);
  void eval_form(MultiComponentVectorFormSurf<Scalar>* vfs, 
    Hermes::vector<Solution<Scalar>*> u_ext, 
    PrecalcShapeset* fv, RefMap* rv, SurfPos* surf_pos, Hermes::vector<Scalar>& result);

  int calc_order_vector_form_surf(VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, SurfPos* surf_pos);
  int calc_order_vector_form_surf(MultiComponentVectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, SurfPos* surf_pos);

  Scalar eval_form_subelement(int order, VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, SurfPos* surf_pos);

  Scalar eval_form_adaptive(int order_init, Scalar result_init,
    VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, SurfPos* surf_pos);

  // DG forms.

  int calc_order_dg_matrix_form(MatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, SurfPos* surf_pos,
    bool neighbor_supp_u, bool neighbor_supp_v, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_u);
  int calc_order_dg_matrix_form(MultiComponentMatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, SurfPos* surf_pos,
    bool neighbor_supp_u, bool neighbor_supp_v, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_u);


  Scalar eval_dg_form(MatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru_central, RefMap* ru_actual, RefMap* rv, 
    bool neighbor_supp_u, bool neighbor_supp_v,
    SurfPos* surf_pos, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_u, int neighbor_index_v);
  void eval_dg_form(MultiComponentMatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru_central, RefMap* ru_actual, RefMap* rv, 
    bool neighbor_supp_u, bool neighbor_supp_v,
    SurfPos* surf_pos, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_u, int neighbor_index_v, Hermes::vector<Scalar>& result);

  int calc_order_dg_vector_form(VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* ru, SurfPos* surf_pos,
    LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_v);
  int calc_order_dg_vector_form(MultiComponentVectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* ru, SurfPos* surf_pos,
    LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_v);

  Scalar eval_dg_form(VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, 
    SurfPos* surf_pos, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_v);
  void eval_dg_form(MultiComponentVectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
    PrecalcShapeset* fv, RefMap* rv, 
    SurfPos* surf_pos, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_v, Hermes::vector<Scalar>& result);

  ExtData<Ord>* init_ext_fns_ord(Hermes::vector<MeshFunction<Scalar>*> &ext);
  ExtData<Ord>* init_ext_fns_ord(Hermes::vector<MeshFunction<Scalar>*> &ext,
    int edge);
  ExtData<Ord>* init_ext_fns_ord(Hermes::vector<MeshFunction<Scalar>*> &ext,
    LightArray<NeighborSearch<Scalar>*>& neighbor_searches);
  ExtData<Scalar>* init_ext_fns(Hermes::vector<MeshFunction<Scalar>*> &ext,  
    RefMap* rm, const int order);
  ExtData<Scalar>* init_ext_fns(Hermes::vector<MeshFunction<Scalar>*> &ext, 
    LightArray<NeighborSearch<Scalar>*>& neighbor_searches,
    int order);

  Func<double>* get_fn(PrecalcShapeset* fu, RefMap* rm, const int order);
  Func<Ord>* get_fn_ord(const int order);

  struct VolVectorFormsKey
  {
    VectorFormVol<Scalar>* vfv;
    int element_id, shape_fn;
    VolVectorFormsKey(VectorFormVol<Scalar>* vfv, int element_id, int shape_fn)
      : vfv(vfv), element_id(element_id), shape_fn(shape_fn) {};
  };


  /// Pure DG functionality.

  /// There is a matrix form set on DG_INNER_EDGE area or not.
  bool DG_matrix_forms_present;
  /// There is a vector form set on DG_INNER_EDGE area or not.
  bool DG_vector_forms_present;

  /// Initialize neighbors.
  void init_neighbors(LightArray<NeighborSearch<Scalar>*>& neighbor_searches, const Stage<Scalar>& stage, const int& isurf);


  /// Initialize the tree for traversing multimesh neighbors.
  void build_multimesh_tree(NeighborNode* root, LightArray<NeighborSearch<Scalar>*>& neighbor_searches);

  /// Recursive insertion function into the tree.
  void insert_into_multimesh_tree(NeighborNode* node, unsigned int* transformations, unsigned int transformation_count);

  /// Return a global (unified list of central element transformations representing the neighbors on the union mesh.
  Hermes::vector<Hermes::vector<unsigned int>*> get_multimesh_neighbors_transformations(NeighborNode* multimesh_tree); 

  /// Traverse the multimesh tree. Used in the function get_multimesh_neighbors_transformations().
  void traverse_multimesh_tree(NeighborNode* node, Hermes::vector<Hermes::vector<unsigned int>*>& running_transformations);

  /// Update the NeighborSearch according to the multimesh tree.
  void update_neighbor_search(NeighborSearch<Scalar>* ns, NeighborNode* multimesh_tree);

  /// Finds a node in the multimesh tree that corresponds to the array transformations, with the length of transformation_count,
  /// starting to look for it in the NeighborNode node.
  NeighborNode* find_node(unsigned int* transformations, unsigned int transformation_count, NeighborNode* node);

  /// Updates the NeighborSearch ns according to the subtree of NeighborNode node.
  /// Returns 0 if no neighbor was deleted, -1 otherwise.
  unsigned int update_ns_subtree(NeighborSearch<Scalar>* ns, NeighborNode* node, unsigned int ith_neighbor);

  /// Traverse the multimesh subtree. Used in the function update_ns_subtree().
  void traverse_multimesh_subtree(NeighborNode* node, Hermes::vector<Hermes::vector<unsigned int>*>& running_central_transformations,
    Hermes::vector<Hermes::vector<unsigned int>*>& running_neighbor_transformations, const typename NeighborSearch<Scalar>::NeighborEdgeInfo& edge_info, const int& active_edge, const int& mode);

  /// Minimum identifier of the meshes used in DG assembling in one stage.
  unsigned int min_dg_mesh_seq;


  /// Members.
  WeakForm<Scalar>* wf;

  Mesh::ElementMarkersConversion* element_markers_conversion;
  Mesh::BoundaryMarkersConversion* boundary_markers_conversion;

  Geom<Ord> geom_ord;

  /// If the problem has only constant test functions, there is no need for order calculation,
  /// which saves time.
  bool is_fvm;

  /// Experimental caching of vector valued forms.
  bool vector_valued_forms;

  bool is_linear;

  int ndof;
  int* sp_seq;
  int wf_seq;
  Hermes::vector<Space<Scalar>*> spaces;

  Scalar** matrix_buffer;                // buffer for holding square matrix (during assembling)
  int matrix_buffer_dim;                 // dimension of the matrix held by 'matrix_buffer'
  Scalar** get_matrix_buffer(int n);

  bool have_spaces;
  bool have_matrix;

  bool values_changed;
  bool struct_changed;
  bool is_up_to_date();

  PrecalcShapeset** pss;    // This is different from H3D.
  int num_user_pss;         // This is different from H3D.


  /// Geometry and jacobian* weights caches.
  Geom<double>* cache_e[g_max_quad + 1 + 4* g_max_quad + 4];
  double* cache_jwt[g_max_quad + 1 + 4* g_max_quad + 4];

  /// Functions handling the above caches, and also other caches.
  void init_cache();
  void delete_cache();
  void delete_single_geom_cache(int order);

  /// Class handling various caches used in assembling.
  class AssemblingCaches {
  public:
    /// Basic constructor and destructor.
    AssemblingCaches();
    ~AssemblingCaches();

    /// Key for caching precalculated shapeset values on transformed elements with constant
    /// jacobians.
    struct KeyConst
    {
      int index;
      int order;
#ifdef _MSC_VER
      UINT64 sub_idx;
#else
      unsigned int sub_idx;
#endif
      int shapeset_type;
      double inv_ref_map[2][2];
#ifdef _MSC_VER
      KeyConst(int index, int order, UINT64 sub_idx, int shapeset_type, double2x2* inv_ref_map) {
        this->index = index;
        this->order = order;
        this->sub_idx = sub_idx;
        this->shapeset_type = shapeset_type;
        this->inv_ref_map[0][0] = (* inv_ref_map)[0][0];
        this->inv_ref_map[0][1] = (* inv_ref_map)[0][1];
        this->inv_ref_map[1][0] = (* inv_ref_map)[1][0];
        this->inv_ref_map[1][1] = (* inv_ref_map)[1][1];
      }
#else
      KeyConst(int index, int order, unsigned int sub_idx, int shapeset_type, double2x2* inv_ref_map) {
        this->index = index;
        this->order = order;
        this->sub_idx = sub_idx;
        this->shapeset_type = shapeset_type;
        this->inv_ref_map[0][0] = (* inv_ref_map)[0][0];
        this->inv_ref_map[0][1] = (* inv_ref_map)[0][1];
        this->inv_ref_map[1][0] = (* inv_ref_map)[1][0];
        this->inv_ref_map[1][1] = (* inv_ref_map)[1][1];
      }
#endif
    };

    /// Functor that compares two above keys (needed e.g. to create a std::map indexed by these keys);
    struct CompareConst {
      bool operator()(KeyConst a, KeyConst b) const {
        if(a.inv_ref_map[0][0] < b.inv_ref_map[0][0]) return true;
        else if(a.inv_ref_map[0][0] > b.inv_ref_map[0][0]) return false;
        else
          if(a.inv_ref_map[0][1] < b.inv_ref_map[0][1]) return true;
          else if(a.inv_ref_map[0][1] > b.inv_ref_map[0][1]) return false;
          else
            if(a.inv_ref_map[1][0] < b.inv_ref_map[1][0]) return true;
            else if(a.inv_ref_map[1][0] > b.inv_ref_map[1][0]) return false;
            else
              if(a.inv_ref_map[1][1] < b.inv_ref_map[1][1]) return true;
              else if(a.inv_ref_map[1][1] > b.inv_ref_map[1][1]) return false;
              else
                if (a.index < b.index) return true;
                else if (a.index > b.index) return false;
                else
                  if (a.order < b.order) return true;
                  else if (a.order > b.order) return false;
                  else
                    if (a.sub_idx < b.sub_idx) return true;
                    else if (a.sub_idx > b.sub_idx) return false;
                    else
                      if (a.shapeset_type < b.shapeset_type) return true;
                      else return false;
      };
    };

    /// PrecalcShapeset stored values for Elements with constant jacobian of the reference mapping.
    /// For triangles.
    std::map<KeyConst, Func<double>* , CompareConst> const_cache_fn_triangles;
    /// For quads
    std::map<KeyConst, Func<double>* , CompareConst> const_cache_fn_quads;

    /// The same setup for elements with non-constant jacobians.
    /// This cache is deleted with every change of the state in assembling.
    struct KeyNonConst {
      int index;
      int order;
#ifdef _MSC_VER
      UINT64 sub_idx;
#else
      unsigned int sub_idx;
#endif
      int shapeset_type;
#ifdef _MSC_VER
      KeyNonConst(int index, int order, UINT64 sub_idx, int shapeset_type) {
        this->index = index;
        this->order = order;
        this->sub_idx = sub_idx;
        this->shapeset_type = shapeset_type;
      }
#else
      KeyNonConst(int index, int order, unsigned int sub_idx, int shapeset_type) {
        this->index = index;
        this->order = order;
        this->sub_idx = sub_idx;
        this->shapeset_type = shapeset_type;
      }
#endif
    };

    /// Functor that compares two above keys (needed e.g. to create a std::map indexed by these keys);
    struct CompareNonConst {
      bool operator()(KeyNonConst a, KeyNonConst b) const {
        if (a.index < b.index) return true;
        else if (a.index > b.index) return false;
        else {
          if (a.order < b.order) return true;
          else if (a.order > b.order) return false;
          else {
            if (a.sub_idx < b.sub_idx) return true;
            else if (a.sub_idx > b.sub_idx) return false;
            else {
              if (a.shapeset_type < b.shapeset_type) return true;
              else return false;
            }
          }
        }
      }
    };

    /// PrecalcShapeset stored values for Elements with constant jacobian of the reference mapping.
    /// For triangles.
    std::map<KeyNonConst, Func<double>* , CompareNonConst> cache_fn_triangles;
    /// For quads
    std::map<KeyNonConst, Func<double>* , CompareNonConst> cache_fn_quads;

    LightArray<Func<Ord>*> cache_fn_ord;
  };
  AssemblingCaches assembling_caches;

  friend class KellyTypeAdapt<Scalar>;
  friend class Hermes2D<Scalar>;
};

#endif
