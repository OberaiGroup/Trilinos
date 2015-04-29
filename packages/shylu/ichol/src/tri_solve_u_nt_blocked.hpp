#pragma once
#ifndef __TRI_SOLVE_U_NT_BLOCKED_HPP__
#define __TRI_SOLVE_U_NT_BLOCKED_HPP__

/// \file trsm_l_u_nt.hpp
/// \brief Sparse triangular solve on given sparse patterns and multiple rhs.
/// \author Kyungjoo Kim (kyukim@sandia.gov)
///

namespace Example {

  using namespace std;

  template<>
  template<typename ParallelForType,
           typename CrsExecViewType,
           typename DenseExecViewType>
  KOKKOS_INLINE_FUNCTION
  int
  TriSolve<Uplo::Upper,Trans::NoTranspose,
           AlgoTriSolve::ByBlocks>
  ::invoke(const typename CrsExecViewType::policy_type::member_type &member,
           const int diagA,
           const CrsExecViewType &A,
           const DenseExecViewType &B) {
    CrsTaskViewType ATL, ATR,      A00, A01, A02,
      /**/          ABL, ABR,      A10, A11, A12,
      /**/                         A20, A21, A22;

    DenseTaskViewType BT,      B0,
      /**/            BB,      B1,
      /**/                     B2;

    Part_2x2(A,  ATL, ATR,
             /**/ABL, ABR,
             0, 0, Partition::BottomRight);

    Part_2x1(B,  BT,
             /**/BB,
             0, Partition::Bottom);

    while (ABR.NumRows() < A.NumRows()) {
      Part_2x2_to_3x3(ATL, ATR, /**/  A00, A01, A02,
                      /*******/ /**/  A10, A11, A12,
                      ABL, ABR, /**/  A20, A21, A22,
                      1, 1, Partition::TopLeft);

      Part_2x1_to_3x1(BT,  /**/  B0,
                      /**/ /**/  B1,
                      BB,  /**/  B2,
                      1, Partition::Top);

      // -----------------------------------------------------
      A11.fillRowArrays();
      A12.fillRowArrays();

      // B1 = B1 - A12*B2;
      Gemm<Trans::NoTranspose,Trans::NoTranspose,AlgoGemm::ForTriSolveBlocked>
        ::TaskFunctor<ParallelForType,double,
        CrsExecViewType,DenseExecViewType,DenseExecViewType>(member, -1.0, A12, B2, 1.0, B1);

      // B1 = inv(triu(A11))*B1
      Trsm<Side::Left,Uplo::Upper,Trans::NoTranspose,AlgoTrsm::ForTriSolveBlocked>
        ::TaskFunctor<ParallelForType,double,
        CrsExecViewType,DenseExecViewType>(member, diagA, 1.0, A11, B1);

      // -----------------------------------------------------
      Merge_3x3_to_2x2(A00, A01, A02, /**/ ATL, ATR,
                       A10, A11, A12, /**/ /******/
                       A20, A21, A22, /**/ ABL, ABR,
                       Partition::BottomRight);

      Merge_3x1_to_2x1(B0, /**/   BT,
                       B1, /**/  /**/
                       B2, /**/   BB,
                       Partition::Bottom);
    }
    return 0;
  }

}

#endif
