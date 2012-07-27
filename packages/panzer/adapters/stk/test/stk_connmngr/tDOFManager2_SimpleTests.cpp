// STL includes
#include <iostream>
#include <vector>
#include <set>

// Teuchos includes
#include "Teuchos_ConfigDefs.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_FancyOStream.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_ArrayView.hpp"

//Teuchos Testing
#include "Teuchos_UnitTestHarness.hpp"
#include <Teuchos_Tuple.hpp>

//Tpetra includes
#include "Tpetra_DefaultPlatform.hpp"
#include "Tpetra_Map.hpp"

// Intrepid includes
#include "Intrepid_HGRAD_QUAD_C1_FEM.hpp"
#include "Intrepid_HGRAD_QUAD_C2_FEM.hpp"
#include "Intrepid_HGRAD_TET_C1_FEM.hpp"
#include "Intrepid_HCURL_TET_I1_FEM.hpp"
#include "Intrepid_HGRAD_TRI_C1_FEM.hpp"

// Panzer includes
#include "Panzer_ConnManager.hpp"
#include "Panzer_FieldPattern.hpp"
#include "Panzer_IntrepidFieldPattern.hpp"
#include "Panzer_GeometricAggFieldPattern.hpp"
#include "Panzer_FieldAggPattern.hpp"

// Panzer_STK includes
#include "Panzer_STK_config.hpp"
#include "Panzer_STK_Interface.hpp"
#include "Panzer_STK_SquareQuadMeshFactory.hpp"
#include "Panzer_STK_CubeTetMeshFactory.hpp"
#include "Panzer_STKConnManager.hpp"

#include "Panzer_DOFManager2.cpp"

#ifdef HAVE_MPI
   #include "Epetra_MpiComm.h"
   #include "mpi.h"
#endif

#include <iostream>

typedef int LO;
typedef int GO;
using Teuchos::RCP;
using Teuchos::Array;
using Teuchos::ArrayView;
using Teuchos::rcp;
typedef Intrepid::FieldContainer<double> FieldContainer;

namespace {

  TEUCHOS_UNIT_TEST( DOFManager2_tests, BasicCreation ){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",2);
    pl->set("X Elements",4);
    pl->set("Y Elements",4);
    
    panzer_stk::SquareQuadMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<LO,GO> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<LO,GO>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<LO,GO> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_QUAD_C1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], pattern);

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    my_DOFManager2->buildGlobalUnknowns();
    //Now that we have created the SingleBlockDOFManager, we can ensure it was created correctly.
    TEST_EQUALITY(my_DOFManager2->getConnManager(),conn);


    TEST_EQUALITY(my_DOFManager2->getFieldNum("Velocity"),0);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Temperature"),1);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Radiation Levels"),2);

    TEST_EQUALITY(my_DOFManager2->getNumFields(), 3);

    const std::vector<GO> & vel_offests = my_DOFManager2->getGIDFieldOffsets("eblock-0_0",0); 
    const std::vector<GO> & tem_offests = my_DOFManager2->getGIDFieldOffsets("eblock-0_0",1); 
    const std::vector<GO> & rad_offests = my_DOFManager2->getGIDFieldOffsets("eblock-0_0",2); 

    TEST_EQUALITY(vel_offests.size(),tem_offests.size());
    TEST_EQUALITY(tem_offests.size(),rad_offests.size());
  }
  TEUCHOS_UNIT_TEST( DOFManager2_tests, ReorderingFields ){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",2);
    pl->set("X Elements",4);
    pl->set("Y Elements",4);
    
    panzer_stk::SquareQuadMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<LO,GO> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<LO,GO>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<LO,GO> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_QUAD_C1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    names.push_back("Frequency");
    names.push_back("Pitch");
    names.push_back("Yaw");
    names.push_back("Brightness");
    names.push_back("Momentum");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], pattern);
    my_DOFManager2->addField(names[3], pattern);
    my_DOFManager2->addField(names[4], pattern);
    my_DOFManager2->addField(names[5], pattern);
    my_DOFManager2->addField(names[6], pattern);
    my_DOFManager2->addField(names[7], pattern);


    TEST_EQUALITY(my_DOFManager2->getNumFields(),8);

    TEST_EQUALITY(my_DOFManager2->getFieldNum("Velocity"),0);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Temperature"),1);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Radiation Levels"),2);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Frequency"),3);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Pitch"),4);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Yaw"),5);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Brightness"),6);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Momentum"),7);


    std::vector<std::string> invalid_names;
    invalid_names.push_back("Verocity");
    invalid_names.push_back("Temperature");
    invalid_names.push_back("Irradiation Levels");
    invalid_names.push_back("Infrequency");
    invalid_names.push_back("Off-Pitch");
    invalid_names.push_back("Yawn");
    invalid_names.push_back("Brightness");
    invalid_names.push_back("Momentum");

    TEST_ASSERT(my_DOFManager2->validFieldOrder(names));
    TEST_ASSERT(!(my_DOFManager2->validFieldOrder(invalid_names)));
    TEST_THROW(my_DOFManager2->setFieldOrder(invalid_names),std::logic_error);

    TEST_EQUALITY(my_DOFManager2->getFieldNum("Velocity"),0);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Temperature"),1);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Radiation Levels"),2);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Frequency"),3);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Pitch"),4);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Yaw"),5);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Brightness"),6);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Momentum"),7);

    std::vector<std::string> new_names;
    new_names.push_back("Momentum");
    new_names.push_back("Frequency");
    new_names.push_back("Velocity");
    new_names.push_back("Temperature");
    new_names.push_back("Pitch");
    new_names.push_back("Brightness");
    new_names.push_back("Yaw");
    new_names.push_back("Radiation Levels");

    TEST_ASSERT(my_DOFManager2->validFieldOrder(new_names));
    TEST_NOTHROW(my_DOFManager2->setFieldOrder(new_names));

    TEST_EQUALITY(my_DOFManager2->getFieldNum("Velocity"),2);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Temperature"),3);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Radiation Levels"),7);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Frequency"),1);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Pitch"),4);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Yaw"),6);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Brightness"),5);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Momentum"),0);

    TEST_NOTHROW(my_DOFManager2->addField("Current",pattern));

    TEST_EQUALITY(my_DOFManager2->getNumFields(),9);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Current"),8);

    TEST_NOTHROW(my_DOFManager2->buildGlobalUnknowns());

    TEST_EQUALITY(my_DOFManager2->getFieldNum("Current"),8);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Velocity"),2);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Temperature"),3);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Radiation Levels"),7);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Frequency"),1);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Pitch"),4);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Yaw"),6);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Brightness"),5);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Momentum"),0);

    new_names.push_back("Current");

    //You can't reassign anything.
    TEST_THROW(my_DOFManager2->setFieldOrder(new_names),std::logic_error);
    TEST_THROW(my_DOFManager2->buildGlobalUnknowns(),std::logic_error);
    TEST_THROW(my_DOFManager2->addField("Tide",pattern),std::logic_error);
    TEST_THROW(my_DOFManager2->setConnManager(conn,MPI_COMM_WORLD),std::logic_error);

    //All of the fields shoudl be in this block.
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Pitch","eblock-0_0"));
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Yaw","eblock-0_0"));
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Velocity","eblock-0_0"));

    std::vector<int> all_values = my_DOFManager2->getBlockFieldNumbers("eblock-0_0");
    std::vector<int> compare_values(9);
    compare_values[0]=0;
    compare_values[1]=1;
    compare_values[2]=2;
    compare_values[3]=3;
    compare_values[4]=4;
    compare_values[5]=5;
    compare_values[6]=6;
    compare_values[7]=7;
    compare_values[8]=8;
    bool all_so_far=true;
    for (size_t i = 0; i < all_values.size(); ++i) {
      bool there=false;
      for (size_t j = 0; j < compare_values.size(); ++j) {
        if(all_values[i]==compare_values[j])
          there=true;
      }
      if(all_so_far && !there)
        all_so_far=false;
    }
    TEST_ASSERT(all_so_far);
  }

  TEUCHOS_UNIT_TEST( DOFManager2_tests, myOwnedwithGhosted){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",2);
    pl->set("X Elements",4);
    pl->set("Y Elements",4);
    
    panzer_stk::SquareQuadMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<int,int> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<int,int>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<int,int> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_QUAD_C1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], pattern);

    my_DOFManager2->buildGlobalUnknowns();

    std::vector<int> myo;
    std::vector<int> myog;

    my_DOFManager2->getOwnedIndices(myo);
    my_DOFManager2->getOwnedAndSharedIndices(myog);
    //every value in my owned should be in my owned and ghosted.
    for (size_t i = 0; i < myo.size(); ++i) {
      bool found=false;
      for (size_t j = 0; j < myog.size(); ++j) {
        if(myo[i]==myog[j])
          found=true;
      }
      TEST_ASSERT(found);
    }
  }

  TEUCHOS_UNIT_TEST( DOFManager2_tests, gidsAreSet){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",2);
    pl->set("X Elements",4);
    pl->set("Y Elements",4);
    
    panzer_stk::SquareQuadMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<int,int> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<int,int>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<int,int> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_QUAD_C1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], pattern);

    my_DOFManager2->buildGlobalUnknowns();

    std::vector<std::string> block_names;
    conn->getElementBlockIds(block_names);
    for (size_t b = 0; b < block_names.size(); ++b) {
      const std::vector<int> & myElements = conn->getElementBlock(block_names[b]);
      for (size_t e = 0; e < myElements.size(); ++e) {
        std::vector<int> acquiredGIDs;
        my_DOFManager2->getElementGIDs(myElements[e],acquiredGIDs);
        //Now we need to make sure that acquiredGIDs form sets.
        for (size_t i = 0; i < acquiredGIDs.size(); ++i) {
          bool found=false;
          for (size_t j = i+1; j < acquiredGIDs.size(); ++j) {
            if(acquiredGIDs[i]==acquiredGIDs[j])
              found=true;
          }
          TEST_ASSERT(!found);
        }
      }
    }

  }

  TEUCHOS_UNIT_TEST( DOFManager2_tests, gidFieldAssociations){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",1);
    pl->set("X Elements",4);
    pl->set("Y Elements",2);
    
    panzer_stk::SquareQuadMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<int,int> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<int,int>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<int,int> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_QUAD_C1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], pattern);

    my_DOFManager2->buildGlobalUnknowns();

    //Stores associated gid values. Indexed according to names. Local.
    //It would be helpful to do this test cross processor, but I'm not sure how.
    std::vector<std::vector<int> > gids(3);

    //Load

    std::vector<std::string> block_names;
    conn->getElementBlockIds(block_names);
    for (size_t b = 0; b < block_names.size(); ++b) {
      const std::vector<int> & myElements = conn->getElementBlock(block_names[b]);
      for (size_t e = 0; e < myElements.size(); ++e) {
        std::vector<int> acquiredGIDs;
        my_DOFManager2->getElementGIDs(myElements[e],acquiredGIDs);
        for (size_t i = 0; i < names.size(); ++i) {
          const std::vector<int> offsets = my_DOFManager2->getGIDFieldOffsets(block_names[b],my_DOFManager2->getFieldNum(names[i]));
          for (size_t j = 0; j < offsets.size(); ++j) {
            gids[i].push_back(acquiredGIDs[offsets[j]]);
          }
        }
      }
    }

    //Test
    for (size_t i = 0; i < gids.size(); ++i) {
      for (size_t j = 0; j < gids[i].size(); ++j) {
        //We need to make sure value gids[i][j] does not occur in any of the other vectors
        bool found=false;
        for (size_t k = 0; k < gids.size(); ++k) {
          if (k!=i) {
            for (size_t l = 0; l < gids[k].size(); ++l) {
              if(gids[i][j]==gids[k][l])
                found=true;
            }
          }
        }
        TEST_ASSERT(!found);
      }
    }

  }


//=============================================================

  TEUCHOS_UNIT_TEST( DOFManager2_tests, 3dmyOwnedwithGhosted){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",1);
    pl->set("Z Blocks",1);
    pl->set("X Elements",4);
    pl->set("Y Elements",2);
    pl->set("Z Elements",2);
    
    panzer_stk::CubeTetMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<int,int> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<int,int>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<int,int> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_TET_C1_FEM<double,FieldContainer>);
    RCP<Intrepid::Basis<double,FieldContainer> > secbasis = Teuchos::rcp(new Intrepid::Basis_HCURL_TET_I1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));
    RCP< const panzer::FieldPattern> secpattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(secbasis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], secpattern);

    my_DOFManager2->buildGlobalUnknowns();

    std::vector<int> myo;
    std::vector<int> myog;

    my_DOFManager2->getOwnedIndices(myo);
    my_DOFManager2->getOwnedAndSharedIndices(myog);
    //every value in my owned should be in my owned and ghosted.
    for (size_t i = 0; i < myo.size(); ++i) {
      bool found=false;
      for (size_t j = 0; j < myog.size(); ++j) {
        if(myo[i]==myog[j])
          found=true;
      }
      TEST_ASSERT(found);
    }
  }

  TEUCHOS_UNIT_TEST( DOFManager2_tests, 3dgidsAreSet){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",1);
    pl->set("Z Blocks",1);
    pl->set("X Elements",4);
    pl->set("Y Elements",2);
    pl->set("Z Elements",2);
    
    panzer_stk::CubeTetMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<int,int> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<int,int>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<int,int> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_TET_C1_FEM<double,FieldContainer>);
    RCP<Intrepid::Basis<double,FieldContainer> > secbasis = Teuchos::rcp(new Intrepid::Basis_HCURL_TET_I1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));
    RCP< const panzer::FieldPattern> secpattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(secbasis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], secpattern);

    my_DOFManager2->buildGlobalUnknowns();

    std::vector<std::string> block_names;
    conn->getElementBlockIds(block_names);
    for (size_t b = 0; b < block_names.size(); ++b) {
      const std::vector<int> & myElements = conn->getElementBlock(block_names[b]);
      for (size_t e = 0; e < myElements.size(); ++e) {
        std::vector<int> acquiredGIDs;
        my_DOFManager2->getElementGIDs(myElements[e],acquiredGIDs);
        //Now we need to make sure that acquiredGIDs form sets.
        for (size_t i = 0; i < acquiredGIDs.size(); ++i) {
          bool found=false;
          for (size_t j = i+1; j < acquiredGIDs.size(); ++j) {
            if(acquiredGIDs[i]==acquiredGIDs[j])
              found=true;
          }
          TEST_ASSERT(!found);
        }
      }
    }

  }

  TEUCHOS_UNIT_TEST( DOFManager2_tests, 3dgidFieldAssociations){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",1);
    pl->set("Z Blocks",1);
    pl->set("X Elements",4);
    pl->set("Y Elements",2);
    pl->set("Z Elements",2);
    
    panzer_stk::CubeTetMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<int,int> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<int,int>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<int,int> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_TET_C1_FEM<double,FieldContainer>);
    RCP<Intrepid::Basis<double,FieldContainer> > secbasis = Teuchos::rcp(new Intrepid::Basis_HCURL_TET_I1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));
    RCP< const panzer::FieldPattern> secpattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(secbasis));

    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->addField(names[0], pattern);
    my_DOFManager2->addField(names[1], pattern);
    my_DOFManager2->addField(names[2], secpattern);

    my_DOFManager2->buildGlobalUnknowns();

    //Stores associated gid values. Indexed according to names. Local.
    //It would be helpful to do this test cross processor, but I'm not sure how.
    std::vector<std::vector<int> > gids(3);

    //Load

    std::vector<std::string> block_names;
    conn->getElementBlockIds(block_names);
    for (size_t b = 0; b < block_names.size(); ++b) {
      const std::vector<int> & myElements = conn->getElementBlock(block_names[b]);
      for (size_t e = 0; e < myElements.size(); ++e) {
        std::vector<int> acquiredGIDs;
        my_DOFManager2->getElementGIDs(myElements[e],acquiredGIDs);
        for (size_t i = 0; i < names.size(); ++i) {
          const std::vector<int> offsets = my_DOFManager2->getGIDFieldOffsets(block_names[b],my_DOFManager2->getFieldNum(names[i]));
          for (size_t j = 0; j < offsets.size(); ++j) {
            gids[i].push_back(acquiredGIDs[offsets[j]]);
          }
        }
      }
    }

    //Test
    for (size_t i = 0; i < gids.size(); ++i) {
      for (size_t j = 0; j < gids[i].size(); ++j) {
        //We need to make sure value gids[i][j] does not occur in any of the other vectors
        bool found=false;
        for (size_t k = 0; k < gids.size(); ++k) {
          if (k!=i) {
            for (size_t l = 0; l < gids[k].size(); ++l) {
              if(gids[i][j]==gids[k][l])
                found=true;
            }
          }
        }
        TEST_ASSERT(!found);
      }
    }

  }

  
  TEUCHOS_UNIT_TEST( DOFManager2_tests, multiBloc){
    RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",1);
    pl->set("Y Blocks",2);
    pl->set("X Elements",5);
    pl->set("Y Elements",5);
    
    panzer_stk::SquareQuadMeshFactory factory; 
    factory.setParameterList(pl);
    RCP<panzer_stk::STK_Interface> mesh = factory.buildMesh(MPI_COMM_WORLD);

    RCP<panzer::DOFManager2<LO,GO> > my_DOFManager2 = Teuchos::rcp(new panzer::DOFManager2<LO,GO>());
    TEST_EQUALITY(my_DOFManager2->getComm(),Teuchos::null);

    RCP<panzer::ConnManager<LO,GO> > conn = rcp(new panzer_stk::STKConnManager(mesh));

    RCP<Intrepid::Basis<double,FieldContainer> > basis = Teuchos::rcp(new Intrepid::Basis_HGRAD_QUAD_C1_FEM<double,FieldContainer>);
     
    RCP< const panzer::FieldPattern> pattern = Teuchos::rcp(new panzer::IntrepidFieldPattern(basis));

    std::vector<std::string> names;
    names.push_back("Velocity");
    names.push_back("Temperature");
    names.push_back("Radiation Levels");
    my_DOFManager2->setConnManager(conn, MPI_COMM_WORLD);
    my_DOFManager2->addField("eblock-0_0",names[0], pattern);
    my_DOFManager2->addField("eblock-0_1",names[1], pattern);
    my_DOFManager2->addField(names[2], pattern);


    my_DOFManager2->buildGlobalUnknowns();
    //Now that we have created the SingleBlockDOFManager, we can ensure it was created correctly.
    TEST_EQUALITY(my_DOFManager2->getConnManager(),conn);


    TEST_EQUALITY(my_DOFManager2->getFieldNum("Velocity"),0);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Temperature"),1);
    TEST_EQUALITY(my_DOFManager2->getFieldNum("Radiation Levels"),2);

    TEST_EQUALITY(my_DOFManager2->getNumFields(), 3);

    //const std::vector<GO> & vel_offests = my_DOFManager2->getGIDFieldOffsets("eblock-0_0",0); 
    //const std::vector<GO> & tem_offests = my_DOFManager2->getGIDFieldOffsets("eblock-0_0",1); 
    //const std::vector<GO> & rad_offests = my_DOFManager2->getGIDFieldOffsets("eblock-0_0",2); 
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Velocity","eblock-0_0"));
    TEST_ASSERT(!my_DOFManager2->fieldInBlock("Velocity","eblock-0_1"));
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Temperature","eblock-0_1"));
    TEST_ASSERT(!my_DOFManager2->fieldInBlock("Temperature","eblock-0_0"));
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Radiation Levels","eblock-0_1"));
    TEST_ASSERT(my_DOFManager2->fieldInBlock("Radiation Levels","eblock-0_0"));

    //TEST_EQUALITY(vel_offests.size(),tem_offests.size());
    //TEST_EQUALITY(tem_offests.size(),rad_offests.size());
  }


} /*generic namespace*/ 
