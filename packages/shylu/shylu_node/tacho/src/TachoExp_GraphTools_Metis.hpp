#ifndef __TACHOEXP_GRAPH_TOOLS_METIS_HPP__
#define __TACHOEXP_GRAPH_TOOLS_METIS_HPP__

/// \file TachoExp_GraphTools_Scotch.hpp
/// \author Kyungjoo Kim (kyukim@sandia.gov)

#if defined(TACHO_HAVE_METIS)
#include "TachoExp_Util.hpp"
#include "TachoExp_Graph.hpp"

#include "metis.h"

namespace Tacho {

  namespace Experimental {

    class GraphTools_Metis {
    public:
      typedef Kokkos::DefaultHostExecutionSpace host_exec_space;
      typedef Kokkos::View<idx_t*,host_exec_space> idx_t_array;
      typedef Kokkos::View<ordinal_type*,host_exec_space> ordinal_type_array;

    private:
        
      // metis main data structure
      idx_t _nvts;
      idx_t_array _xadj, _adjncy, _vwgt;
      
      idx_t _options[METIS_NOPTIONS];

      // metis output
      idx_t_array _perm_t, _peri_t;
      ordinal_type_array _perm, _peri;

      // status flag
      bool _is_ordered, _verbose;

    public:
      GraphTools_Metis() = default;
      GraphTools_Metis(const GraphTools_Metis &b) = default;

      ///
      /// construction of scotch graph
      ///
      GraphTools_Metis(const Graph &g) {
        _is_ordered = false;
        _verbose = false;
        
        // input 
        _nvts = g.NumRows();

        _xadj   = idx_t_array("idx_t_xadj",   g.RowPtr().dimension_0());
        _adjncy = idx_t_array("idx_t_adjncy", g.ColIdx().dimension_0());
        _vwgt   = idx_t_array();

        const auto &g_row_ptr = g.RowPtr();
        const auto &g_col_idx = g.ColIdx();

        for (ordinal_type i=0;i<static_cast<ordinal_type>(_xadj.dimension_0());++i)
          _xadj(i) = g_row_ptr(i);
        for (ordinal_type i=0;i<static_cast<ordinal_type>(_adjncy.dimension_0());++i)
          _adjncy(i) = g_col_idx(i);
        
        METIS_SetDefaultOptions(_options);
        _options[METIS_OPTION_NUMBERING] = 0;

        _perm_t = idx_t_array("idx_t_perm", _nvts);
        _peri_t = idx_t_array("idx_t_peri", _nvts);    

        // output
        _perm  = ordinal_type_array("Metis::PermutationArray", _nvts);
        _peri  = ordinal_type_array("Metis::InvPermutationArray", _nvts);    
      }
      virtual~GraphTools_Metis() {}

      ///
      /// setup metis parameters
      ///

      void setVerbose(const bool verbose) { _verbose = verbose; }
      void setOption(const int id, const idx_t value) {
        _options[id] = value;
      }

      ///
      /// reorder by metis
      ///

      void reorder(const ordinal_type verbose = 0) {
        Kokkos::Impl::Timer timer;
        double t_metis = 0; 

        int ierr = 0;

        idx_t *xadj   = (idx_t*)_xadj.data();
        idx_t *adjncy = (idx_t*)_adjncy.data();
        idx_t *vwgt   = (idx_t*)_vwgt.data();

        idx_t *perm   = (idx_t*)_perm_t.data();
        idx_t *peri   = (idx_t*)_peri_t.data();

        timer.reset();
        ierr = METIS_NodeND(&_nvts, xadj, adjncy, vwgt, _options, 
                            perm, peri);
        t_metis = timer.seconds();

        for (idx_t i=0;i<_nvts;++i) {
          _perm(i) = _perm_t(i);
          _peri(i) = _peri_t(i);
        }

        TACHO_TEST_FOR_EXCEPTION(ierr != METIS_OK, 
                                 std::runtime_error,
                                 "Failed in METIS_NodeND");
        _is_ordered = true;

        if (verbose) {
          printf("Summary: GraphTools (Metis)\n");
          printf("===========================\n");

          switch (verbose) {
          case 1: {
            printf("  Time\n");
            printf("             time for reordering: %10.6f s\n", t_metis);
            printf("\n");
          }
          }
        }

      }

      ordinal_type_array PermVector()    const { return _perm; }
      ordinal_type_array InvPermVector() const { return _peri; }
        
      std::ostream& showMe(std::ostream &os, const bool detail = false) const {
        std::streamsize prec = os.precision();
        os.precision(4);
        os << std::scientific;

        if (_is_ordered)
          os << " -- Metis Ordering -- " << std::endl
             << "  PERM     PERI     " << std::endl;
        else 
          os << " -- Not Ordered -- " << std::endl;

        if (detail) {
          const ordinal_type w = 6, m = _perm.dimension_0();
          for (ordinal_type i=0;i<m;++i)
            os << std::setw(w) << _perm[i] << "   "
               << std::setw(w) << _peri[i] << "   "
               << std::endl;
        }
        os.unsetf(std::ios::scientific);
        os.precision(prec);

        return os;
      }

    };
  }
}
#endif
#endif