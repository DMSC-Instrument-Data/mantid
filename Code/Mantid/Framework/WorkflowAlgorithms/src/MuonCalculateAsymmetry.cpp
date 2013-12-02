/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MuonCalculateAsymmetry)
  
  //----------------------------------------------------------------------------------------------
  /**
   * Constructor
   */
  MuonCalculateAsymmetry::MuonCalculateAsymmetry()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /**
   * Destructor
   */
  MuonCalculateAsymmetry::~MuonCalculateAsymmetry()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MuonCalculateAsymmetry::name() const { return "MuonCalculateAsymmetry";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MuonCalculateAsymmetry::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MuonCalculateAsymmetry::category() const { return "Workflow\\Muon"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MuonCalculateAsymmetry::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MuonCalculateAsymmetry::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("FirstPeriodWorkspace","",Direction::Input), 
      "First period data. If second period is not specified - the only one used.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("SecondPeriodWorkspace","",Direction::Input, 
      PropertyMode::Optional), "Second period data. If not spefied - first period used only.");

    std::vector<std::string> allowedOperations;
    allowedOperations.push_back("+");
    allowedOperations.push_back("-");
    declareProperty("PeriodOperation","+", boost::make_shared<StringListValidator>(allowedOperations),
      "If two periods specified, what operation to apply to workspaces to get a final one.");

    std::vector<std::string> allowedTypes;
    allowedTypes.push_back("PairAsymmetry");
    allowedTypes.push_back("GroupAsymmetry");
    allowedTypes.push_back("GroupCounts");
    declareProperty("OutputType", "PairAsymmetry", boost::make_shared<StringListValidator>(allowedTypes),
      "What kind of workspace required for analysis.");

    declareProperty("PairFirstIndex", EMPTY_INT(), 
      "Workspace index of the first group of the pair. Used when OutputType is PairAsymmetry.");

    declareProperty("PairSecondIndex", EMPTY_INT(),
      "Workspace index of the second group of the pair. Used when OutputType is PairAsymmetry.");

    declareProperty("Alpha", 1.0, 
      "Alpha value of the pair. Used when OutputType is PairAsymmetry.");

    declareProperty("GroupIndex", EMPTY_INT(),
      "Workspace index of the group. Used then OutputType is GroupAsymmetry or GroupCounts.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), 
      "Output workspace. Type of the data depends on the OutputType.");
  }

  //----------------------------------------------------------------------------------------------
  /** 
   * Execute the algorithm.
   */
  void MuonCalculateAsymmetry::exec()
  {
    MatrixWorkspace_sptr firstPeriodWS = getProperty("FirstPeriodWorkspace");
    MatrixWorkspace_sptr secondPeriodWS = getProperty("SecondPeriodWorkspace");

    MatrixWorkspace_sptr firstConverted = convertWorkspace( firstPeriodWS );

    if ( secondPeriodWS )
    {
      // Two periods
      MatrixWorkspace_sptr secondConverted = convertWorkspace( secondPeriodWS );

      setProperty( "OutputWorkspace", mergePeriods(firstConverted, secondConverted) );
    }
    else
    {
      // Single period only

      setProperty("OutputWorkspace", firstConverted);
    }
  }

  /**
   * TODO: comment
   */
  MatrixWorkspace_sptr MuonCalculateAsymmetry::convertWorkspace(MatrixWorkspace_sptr ws)
  {
    const std::string type = getPropertyValue("OutputType");

    if ( type == "GroupCounts" )
    {
      // The simpliest one - just copy the counts of some group

      int groupIndex = getProperty("GroupIndex");

      if ( groupIndex == EMPTY_INT() )
        throw std::runtime_error("GroupIndex is not specified");

      IAlgorithm_sptr alg = createChildAlgorithm("ExtractSingleSpectrum");
      alg->initialize();
      alg->setProperty("InputWorkspace", ws);
      alg->setProperty("WorkspaceIndex", groupIndex);
      alg->execute();
      
      return alg->getProperty("OutputWorkspace");
    }
  }

  /**
   * TODO: comment
   */
  MatrixWorkspace_sptr MuonCalculateAsymmetry::mergePeriods(MatrixWorkspace_sptr ws1, MatrixWorkspace_sptr ws2)
  {
    std::string op = getProperty("PeriodOperation");

    std::string algorithmName;

    if ( op == "+" )
    {
      algorithmName = "Plus";
    }
    else if ( op == "-" )
    {
      algorithmName = "Minus";
    }

    IAlgorithm_sptr alg = createChildAlgorithm(algorithmName);
    alg->initialize();
    alg->setProperty("LHSWorkspace",ws1);
    alg->setProperty("RHSWorkspace",ws2);
    alg->execute();

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    return outWS;
  }
} // namespace WorkflowAlgorithms
} // namespace Mantid
