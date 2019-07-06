#ifndef draw_h
#define draw_h 1

#include "drawConfig.hh"
#include <vector>
#include <RQ_OBJECT.h>
#include <TQObject.h>
#include <TFile.h>
#include <TString.h>
#include <TTree.h>
#include <TChain.h>
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include <TCut.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <TGButton.h>
#include "TGLabel.h"
#include "TGString.h"

class DrawGUI {
  // Class that takes care of the GUI
  RQ_OBJECT("DrawGUI")
private:
  TGMainFrame                      *fMain;
  TGHorizontalFrame                *fTopframe;
  TGVerticalFrame                  *vframe;
  TGRadioButton                    *fRadioPage[50];
  TGPictureButton                  *wile;
  TRootEmbeddedCanvas              *fEcanvas;
  TGHorizontalFrame                *fBottomFrame;
  TGHorizontalFrame                *hframe;
  TGTextButton                     *fNext;
  TGTextButton                     *fPrev;
  TGTextButton                     *fExit;
  TGLabel                          *fRunNumber;
  TGTextButton                     *fPrint;
  TCanvas                          *fCanvas; // Present Embedded canvas
  DrawConfig                       *fConfig;
  UInt_t                            current_page;
  TFile*                            fRootFile; // the first (default) root file
  std::vector <TFile*>              fRootFiles; // all root files
  TFile*                            fGoldenFile;
  Bool_t                            doGolden;
  std::vector <TChain*>             fRootTree;
  std::vector <Int_t>               fTreeEntries;
  std::vector < std::pair <TString,TString> > fileObjects;
  std::vector < std::vector <TString> >       treeVars;
  std::vector <UInt_t>             runNumbers;  // all run numbers
  Bool_t                            fUpdate;
  Bool_t                            fFilesAlive;  // true only when all files are alive
  Bool_t                            fPrintOnly;
  TH1D                             *mytemp1d;
  TH2D                             *mytemp2d;
  TH3D                             *mytemp3d;
  TH1D                             *mytemp1d_golden;
  TH2D                             *mytemp2d_golden;
  TH3D                             *mytemp3d_golden;

  int fVerbosity;

public:
  DrawGUI(DrawConfig&, Bool_t,int);
  void CreateGUI(const TGWindow *p, UInt_t w, UInt_t h);
  virtual ~DrawGUI();
  void DoDraw();
  void DrawPrev();
  void DrawNext();
  void DoRadio();
  void CheckPageButtons();
  // Specific Draw Methods
  Bool_t IsHistogram(TString);
  void GetFileObjects();
  void GetTreeVars();
  void GetRootTree();
  UInt_t GetTreeIndex(TString);
  UInt_t GetTreeIndexByName(TString);
  void TreeDraw(std::vector <TString>); 
  void HistDraw(std::vector <TString>);
  void MacroDraw(std::vector <TString>);
  void DoDrawClear();
  void BadDraw(TString);
  void CheckRootFiles();
  Int_t OpenRootFiles();
  void PrintToFile();
  void PrintPages();
  void MyCloseWindow();
  void CloseGUI();
  void SetVerbosity(int ver){fVerbosity=ver;}
  ClassDef(DrawGUI,0);
};
#endif //draw_h
