// @HEADER
// ***********************************************************************
// 
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
// @HEADER

#include <MatrixMarket_Tpetra.hpp>
#include <Tpetra_DefaultPlatform.hpp>

#include <Kokkos_ConfigDefs.hpp>
#include <Kokkos_SerialNode.hpp>

#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_CommHelpers.hpp>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_oblackholestream.hpp>
#include <algorithm>

namespace {
  /// \fn getNode
  /// \brief Return an RCP to a Kokkos Node
  ///
  template<class NodeType>
  Teuchos::RCP<NodeType>
  getNode() {
    throw std::runtime_error ("This Kokkos Node type not supported (compile-time error)");
  }

  template<>
  Teuchos::RCP<Kokkos::SerialNode>
  getNode() {
    Teuchos::ParameterList defaultParams;
    return Teuchos::rcp (new Kokkos::SerialNode (defaultParams));
  }

  // Ensure that X and Y have the same dimensions, and that the
  // corresponding columns of X and Y have 2-norms that differ by no
  // more than a prespecified tolerance (that accounts for rounding
  // errors in computing the 2-norm).
  template<class MV>
  void
  assertMultiVectorsEqual (const Teuchos::RCP<const MV>& X, 
			   const Teuchos::RCP<const MV>& Y)
  {
    using Teuchos::RCP;
    using std::cerr;
    using std::cout;
    using std::endl;

    typedef typename MV::scalar_type scalar_type;
    typedef typename MV::local_ordinal_type local_ordinal_type;
    typedef typename MV::global_ordinal_type global_ordinal_type;
    typedef typename MV::node_type node_type;

    typedef Teuchos::ScalarTraits<scalar_type> STS;
    typedef typename STS::magnitudeType magnitude_type;
    typedef Teuchos::ScalarTraits<magnitude_type> STM;
    typedef Tpetra::Map<local_ordinal_type, global_ordinal_type, node_type> 
      map_type;

    TEUCHOS_TEST_FOR_EXCEPTION(X->getGlobalLength() != Y->getGlobalLength(),
      std::logic_error, "Y has a different number of rows than X.");
    TEUCHOS_TEST_FOR_EXCEPTION(X->getNumVectors() != Y->getNumVectors(),
      std::logic_error, "Y has a different number of columns than X.");
      
    const size_t numVecs = X->getNumVectors();
    Teuchos::Array<magnitude_type> X_norm2 (numVecs);
    Teuchos::Array<magnitude_type> Y_norm2 (numVecs);
    X->norm2 (X_norm2 ());
    Y->norm2 (Y_norm2 ());

    // What tolerance should I pick?  I'm using the typical heuristic
    // of the square root of the number of floating-point numbers
    // involved in computing the norm for one column.  Our output
    // routine is careful to use enough digits, so the input matrix
    // shouldn't be that much different.
    const magnitude_type tol = STS::squareroot (STS::magnitude (STS::eps ()) * Teuchos::as<magnitude_type> (X->getGlobalLength ()));
    std::vector<size_t> badColumns;
    for (size_t j = 0; j < numVecs; ++j) {
      // If the norm of the current column of X is zero, use the
      // absolute difference; otherwise use the relative difference.
      if ((X_norm2[j] == STM::zero() && STS::magnitude (Y_norm2[j]) > tol) ||
	  STS::magnitude (X_norm2[j] - Y_norm2[j]) > tol) {
	badColumns.push_back (j);
      }
    }
    
    if (badColumns.size() > 0) {
      const size_t numBad = badColumns.size();
      std::ostringstream os;
      std::copy (badColumns.begin(), badColumns.end(), 
		 std::ostream_iterator<size_t> (os, " "));
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::logic_error, "Column" 
        << (numBad != 1 ? "s" : "") << " [" << os.str() << "] of X and Y have "
	"norms that differ relatively by more than " << tol << ".");
    }
  }

  // Test Tpetra::MatrixMarket::Reader::readDenseFile()
  //
  // \tparam MV Tpetra::MultiVector specialization
  //
  // \param inputFilename [in] Name of the Matrix Market format dense
  //   matrix file to read (on Proc 0 of comm only).
  // \param outputFilename [in] Filename to which we can validly write
  //   (on Proc 0 of comm only); used as a sanity test.  If empty
  //   (""), the file is not written.
  // \param comm [in] Communicator, over whose process(es) to
  //   distribute the returned Tpetra::MultiVector.
  // \param node [in] Kokkos node instance (needed for creating the
  //   Tpetra::MultiVector to return).
  // \param tolerant [in] Whether or not to parse the file tolerantly.
  // \param verbose [in] Whether to print verbose output.
  // \param debug [in] Whether to print debugging output.
  template<class MV>
  Teuchos::RCP<MV>
  testReadDenseFile (const std::string& inputFilename, 
		     const std::string& outputFilename,
		     const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
		     const Teuchos::RCP<typename MV::node_type>& node,
		     const bool tolerant, 
		     const bool verbose,
		     const bool debug)
  {
    using Teuchos::RCP;
    using std::cerr;
    using std::cout;
    using std::endl;

    typedef typename MV::scalar_type scalar_type;
    typedef typename MV::local_ordinal_type local_ordinal_type;
    typedef typename MV::global_ordinal_type global_ordinal_type;
    typedef typename MV::node_type node_type;

    typedef Teuchos::ScalarTraits<scalar_type> STS;
    typedef typename STS::magnitudeType magnitude_type;
    typedef Tpetra::Map<local_ordinal_type, global_ordinal_type, node_type> 
      map_type;

    // The reader and writer classes are templated on the
    // Tpetra::CrsMatrix specialization, from which the
    // Tpetra::MultiVector specialization is derived.
    typedef Tpetra::CrsMatrix<scalar_type, local_ordinal_type, 
      global_ordinal_type, node_type> sparse_matrix_type;

    const int myRank = comm->getRank ();
    if (verbose && myRank == 0) {
      cout << "testReadDenseFile: reading Matrix Market file \"" 
	   << inputFilename << "\":" << endl;
    }

    // Map describing multivector distribution; starts out null and is
    // an output argument of readDenseFile().
    RCP<const map_type> map = Teuchos::null; 

    // Read the dense matrix from the given Matrix Market file.
    // This routine acts like an MPI barrier.
    typedef Tpetra::MatrixMarket::Reader<sparse_matrix_type> reader_type;
    RCP<MV> X = reader_type::readDenseFile (inputFilename, comm, node,
					    map, tolerant, debug);
    TEUCHOS_TEST_FOR_EXCEPTION(X.is_null(), std::runtime_error,
      "The Tpetra::MultiVector returned from readDenseFile() is null.");
    TEUCHOS_TEST_FOR_EXCEPTION(map.is_null(), std::runtime_error,
      "The Tpetra::Map returned from readDenseFile() is null.");
    if (! X.is_null() && verbose && myRank == 0) {
      cout << "Finished reading file." << endl;
    }

    // Sanity check: write the multivector X to a file, read it back
    // in as a different multivector Y, then make sure it has the same
    // column norms as X.  This assumes that writing a multivector
    // works.  Only do this if the caller supplied a nonempty output
    // file name.
    if (outputFilename != "") {
      typedef Tpetra::MatrixMarket::Writer<sparse_matrix_type> writer_type;
      writer_type::writeDenseFile (outputFilename, X);
      // For the sanity test, we also check that reading in the
      // multivector produces a Map which is the same as X's Map.
      // We're not testing here the functionality of readDenseFile()
      // to reuse an existing Map.
      RCP<const map_type> testMap;
      RCP<MV> Y = reader_type::readDenseFile (inputFilename, comm, node, 
					      testMap, tolerant, debug);
      TEUCHOS_TEST_FOR_EXCEPTION(! map->isSameAs (*testMap), std::logic_error,
        "Writing the read-in Tpetra::MultiVector X to a file and reading it "
        "back in resulted in a multivector Y with a different Map.  Please "
        "report this bug to the Tpetra developers.");
      assertMultiVectorsEqual<MV> (X, Y);
    }
    return X;
  }

  // Test Tpetra::MatrixMarket::Reader::writeDenseFile()
  //
  // \tparam MV Tpetra::MultiVector specialization
  //
  // \param outputFilename [in] Name of the Matrix Market format dense
  //   matrix file to write (on Proc 0 in X's communicator only).
  // \param X [in] The nonnull Tpetra::MultiVector instance to write
  //   to the given file.
  // \param echo [in] Whether or not to echo the output to stdout (on
  //   Proc 0 in X's communicator only).
  // \param verbose [in] Whether to print verbose output.
  // \param debug [in] Whether to print debugging output.
  template<class MV>
  void
  testWriteDenseFile (const std::string& outputFilename,
		      const Teuchos::RCP<const MV>& X,
		      const bool echo,
		      const bool verbose,
		      const bool debug)
  {
    using Teuchos::RCP;
    using std::cerr;
    using std::cout;
    using std::endl;

    typedef typename MV::scalar_type scalar_type;
    typedef typename MV::local_ordinal_type local_ordinal_type;
    typedef typename MV::global_ordinal_type global_ordinal_type;
    typedef typename MV::node_type node_type;

    typedef Teuchos::ScalarTraits<scalar_type> STS;
    typedef typename STS::magnitudeType magnitude_type;
    typedef Tpetra::Map<local_ordinal_type, global_ordinal_type, node_type> 
      map_type;

    // The reader and writer classes are templated on the
    // Tpetra::CrsMatrix specialization, from which the
    // Tpetra::MultiVector specialization is derived.
    typedef Tpetra::CrsMatrix<scalar_type, local_ordinal_type, 
      global_ordinal_type, node_type> sparse_matrix_type;

    TEUCHOS_TEST_FOR_EXCEPTION(X.is_null(), std::invalid_argument,
      "testWriteDenseFile: The input Tpetra::MultiVector instance X is null.");

    RCP<const map_type> map = X->getMap();
    RCP<const Teuchos::Comm<int> > comm = map->getComm();
    const int myRank = comm->getRank ();

    if (verbose && myRank == 0) {
      cout << "testWriteDenseFile: writing Tpetra::MultiVector instance to "
	"file \"" << outputFilename << "\"" << endl;
    }
    // If specified, write the read-in sparse matrix to a file and/or
    // echo it to stdout.
    typedef Tpetra::MatrixMarket::Writer<sparse_matrix_type> writer_type;
    if (outputFilename != "") {
      writer_type::writeDenseFile (outputFilename, X);
    }
    if (verbose && myRank == 0) {
      cout << "Finished writing file." << endl;
    }
    if (echo) {
      if (verbose && myRank == 0) {
	cout << "Echoing output to stdout." << endl;
      }
      writer_type::writeDense (cout, X);
    }

    // Sanity check: read in the multivector again, and make sure that
    // it has the same column norms as the input multivector.  This
    // assumes that reading a multivector works.
    typedef Tpetra::MatrixMarket::Reader<sparse_matrix_type> reader_type;
    RCP<node_type> node = map->getNode();
    const bool tolerant = false; // Our own output shouldn't need tolerant mode.
    RCP<MV> Y = reader_type::readDenseFile (outputFilename, comm, node, 
					    map, tolerant, debug);
    assertMultiVectorsEqual<MV> (X, Y);
  }

} // namespace (anonymous)


int 
main (int argc, char *argv[]) 
{
  using Teuchos::Comm;
  using Teuchos::CommandLineProcessor;
  using Teuchos::ParameterList;
  using Teuchos::RCP;
  using Teuchos::rcp;
  using std::cout;
  using std::endl;

  typedef double scalar_type;
  typedef int local_ordinal_type;
  typedef int global_ordinal_type;
  typedef Kokkos::SerialNode node_type;

  Teuchos::GlobalMPISession mpiSession (&argc, &argv, &cout);
  RCP<const Comm<int> > comm = 
    Tpetra::DefaultPlatform::getDefaultPlatform().getComm();
  const int myRank = comm->getRank();

  std::string inputFilename;  // Matrix Market file to read
  std::string outputFilename; // Matrix Market file to write (if applicable)
  bool testWrite = true; // Test Matrix Market output?
  bool tolerant = false; // Parse the file tolerantly?
  bool echo = false;     // Echo the read-in matrix back?
  bool verbose = false;  // Verbosity of output
  bool debug = false;    // Print debugging info?

  CommandLineProcessor cmdp (false, true);
  cmdp.setOption ("inputFilename", &inputFilename,
		  "Name of the Matrix Market dense matrix file to read.");
  cmdp.setOption ("outputFilename", &outputFilename,
		  "If --testWrite is true, then write the read-in matrix to "
		  "this file in Matrix Market format on (MPI) Proc 0.  "
		  "Otherwise, this argument is ignored.  Note that the output "
		  "file may not be identical to the input file.");
  cmdp.setOption ("testWrite", "noTestWrite", &testWrite,
		  "Whether to test Matrix Market file output.  Ignored if no "
		  "--outputFilename value is given.");
  cmdp.setOption ("tolerant", "strict", &tolerant, 
		  "Whether to parse the input Matrix Market file tolerantly.");
  cmdp.setOption ("echo", "noecho", &echo,
		  "Whether to echo the read-in matrix back to stdout on Rank 0 "
		  "in Matrix Market format.  Note that the echoed matrix may "
		  "not be identical to the input file.");
  cmdp.setOption ("verbose", "quiet", &verbose, "Print messages and results.");
  cmdp.setOption ("debug", "nodebug", &debug, "Print debugging information.");

  // Parse the command-line arguments.
  {
    const CommandLineProcessor::EParseCommandLineReturn parseResult = 
      cmdp.parse (argc,argv);
    // If the caller asks us to print the documentation, or does not
    // explicitly say to run the benchmark, we let this "test" pass
    // trivially.
    if (parseResult == CommandLineProcessor::PARSE_HELP_PRINTED) {
      if (myRank == 0) {
	cout << "End Result: TEST PASSED" << endl;
      }
      return EXIT_SUCCESS;
    }
    TEUCHOS_TEST_FOR_EXCEPTION(parseResult != CommandLineProcessor::PARSE_SUCCESSFUL, 
      std::invalid_argument, "Failed to parse command-line arguments.");
  }

  // Get a Kokkos Node instance for the particular Node type.
  RCP<node_type> node = getNode<node_type>();

  // Test reading in the sparse matrix.  If no filename or an empty
  // filename is specified, we don't invoke the test and report a
  // "TEST PASSED" message.
  if (inputFilename != "") {
    // Convenient abbreviations
    typedef scalar_type ST;
    typedef local_ordinal_type LO;
    typedef global_ordinal_type GO;
    typedef node_type NT;
    typedef Tpetra::MultiVector<ST, LO, GO, NT> MV;

    // If not testing writes, don't do the sanity check that tests
    // input by comparing against output.
    std::string outFilename = testWrite ? outputFilename : "";
    RCP<MV> X = testReadDenseFile<MV> (inputFilename, outFilename, comm,
				       node, tolerant, verbose, debug);
    if (outFilename != "") {
      testWriteDenseFile<MV> (outFilename, X, echo, verbose, debug);
    }
  }

  // Only Rank 0 gets to write to cout.
  if (myRank == 0) {
    cout << "End Result: TEST PASSED" << endl;
  }
  return EXIT_SUCCESS;
}



