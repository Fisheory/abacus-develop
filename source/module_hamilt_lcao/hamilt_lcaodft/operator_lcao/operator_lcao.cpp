#include "operator_lcao.h"
#include "module_base/timer.h"
#include "module_base/tool_title.h"
#include "module_hsolver/hsolver_lcao.h"
#include "module_hamilt_lcao/module_hcontainer/hcontainer_funcs.h"

#ifdef __ELPA
#include "module_hsolver/diago_elpa.h"
#endif

namespace hamilt
{

template<>
void OperatorLCAO<double, double>::get_hs_pointers()
{
    ModuleBase::timer::tick("OperatorLCAO", "get_hs_pointers");
    this->hmatrix_k = this->LM->Hloc.data();
    if ((this->new_e_iteration && ik == 0) || hsolver::HSolverLCAO::out_mat_hs)
    {
        if (this->smatrix_k == nullptr)
        {
            this->smatrix_k = new double[this->LM->Sloc.size()];
            this->allocated_smatrix = true;
        }
        const int inc = 1;
        BlasConnector::copy(this->LM->Sloc.size(), this->LM->Sloc.data(), inc, this->smatrix_k, inc);
#ifdef __ELPA
        hsolver::DiagoElpa::DecomposedState = 0;
#endif
        this->new_e_iteration = false;
    }
    ModuleBase::timer::tick("OperatorLCAO", "get_hs_pointers");
}

template<>
void OperatorLCAO<std::complex<double>, double>::get_hs_pointers()
{
    this->hmatrix_k = this->LM->Hloc2.data();
    this->smatrix_k = this->LM->Sloc2.data();
}

template<>
void OperatorLCAO<std::complex<double>, std::complex<double>>::get_hs_pointers()
{
    this->hmatrix_k = this->LM->Hloc2.data();
    this->smatrix_k = this->LM->Sloc2.data();
}

template<>
void OperatorLCAO<double, double>::refresh_h()
{
    // Set the matrix 'H' to zero.
    this->LM->zeros_HSgamma('H');
}

template<>
void OperatorLCAO<std::complex<double>, double>::refresh_h()
{
    // Set the matrix 'H' to zero.
    this->LM->zeros_HSk('H');
}

template<>
void OperatorLCAO<std::complex<double>, std::complex<double>>::refresh_h()
{
    // Set the matrix 'H' to zero.
    this->LM->zeros_HSk('H');
}

template<>
void OperatorLCAO<double, double>::folding_fixed(const int ik, const std::vector<ModuleBase::Vector3<double>>& kvec_d)
{
    ModuleBase::TITLE("OperatorLCAO", "folding_fixed");
    ModuleBase::timer::tick("OperatorLCAO", "folding_fixed");
    //add T+VNL matrix.
	this->LM->update_Hloc();
    ModuleBase::timer::tick("OperatorLCAO", "folding_fixed");
}

template<>
void OperatorLCAO<std::complex<double>, double>::folding_fixed(const int ik, const std::vector<ModuleBase::Vector3<double>>& kvec_d)
{
    ModuleBase::TITLE("OperatorLCAO", "folding_fixed");
    ModuleBase::timer::tick("OperatorLCAO", "folding_fixed");
    //-----------------------------------------
    // folding matrix here: T(k)+Vnl(k)
    // (Hloc_fixed->Hloc_fixed2)
    //-----------------------------------------
    this->LM->folding_fixedH(ik, kvec_d);

    //------------------------------------------
    // Add T(k)+Vnl(k)+Vlocal(k)
    // (Hloc2 += Hloc_fixed2), (std::complex matrix)
    //------------------------------------------
	this->LM->update_Hloc2(ik);
    ModuleBase::timer::tick("OperatorLCAO", "folding_fixed");
}

template<>
void OperatorLCAO<std::complex<double>, std::complex<double>>::folding_fixed(const int ik, const std::vector<ModuleBase::Vector3<double>>& kvec_d)
{
    ModuleBase::TITLE("OperatorLCAO", "folding_fixed");
    ModuleBase::timer::tick("OperatorLCAO", "folding_fixed");

    //------------------------------------------
    // Add T(k)+Vnl(k)+Vlocal(k)
    // (Hloc2 += Hloc_fixed2), (std::complex matrix)
    //------------------------------------------
	this->LM->update_Hloc2(ik);
    ModuleBase::timer::tick("OperatorLCAO", "folding_fixed");
}

template<typename TK, typename TR>
void OperatorLCAO<TK, TR>::init(const int ik_in)
{
    ModuleBase::TITLE("OperatorLCAO", "init");
    ModuleBase::timer::tick("OperatorLCAO", "init");
    if(this->is_first_node)
    {
        // refresh HK
        this->refresh_h();
        // refresh HR
        this->hR->set_zero();
    }
    switch(this->cal_type)
    {
        case lcao_overlap:
        {
            //cal_type=lcao_overlap refer to overlap matrix operators, which are only rely on stucture, and not changed during SCF

            //update SR first
            //in cal_type=lcao_overlap, SR should be updated by each sub-chain nodes
            OperatorLCAO<TK, TR>* last = this;
            while(last != nullptr)
            {
                last->contributeHR();
                last = dynamic_cast<OperatorLCAO<TK, TR>*>(last->next_sub_op);
            }

            //update SK next
            //in cal_type=lcao_overlap, SK should be update here
            this->contributeHk(ik_in);

            break;
        }
        case lcao_fixed:
        {
            //cal_type=lcao_fixed refer to fixed matrix operators, which are only rely on stucture, and not changed during SCF

            //update HR first
            //in cal_type=lcao_fixed, HR should be updated by each sub-chain nodes
            OperatorLCAO<TK, TR>* last = this;
            while(last != nullptr)
            {
                last->contributeHR();
                last = dynamic_cast<OperatorLCAO<TK, TR>*>(last->next_sub_op);
            }

            //update HK next
            //in cal_type=lcao_fixed, HK will update in the last node with OperatorLCAO::contributeHk()

            break;
        }
        case lcao_gint:
        {
            //cal_type=lcao_gint refer to grid integral operators, which are relied on stucture and potential based on real space grids
            //and should be updated each SCF steps

            OperatorLCAO<TK, TR>* last = this;
            while(last != nullptr)
            {
                //update HR first
                //in cal_type=lcao_gint, HR should be updated by every sub-node.
                last->contributeHR();

                //update HK next
                //in cal_type=lcao_gint, HK should be updated by every sub-node.
                last->contributeHk(ik_in);

                last = dynamic_cast<OperatorLCAO<TK, TR>*>(last->next_sub_op);
            }

            break;
        }
        case lcao_deepks:
        {
            //update HR first
            //in cal_type=lcao_deepks, HR should be updated
            this->contributeHR();

            break;

        }
        case lcao_dftu:
        {
            //only HK should be updated when cal_type=lcao_dftu
            //in cal_type=lcao_dftu, HK only need to update from one node
            this->contributeHk(ik_in);

            break;
        }
        case lcao_exx:
        {
            //update HR first
            //in cal_type=lcao_exx, HR should be updated by most priority sub-chain nodes
            this->contributeHR();

            //update HK next
            //in cal_type=lcao_exx, HK only need to update from one node
            this->contributeHk(ik_in);

            break;
        }
        default:
        {
            ModuleBase::WARNING_QUIT("OperatorLCAO::init", "unknown cal_type");
            break;
        }
    }
    if(this->next_op != nullptr)
    {//it is not the last node, loop next init() function
        this->next_op->init(ik_in);
    }
    else 
    {//it is the last node, update HK with the current total HR
        OperatorLCAO<TK, TR>::contributeHk(ik_in);
    }

    ModuleBase::timer::tick("OperatorLCAO", "init");
}

// contributeHk()
template<typename TK, typename TR>
void OperatorLCAO<TK, TR>::contributeHk(int ik)
{
    ModuleBase::TITLE("OperatorLCAO", "contributeHk");
    ModuleBase::timer::tick("OperatorLCAO", "contributeHk");
    if(ModuleBase::GlobalFunc::IS_COLUMN_MAJOR_KS_SOLVER())
    {
        const int nrow = this->LM->ParaV->get_row_size();
        hamilt::folding_HR(*this->hR, this->hK->data(), this->kvec_d[ik], nrow, 1);
    }
    else
    {
        const int ncol = this->LM->ParaV->get_col_size();
        hamilt::folding_HR(*this->hR, this->hK->data(), this->kvec_d[ik], ncol, 0);
    }
    ModuleBase::timer::tick("OperatorLCAO", "contributeHk");
}

template class OperatorLCAO<double, double>;
template class OperatorLCAO<std::complex<double>, double>;
template class OperatorLCAO<std::complex<double>, std::complex<double>>;
}  // namespace hamilt
