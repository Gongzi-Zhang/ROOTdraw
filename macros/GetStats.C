void GetStats(){
  TPad *a1 = new TPad("a1", "a1", 0,0,1,1);
  a1->Draw();

  const int Nstats = 4;
  TString statStr[Nstats];
  TString statStrNumbers[Nstats];

//  TTree* r = (TTree*)gROOT->FindObject("evt");  // this doesn't work with multiple runs
  TChain * r = new TChain("evt");
  int nFiles = gROOT->GetListOfFiles()->GetEntries();
// cout << "Deubgging: copy of root files " << nFiles << endl;
  list<TString> files;
  vector<TString> unique_files;
  for (int i=0; i<nFiles; i++) {
    files.push_back(gROOT->GetListOfFiles()->At(i)->GetName());
  }
  files.unique();
//  printf("debugging: copy of root file %d\n", files.size()); 
  while(files.size() > 0) {
    TString file_name = files.front();
    bool found = false;
    for(int i=0; i<unique_files.size(); i++) {
      if (unique_files[i] == file_name) {
        found = true;
        break;
      }
    }
    if (!found) unique_files.push_back(file_name);
    files.pop_front();
  }
//  printf("debugging: copy of root file %d\n", unique_files.size()); 

// cout << "Deubgging: number of root files " << files.size() << endl;
  for(int i=0; i < unique_files.size(); i++) {
//    printf("debugging: unique root file name: %s\n", unique_files.front().Data());
    // cout << "Debugging: root file: " << files.front() << endl;
    r->Add(unique_files[i]);
  }
  TH1D* hist = new TH1D("CodaEventNumber", "CodaEventNumber", 100, 0, 10e9);
 
  r->Project(hist->GetName(), "CodaEventNumber", "1");
  Int_t tot = hist->GetEntries();
  statStr[0] = Form("Total Number of events");
  statStrNumbers[0] = Form(" = %d", tot);

  r->Project(hist->GetName(), "CodaEventNumber", "ErrorFlag==0");
  Int_t ok = hist->GetEntries();
  Double_t perok= ok*100./tot;
  statStr[1] = Form("EventCut passing events");
  statStrNumbers[1] = Form(" = %d (%.2f%%)", ok, perok);

  r->Project(hist->GetName(), "CodaEventNumber", "bcm_an_us<48");
  Int_t low = hist->GetEntries();
  Double_t perlow= low*100./tot;
  statStr[2] = Form("Current < 48 #muA events");
  statStrNumbers[2] = Form(" = %d (%.2f%%)", low, perlow);

  r->Project(hist->GetName(), "CodaEventNumber", "bpm12XP>50000 || bpm12XM>50000 || bpm12YP>50000 || bpm12YM>50000");
  Int_t bpmsat = hist->GetEntries();
  Double_t perbpmsat= bpmsat*100./tot;
  statStr[3] = Form("BPM12 wire saturation events");
  statStrNumbers[3] = Form(" = %d (%.2f%%)", bpmsat, perbpmsat);

  hist->Delete();
  TLatex text;
  text.SetTextSize(0.08);
  a1->cd(); 
  Float_t iniY = 0.90, diffY = 0.05;
  text.DrawLatex(0.10, 0.93, "Event Stats, Full Run"); 
  text.SetTextSize(0.06);
  for (Int_t j = 0 ; j < Nstats ; j++){
    text.DrawLatex(0.10, iniY - (j*2.5+1)*diffY, statStr[j]); 
    text.DrawLatex(0.10, iniY - (j*2.5+2)*diffY, statStrNumbers[j]); 
  }
}
