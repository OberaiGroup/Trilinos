/// \file Tacho_SymbolicTools.hpp
/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "Tacho_Util.hpp"
#include "Tacho_SymbolicTools.hpp"

namespace Tacho {

  using ordinal_type_array = SymbolicTools::ordinal_type_array;
  using size_type_array = SymbolicTools::size_type_array;
  
  void SymbolicTools::
  computeEliminationTree(const ordinal_type m,
                         const size_type_array &ap,
                         const ordinal_type_array &aj,
                         const ordinal_type_array &perm,
                         const ordinal_type_array &peri,
                         const ordinal_type_array &parent,
                         const ordinal_type_array &ancestor) {
    for (ordinal_type i=0;i<m;++i) {
      parent(i) = -1;
      ancestor(i) = -1;
      
      const ordinal_type idx = perm(i);
      for (size_type p=ap(idx);p<ap(idx+1);++p) {
        ordinal_type j = peri(aj(p));
        for ( ;j!=-1 && j<i;) {
          const ordinal_type next = ancestor(j);
          ancestor(j) = i;
          if (next == -1) parent(j) = i;
          j = next;
        }
      }
    }
  }
  
  ordinal_type SymbolicTools::
  TreeDepthFirstSearch(const ordinal_type j,
                       const ordinal_type c,
                       const ordinal_type_array &head,
                       const ordinal_type_array &next,
                       const ordinal_type_array &post,
                       const ordinal_type_array &stack) {
    ordinal_type top = 0, k = c;
    stack(top) = j;
    while (top >= 0) {
      const ordinal_type p = stack(top);
      const ordinal_type i = head(p);
      if (i == -1) {
        --top;
        post(k++) = p;
      } else {
        head(p) = next(i);
        stack(++top) = i;
      }
    }
    return k;
  }
  
  void SymbolicTools::
  computePostOrdering(const ordinal_type m,
                      const ordinal_type_array &parent,
                      const ordinal_type_array &post,
                      const ordinal_type_array &work) {
    auto head  = Kokkos::subview(work, range_type(0*m, 1*m));
    auto next  = Kokkos::subview(work, range_type(1*m, 2*m));
    auto stack = Kokkos::subview(work, range_type(2*m, 3*m));
    
    for (ordinal_type i=0;i<m;++i)
      head(i) = -1;
    
    for (ordinal_type i=m-1;i>=0;--i) {
      const ordinal_type p = parent(i);
      if (p != -1) {
        next(i) = head(p);
        head(p) = i;
      }
    }
    ordinal_type k = 0;
    for (ordinal_type i=0;i<m;++i) 
      if (parent(i) == -1) 
        k = TreeDepthFirstSearch(i, k, head, next, post, stack);
  }
  
  void SymbolicTools::
  computeFillPatternUpper(const ordinal_type m, 
                          const size_type_array &ap,
                          const ordinal_type_array &aj,
                          const ordinal_type_array &perm,
                          const ordinal_type_array &peri,
                          /* */ size_type_array &up,
                          /* */ ordinal_type_array &uj,
                          const ordinal_type_array &work) {        
    auto parent        = Kokkos::subview(work, range_type(0*m, 1*m));
    auto flag          = Kokkos::subview(work, range_type(1*m, 2*m));
    auto upper_row_cnt = Kokkos::subview(work, range_type(2*m, 3*m));
    
    // count nnz per row
    for (ordinal_type i=0;i<m;++i) {
      parent(i) = -1;
      flag(i) = -1;
      upper_row_cnt(i) = 1; // add diagonal entry
      
      const ordinal_type ii = perm(i);          
      for (size_type p=ap(ii);p<ap(ii+1);++p) {
        for (ordinal_type j=peri(aj(p));flag(j)!=i && j<i;j=parent(j)) {
          if (parent(j) == -1) parent(j) = i;
          ++upper_row_cnt(j);
          flag(j) = i;
        }
      }
    }
    
    // prefix scan
    up = size_type_array(do_not_initialize_tag("up"), m+1); up(0) = size_type();
    for (ordinal_type i=0;i<m;++i)
      up(i+1) = up(i) + upper_row_cnt(i);
    
    // fill-in
    uj = ordinal_type_array("uj", up(m));
    for (ordinal_type i=0;i<m;++i) {
      parent(i) = -1;
      flag(i) = -1;
      
      // diagonal entry
      upper_row_cnt(i) = 1; 
      uj(up(i)) = i;
      
      const ordinal_type ii = perm(i);          
      for (size_type p=ap(ii);p<ap(ii+1);++p) {
        for (ordinal_type j=peri(aj(p));flag(j)!=i && j<i;j=parent(j)) {
          if (parent(j) == -1) parent(j) = i;
          uj(up(j)+upper_row_cnt(j)) = i;
          ++upper_row_cnt(j);
          flag(j) = i;
        }
      }
    }
  }
  
  void SymbolicTools::
  computeSupernodes(const ordinal_type m, 
                    const size_type_array &ap,
                    const ordinal_type_array &aj,
                    const ordinal_type_array &perm,
                    const ordinal_type_array &peri,
                    const ordinal_type_array &parent,
                    /* */ ordinal_type_array &supernodes,
                    const ordinal_type_array &work) {        
    // workspace
    auto flag  = Kokkos::subview(work, range_type(0*m, 1*m));
    auto count = Kokkos::subview(work, range_type(1*m, 2*m));
    auto prev  = Kokkos::subview(work, range_type(2*m, 3*m));
    
    // initialize workspace
    Kokkos::deep_copy(work, 0);
    
    // count # of children of parent
    for (ordinal_type i=0;i<m;++i) if (parent(i) >= 0) ++count(parent(i));
    
    // parent has more than a child, it becomes a supernode candidate 
    // roots are supernodes
    for (ordinal_type i=0;i<m;++i) if (count(i) > 1 || parent(i) < 0) flag(i) = true;
    
    // accumulate subtree sizes in count.
    for (ordinal_type i=0;i<m;++i) count(i) = 1;
    for (ordinal_type i=0;i<m;++i) if (parent(i) >= 0) count(parent(i)) += count(i);
    
    // tree leaves are also supernode candidate (not easy to understand this)
    for (ordinal_type i=0;i<m;++i) {
      const ordinal_type ii = perm(i);
      for (size_type p=ap(ii);p<ap(ii+1);++p) {
        const ordinal_type j = peri(aj(p));
        if (i < j) {
          const ordinal_type k = prev(j);
          if (k < (i-count(i)+1)) flag(i) = true;
          prev(j) = i;
        }
      }
    }
    
    // count # of supernodes 
    {
      ordinal_type k = 0; 
      flag(k) = true; // supernodes begin
      
      for (ordinal_type i=0;i<m;++i) k += flag(i);
      supernodes = ordinal_type_array(do_not_initialize_tag("supernodes"), k+1);
      
      // record supernodes 
      k = 0;
      for (ordinal_type i=0;i<m;++i) if (flag(i)) supernodes(k++) = i;
      supernodes(k) = m; // supernodes end
    }
  }
  
  void SymbolicTools::
  allocateSupernodes(const ordinal_type m,
                     const size_type_array &ap,
                     const ordinal_type_array &aj,
                     const ordinal_type_array &supernodes,
                     const ordinal_type_array &work,
                     /* */ size_type_array &gid_super_panel_ptr,
                     /* */ ordinal_type_array &gid_super_panel_colidx,
                     /* */ size_type_array &sid_super_panel_ptr,
                     /* */ ordinal_type_array &sid_super_panel_colidx,
                     /* */ ordinal_type_array &blk_super_panel_colidx) {
    const ordinal_type numSupernodes = supernodes.extent(0) - 1;
    
    // for each supernode
    auto clear_flag = [](const ordinal_type cnt,
                         const ordinal_type_array &id,
                         const ordinal_type_array &flag) {
      for (ordinal_type i=0;i<cnt;++i) flag(id(i)) = 0;
    };
    
    auto clear_array = [](const ordinal_type cnt,
                          const ordinal_type_array &a) {
      memset(a.data(), 0, cnt*sizeof(typename ordinal_type_array::value_type));
    };
    
    auto colmap_per_supernode = [](const size_type_array &ap_,
                                   const ordinal_type_array &aj_,
                                   const ordinal_type sbeg, 
                                   const ordinal_type send,
                                   const ordinal_type_array &cid,
                                   const ordinal_type_array &flag) -> ordinal_type {
      // # of columns accessed by this super node
      ordinal_type cnt = 0;
      
      // loop over super node cols (diagonal block)
      for (ordinal_type col=sbeg;col<send;++col) {
        flag(col) = true; // visitation flag on
        cid(cnt++) = col; // record the column indicies
      }
      
      // visit each super node row (off diagonal block)
      for (ordinal_type i=sbeg;i<send;++i) {
        for (size_type j=ap_(i);j<ap_(i+1);++j) {
          const ordinal_type col = aj_(j);
          if (flag(col)) {
            // already visited; pass on
          } else {
            flag(col) = true;
            cid(cnt++) = col;
          }
        }
      }
      return cnt;
    };
    
    auto sidmap_per_supernode = [](const ordinal_type ndofs,
                                   const ordinal_type_array &cid,
                                   const ordinal_type_array &sid_colored_in_rows,
                                   const ordinal_type_array &sids_connected_to_this_row,
                                   const ordinal_type_array &count,
                                   const ordinal_type_array &blks_connected_to_this_row) -> ordinal_type {
      ordinal_type cnt = 0;
      for (ordinal_type k=0;k<ndofs;++k) {
        const ordinal_type sid = sid_colored_in_rows(cid(k));
        if (count(sid) == 0) 
          sids_connected_to_this_row(cnt++) = sid;
        ++count(sid);
      }
      if (blks_connected_to_this_row.data() != NULL) {
        for (ordinal_type k=0;k<cnt;++k) {
          const ordinal_type sid = sids_connected_to_this_row(k);
          blks_connected_to_this_row(k) = count(sid);
        }
      }
      return cnt;
    };
    
    auto flag = Kokkos::subview(work, range_type(0*m, 1*m));
    auto cid  = Kokkos::subview(work, range_type(1*m, 2*m));
    auto rid  = Kokkos::subview(work, range_type(2*m, 3*m));
    auto tmp  = Kokkos::subview(work, range_type(3*m, 4*m));
    
    // zeros
    Kokkos::deep_copy(work, 0);
    
    ///
    /// super color in rows
    ///
    for (ordinal_type sid=0;sid<numSupernodes;++sid) {
      const ordinal_type sbeg = supernodes(sid), send = supernodes(sid+1);
      for (ordinal_type i=sbeg;i<send;++i) 
        rid(i) = sid;
    }
    
    ///
    /// super_panel_ptr, colidx_super_panel
    ///
    
    const ordinal_type_array null_ordinal_type_array;
    
    /// count the # of associated columns to all supernodes
    gid_super_panel_ptr = size_type_array(do_not_initialize_tag("gid_super_panel_ptr"), numSupernodes+1); gid_super_panel_ptr(0) = size_type();
    sid_super_panel_ptr = size_type_array(do_not_initialize_tag("sid_super_panel_ptr"), numSupernodes+1); sid_super_panel_ptr(0) = size_type();
    
    for (ordinal_type sid=0;sid<numSupernodes;++sid) {
      const ordinal_type sbeg = supernodes(sid), send = supernodes(sid+1);
      
      const ordinal_type ndofs = colmap_per_supernode(ap, aj, sbeg, send, cid, flag);
      gid_super_panel_ptr(sid+1) = gid_super_panel_ptr(sid) + ndofs;
      clear_flag(ndofs, cid, flag);
      
      const ordinal_type nsids = sidmap_per_supernode(ndofs, cid, rid, tmp, flag,
                                                      null_ordinal_type_array);
      sid_super_panel_ptr(sid+1) = sid_super_panel_ptr(sid) + nsids + 1;
      clear_flag(nsids, tmp, flag); 
      
      clear_array(ndofs, cid);
      clear_array(nsids, tmp);
    }
    gid_super_panel_colidx = ordinal_type_array("gid_super_panel_colidx", gid_super_panel_ptr(numSupernodes));
    sid_super_panel_colidx = ordinal_type_array("sid_super_panel_colidx", sid_super_panel_ptr(numSupernodes));
    blk_super_panel_colidx = ordinal_type_array("blk_super_panel_colidx", sid_super_panel_ptr(numSupernodes));
    
    /// sort and column indices per supernode
    for (ordinal_type sid=0;sid<numSupernodes;++sid) {
      const ordinal_type sbeg = supernodes(sid), send = supernodes(sid+1);
      
      // *** sort and construct gid map to sparse matrix
      const auto gid_range = range_type(gid_super_panel_ptr(sid), gid_super_panel_ptr(sid+1));
      
      auto colidx = Kokkos::subview(gid_super_panel_colidx, gid_range);
      const ordinal_type ndofs = colmap_per_supernode(ap, aj, sbeg, send, colidx, flag);
      
      std::sort(colidx.data() + (send-sbeg), colidx.data() + ndofs);
      clear_flag(ndofs, colidx, flag);          
      
      // *** sort associated supernodes to sid; stored in the work array cid
      const auto sid_range = range_type(sid_super_panel_ptr(sid), sid_super_panel_ptr(sid+1));
      
      auto sididx = Kokkos::subview(sid_super_panel_colidx, sid_range);
      auto blkidx = Kokkos::subview(blk_super_panel_colidx, sid_range);
      
      // cid and tmp are used for temporary storage
      const ordinal_type nsids = sidmap_per_supernode(ndofs, colidx, rid, cid, flag, tmp);
      
      // flag is now used for permutation vector
      clear_flag(nsids, cid, flag); 
      for (ordinal_type i=0;i<nsids;++i) flag(i) = i;
      
      std::sort(flag.data(), flag.data()+nsids, 
                [&](std::size_t i, std::size_t j){ return cid(i) < cid(j); });
      std::transform(flag.data(), flag.data()+nsids, 
                     sididx.data(),[&](std::size_t i) { return cid(i); });
      std::transform(flag.data(), flag.data()+nsids, 
                     cid.data(),[&](std::size_t i) { return tmp(i); });
      
      for (ordinal_type i=0;i<nsids;++i) blkidx(i+1) = blkidx(i) + cid(i);
      
      clear_array(nsids, cid);
      clear_array(nsids, flag);
    }
  }
  
  void SymbolicTools::
  computeSupernodesAssemblyTree(const ordinal_type_array &parent,
                                const ordinal_type_array &supernodes,
                                /* */ ordinal_type_array &stree_level,
                                /* */ ordinal_type_array &stree_parent,
                                /* */ size_type_array &stree_ptr,
                                /* */ ordinal_type_array &stree_children,
                                /* */ ordinal_type_array &stree_roots,
                                const ordinal_type_array &work) {
    const ordinal_type numSupernodes = supernodes.extent(0) - 1;
    const ordinal_type m = supernodes(numSupernodes);
    
    stree_parent = ordinal_type_array("stree_parent", m);
    auto flag    = Kokkos::subview(work, range_type(0*m, 1*m));        
    
    // color flag with supernodes (for the ease to detect supernode id from dofs)
    for (ordinal_type i=0;i<numSupernodes;++i) 
      for (ordinal_type j=supernodes(i);j<supernodes(i+1);++j) 
        flag(j) = i;
    
    // coarse parent into stree_parent
    for (ordinal_type sid=0;sid<numSupernodes;++sid) {
      stree_parent(sid) = -1;
      for (ordinal_type i=supernodes(sid);i<supernodes(sid+1);++i) {
        if (parent(i) >= 0) {
          const ordinal_type sidpar = flag(parent(i));
          if (sidpar != sid) stree_parent(sid) = sidpar;
        }
      }
    }
    
    auto clear_array = [](const ordinal_type cnt,
                          const ordinal_type_array &a) {
      memset(a.data(), 0, cnt*sizeof(typename ordinal_type_array::value_type));
    };
    
    // construct parent - child relations
    {
      clear_array(m, flag);
      ordinal_type cnt = 0;
      for (ordinal_type sid=0;sid<numSupernodes;++sid) {
        const ordinal_type sidpar = stree_parent(sid); 
        if (sidpar == -1) 
          ++cnt;
        else
          ++flag(stree_parent(sid));
      }
      stree_roots = ordinal_type_array(do_not_initialize_tag("stree_roots"), cnt);
    }
    
    // prefix scan
    {
      stree_ptr = size_type_array(do_not_initialize_tag("stree_ptr"), numSupernodes + 1); stree_ptr(0) = size_type();
      for (ordinal_type sid=0;sid<numSupernodes;++sid)
        stree_ptr(sid+1) = stree_ptr(sid) + flag(sid); 
    }
    
    {
      clear_array(numSupernodes, flag);
      ordinal_type cnt = 0;
      stree_children = ordinal_type_array(do_not_initialize_tag("stree_children"), stree_ptr(numSupernodes));
      for (ordinal_type sid=0;sid<numSupernodes;++sid) {
        const ordinal_type sidpar = stree_parent(sid);                       
        if (sidpar == -1) 
          stree_roots(cnt++) = sid;
        else
          stree_children(stree_ptr(sidpar)+flag(sidpar)++) = sid;
      }
    }
    
    {
      // this can be host parallel; but maybe this is too small workload
      stree_level = ordinal_type_array(do_not_initialize_tag("stree_level"), numSupernodes);
      for (ordinal_type sid=0;sid<numSupernodes;++sid) {
        ordinal_type self = sid, level = 0;
        for (; stree_parent(self) != -1; ++level)
          self = stree_parent(self);
        stree_level(sid) = level;
      }
    }
  }
  
  SymbolicTools::SymbolicTools() = default;
  SymbolicTools::SymbolicTools(const SymbolicTools &b) = default;

  SymbolicTools::SymbolicTools(const ordinal_type m,
                               const size_type_array &ap,
                               const ordinal_type_array &aj,
                               const ordinal_type_array &perm,
                               const ordinal_type_array &peri) 
    : _m(m),
      _ap(ap),
      _aj(aj),
      _perm(perm),
      _peri(peri) {}
  
  ordinal_type SymbolicTools::NumSupernodes() const { return _supernodes.extent(0) - 1; }      
  ordinal_type_array SymbolicTools::Supernodes() const { return _supernodes; }
  size_type_array SymbolicTools::gidSuperPanelPtr() const { return _gid_super_panel_ptr; }
  ordinal_type_array SymbolicTools::gidSuperPanelColIdx() const { return _gid_super_panel_colidx; }
  size_type_array SymbolicTools::sidSuperPanelPtr() const { return _sid_super_panel_ptr; }
  ordinal_type_array SymbolicTools::sidSuperPanelColIdx() const { return _sid_super_panel_colidx; }
  ordinal_type_array SymbolicTools::blkSuperPanelColIdx() const { return _blk_super_panel_colidx; }
  ordinal_type_array SymbolicTools::SupernodesTreeParent() const { return _stree_parent; }
  size_type_array SymbolicTools::SupernodesTreePtr() const { return _stree_ptr; }
  ordinal_type_array SymbolicTools::SupernodesTreeChildren() const { return _stree_children; }
  ordinal_type_array SymbolicTools::SupernodesTreeRoots() const { return _stree_roots; }
  ordinal_type_array SymbolicTools::SupernodesTreeLevel() const { return _stree_level; }
  ordinal_type_array SymbolicTools::PermVector() const { return _perm; }
  ordinal_type_array SymbolicTools::InvPermVector() const { return _peri; }

  void SymbolicTools::
  symbolicFactorize(const ordinal_type verbose) {
    Kokkos::Impl::Timer timer;
    double t_symfact = 0, t_supernode = 0, t_extra = 0, m_used =0, m_peak = 0;
    
    auto track_alloc = [&](const double in) {
      m_used += in;
      m_peak  = std::max(m_used, m_peak);
    };
    auto track_free = [&](const double out) {
      m_used -= out;
    };
    
    stat.nrows = _m;
    stat.nnz_a = _ap(_m);
    
    // compute elimination tree
    timer.reset();
    ordinal_type_array work(do_not_initialize_tag("work"), _m*4); 
    ordinal_type_array parent(do_not_initialize_tag("parent"), _m); 
    
    track_alloc(work.span()*sizeof(ordinal_type));
    track_alloc(parent.span()*sizeof(ordinal_type));
    {
      auto post = Kokkos::subview(work, range_type(0*_m, 1*_m));
      auto w    = Kokkos::subview(work, range_type(1*_m, 4*_m));
      
      // compute elimination tree and its post ordering
      computeEliminationTree(_m, _ap, _aj, _perm, _peri, parent, w);
      computePostOrdering(_m, parent, post, w);
      
      // update permutation vector and elimination tree
      ordinal_type *w0 = w.data(), *w1 = w0 + _m, *w2 = w1 + _m;
      for (ordinal_type i=0;i<_m;++i) {  
        w0[i]       = _perm(i);     
        w1[post(i)] = i; 
      }
      for (ordinal_type i=0;i<_m;++i) { 
        const ordinal_type q =  w0[post(i)];
        _perm(i) = q;
        _peri(q) = i; 
        const ordinal_type p = parent(post(i));
        if (p == -1) w2[i] = p;
        else         w2[i] = w1[p];
      }
      for (ordinal_type i=0;i<_m;++i) { 
        parent(i) = w2[i];
      }
      
      // do not compute etree again
      //computeEliminationTree(_m, _ap, _aj, _perm, _peri, parent, w);
    }
    t_symfact += timer.seconds();
    
    timer.reset();
    size_type_array ap;
    ordinal_type_array aj;        
    {
      // compute super node structure
      computeSupernodes(_m, _ap, _aj, _perm, _peri, parent, _supernodes, work);
      
      // compute fill pattern
      computeFillPatternUpper(_m, _ap, _aj, _perm, _peri, ap, aj, work);          
      
      // allocate supernodes
      allocateSupernodes(_m, ap, aj, _supernodes, work, 
                         _gid_super_panel_ptr,
                         _gid_super_panel_colidx,
                         _sid_super_panel_ptr,
                         _sid_super_panel_colidx,
                         _blk_super_panel_colidx);
    }
    t_supernode += timer.seconds();
    
    track_alloc(ap.span()*sizeof(size_type));
    track_alloc(aj.span()*sizeof(ordinal_type));
    
    track_alloc(_supernodes.span()*sizeof(ordinal_type));        
    track_alloc(_gid_super_panel_ptr.span()*sizeof(size_type));
    track_alloc(_gid_super_panel_colidx.span()*sizeof(ordinal_type));
    track_alloc(_sid_super_panel_ptr.span()*sizeof(size_type));
    track_alloc(_sid_super_panel_colidx.span()*sizeof(ordinal_type));
    track_alloc(_blk_super_panel_colidx.span()*sizeof(ordinal_type));
    
    timer.reset();
    {
      // supernode assembly tree
      computeSupernodesAssemblyTree(parent,                                                 
                                    _supernodes,                                             
                                    _stree_level,
                                    _stree_parent,
                                    _stree_ptr,
                                    _stree_children,
                                    _stree_roots,
                                    work);                        
    }
    t_extra += timer.seconds();
    
    track_alloc(_stree_level.span()*sizeof(ordinal_type));
    track_alloc(_stree_parent.span()*sizeof(ordinal_type));
    track_alloc(_stree_ptr.span()*sizeof(size_type));
    track_alloc(_stree_children.span()*sizeof(ordinal_type));
    track_alloc(_stree_roots.span()*sizeof(ordinal_type));
    
    track_free(ap.span()*sizeof(size_type));
    track_free(aj.span()*sizeof(ordinal_type));
    
    track_free(work.span()*sizeof(ordinal_type));
    track_free(parent.span()*sizeof(ordinal_type));
    
    stat.nnz_u = ap(_m);
    stat.nsupernodes = _supernodes.extent(0) - 1;
    stat.max_nchildren = 0;
    stat.largest_supernode = 0;
    stat.largest_schur = 0;
    
    for (ordinal_type sid=0;sid<stat.nsupernodes;++sid) {
      const ordinal_type m = _supernodes(sid+1) - _supernodes(sid);
      const ordinal_type n = _blk_super_panel_colidx(_sid_super_panel_ptr(sid+1)-1);
      const ordinal_type nchildren = _stree_ptr(sid+1) - _stree_ptr(sid);
      
      stat.max_nchildren     = max(stat.max_nchildren, nchildren); 
      stat.largest_supernode = max(stat.largest_supernode, m);
      stat.largest_schur     = max(stat.largest_schur,     n-m);
    }
    stat.nroots = _stree_roots.extent(0);
    
    if (verbose) {
      printf("Summary: SymbolicFactorize\n");
      printf("==========================\n");
      
      stat.height = 0;
      for (ordinal_type i=0;i<stat.nsupernodes;++i) {
        stat.height = max(_stree_level(i), stat.height);
      }
      
      stat.nleaves = 0;
      for (ordinal_type i=0;i<stat.nsupernodes;++i) {
        const ordinal_type nchildren = _stree_ptr(i+1) - _stree_ptr(i);
        stat.nleaves += (nchildren == 0);
      }
      
      const double kilo(1024);
      switch (verbose) {
      case 1: {
        printf("  Time\n");
        printf("             time for symbolic factorization:                 %10.6f s\n", t_symfact);
        printf("             time for allocation of supernode data structure: %10.6f s\n", t_supernode);
        printf("             time for additional calculations:                %10.6f s\n", t_extra);
        printf("             total time spent:                                %10.6f s\n", (t_symfact+t_supernode+t_extra));
        printf("\n");            
        printf("  Linear system A\n");
        printf("             number of equations:                             %10d\n", stat.nrows);
        printf("             number of nonzeros:                              %10.0f (%5.2f %% )\n", double(stat.nnz_a), double(stat.nnz_a)/(double(stat.nrows)*double(stat.nrows))*100.0);
        printf("\n");
        printf("  Factors U\n");
        printf("             number of nonzeros:                              %10.0f (%5.2f %% )\n", double(stat.nnz_u), double(stat.nnz_u)/(double(stat.nrows)*double(stat.nrows))*50.0);
        printf("             number of subgraphs:                             %10d\n", stat.nroots);
        printf("             number of supernodes:                            %10d\n", stat.nsupernodes);
        printf("             height of supernodal tree:                       %10d\n", stat.height);
        printf("             number of leaf supernodes:                       %10d\n", stat.nleaves);
        printf("             max number of children in tree:                  %10d\n", stat.max_nchildren);
        printf("             size of largest supernode:                       %10d\n", stat.largest_supernode);
        printf("             size of largest schur size:                      %10d\n", stat.largest_schur);
        printf("\n");
        printf("  Memory\n");
        printf("             memory used:                                     %10.4f MB\n", m_used/kilo/kilo);
        printf("             peak memory used:                                %10.4f MB\n", m_peak/kilo/kilo);
        printf("\n");
      }          
      }
    }
  }  

  void SymbolicTools::
  evaporateSymbolicFactors(const ordinal_type_array &aw,
                           const ordinal_type verbose) {
    Kokkos::Impl::Timer timer;
    double t_evaporate = 0, m_used =0, m_peak = 0;
    
    auto track_alloc = [&](const double in) {
      m_used += in;
      m_peak  = std::max(m_used, m_peak);
    };
    auto track_free = [&](const double out) {
      m_used -= out;
    };
    
    {
      const ordinal_type w_extent(aw.extent(0));
      if (_m > w_extent) {
        return;
      }
    }

    timer.reset();
    
    ///
    /// scan weights to compute the size of the system of linear equations
    ///
    size_type_array as(do_not_initialize_tag("as"), _m+1); 
    size_type_array aq(do_not_initialize_tag("aq"), _m+1); 
    
    track_alloc(as.span());
    track_alloc(aq.span());

    as(0) = 0; aq(0) = 0;
    for (ordinal_type i=0;i<_m;++i) {
      as(i+1) = as(i) + aw(i);
      aq(i+1) = aq(i) + aw(_peri(i));
    }
    TACHO_TEST_FOR_EXCEPTION(as(_m) != aq(_m), std::logic_error, 
                             "Error: SymbolicTools::evaporateSymbolicFactors, # of equations do not match between as and aq");

    ///
    /// Evaporate condensed graph for a debugging purpose
    ///
    const size_type m = as(_m);
#if 0
    {
      size_type_array ap(do_not_initialize_tag("ap"), m+1); 
      ap(0) = 0;
      {
        ordinal_type ii(0);
        for (ordinal_type i=0;i<_m;++i) {
          ordinal_type cnt(0);
          const ordinal_type 
            jbeg = _ap(i), 
            jend = _ap(i+1);
          for (ordinal_type j=jbeg;j<jend;++j) {
            const ordinal_type idx = _aj(j);
            cnt += (as(idx+1) - as(idx));
          }
          const ordinal_type kbeg = as(i), kend = as(i+1);
          for (ordinal_type k=kbeg;k<kend;++k,++ii) {
            ap(k+1) = ap(k) + cnt;
          }
        }
        TACHO_TEST_FOR_EXCEPTION(ii != m, std::logic_error, 
                                 "Error: SymbolicTools::evaporateSymbolicFactors, evaporation of ap fails");      
      }
      {
        printf("Evaporated # of rows: %d\n", m);
        for (ordinal_type i=0,iend=ap.extent(0);i<iend;++i) 
          printf("%d\n", ap(i));
      }
      ordinal_type_array aj(do_not_initialize_tag("ap"), ap(m));
      {
        for (ordinal_type i=0;i<_m;++i) {
          const ordinal_type 
            jbeg = _ap(i), 
            jend = _ap(i+1), 
            jjbeg = ap(as(i));
          for (ordinal_type j=jbeg,jj=jjbeg;j<jend;++j) {
            const ordinal_type idx = _aj(j);
            const ordinal_type kbeg = as(idx), kend = as(idx+1);
            for (ordinal_type k=kbeg;k<kend;++k,++jj)
              aj(jj) = k;        
          }
          const ordinal_type 
            kbeg = as(i), 
            kend = as(i+1),
            nnz_per_row = ap(kbeg+1)-ap(kbeg);

          for (ordinal_type k=kbeg+1;k<kend;++k) 
            memcpy(aj.data()+ap(k),
                   aj.data()+ap(kbeg),
                   sizeof(ordinal_type)*nnz_per_row);
        }
      }
      {
        printf("Evaporated # of nonzeros: %d\n", ordinal_type(aj.extent(0)));
        for (ordinal_type i=0,iend=aj.extent(0);i<iend;++i) 
          printf("%d\n", aj(i));
      }
    }
#endif

    ///
    /// Evaporate perm and peri
    ///
    ordinal_type_array perm(do_not_initialize_tag("perm"), m);
    ordinal_type_array peri(do_not_initialize_tag("peri"), m);

    track_alloc(perm.span());
    track_alloc(peri.span());

    for (ordinal_type i=0;i<_m;++i) {
      const ordinal_type idx = _perm(i), ndof = aw(i);
      const ordinal_type offt = aq(idx), offs = as(i);
      for (ordinal_type j=0;j<ndof;++j) {
        const ordinal_type tgt = offt+j, src = offs+j;
        perm(src) = tgt;
        peri(tgt) = src;
      }
    }

    ///
    /// Evaporate supernodes and gid colidx
    ///
    const ordinal_type nsupernodes = _supernodes.extent(0) - 1;
    size_type_array gid_super_panel_ptr(do_not_initialize_tag("gid_super_panel_ptr"), nsupernodes+1);
    track_alloc(gid_super_panel_ptr.span());
    {
      gid_super_panel_ptr(0) = 0;
      ordinal_type jbeg = _supernodes(0);
      for (ordinal_type i=0;i<nsupernodes;++i) {
        {
          const ordinal_type 
            jend = _supernodes(i+1);
          ordinal_type blk(0);
          for (ordinal_type j=jbeg;j<jend;++j) {
            const ordinal_type idx = _peri(j);
            blk += aw(idx);
          }
          _supernodes(i+1) = _supernodes(i) + blk;
          jbeg = jend;
        }
        {
          const ordinal_type
            kbeg = _gid_super_panel_ptr(i),
            kend = _gid_super_panel_ptr(i+1);
          ordinal_type blk(0);
          for (ordinal_type k=kbeg;k<kend;++k) { 
            const ordinal_type idx = _gid_super_panel_colidx(k);
            const ordinal_type ndof = (aq(idx+1) - aq(idx));
            blk += ndof;
          }
          gid_super_panel_ptr(i+1) = gid_super_panel_ptr(i) + blk;
        }
      }
    }

    ordinal_type_array gid_super_panel_colidx(do_not_initialize_tag("gid_super_panel_colidx"), gid_super_panel_ptr(nsupernodes));
    track_alloc(gid_super_panel_colidx.span());
    {
      for (ordinal_type i=0;i<nsupernodes;++i) {
        const ordinal_type
          jbeg = _gid_super_panel_ptr(i),
          jend = _gid_super_panel_ptr(i+1);
        ordinal_type cnt(gid_super_panel_ptr(i));
        for (ordinal_type j=jbeg;j<jend;++j) { 
          const ordinal_type idx = _gid_super_panel_colidx(j);
          const ordinal_type
            kbeg = aq(idx),
            kend = aq(idx+1);
          for (ordinal_type k=kbeg;k<kend;++k,++cnt) 
            gid_super_panel_colidx(cnt) = k;
        }
      }
    }
    
    ordinal_type_array blk_super_panel_colidx(do_not_initialize_tag("blk_super_panel_colidx"), _sid_super_panel_ptr(nsupernodes));
    track_alloc(blk_super_panel_colidx.span());
    {
      for (ordinal_type i=0;i<nsupernodes;++i) {
        const ordinal_type 
          jbeg = _sid_super_panel_ptr(i), 
          jend = _sid_super_panel_ptr(i+1)-1;
        const ordinal_type offs = _gid_super_panel_ptr(i);
        blk_super_panel_colidx(jbeg) = 0;
        for (ordinal_type j=jbeg;j<jend;++j) {
          const ordinal_type 
            kbeg = _blk_super_panel_colidx(j),
            kend = _blk_super_panel_colidx(j+1);
          ordinal_type blk(0);
          for (ordinal_type k=kbeg;k<kend;++k) {
            const ordinal_type idx = _gid_super_panel_colidx(offs+k);
            const ordinal_type ndof = aq(idx+1) - aq(idx);
            blk += ndof;
          }
          blk_super_panel_colidx(j+1) = blk_super_panel_colidx(j) + blk;
        }
      }
    }

    ///
    /// assign new objects
    ///
    track_free(_perm.span());
    track_free(_peri.span());
    track_free(_gid_super_panel_ptr.span());
    track_free(_gid_super_panel_colidx.span());
    track_free(_blk_super_panel_colidx.span());

    _m = m;
    /// _ap and _aj is not updated and remains as representing the input condensed graph
    _perm = perm;
    _peri = peri;
    _gid_super_panel_ptr = gid_super_panel_ptr;
    _gid_super_panel_colidx = gid_super_panel_colidx;
    _blk_super_panel_colidx = blk_super_panel_colidx;

    t_evaporate = timer.seconds();

    ///
    /// verbose output
    ///
    stat.nrows = _m;

    if (verbose) {
      printf("Summary: EvaporateSymbolicFactors\n");
      printf("=================================\n");

      const double kilo(1024);      
      switch (verbose) {
      case 1: {
        printf("  Time\n");
        printf("             time for evaporation:                            %10.6f s\n", t_evaporate);
        //printf("             total time spent:                                %10.6f s\n", t_evaporate);
        printf("\n");            
        printf("  Linear system A\n");
        printf("             number of equations:                             %10d\n", stat.nrows);
        printf("\n");
        printf("  Memory\n");
        printf("             memory used:                                     %10.4f MB\n", m_used/kilo/kilo);
        printf("             peak memory used:                                %10.4f MB\n", m_peak/kilo/kilo);
        printf("\n");
      }          
      }
    }
  }

}

