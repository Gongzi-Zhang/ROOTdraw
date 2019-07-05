#include "draw.hh"
#include <TApplication.h>
#include <TString.h>
#include <TROOT.h>
#include <iostream>
#include <ctime>

using namespace std;

clock_t tStart;
void Usage();
void draw(TString type="standard", std::vector <UInt_t> runs = std::vector <UInt_t> () , Bool_t printonly=kFALSE, int verbosity=0);

int main(int argc, char **argv){
  tStart = clock();
 
  TString type="default";
  std::vector <UInt_t> runs(0);
  Bool_t printonly=kFALSE;
  Bool_t showedUsage=kFALSE;
  int verbosity(0);

  TApplication theApp("App",&argc,argv,NULL,-1);

  cout<<"Starting processing arg. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

  for(Int_t i=1;i<theApp.Argc();i++) {
    TString sArg = theApp.Argv(i);
    if(sArg=="-f") {
      type = theApp.Argv(++i);
      cout << " File specifier: " <<  type << endl;
    } else if (sArg=="-r") { // all the arguments following this option will be regarded as run number, until next "-"
      TString runNumber; 
      if ((i+1)<theApp.Argc())
        runNumber = theApp.Argv(i+1);
      else {
        cerr << "Error: at least one run number is expected. Quitting!" << endl;
        exit(1);
      }
      while (!runNumber.BeginsWith("-")) {
        runNumber = theApp.Argv(++i);
        if (atoi(runNumber.Data()) == 0) {
         cerr << "Error: Invalid runnumber, please check your runnumber input. Only digits are allowed in run number" << endl; 
         exit(1);
        }
        runs.push_back(atoi(runNumber.Data()));
        if ((i+1)<theApp.Argc())
          runNumber = theApp.Argv(i+1);
        else 
          break;
      }

      cout << " You input " << runs.size() << " Runnumbers:";
      for(Int_t j=0; j<runs.size(); j++) {
        cout << "\t" << runs[j];
      }
	    cout << endl;
    } else if (sArg=="-v") {
      verbosity = atoi(theApp.Argv(++i));
    } else if (sArg=="-P") {
      printonly = kTRUE;
      cout <<  " PrintOnly" << endl;
    } else if (sArg=="-h") {
      if(!showedUsage) Usage();
      showedUsage=kTRUE;
      return 0;
    } else {
      cerr << "\"" << sArg << "\"" << " not recognized.  Ignored." << endl;
      if(!showedUsage) Usage();
      showedUsage=kTRUE;
    }
  }
  cout << "Verbosity level set to "<<verbosity<<endl;

  cout<<"Finished processing arg. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

  draw(type,runs,printonly,verbosity);
  theApp.Run();

  cout<<"Done. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

  return 0;
}


void draw(TString type,std::vector <UInt_t> runs,Bool_t printonly, int ver){

  if(printonly) {
    if(!gROOT->IsBatch()) {
      gROOT->SetBatch();
    }
  }

  cout<<"Starting processing cfg. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

  DrawConfig *fconfig = new DrawConfig(type);
  fconfig->SetVerbosity(ver);
  if(!fconfig->ParseConfig()) {
    gApplication->Terminate();
  }

  if(runs.size()!=0) fconfig->OverrideRootFile(runs);

  cout<<"Finished processing cfg. Init DrawGUI. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

  new DrawGUI(*fconfig,printonly,ver);

  cout<<"Finished init DrawGUI. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

}

void Usage(){
  cerr << "Usage: draw [-r run1 [run2 ...]] [-f config.cfg] [-v verbosity_level] [-P]" << endl;
  cerr << "Options:" << endl;
  cerr << "  -r : runnumber(s)" << endl;
  cerr << "  -f : configuration file" << endl;
  cerr << "  -v : verbosity level (>0)" << endl;
  cerr << "  -P : Only Print Summary Plots" << endl;
  cerr << endl;
}
