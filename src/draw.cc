#include "draw.hh"
#include "libstrparse.hh"
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <TMath.h>
#include <TBranch.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TGImageMap.h>
#include <TGFileDialog.h>
#include <TKey.h>
#include <TSystem.h>
#include <TLatex.h>
#include <TText.h>
#include "TPaveText.h"
#include <TApplication.h>
#include "TChain.h"
#include "TEnv.h"
#include "TRegexp.h"
#include "TGraph.h"
#include "TPaveStats.h"

using namespace std;

///////////////////////////////////////////////////////////////////
//  Class: DrawGUI
//
//    Creates a GUI to display the commands used in DrawConfig
//
//

DrawGUI::DrawGUI(DrawConfig& config, Bool_t printonly=0, int ver=0):
  runNumbers(0),
  fFilesAlive(kTRUE),
  fVerbosity(ver)
{
  // Constructor.  Get the config pointer, and make the GUI.

  fConfig = &config;
  int bin2Dx(0), bin2Dy(0);
  fConfig->Get2DnumberBins(bin2Dx,bin2Dy);    
  if(bin2Dx>0 && bin2Dy>0){
    gEnv->SetValue("Hist.Binning.2D.x",bin2Dx);
    gEnv->SetValue("Hist.Binning.2D.y",bin2Dy);
    if(fVerbosity>1){
      cout<<"Set 2D default bins to x, y: "<<bin2Dx<<", "<<bin2Dy<<endl;
    }
  }

  if(printonly) {
    fPrintOnly=kTRUE;
    PrintPages();
  } else {
    fPrintOnly=kFALSE;
    CreateGUI(gClient->GetRoot(),800,600);
  }
}

void DrawGUI::CreateGUI(const TGWindow *p, UInt_t w, UInt_t h) 
{
  
  OpenRootFiles();
  // Create the main frame
  fMain = new TGMainFrame(p,w,h);
  fMain->Connect("CloseWindow()", "DrawGUI", this, "MyCloseWindow()");
  ULong_t lightgreen, lightblue, red, mainguicolor;
  gClient->GetColorByName("lightgreen",lightgreen);
  gClient->GetColorByName("lightblue",lightblue);
  gClient->GetColorByName("red",red);

  Bool_t good_color=kFALSE; 
  TString usercolor = fConfig->GetGuiColor();
  if(!usercolor.IsNull()) {
    good_color = gClient->GetColorByName(usercolor,mainguicolor);
  }

  if(!good_color) {
    if(!usercolor.IsNull()) {
      cout << "Bad guicolor (" << usercolor << ").. using default." << endl;
    }
  // Default background color
  mainguicolor = lightgreen;
  // mainguicolor = lightblue;
  }

  fMain->SetBackgroundColor(mainguicolor);

  // Top frame, to hold page buttons and canvas
  fTopframe = new TGHorizontalFrame(fMain,w,UInt_t(h*0.9));
  fTopframe->SetBackgroundColor(mainguicolor);
  fMain->AddFrame(fTopframe, new TGLayoutHints(kLHintsExpandX 
					       | kLHintsExpandY,10,10,10,1));

  // Create a verticle frame widget with radio buttons
  //  This will hold the page buttons
  vframe = new TGVerticalFrame(fTopframe,UInt_t(w*0.3),UInt_t(h*0.9));
  vframe->SetBackgroundColor(mainguicolor);
  TString buff;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++) {
    buff = fConfig->GetPageTitle(i);
    fRadioPage[i] = new TGRadioButton(vframe,buff,i);
    fRadioPage[i]->SetBackgroundColor(mainguicolor);
  }

  fRadioPage[0]->SetState(kButtonDown);
  current_page = 0;

  for (UInt_t i=0; i<fConfig->GetPageCount(); i++) {
    vframe->AddFrame(fRadioPage[i], new TGLayoutHints(kLHintsLeft |
						      kLHintsCenterY,5,5,3,4));
    fRadioPage[i]->Connect("Pressed()", "DrawGUI", this, "DoRadio()");
  }

  wile = new TGPictureButton(vframe,gClient->GetPicture(macroDir+"/panguin.xpm"));
  wile->Connect("Pressed()","DrawGUI", this,"DoDrawClear()");
  wile->SetBackgroundColor(mainguicolor);

  vframe->AddFrame(wile,new TGLayoutHints(kLHintsBottom|kLHintsLeft,5,10,4,2));


  fTopframe->AddFrame(vframe,new TGLayoutHints(kLHintsLeft|
                                               kLHintsCenterY,2,2,2,2));
  
  // Create canvas widget
  fEcanvas = new TRootEmbeddedCanvas("Ecanvas", fTopframe, UInt_t(w*0.7), UInt_t(h*0.9));
  fEcanvas->SetBackgroundColor(mainguicolor);
  fTopframe->AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY
              ,10,10,10,1));
  fCanvas = fEcanvas->GetCanvas();

  // Create the bottom frame.  Contains control buttons
  fBottomFrame = new TGHorizontalFrame(fMain,w,UInt_t(h*0.1));
  fBottomFrame->SetBackgroundColor(mainguicolor);
  fMain->AddFrame(fBottomFrame, new TGLayoutHints(kLHintsExpandX,10,10,10,10));
  
  // Create a horizontal frame widget with buttons
  hframe = new TGHorizontalFrame(fBottomFrame,1200,40);
  hframe->SetBackgroundColor(mainguicolor);
  fBottomFrame->AddFrame(hframe,new TGLayoutHints(kLHintsExpandX,2,2,2,2));

  fPrev = new TGTextButton(hframe,"Prev");
  fPrev->SetBackgroundColor(mainguicolor);
  fPrev->Connect("Clicked()","DrawGUI",this,"DrawPrev()");
  hframe->AddFrame(fPrev, new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  fNext = new TGTextButton(hframe,"Next");
  fNext->SetBackgroundColor(mainguicolor);
  fNext->Connect("Clicked()","DrawGUI",this,"DrawNext()");
  hframe->AddFrame(fNext, new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  fExit = new TGTextButton(hframe,"Exit GUI");
  fExit->SetBackgroundColor(red);
  fExit->Connect("Clicked()","DrawGUI",this,"CloseGUI()");

  hframe->AddFrame(fExit, new TGLayoutHints(kLHintsCenterX,5,5,1,1));
  
  TString Buff;
  if(runNumbers.size()==0) {
    Buff = "";
  } else {
    Buff = "Run #";
    Buff += runNumbers[0];
    for (int i=1; i<runNumbers.size(); i++) {
      Buff += " + #";
      Buff += runNumbers[i];
    }
  }
  TGString labelBuff(Buff);
  
  fRunNumber = new TGLabel(hframe,Buff);
  fRunNumber->SetBackgroundColor(mainguicolor);
  hframe->AddFrame(fRunNumber,new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  fPrint = new TGTextButton(hframe,"Print To &File");
  fPrint->SetBackgroundColor(mainguicolor);
  fPrint->Connect("Clicked()","DrawGUI",this,"PrintToFile()");
  hframe->AddFrame(fPrint, new TGLayoutHints(kLHintsCenterX,5,5,1,1));


  // Set a name to the main frame
  fMain->SetWindowName("PREX-II Analysis GUI");

  // Map all sub windows to main frame
  fMain->MapSubwindows();
  
  // Initialize the layout algorithm
  fMain->Resize(fMain->GetDefaultSize());
  
  // Map main frame
  fMain->MapWindow();

  if(fVerbosity>=1)
    fMain->Print();

  if(fFilesAlive) DoDraw();
}

void DrawGUI::DoDraw() 
{
  // The main Drawing Routine.

  // clean previous plot
  TIter cnext(fCanvas->GetListOfPrimitives());
  TObject *padObj, *histObj;
  while ((padObj=cnext())) {
    if (padObj->InheritsFrom("TPad")) {
      TIter pnext(((TPad*)padObj)->GetListOfPrimitives());
      while ((histObj=pnext())) {
	if (histObj->InheritsFrom("TH1")) {
	  histObj->Delete();
	}
      }
      padObj->Delete();
    }
  }
  fCanvas->Clear();

  gStyle->SetOptStat(1110);
  gStyle->SetOptTitle(0);
  gStyle->SetStatFontSize(0.08);
  if (fConfig->IsLogy(current_page)) {
    printf("\nFound a logy!!!\n\n");
    gStyle->SetOptLogy(1);
  } else {
    gStyle->SetOptLogy(0);
  }
  //   gStyle->SetTitleH(0.10);
  //   gStyle->SetTitleW(0.40);
  // gStyle->SetTitleX(0.350);
  // gStyle->SetTitleY(0.995);
  // gStyle->SetTitleH(0.10);
  // gStyle->SetTitleW(0.60);
  // gStyle->SetStatH(0.70);
  // gStyle->SetStatW(0.35);
  //   gStyle->SetLabelSize(0.10,"X");
  //   gStyle->SetLabelSize(0.10,"Y");
  gStyle->SetLabelSize(0.05,"X");
  gStyle->SetLabelSize(0.05,"Y");
  gStyle->SetPadLeftMargin(0.09);
  gStyle->SetPadRightMargin(0.09);
  gStyle->SetPadTopMargin(0.11);
  gStyle->SetPadBottomMargin(0.05);
  gStyle->SetNdivisions(505,"X");
  gStyle->SetNdivisions(404,"Y");
  gROOT->ForceStyle();

  // Determine the dimensions of the canvas..
  UInt_t draw_count = fConfig->GetDrawCount(current_page);
  if(draw_count>=8) {
    gStyle->SetLabelSize(0.08,"X");
    gStyle->SetLabelSize(0.08,"Y");
  }
  //   Int_t dim = Int_t(round(sqrt(double(draw_count))));
  pair <UInt_t,UInt_t> dim = fConfig->GetPageDim(current_page);

  if(fVerbosity>=1)
    cout << "Dimensions: " << dim.first << "X" << dim.second << endl;

  // Create a nice clean canvas.
  fCanvas->Clear();
  fCanvas->Divide(dim.first,dim.second);

  vector <TString> drawCommand;
  // Draw the histograms.
  for(UInt_t i=0; i<draw_count; i++) {    
    drawCommand = fConfig->GetDrawCommand(current_page,i);
    fCanvas->cd(i+1);
//    int nDraw = drawcommand.size()/6; // FIXME, remember to change this value when increase new options 
    if (drawCommand[0] == "macro") {
      MacroDraw(drawCommand);
    } else if (IsHistogram(drawCommand[0])) {
      HistDraw(drawCommand);
    } else {
      list <TString> treeDrawCommand = fConfig->GetTreeDrawCommand(current_page,i);
      TreeDraw(treeDrawCommand);
    }
  }
      
  fCanvas->cd();
  fCanvas->Update();

  if(!fPrintOnly) {
    CheckPageButtons();
  }

}

void DrawGUI::DrawNext()
{
  // Handler for the "Next" button.
  fRadioPage[current_page]->SetState(kButtonUp);
  // The following line triggers DoRadio(), or at least.. used to
  fRadioPage[current_page+1]->SetState(kButtonDown);
  current_page++;
  DoDraw();
}

void DrawGUI::DrawPrev()
{
  // Handler for the "Prev" button.
  fRadioPage[current_page]->SetState(kButtonUp);
  // The following line triggers DoRadio(), or at least.. used to
  fRadioPage[current_page-1]->SetState(kButtonDown);
  current_page--;
  DoDraw();  
}

void DrawGUI::DoRadio()
{
  // Handle the radio buttons
  //  Find out which button has been pressed..
  //   turn off the previous button...
  //   then draw the appropriate page.
  // This routine also handles the Draw from the Prev/Next buttons
  //   - makes a call to DoDraw()

  UInt_t pagecount = fConfig->GetPageCount();
  TGButton *btn = (TGButton *) gTQSender;
  UInt_t id = btn->WidgetId();
  
  if (id <= pagecount) {  
    fRadioPage[current_page]->SetState(kButtonUp);
  }

  current_page = id;
  DoDraw();
}

void DrawGUI::CheckPageButtons() 
{
  // Checks the current page to see if it's the first or last page.
  //  If so... turn off the appropriate button.
  //  If not.. turn on both buttons.

  if(current_page==0) {
    fPrev->SetState(kButtonDisabled);
    if(fConfig->GetPageCount()!=1)
      fNext->SetState(kButtonUp);
  } else if(current_page==fConfig->GetPageCount()-1) {
    fNext->SetState(kButtonDisabled);
    if(fConfig->GetPageCount()!=1)
      fPrev->SetState(kButtonUp);
  } else {
    fPrev->SetState(kButtonUp);
    fNext->SetState(kButtonUp);
  }
}

Bool_t DrawGUI::IsHistogram(TString objectname) 
{
  // Utility to determine if the objectname provided is a histogram

  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if (fileObjects[i].first.Contains(objectname)) {
      if(fVerbosity>=2)
        cout << fileObjects[i].first << "      " << fileObjects[i].second << endl;

      if(fileObjects[i].second.Contains("TH"))
        return kTRUE;
    }
  }

  return kFALSE;
}

void DrawGUI::GetFileObjects() 
{
  // Utility to find all of the objects within a File (TTree, TH1F, etc).
  //  The pair stored in the vector is <ObjName, ObjType>
  //  If there's no good keys.. do nothing.
  if(fVerbosity>=1)
    cout << "Keys = " << fRootFile->ReadKeys() << endl;

  if(fRootFile->ReadKeys()==0) {
    fUpdate = kFALSE;
    //     delete fRootFile;
    //     fRootFile = 0;
    //     CheckRootFile();
    return;
  }
  fileObjects.clear();
  TIter next(fRootFile->GetListOfKeys());
  TKey *key = new TKey();

  // Do the search
  while((key=(TKey*)next())!=0) {
    if(fVerbosity>=1)
      cout << "Key = " << key << endl;    

    TString objname = key->GetName();
    TString objtype = key->GetClassName();

    if(fVerbosity>=1)
      cout << objname << " " << objtype << endl;

    fileObjects.push_back(make_pair(objname,objtype));
  }
  fUpdate = kTRUE;
  delete key;
}

void DrawGUI::GetTreeVars() 
{
  // Utility to find all of the variables (leaf's/branches) within a
  // Specified TTree and put them within the treeVars vector.
  treeVars.clear();
  TObjArray *branchList;
  vector <TString> currentTree;

  for(UInt_t i=0; i<fRootTree.size(); i++) {
    currentTree.clear();
    branchList = fRootTree[i]->GetListOfBranches();
    TIter next(branchList);
    TBranch *brc;

    while((brc=(TBranch*)next())!=0) {
      TString found = brc->GetName();
      // Not sure if the line below is so smart...
      currentTree.push_back(found);
    }
    treeVars.push_back(currentTree);
  }

  if(fVerbosity>=5){
    for(UInt_t iTree=0; iTree<treeVars.size(); iTree++) {
      cout << "In Tree " << iTree << ": " << endl;
      for(UInt_t i=0; i<treeVars[iTree].size(); i++) {
        cout << treeVars[iTree][i] << endl;
      }
    }
  }
}


void DrawGUI::GetRootTree() {
  // Utility to search a ROOT File for ROOT Trees
  // Fills the fRootTree vector
  fRootTree.clear();

  list <TString> found;
  for(UInt_t i=0; i<fileObjects.size(); i++) {
    
    if(fVerbosity>=2)
      cout << "Object = " << fileObjects[i].second 
           << "\tName = " << fileObjects[i].first << endl;

    if(fileObjects[i].second.Contains("TTree"))
      found.push_back(fileObjects[i].first);
  }

  // Remove duplicates, then insert into fRootTree
  found.unique();
  UInt_t nTrees = found.size();

  for(UInt_t i=0; i<nTrees; i++) {
    TString treeName = found.front();
    TChain * ch = new TChain(found.front());
    if(!ch) {
      cerr << "Error: Unable to create TChain from tree: " << treeName << ". Quitting!" << endl;
      exit(1);
    }

    for (int j=1; j<fConfig->GetNumberOfRuns(); j++) { // make sure every rootfile has the tree
      if(!fRootFiles[j]->GetListOfKeys()->Contains(treeName)) {
        cerr << "Error: root file: " << fConfig->GetRootFile(j) << " doesn't contains tree: " 
             << treeName << ". Quitting!" << endl;
        exit(1);
      }
    }

    for(int j=0; j<fConfig->GetNumberOfRuns(); j++)
      if(ch->Add(fConfig->GetRootFile(j)) != 1) {
        cerr << "Error: unable to add rootfile: " << fConfig->GetRootFile(j) << " to tchain: " 
             << ch->GetName() << ". Quitting!" << endl;
        exit(1);
      }
    fRootTree.push_back(ch);
    found.pop_front();
  }  
  // Initialize the fTreeEntries vector
  fTreeEntries.clear();
  for(UInt_t i=0;i<fRootTree.size();i++) {
    fTreeEntries.push_back(0);
  }
  
}

UInt_t DrawGUI::GetTreeIndex(TString var) { // FIXME need a more robust parsing algorithm
  // Utility to find out which Tree (in fRootTree) has the specified
  // variable "var".  If the variable is a collection of Tree
  // variables (e.g. bcm1:lumi1), will only check the first (e.g. bcm1).  
  // Returns the correct index.  if not found returns -1

  //  This is for 2d draws... look for the first only
  if(var.Contains(":")) {
    var = strparse::split(var,":")[0];
  }
  if(var.Contains("-")) {
    var = strparse::split(var,"-")[0];
  }
  if(var.Contains("/")) {
    var = strparse::split(var,"/")[0];
  }
  if(var.Contains("*")) {
    var = strparse::split(var,"*")[0];
  }
  if(var.Contains("+")) {
    var = strparse::split(var,"+")[0];
  }
  if(var.Contains(".")) {
    var = strparse::split(var,".")[0];
  }
  if(var.Contains("(")) { // FIXME
    var = strparse::split(var,"(")[0];
  }
  //  This is for variables with multiple dimensions.
  if(var.Contains("[")) {
    var = strparse::split(var,"[")[0];
  }

  if(fVerbosity>=3)
    cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl
	<<"\t looking for variable: "<<var<<endl;
  for(UInt_t iTree=0; iTree<treeVars.size(); iTree++) {
    for(UInt_t ivar=0; ivar<treeVars[iTree].size(); ivar++) {
      if(fVerbosity>=4)
        cout<<"Checking tree "<<iTree<<" name:"<<fRootTree[iTree]->GetName()
            <<" \t var "<<ivar<<" >> "<<treeVars[iTree][ivar]<<endl;
      if(var == treeVars[iTree][ivar]) 
        return iTree;
    }
  }

  return -1;
}

UInt_t DrawGUI::GetTreeIndexByName(TString name) {
  // Called by TreeDraw().  Tries to find the Tree index provided the
  //  name.  If it doesn't match up, return -1
  for(UInt_t iTree=0; iTree<fRootTree.size(); iTree++) {
    TString treename = fRootTree[iTree]->GetName();
    if(name == treename) {
      return iTree;
    }
  }
  return -1;
}

void DrawGUI::MacroDraw(vector <TString> command) {
  // Called by DoDraw(), this will make a call to the defined macro, and
  //  plot it in it's own pad.  One plot per macro, please.

  if(command[1].IsNull()) {
    cout << "macro command doesn't contain a macro to execute" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gROOT->Macro(command[1]);
  

}

void DrawGUI::DoDrawClear() {
  // Utility to grab the number of entries in each tree.  This info is
  // then used, if watching a file, to "clear" the TreeDraw
  // histograms, and begin looking at new data.
  for(UInt_t i=0; i<fTreeEntries.size(); i++) {
    fTreeEntries[i] = (Int_t) fRootTree[i]->GetEntries();
  }
}

void DrawGUI::BadDraw(TString errMessage) {
  // Routine to display (in Pad) why a particular draw method has
  // failed.
  TPaveText *pt = new TPaveText(0.1,0.1,0.9,0.9,"brNDC");
  pt->SetBorderSize(3);
  pt->SetFillColor(10);
  pt->SetTextAlign(22);
  pt->SetTextFont(72);
  pt->SetTextColor(2);
  pt->AddText(errMessage.Data());
  pt->Draw();
  //   cout << errMessage << endl;

}


void DrawGUI::CheckRootFiles() {
  // Check the path to the rootfile (should follow symbolic links)
  // ... If found:
  //   Reopen new root file, 

  if(gSystem->AccessPathName(fConfig->GetRootFile())==0) {
    cout << "Found the new run" << endl;
  } else {
    TString rnBuff = "Waiting for run";
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }
}

void DrawGUI::OpenRootFiles() {
  // Open RootFiles.  Die if anyone can't be opened
  runNumbers.clear();
  fRootFiles.clear();
  for(int i=0; i<fConfig->GetNumberOfRuns(); i++) {
    TFile* rootFile = new TFile(fConfig->GetRootFile(i),"READ");
    if(!rootFile->IsOpen()) {
      fFilesAlive = kFALSE;
      cout << "ERROR:  rootfile: " << fConfig->GetRootFile(i) << " Can not be opened" << endl;
      gApplication->Terminate();
    } else {
      fRootFiles.push_back(rootFile);
      runNumbers.push_back(fConfig->GetRunNumber(i));
    }
  }
  if (fFilesAlive) {
    fRootFile = fRootFiles[0];
    GetFileObjects(); // assume all root files have the same objects ? How to judge ???
    GetRootTree();
    GetTreeVars();
    for(UInt_t i=0; i<fRootTree.size(); i++) {
      if(fRootTree[i]==0) {
	fRootTree.erase(fRootTree.begin() + i);
      }
    }
  }

  TString goldenfilename=fConfig->GetGoldenFile();
  if(!goldenfilename.IsNull()) {
    fGoldenFile = new TFile(goldenfilename,"READ");
    if(!fGoldenFile->IsOpen()) {
      cout << "ERROR: goldenrootfile: " << goldenfilename
	   << " does not exist.  Oh well, no comparison plots."
	   << endl;
      doGolden = kFALSE;
      fGoldenFile=NULL;
    } else {
      doGolden = kTRUE;
    }
  } else {
    doGolden=kFALSE;
    fGoldenFile=NULL;
  }
}

void DrawGUI::HistDraw(vector <TString> command) {
  // Called by DoDraw(), this will plot a histogram.

  TString histName = command[0];
  Bool_t showGolden=kFALSE;
  if(doGolden) showGolden=kTRUE;

  if(command.size()==2)
    if(command[1] == "noshowgolden") {
      showGolden = kFALSE;
    }
  cout<<"showGolden= "<<showGolden<<endl;

  // Determine dimensionality of histogram
  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if (fileObjects[i].first.Contains(histName)) {
      if(fileObjects[i].second.Contains("TH1")) {
        if(showGolden) fRootFile->cd();
        mytemp1d = (TH1D*)gDirectory->Get(histName);
        if(mytemp1d->GetEntries()==0) {
          BadDraw("Empty Histogram");
	} else {
	  if(showGolden) {
	    fGoldenFile->cd();
	    mytemp1d_golden = (TH1D*)gDirectory->Get(histName);
	    mytemp1d_golden->SetLineColor(30);
	    mytemp1d_golden->SetFillColor(30);
	    Int_t fillstyle=3027;
	    if(fPrintOnly) fillstyle=3010;
	    mytemp1d_golden->SetFillStyle(fillstyle);
	    mytemp1d_golden->Draw();
	    cout<<"one golden histo drawn"<<endl;
	    mytemp1d->Draw("sames");
	  } else {
	    mytemp1d->Draw();
	  }
	}
	break;
      }
      if(fileObjects[i].second.Contains("TH2")) {
	if(showGolden) fRootFile->cd();
	mytemp2d = (TH2D*)gDirectory->Get(histName);
	if(mytemp2d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else {
	  // These are commented out for some reason (specific to DVCS?)
	  // 	  if(showGolden) {
	  // 	    fGoldenFile->cd();
	  // 	    mytemp2d_golden = (TH2D*)gDirectory->Get(histName);
	  // 	    mytemp2d_golden->SetMarkerColor(2);
	  // 	    mytemp2d_golden->Draw();
	  //mytemp2d->Draw("sames");
	  // 	  } else {
	  mytemp2d->Draw();
	  // 	  }
	}
	break;
      }
      if(fileObjects[i].second.Contains("TH3")) {
	if(showGolden) fRootFile->cd();
	mytemp3d = (TH3D*)gDirectory->Get(histName);
	if(mytemp3d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else {
	  mytemp3d->Draw();
	  if(showGolden) {
	    fGoldenFile->cd();
	    mytemp3d_golden = (TH3D*)gDirectory->Get(histName);
	    mytemp3d_golden->SetMarkerColor(2);
	    mytemp3d_golden->Draw();
	    mytemp3d->Draw("sames");
	  } else {
	    mytemp3d->Draw();
	  }
	}
	break;
      }
    }
  }
}

void DrawGUI::TreeDraw(list <TString> command) {
  // Called by DoDraw(), this will plot a Tree Variable

  int nField = 4;  // FIXME, remember to change this value when increase new options 
  int nDraw = (command.size()-2)/nField;  // -2 for title and grid

  TString title = command.front(); command.pop_front();
  TString grid = command.front(); command.pop_front();
  for( int iDraw=0; iDraw<nDraw; iDraw++){

    /* extract draw info: var, cut and drawopt */

    // var
    TString var = command.front();  command.pop_front();
    //  Check to see if we're projecting to a specific histogram
    TString histoname = var(TRegexp(">>.+(?"));
    if (histoname.Length()>0){
      histoname.Remove(0,2);
      Int_t bracketindex = histoname.First("(");
      if (bracketindex>0) histoname.Remove(bracketindex);
      if(fVerbosity>=3)
        std::cout << histoname << " "<< var(TRegexp(">>.+(?")) <<std::endl;
    } else {
//      histoname = "htemp";
      histoname = Form("%s_%d", var.Data(), iDraw);
    }
    
    // Combine the cuts (definecuts and specific cuts)
    TString cut = command.front(); command.pop_front();
    if(!cut.IsNull()) {
      vector <TString> cutIdents = fConfig->GetCutIdent();
      for(UInt_t i=0; i<cutIdents.size(); i++) {
        if(cut.Contains(cutIdents[i])) {
          TString cut_found = (TString)fConfig->GetDefinedCut(cutIdents[i]);
          cut.ReplaceAll(cutIdents[i],cut_found); // FIXME
        }
      }
    }

    // Determine which Tree the variable comes from, then draw it.
    TString tree = command.front(); command.pop_front();
    UInt_t iTree;
    if(tree.IsNull()) {
      iTree = GetTreeIndex(var);
      if(fVerbosity>=2)
        cout<<"got tree index from variable: "<<iTree<<endl;
    } else {
      iTree = GetTreeIndexByName(tree);
      if(fVerbosity>=2)
        cout<<"got tree index from command: "<<iTree<<endl;
    }

    // draw options
    TString drawopt = command.front();  command.pop_front();
    if(fVerbosity>=3)
      cout<<"\tDraw option:"<<drawopt<<" and histo name "<<histoname<<endl;
    if(iDraw>0) {
      if (drawopt.IsNull())
        drawopt = "same"; // so that multiplots will be drew
      else
        drawopt += " same";
      /* !!! no space before same !!!
       * drawopt += " same" 
       * will draw only the first plot
       */
    }


    // draw histogram
    Int_t errcode=0;
    if(iTree == -1) {
      BadDraw(var+" not found");
    } else {
      if(fVerbosity>=1){
        cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;
        cout<< "subdraw command " << iDraw << ":\t" << var <<"\t"<< cut << "\t" << tree
            <<"\t"<< drawopt << endl;
        if(fVerbosity>=2)
        cout<<"\tProcessing from tree: "<<iTree<<"\t"<<fRootTree[iTree]->GetTitle()<<"\t"
        <<fRootTree[iTree]->GetName()<<endl;
      }

      errcode = fRootTree[iTree]->Draw(Form("%s>>%s", var.Data(), histoname.Data()),cut,drawopt);
//      errcode = fRootTree[iTree]->Draw(Form("%s", var.Data()),cut,drawopt);

      if(fVerbosity>=3)
        cout<<"Finished drawing with error code "<<errcode<<endl;

      TH1 *hist = (TH1*)gPad->FindObject(histoname);
      if (iDraw == 0) {
        if (title.IsNull()) 
          title = hist->GetTitle();
//        TH1 *hobj = (TH1*)gPad->FindObject(histoname);
//        if 
      }

      if(iDraw>0) {
        hist->SetLineColor(2*iDraw);
        hist->SetMarkerColor(2*iDraw);
      }


//      TH1* temphist = (TH1*) hobj;
//      gPad->Update(); // always remember to update pad in order to find stats box
//      TPaveStats * st = (TPaveStats*) gPad->GetPrimitive("stats");
//      st->SetName("myStats"); // why this so important ???
//      TList* listOfLines = st->GetListOfLines();
//      TText * rm = st->GetLineWith("Dev");
//      Float_t size = rm->GetTextSize();
//      // listOfLines->Remove(rm);
//      TLatex* meanError = new TLatex(0, 0, Form("MeanError = %.4g", temphist->GetMeanError()));
//      meanError->SetTextSize(size);
//      listOfLines->Add(meanError);
//      temphist->SetStats(0);
//      gPad->Modified();

//      Float_t ratio = abs(temphist->GetMean())/temphist->GetMeanError();
//      printf("var: %s\t mean: %.4g\t meanerror: %.4g\t ratio: %.4g\n", var.Data(), temphist->GetMean(), temphist->GetMeanError(), ratio);

      if(errcode==-1) {
        BadDraw(var+" not found");
      } else if (errcode==0) {
        BadDraw("Empty Histogram");
      }
        // if(!var.IsNull()) {
          //  Generate a "unique" histogram name based on the MD5 of the drawn variable, cut, drawopt,
          //  and plot title.
          //  Makes it less likely to cause a name collision if two plot titles are the same.
          //  If you draw the exact same plot twice, the histograms will have the same name, but
          //  since they are exactly the same, you likely won't notice (or it will complain at you).
          // TString tmpstring(var);
          // tmpstring += cut.GetTitle();
          // tmpstring += drawopt;
          // TString myMD5 = tmpstring.MD5();
          // TH1* thathist = (TH1*)hobj;
        // }
    }
  }
  TLatex *tTitle = new TLatex(0.09, 0.91, title);
  tTitle->SetNDC();
  tTitle->SetTextSize(0.07);
  tTitle->Draw();

  if (!grid.IsNull())
    gPad->SetGrid();
}

void DrawGUI::PrintToFile()
{
  // Routine to print the current page to a File.
  //  A file dialog pops up to request the file name.
  fCanvas = fEcanvas->GetCanvas();
  gStyle->SetPaperSize(20,24);
  static TString dir("printouts");
  TGFileInfo fi;
  const char *myfiletypes[] = 
    { "All files","*",
      "Portable Document Format","*.pdf",
      "PostScript files","*.ps",
      "Encapsulated PostScript files","*.eps",
      "GIF files","*.gif",
      "JPG files","*.jpg",
      0,               0 };
  fi.fFileTypes = myfiletypes;
  fi.fIniDir    = StrDup(dir.Data());

  new TGFileDialog(gClient->GetRoot(), fMain, kFDSave, &fi);
  if(fi.fFilename!=NULL) fCanvas->Print(fi.fFilename);
}

void DrawGUI::PrintPages() {
  // Routine to go through each defined page, and print the output to 
  // a postscript file. (good for making sample histograms).
  
  OpenRootFiles();

  fCanvas = new TCanvas("fCanvas","trythis",1000,800);
  TLatex *lt = new TLatex();

  TString plotsdir = fConfig->GetPlotsDir();
  if(plotsdir.IsNull()) plotsdir=".";

  Bool_t pagePrint = kFALSE;
  TString printFormat = fConfig->GetPlotFormat();
  if(printFormat.IsNull()) printFormat="pdf";
  if(printFormat!="pdf") pagePrint = kTRUE;

  TString filename = "summaryPlots_";
  filename += fConfig->GetRunNumber(0);
  for(int i=1; i<fConfig->GetNumberOfRuns(); i++) {
    filename += "+";
    filename += fConfig->GetRunNumber(i);
  }

  filename.Prepend(plotsdir+"/");
  if(pagePrint) 
    filename += "_pageXXXX";
  TString fConfName = fConfig->GetConfFileName();
  TString fCfgNm = fConfName(fConfName.Last('/')+1,fConfName.Length());
  filename += "_" + fCfgNm(0,fCfgNm.Last('.'));
  filename += "."+printFormat;

  TString pagehead = "Summary Plots: ";
  pagehead += fConfig->GetRunNumber(0);
  for(int i=1; i<fConfig->GetNumberOfRuns(); i++) {
    pagehead += "+";
    pagehead += fConfig->GetRunNumber(i);
  }

  gStyle->SetPalette(1);
  gStyle->SetTitleX(0.350);
  gStyle->SetTitleY(0.995);
  gStyle->SetPadBorderMode(0);
  gStyle->SetHistLineColor(1);
//  gStyle->SetHistFillColor(1);
  if(!pagePrint) fCanvas->Print(filename+"[");
  TString origFilename = filename;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++) {
    current_page=i;
    DoDraw();
    TString pagename = pagehead;
    pagename += " ";   
    pagename += i;
    pagename += ": ";
    pagename += fConfig->GetPageTitle(current_page);
    lt->SetTextSize(0.025);
    lt->DrawLatex(0.05,0.98,pagename);
    if(pagePrint) {
      filename = origFilename;
      filename.ReplaceAll("XXXX",Form("%02d",current_page));
      cout << "Printing page " << current_page 
	   << " to file = " << filename << endl;
    }
    fCanvas->Print(filename);
  }
  if(!pagePrint) fCanvas->Print(filename+"]");
  
  gApplication->Terminate();
}

void DrawGUI::MyCloseWindow()
{
  fMain->SendCloseMessage();
  cout << "DrawGUI Closed." << endl;
  delete fPrint;
  delete fExit;
  delete fRunNumber;
  delete fPrev;
  delete fNext;
  delete wile;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++) 
    delete fRadioPage[i];
  delete hframe;
  delete fEcanvas;
  delete fBottomFrame;
  delete vframe;
  delete fTopframe;
  delete fMain;
  if(fGoldenFile!=NULL) delete fGoldenFile;
  if(fRootFile!=NULL) delete fRootFile;
  delete fConfig;

  gApplication->Terminate();
}

void DrawGUI::CloseGUI() 
{
  // Routine to take care of the Exit GUI button
  fMain->SendCloseMessage();
}

DrawGUI::~DrawGUI()
{
  //  fMain->SendCloseMessage();
  delete fPrint;
  delete fExit;
  delete fRunNumber;
  delete fPrev;
  delete fNext;
  delete wile;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++) 
    delete fRadioPage[i];
  delete hframe;
  delete fEcanvas;
  delete vframe;
  delete fBottomFrame;
  delete fTopframe;
  delete fMain;
  if(fGoldenFile!=NULL) delete fGoldenFile;
  if(fRootFile!=NULL) delete fRootFile;
  delete fConfig;
}
