#include "drawConfig.hh"
#include "libstrparse.hh"
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
// #include <regex>
#include <TMath.h>
#include <unistd.h>
#include <dirent.h>
#include "TRegexp.h"
#include "TPRegexp.h"

using namespace std;

DrawConfig::DrawConfig() 
  :hist2D_nBinsX(0),hist2D_nBinsY(0), 
   fPlotFormat(""),fRunNumbers(0)
{
  // Constructor.  Without an argument, will use default "standard" config
  fVerbosity = 0;
  DrawConfig("standard");
}

DrawConfig::DrawConfig(TString anatype, int ver): 
  confFileName(anatype),fVerbosity(ver),
  hist2D_nBinsX(0),hist2D_nBinsY(0), 
  fPlotFormat(""),fRunNumbers(0)
{
  // Constructor.  Takes the config anatype as the only argument.
  //  Loads up the configuration file, and stores it's contents for access.
  
  //confFileName += ".cfg";//Not sure what this would be needed DELETEME cg
  fFoundCfg = kFALSE;

  fConfFile = new ifstream(confFileName.Data());
  if ( ! (*fConfFile) ) {    cerr << "DrawConfig() WARNING: config file " << confFileName.Data()
         << " does not exist" << endl;
    cerr << " Checking the " << macroDir << " directory" << endl;
    confFileName.Prepend(macroDir+"/");
    fConfFile = new ifstream(confFileName.Data());
    if ( ! (*fConfFile) ) {
      confFileName = macroDir+"/default.cfg";
      cout << "DrawConfig()  Trying " << confFileName.Data() 
	   << " as default configuration." << endl
	   << " (May be ok.)" << endl;
      fConfFile = new ifstream(confFileName.Data());
      if ( ! (*fConfFile) ) {
	cerr << "DrawConfig() WARNING: no file "
	     << confFileName.Data() <<endl;
	cerr << "You need a configuration to run.  Ask an expert."<<endl;
	fFoundCfg = kFALSE;
	//      return;
      } else {
	fFoundCfg = kTRUE;
      }
    } else {
      fFoundCfg = kTRUE;
    }
  } else {
    fFoundCfg = kTRUE;
  }

  if(fFoundCfg) {
    clog << "GUI Configuration loading from " << confFileName.Data() << endl;
  }

  ParseFile();

  fConfFile->close();
  delete fConfFile;
}

void DrawConfig::ParseFile() {
  // Reads in the Config File, and makes the proper calls to put
  //  the information contained into memory.

  if(!fFoundCfg) {
    return;
  }

  const char comment = '#';
  vector<TString> strvect;
  TString sinput, sline;
  while (sline.ReadLine(*fConfFile)) {

    // remove ';' and space until the first valid char
    while (sline.BeginsWith(';') || sline.BeginsWith(' ') || sline.BeginsWith('\t')) { 
      sline.Remove(0, 1);
    }
    if (sline.Length() == 0) {  // line contains only ';' and space
      continue;
    }

    // deal with comment
    if(sline.BeginsWith(comment)) continue;

    // strip comments
    // if # lies within pair of quotes: '#' or "#", it will not be regarded as comments
    Int_t cpos = strparse::first_unquoted(sline, comment);
    if (cpos == -2) { // unmatched quote
      cerr << "Error: Find unmatched quote in line: " << endl
           << "\t" << sline << endl
           << "Please check it." << endl;
      exit(1);
    } else if (cpos > 0) {  // has comment
      sline = sline(0, cpos);
      sline = strparse::strip_tail_space(sline);
      TPRegexp(";[ ;]+").Substitute(sline, ";"); // FIXME, how to deal with tab (\t); replace all
      // in rare case, three may be multiple ; within quotes, which will also be replaced
      sConfFile.push_back(sline);
      continue;
    }
    // if (cpos == -1), do nothing

    // deal with multi lines connected by \newline
    sline = strparse::strip_tail_space(sline);
    while (sline.EndsWith("\\")){  // connected to next line
      TString nextLine;
      nextLine.ReadLine(*fConfFile);
      nextLine = strparse::strip_head_space(nextLine);
      sline = sline.Replace(sline.Last('\\'), 1, ""); // remove the ending connector
      sline += nextLine;

      Int_t cpos = strparse::first_unquoted(sline, comment);
      if (cpos == -2) { // unmatched quote
        cerr << "Error: Find unmatched quote in line: " << endl
             << "\t" << sline << endl
             << "Please check it." << endl;
        exit(1);
      } else if (cpos > 0) {
        sline = sline(0, cpos);
        sline = strparse::strip_tail_space(sline);
        break;
      }
      sline = strparse::strip_tail_space(sline);
    }

    TPRegexp(";[ ;]+").Substitute(sline, ";"); // FIXME, how to deal with tab (\t)? I need to replace all accurence, not just the first one
    sConfFile.push_back(sline);
  }

  if(fVerbosity>=1){
    cout << "DrawConfig::ParseFile()\n";
    for(UInt_t ii=0; ii<sConfFile.size(); ii++) {
      cout << "Line " << ii << ":\t"<< sConfFile[ii] << "endl" << endl;
    }
  }

  cout << "\t" << sConfFile.size() << " lines read from " << confFileName << endl;
}

Bool_t DrawConfig::ParseConfig() {
  //  Goes through each line of the config [must have been ParseFile()'d]
  //   and interprets.

  if(!fFoundCfg) {
    return kFALSE;
  }

  UInt_t command_cnt=0;
  UInt_t j=0;
  for(UInt_t i=0;i<sConfFile.size();i++) {
    vector<TString> command = strparse::split(sConfFile[i], " ");
    // "newpage" command
    if(command[0] == "newpage") {
      // command is first of pair
      for(j=i+1;j<sConfFile.size();j++) {
        if( (strparse::split(sConfFile[j], " "))[0] != "newpage" ) {
          // Count how many commands within the page
          command_cnt++;
        } else break;
      }
      pageInfo.push_back(make_pair(i,command_cnt));
      i += command_cnt;
      command_cnt=0;
    }
    if(command[0] == "2DbinsX") {
      if (hist2D_nBinsX) {
        cerr << "Error: redefinition of hist2D_nBinsX in command: " << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      hist2D_nBinsX = atoi(command[1]);
    }
    if(command[0] == "2DbinsY") {
      if (hist2D_nBinsY) {
        cerr << "Error: redefinition of hist2D_nBinsY in command: " << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      hist2D_nBinsY = atoi(command[1]);
    }
    if(command[0] == "definecut") {
      if(command.size()>3) {
        cerr << "Warning: too many arguments for cut in command:" << endl
             << "\t" << sConfFile[i] << endl;
        continue;
      }
      for(int k = 0; k<cutList.size(); k++) {
        if ((TString)cutList[k].GetName() == command[1] ) {
          cerr << "Error: redefinition of cut: " << command[1] << "\tin command: " << endl
               << "\t" << sConfFile[i] << endl;
          exit(1);
        }
      }
      TCut tempCut(command[1],command[2]);
      cutList.push_back(tempCut);
    }
    if(command[0] == "rootfile") {
      if(command.size() != 2) {
        cerr << "Error: incorrect number of arguments for rootfile in command:" << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      rootfilenames.push_back(command[1]);
      TString temp = command[1](command[1].Last('_')+1,command[1].Length());
      fRunNumbers.push_back(atoi(temp(0,temp.Last('.')).Data()));
    }
    if(command[0] == "goldenrootfile") {
      if(command.size() != 2) {
        cerr << "Error: incorrect number of arguments for goldenrootfile in command:" << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      if(!goldenrootfilename.IsNull()) {
        cerr << "Error: redefinition of goldenrootfile in command:" << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      goldenrootfilename = command[1];
    }
    if(command[0] == "protorootfile") {
      if(command.size() != 2) {
        cerr << "Error: incorrect number of arguments for protorootfile in command:" << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      if(!protorootfile.IsNull()) {
        cerr << "Error: redefinition of protorootfile in command:" << endl
             << "\t" << sConfFile[i] << endl;
        exit(1);
      }
      protorootfile = command[1];
    }
    if(command[0] == "guicolor") {
      if(command.size() != 2) {
        cerr << "WARNING: guicolor command does not have the correct number of arguments (only 1)"
             << endl;
        continue;
      }
      if(!guicolor.IsNull()) {
        cerr << "WARNING: redefinition of guicolor (will use the later one) in command" << endl
             << "\t" << sConfFile[i] << endl;
      }
      guicolor = command[1];
    }
    if(command[0] == "plotsdir") {
      if(command.size() != 2) {
        cerr << "WARNING: Incorrect number of arguments for plotsdir in command" << endl 
             << "\t" << sConfFile[i] << endl;
        continue;
      }
      if(!plotsdir.IsNull()) {
        cerr << "WARNING: redefinition of plotsdir (will use the later one) in command" << endl
             << "\t" << sConfFile[i] << endl;
      }
      plotsdir = command[1];
    }
    if(command[0] == "plotFormat") {
      if(command.size() != 2) {
        cerr << "WARNING: Incorrect number of arguments for plotsdir in command" << endl 
             << "\t" << sConfFile[i] << endl;
        continue;
      }
      if(!fPlotFormat.IsNull()) {
        cerr << "WARNING: redefinition of plotsformat (will use the later one) in command" << endl
             << "\t" << sConfFile[i] << endl;
      }
      fPlotFormat = command[1];
    }

  }

  if(fVerbosity>=3){
    cout << "DrawConfig::ParseConfig()\n";
    for(UInt_t i=0; i<GetPageCount(); i++) {
      cout << "Page " << i << " (" << GetPageTitle(i) << ")" << " will draw " << GetDrawCount(i) 
	         << " histograms." << endl;
    }
  }

  cout << "Number of pages defined = " << GetPageCount() << endl;
  cout << "Number of cuts defined = " << cutList.size() << endl;

  if(!goldenrootfilename.IsNull()) {
    cout << "Will compare chosen histrograms with the golden rootfile: " 
         << goldenrootfilename << endl;
  }

  if (fRunNumbers.size() != rootfilenames.size()) {  // always check # of runs and # of rootfiles
    cerr << "WARNING: unmatched run numbers and root files" << endl;
    exit(1);
  }
    
  return kTRUE;

}

TCut DrawConfig::GetDefinedCut(TString ident) {
  // Returns the defined cut, according to the identifier

  for(UInt_t i=0; i<cutList.size(); i++) {
    if((TString)cutList[i].GetName() == ident.Data()) {
      TCut tempCut = cutList[i].GetTitle();
      return tempCut;
    }
  }
  return "";
}

vector <TString> DrawConfig::GetCutIdent() {
// Returns a vector of the cut identifiers, specified in config
  vector <TString> out;
  for(UInt_t i=0; i<cutList.size(); i++) {
  out.push_back(cutList[i].GetName());
  }
  return out;
}

Bool_t DrawConfig::IsLogy(UInt_t page) {
  // Check if last word on line is "logy"

  UInt_t page_index = pageInfo[page].first;
  vector <TString> command = strparse::split(sConfFile[page_index], " ");
  Int_t word_index = command.size()-1;
  if (word_index <= 0) return kFALSE;
  TString option = command[word_index];  
  if(fVerbosity>=1){
    cout << "DrawConfig::IsLogy()\t" << "option: " << option << " in page " << page_index << endl;
    for (Int_t i= 0; i < command.size(); i++) {
      cout << command[i] << " ";
    }
    cout << endl;
  }

  if(option == "logy") 
    return kTRUE;
  else 
    return kFALSE;
}

pair <UInt_t, UInt_t> DrawConfig::GetPageDim(UInt_t page) 
{
  // If defined in the config, will return those dimensions
  //  for the indicated page.  Otherwise, will return the
  //  calculated dimensions required to fit all histograms.

  pair <UInt_t, UInt_t> outDim;

  // This is the page index in sConfFile.
  UInt_t page_index = pageInfo[page].first;
  vector<TString> command = strparse::split(sConfFile[page_index], " ");
  
  UInt_t size1 = 2;
  if (IsLogy(page)) size1 = 3;  // last word is "logy"
  
  // If the dimensions are defined, return them.
  if(command.size()>size1-1) {
    if(command.size() == size1) {
      outDim = make_pair(UInt_t(atoi(command[1])), UInt_t(atoi(command[1])));
      return outDim;
    } else if (command.size() == size1+1) { // do nothing
    } else {
      cerr << "Warning: too many dimension arguments for page (will use the first two) in command" 
           << endl
           << "\t" << sConfFile[page_index] << endl;
    }

    outDim = make_pair(UInt_t(atoi(command[1])), UInt_t(atoi(command[2])));
    return outDim;
  } else {
    cerr << "Warning: no dimension arguments for page (will use default value) in command" << endl
         << "\t" << sConfFile[page_index] << endl;
  }
  
  // If not defined, return the "default."
  UInt_t draw_count = GetDrawCount(page);
  UInt_t dim = UInt_t(TMath::Nint(sqrt(double(draw_count+1))));
  outDim = make_pair(dim,dim);

  return outDim;
}

TString DrawConfig::GetPageTitle(UInt_t page) 
{
  // Returns the title of the page.
  //  if it is not defined in the config, then return "Page #"

  TString title;

  UInt_t lineIndex = pageInfo[page].first + 1;  // check the first following non-blank line 
  vector<TString> command = strparse::split(sConfFile[lineIndex], " ");

  if(command[0] == "title") { 
    pair <int, int> quotes = strparse::get_quotes(sConfFile[lineIndex], 5);
    if (quotes.first == -1 && quotes.second == -1) {  // find unquoted title
      if (command.size() > 2) {
        cerr << "Error: too many arguments for title in command: " << endl
             << "\t" << sConfFile[lineIndex] << endl
             << "Did you forget quotes" << endl;
        exit(1);
      }
      title = command[1];
    } else if (quotes.first == -1) { // unmatched quote
      cerr << "Error: unmatched quote in command: " << endl
           << "\t" << sConfFile[lineIndex] << endl;
      exit(1);
    } else {  // find quoted title
      title = sConfFile[lineIndex](quotes.first+1, quotes.second-1);
      if (quotes.second < sConfFile[lineIndex].Length()-1) {
        cerr << "Warning: too many arguments for title in command: " << endl
             << "\t" << sConfFile[lineIndex] << endl
             << "Take only the first quoted words" << endl;
      }
    }
    title.Chop(); // ???
    return title;
  }
  title = "Page "; 
  title += page;
  return title;
}

vector <UInt_t> DrawConfig::GetDrawIndex(UInt_t page) 
{
  // Returns an index of where to find the draw commands within a page
  //  within the sConfFile vector

  vector <UInt_t> index;
  UInt_t iter_command = pageInfo[page].first+1;

  for(UInt_t i=0; i<pageInfo[page].second; i++) {
    if(strparse::split(sConfFile[iter_command+i], " ")[0] != "title") {
      index.push_back(iter_command+i);
    }
  }

  return index;
}

UInt_t DrawConfig::GetDrawCount(UInt_t page) 
{
  // Returns the number of histograms that have been request for this page
  UInt_t draw_count=0;

  for(UInt_t i=0; i<pageInfo[page].second; i++) {
    if(strparse::split(sConfFile[pageInfo[page].first+i+1], " ")[0] != "title") 
      draw_count++;
  }

  return draw_count;

}

vector <TString> DrawConfig::GetDrawCommand(UInt_t page, UInt_t nCommand) {
  vector <UInt_t> command_vector = GetDrawIndex(page);
  UInt_t index = command_vector[nCommand];
  return strparse::split(sConfFile[index], " ");
}

list <TString> DrawConfig::GetTreeDrawCommand(UInt_t page, UInt_t nCommand) { // only for tree draw
  // Returns the vector of strings pertaining to a specific page, and 
  //   draw command from the config.
  // Return vector of TStrings:
  //  0: title
  //  1: grid
  //
  //  0: var1
  //  1: cut1
  //  2: opt1
  //  3: tree1
  //  0: var2
  //  1: cut2
  //  2: opt2
  //  3: tree2

  vector <UInt_t> command_vector = GetDrawIndex(page);
  UInt_t index = command_vector[nCommand];
  TString commandLine = sConfFile[index];
  vector<TString> subCommands;
  int nField = 4; // number of field per command: var, cut, opt, tree

  // identify how many draw command, seperated by ;
  int sindex = strparse::first_unquoted(commandLine, ';');
  while (sindex != -1 && sindex < (commandLine.Length()-1)) {
    subCommands.push_back(commandLine(0, sindex));
    commandLine.Remove(0, sindex+1);
    // in parsefile, we have make sure that no extra ';' or space follow a ';'
    // commandLine = strparse::strip_head_space(commandLine);
    sindex = strparse::first_unquoted(commandLine, ';');
  }

  if (sindex == -1) {  // end with ';', no further command
    subCommands.push_back(commandLine);
  } else {
    subCommands.push_back(commandLine(0, sindex));
  }

  int nDraw = subCommands.size();
  if(nDraw > 1) {
    cout << "Find " << nDraw << " commands in line: " << endl
         << "\t" << sConfFile[index] << endl;
  }
  if(fVerbosity >= 1) {
    cout << __PRETTY_FUNCTION__ << "\t" << __LINE__ << endl;
    cout << "DrawConfig::GetDrawCommand(" << page << "," << nCommand << ")" << endl;
    for (int i=0; i<nDraw; i++) {
      cout << "\t" << subCommands[i] << endl;
    }
  }
  vector <TString> out_command(nField*nDraw+2);

  if(fVerbosity > 1){
    cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;
    cout << "DrawConfig::GetDrawCommand(" << page << "," << nCommand << ")" << endl;
  }

  for(UInt_t i=0; i<out_command.size(); i++) {
    out_command[i] = "";
  }

  // extract each sub draw command
  for(int iDraw=0; iDraw<nDraw; iDraw++){
    // First word is the variable
    vector <TString> subcommand = strparse::split(subCommands[iDraw], " ");
    if(subcommand.size()>=1) {
      out_command[nField*iDraw+2] = subcommand[0];
    }

    // Second word might be the cut.
    // cut must be the second word
    if(subcommand[1] != "-type" &&
       subcommand[1] != "-title" &&
       subcommand[1] != "-tree" &&
       subcommand[1] != "-grid") {
      out_command[nField*iDraw+3] = subcommand[1];
    }

    // Now go through the rest of that line..
    for (UInt_t i=2; i<subcommand.size(); i++) {
      if(subcommand[i]=="-type") {
        if(out_command[nField*iDraw+5].IsNull()){
          out_command[nField*iDraw+5] = subcommand[i+1];
          i = i+1;
        } else {
          cerr << "Error: Multiple types in line: " << endl
               << "\t" << sConfFile[index] << endl;
          exit(1);
        }
      } else if(subcommand[i]=="-title") {
        // Put the entire title, (must be) surrounded by quotes, as one TString
        TString title;
        UInt_t j=i+1;

//        if (j = subcommand.size() || subcommand[j].BeginsWith('-')) {
//          cerr << "Error: -title option must be followed by a title! in line: " << endl
//               << "\t" << sConfFile[index] << endl;
//          exit(1);
//        }
//
//        pair <int, int> quotes = strparse::get_quotes(subCommands[iDraw], 6);
//        if (quotes.first == -1 && quotes.second == -1) {  // find unquoted title
//          title = subcommand[j];
//          i = j;
//        } else if (quotes.first == -1) { // unmatched quote
//          cerr << "Error: unmatched quote in command: " << endl
//               << "\t" << subCommands[iDraw] << endl;
//          exit(1);
//        } else {  // find quoted title
//          title = subCommands[iDraw](quotes.first+1, quotes.second-1);
//          // FIXME what about i? how to read next option ???
//        }


        /* possible title:
        * 1: single word: title
        * 2: quoted multi-words:  "a long title" or 'a long title'
        * 3: including quote:     " inlcuding 'single quote' " or 'including "double quote" '
        * 4: begin/end with quote: " a title", "a title ", " a title "
        */
        TString quote;
        TString word = subcommand[i+1]; // the next word
        if (word.BeginsWith("\""))
          quote = "\"";
        else if (word.BeginsWith("'")) 
          quote = "'";
        else {  // not quuoted
          title = word;
          i = j;
        }
        if (quote) { // found quote
          for(j=i+1; j<subcommand.size(); j++) {
            TString word = subcommand[j];

            if( word.EqualTo(quote) ){ // single " or '
              if (title.Length() == 0) continue;  // beginning " or '
              else {  // ending "
                i = j;
                break;
              }
            } else if( (word.BeginsWith(quote)) && (word.EndsWith(quote)) ) {
              title += word.ReplaceAll(quote,"");
              i = j;
              break;
            } else if(word.BeginsWith(quote)) {
              title = word.ReplaceAll(quote,"");
            } else if(word.EndsWith(quote)) {
              title += " " + word.ReplaceAll(quote,"");
              i = j;
              break;
            } else if (title.Length()==0){
              title = word;
            } else {
              //  This case uses neither "i = j;" or "break;", because
              //  we want to be able to include all the words in the title.
              //  The title will end before the end of the line only if
              //  it is delimited by quotes.
              title += " " + word;
            }
          }
        }
        if (j==subcommand.size() && !(subcommand[j-1].EndsWith(quote)) ){
          // unmatched quote
          // if there is no front ' or ", quote will be null, so .Endswith(quote)
          // will be true; on the other hand, if there is front ' or ", quote is one
          // of them, so .EndsWith(quote) return false.
          cerr << "Error, unmatched double quote in line " << endl
               << "\t" << subCommands[iDraw] << endl; 
          exit(1);
        }
        if (out_command[0].IsNull()){
          out_command[0] = title;
        } else {
          cerr << "Error: Redifinition of title in line (only one title per command line): " << endl
               << "\t" << sConfFile[index] << endl; 
          exit(1);
        }
      } else if(subcommand[i]=="-tree") {
        if (out_command[nField*iDraw+4].IsNull()){
          out_command[nField*iDraw+4] = subcommand[i+1];
          i = i+1;
        } else {
          cerr << "Error: Multiple trees in line: " << endl
               << "\t" << sConfFile[index] << endl; 
          exit(1);
        }
      } else if(subcommand[i]=="-grid") {
        if (out_command[1].IsNull()){ // grid option only works with TreeDraw
          out_command[1] = "grid";
        } else {
          cerr << "Warning: Multi-times setup of grid in line" << endl
               << "\t" << sConfFile[index] << endl; 
        }
      } else { // unknow option
        cerr << "Error: unknow option: " << subcommand[i] << "\tin line: " << endl
             << "\t" << sConfFile[index] << endl; 
        exit(1);
      }
    }

//    if(fVerbosity>=1){  // print each subcommand
//      cout << subcommand.size() << ": ";
//      for(UInt_t i=0; i<subcommand.size(); i++) {
//        cout << subcommand[i] << " ";
//      }
//      cout << endl;
//    }
  }
  if(fVerbosity >= 1) { // print all command
    for(UInt_t i=0; i<out_command.size(); i++) {
      cout << i << ": " << out_command[i] << endl;
    }
  }

  list <TString> out_list(out_command.begin(), out_command.end());
  return out_list;
}

void DrawConfig::OverrideRootFile(std::vector<UInt_t> runnumbers) 
{
  // Override the ROOT file defined in the cfg file If
  // protorootfile is used, construct filename using it, otherwise
  // uses a helper macro "GetRootFileName.C(UInt_t runnumber)
  cout << endl;
  cout<< "Root file defined before was: " << endl;
  for(int i=0; i<rootfilenames.size(); i++) cout << "\t" << rootfilenames[i] <<endl;
  fRunNumbers.clear();
  rootfilenames.clear();
  if(!protorootfile.IsNull()) {
    TString fullpath = protorootfile;
    TString match = fullpath(TRegexp("$[a-zA-Z0-9_]+"));
    while (!match.IsNull()) {
      TString env = match(1, match.Length()); // get the ENV variable
      TString replace = std::getenv(env.Data());
      fullpath.ReplaceAll(match, replace); // FIXME; problematic if one ENV contains another ENV
      match = fullpath(TRegexp("$[a-zA-Z0-9_]+"));
    }
    for(int i=0; i<runnumbers.size(); i++) {
      char runnostr[10];
      TString tempfile = protorootfile;
      sprintf(runnostr,"%04i",runnumbers[i]);
      TString rootfile = fullpath;
      rootfile.ReplaceAll("XXXXX",runnostr);

      // Check if the given root file exist or not 
      struct stat result;
      if (stat(rootfile.Data(), &result) != 0) { // file doesn't exist
        cerr << "Warning: File " << (tempfile.ReplaceAll("XXXXX", runnostr)).Data() << " doesn't exist, please check your config file." << endl;
        continue;
      }
      rootfilenames.push_back(tempfile.ReplaceAll("XXXXX", runnostr)); 
//      TString temp = rootfilename(rootfilename.Last('_')+1,rootfilename.Length());
//      fRunNumbers.push_back(atoi(temp(0,temp.Last('.')).Data()));
      fRunNumbers.push_back(runnumbers[i]);
      cout << "Protorootfile set, use it: " << rootfilenames[i].Data() << endl;
    }
    if (rootfilenames.size() == 0) { // No valid root files
      cerr << "Warning: No valid root file, please check it." << endl;
      exit(1);
    }
  } else {
    string fnmRoot="/adaq1/work1/apar/japanOutput";
    if(getenv("QW_ROOTFILES"))
      fnmRoot = getenv("QW_ROOTFILES");
    else
      cout<<"QW_ROOTFILES env variable was not found going with default: "<< fnmRoot<<endl;

    cout << " Looking for file with runnumbers "; 
    for (int i=0; i<runnumbers.size(); i++) 
      cout << "\t" << runnumbers[i]; 
    cout <<" in "<<fnmRoot<<endl;

    DIR *dirSearch;
    struct dirent *entSearch;
    const string daqConfigs[3] = {"CH","inj","ALL"};
    int found=0;
    string partialname = "";
    if ((dirSearch = opendir (fnmRoot.c_str())) != NULL) {
      while ((entSearch = readdir (dirSearch)) != NULL) {
        for(int i=0; i<runnumbers.size(); i++) {
          for(int j=0;j<3;j++){
            partialname = Form("prex%s_%d.root",daqConfigs[j].c_str(),runnumbers[i]);
            std::string fullname = entSearch->d_name;
            if(fullname.find(partialname) != std::string::npos){
              rootfilenames.push_back(fnmRoot + "/" + fullname);
              fRunNumbers.push_back(runnumbers[i]);
              found++;
            }
            if(found) break;  // use only the first founded root file 
          }
        }
        if (found) break; // search until find root file, then stop, don't continue with other dir.
      }
      closedir (dirSearch);
    }

    if(found){
      cout<<"\t found file: ";
      for (int i=0; i<rootfilenames.size(); i++)
        cout << "\t\t" << rootfilenames[i] <<endl;
      cout << endl;
    }else{
      cout<<"double check your configurations and files. Quitting"<<endl;
      exit(1);
    }
  }
  
  cout << "Number of root files defined = " << rootfilenames.size() << endl << endl;
}
