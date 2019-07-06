#ifndef libstrparse_h
#define libstrparse_h 1

#include <utility>
#include <TString.h>

namespace strparse {
  int first_unquoted(const TString, const char, int start_pos = 0); // the first accurrence of unquoted character star from start_pos
  TString strip_head_space(const TString);
  TString strip_tail_space(const TString);
  std::pair <int, int> get_quotes(const TString line, int start_pos = 0); // find the first matched quotes after start_pos in line
  std::vector <TString> split(const TString,const TString delim = " "); // split string 
}

#endif
