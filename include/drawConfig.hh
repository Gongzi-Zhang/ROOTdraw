#ifndef drawConfig_h
#define drawConfig_h 1

#include <utility>
#include <fstream>
#include <vector>
#include <list>
#include <TString.h>
#include "TCut.h"

static TString macroDir = "macros";

class DrawConfig {
  // Class that takes care of the config file
private:
  TString confFileName;                   // config filename
  std::ifstream *fConfFile;               // original config file
  void ParseFile();
  std::vector <TString>       sConfFile;  // the config file, in memory
  // pageInfo is the vector of the pages containing the sConfFile index
  //   and how many commands issued within that page (title, 1d, etc.)
  std::vector <TString> rootfilenames;  //  Just the names
  TString goldenrootfilename; // Golden rootfile for comparisons
  TString protorootfile; // Prototype for getting the rootfilename
  TString guicolor; // User's choice of background color
  TString plotsdir; // Where to store sample plots.. automatically stored as .jpg's).
  std::vector < std::pair <UInt_t,UInt_t> > pageInfo; 
  std::vector <TCut> cutList; 
  std::vector <UInt_t> GetDrawIndex(UInt_t);
  Bool_t fFoundCfg;
  int fVerbosity;
  int hist2D_nBinsX,hist2D_nBinsY;
  TString fPlotFormat;
  std::vector <int> fRunNumbers;

public:
  DrawConfig();
  DrawConfig(TString, int ver=0);
  int GetRunNumber(int i=0){return fRunNumbers[i];}
  TString GetConfFileName(){return confFileName;}
  void Get2DnumberBins(int &nX, int &nY){nX = hist2D_nBinsX; nY = hist2D_nBinsY;}
  void SetVerbosity(int ver){fVerbosity=ver;}
  TString GetPlotFormat(){return fPlotFormat;}
  Bool_t ParseConfig();
  TString GetRootFile(int i=0) { return rootfilenames[i]; }
  UInt_t GetNumberOfRuns() { return fRunNumbers.size(); }
  TString GetGoldenFile() { return goldenrootfilename; }
  TString GetGuiColor() { return guicolor; }
  TString GetPlotsDir() { return plotsdir; }
  TCut GetDefinedCut(TString ident);
  std::vector <TString> GetCutIdent();
  // Page utilites
  UInt_t  GetPageCount() { return pageInfo.size(); };
  std::pair <UInt_t,UInt_t> GetPageDim(UInt_t);
  Bool_t IsLogy(UInt_t page);
  TString GetPageTitle(UInt_t);
  UInt_t GetDrawCount(UInt_t);           // Number of histograms in a page
  std::vector <TString> GetDrawCommand(UInt_t,UInt_t);
  std::list <TString> GetTreeDrawCommand(UInt_t,UInt_t);
  void OverrideRootFile(std::vector<UInt_t>);
};

#endif //panguinOnlineConfig_h
