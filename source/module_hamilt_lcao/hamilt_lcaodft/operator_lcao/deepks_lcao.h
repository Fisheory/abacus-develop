#ifndef DEEPKSLCAO_H
#define DEEPKSLCAO_H
#include "module_hamilt_lcao/hamilt_lcaodft/local_orbital_charge.h"
#include "module_basis/module_ao/parallel_orbitals.h"
#include "module_cell/module_neighbor/sltk_grid_driver.h"
#include "module_hamilt_lcao/module_hcontainer/hcontainer.h"
#include "operator_lcao.h"

namespace hamilt
{

#ifndef __DEEPKSTEMPLATE
#define __DEEPKSTEMPLATE

/// The DeePKS class template inherits from class T
/// it is used to calculate the Deep Potential Kohn-Sham correction from DeePKS method
/// Template parameters:
/// - T: base class, it would be OperatorLCAO<TK> or OperatorPW<TK>
/// - TR: data type of real space Hamiltonian, it would be double or std::complex<double>
template <class T>
class DeePKS : public T
{
};

#endif

template <typename TK, typename TR>
class DeePKS<OperatorLCAO<TK, TR>> : public OperatorLCAO<TK, TR>
{
  public:
    DeePKS<OperatorLCAO<TK, TR>>(Local_Orbital_Charge* loc_in,
                            LCAO_Matrix* LM_in,
                            const std::vector<ModuleBase::Vector3<double>>& kvec_d_in,
                            HContainer<TR>* hR_in,
                            std::vector<TK>* hK_in,
                            const UnitCell* ucell_in,
                            Grid_Driver* GridD_in,
                            const int& nks_in);

    virtual void contributeHR() override;

  private:
    Local_Orbital_Charge* loc;

    const UnitCell* ucell = nullptr;

    HContainer<TR>* H_V_delta = nullptr;
#ifdef __DEEPKS

    /**
     * @brief initialize HR, search the nearest neighbor atoms
     * HContainer is used to store the DeePKS real space Hamiltonian correction with specific <I,J,R> atom-pairs
     * the size of HR will be fixed after initialization
     */
    void initialize_HR(Grid_Driver* GridD, const Parallel_Orbitals* paraV);

    /**
     * @brief calculate the DeePKS correction matrix with specific <I,J,R> atom-pairs
     * use the adjs_all to calculate the HR matrix
     */
    void calculate_HR();

    /**
     * @brief calculate the HR local matrix of <I,J,R> atom pair
     */
    void cal_HR_IJR(const int& iat1,
                    const int& iat2,
                    const int& T0,
                    const Parallel_Orbitals* paraV,
                    const std::unordered_map<int, std::vector<double>>& nlm1_all,
                    const std::unordered_map<int, std::vector<double>>& nlm2_all,
                    const int* L0s,
                    const double** gedms,
                    const int size_gedms,
                    TR* data_pointer);

    /**
     * @brief initialize H_V_delta, search the nearest neighbor atoms
     * used for calculate the DeePKS real space Hamiltonian correction with specific <I,J,R> atom-pairs
    */
    std::vector<AdjacentAtomInfo> adjs_all;
#endif
    const int& nks;
};

} // namespace hamilt
#endif