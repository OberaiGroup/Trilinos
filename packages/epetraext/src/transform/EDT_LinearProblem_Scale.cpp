//@HEADER
// ************************************************************************
// 
//          Trilinos: An Object-Oriented Solver Framework
//              Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//   
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//   
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
// 
// ************************************************************************
//@HEADER

#include <EDT_LinearProblem_GraphTrans.h>

#include <Epetra_Export.h>
#include <Epetra_Import.h>
#include <Epetra_LinearProblem.h>
#include <Epetra_CrsGraph.h>
#include <Epetra_CrsMatrix.h>
#include <Epetra_MultiVector.h>
#include <Epetra_Vector.h>
#include <Epetra_IntVector.h>
#include <Epetra_Map.h>
#include <Epetra_Comm.h>

namespace EpetraExt {

LinearProblem_GraphTrans::
~LinearProblem_GraphTrans()
{
  if( Exporter_ ) delete Exporter_;
  if( Importer_ ) delete Importer_;

  if( NewProblem_ ) delete NewProblem_;
  if( NewRHS_ ) delete NewRHS_;
  if( NewLHS_ ) delete NewLHS_;
  if( NewMatrix_ ) delete NewMatrix_;
}

LinearProblem_GraphTrans::NewTypeRef
LinearProblem_GraphTrans::
operator()( OriginalTypeRef orig )
{
  OldProblem_ = &orig;
  OldMatrix_ = dynamic_cast<Epetra_CrsMatrix*>( orig.GetMatrix() );
  OldGraph_ = const_cast<Epetra_CrsGraph*>(&OldMatrix_->Graph());
  OldRHS_ = orig.GetRHS();
  OldLHS_ = orig.GetLHS();
  OldRowMap_ = const_cast<Epetra_Map*>(&OldMatrix_->RowMap());

  int ierr = 0;

  const Epetra_Comm & CommObj = OldRowMap_->Comm();

  if( !OldMatrix_ ) ierr = -2;
  if( !OldRHS_ )    ierr = -3;
  if( !OldLHS_ )    ierr = -4;

  Epetra_CrsGraph & NewGraph = graphTrans_( *OldGraph_ );
  NewMatrix_ = new Epetra_CrsMatrix( Copy, NewGraph );

  Epetra_BlockMap & NewRowMap = const_cast<Epetra_BlockMap&>(NewGraph.RowMap());

  NewRHS_ = new Epetra_MultiVector( NewRowMap, 1 );
  NewLHS_ = new Epetra_MultiVector( NewRowMap, 1 );

  Exporter_ = new Epetra_Export( *OldRowMap_, NewRowMap );
  Importer_ = new Epetra_Import( NewRowMap, *OldRowMap_ );

  NewProblem_ = new Epetra_LinearProblem( NewMatrix_, NewLHS_, NewRHS_ );

  return *NewProblem_;
}

bool
LinearProblem_GraphTrans::
fwd()
{
  NewLHS_->Export( *OldLHS_, *Exporter_, Insert );
  NewRHS_->Export( *OldRHS_, *Exporter_, Insert );
  NewMatrix_->Export( *OldMatrix_, *Exporter_, Insert );

  return true;
}

bool
LinearProblem_GraphTrans::
rvs()
{
  OldLHS_->Import( *NewLHS_, *Exporter_, Insert );
  OldRHS_->Import( *NewRHS_, *Exporter_, Insert );
  OldMatrix_->Import( *NewMatrix_, *Exporter_, Insert );

  return true;
}

} //namespace EpetraExt

