// string parse libraries
// implemented with TString 
// -- Weibin Zhang
// 2019-07-04
#include "libstrparse.hh"
#include <utility>
#include <vector>
// #include <iostream>
#include <TString.h>

// using namespace strparse;
int strparse::first_unquoted(const TString line, const char c, int start_pos) {
  int cpos = -1; // return value:
  // -1 -- no unquoted c
  // -2 -- unmatched quote
  // N  -- position of first unquoted c in line
  // What about empty string ??? -1 ???
  TString left_line = line(start_pos, line.Length());
  int cindex = left_line.First(c);
  cpos = cindex;
  TString before_string;
  // currenttly, only support ' and "
  while (cindex != -1) {
    before_string = left_line(0, cindex);
    int sqindex = before_string.First('\'');
    int dqindex = before_string.First('"');
    int qindex = -1;
    char quote;
    bool unmatched_quote = false;
    while (sqindex != -1 || dqindex != -1) {  // search unmatched quote before char c
      // the first beginning quote
      if (sqindex != -1 && sqindex < dqindex) {
        quote = '\'';
        qindex = sqindex;
      } else {
        quote = '"';
        qindex = dqindex;
      }
      before_string.Remove(0, qindex+1);
      qindex = before_string.First(quote);
      if (qindex == -1) { // find unmatched quote
        unmatched_quote = true;
        break;
      } else { // find match quote; search next one
        before_string.Remove(0, qindex+1);
        sqindex = before_string.First('\'');
        dqindex = before_string.First('"');
      }
    }

    if (unmatched_quote) {
      TString after_string = line(cpos+1, line.Length());
      qindex = after_string.First(quote);
      if (qindex == -1) {
//        cerr << "Error: Find unmatched quote in line: " << endl 
//             << "\t" << line << endl 
//             << "Please check it." << endl;
        return -2;  // 
      } else {
        left_line = after_string(qindex+1, after_string.Length());
        cindex = after_string.First(c);
        if (cindex == -1){ // no further char c
          cpos = -1;
        } else {
          cpos += (qindex + cindex);
        }
      }
    } else {
      return cpos + start_pos;  // find comment
    }
  }
  return (cpos == -1) ? cpos : (cpos + start_pos);
}


// remove beginning space until the first valid char
TString strparse::strip_head_space(const TString line) {
  TString temp = line;
  while (temp.BeginsWith(' ') || temp.BeginsWith('\t'))
    temp.Remove(0,1);

  return temp;
}

// remove tail space
TString strparse::strip_tail_space(const TString line) {
  TString temp = line;
  while (temp.EndsWith(" ") || temp.EndsWith("\t"))
    temp.Chop();

  return temp;
}

// find the first matched quotes after start_pos in line
std::pair <int, int> strparse::get_quotes(const TString line, int start_pos) {
  int begin_quote = -1, end_quote = -1;
  TString left_line = line(start_pos, line.Length());
  int sqindex = left_line.First('\'');
  int dqindex = left_line.First('"');
  char quote;
  if (sqindex == -1 && dqindex == -1) { // no quote
    return std::make_pair(begin_quote, end_quote);
  } else if (sqindex != -1 && sqindex < dqindex) {
    quote = '\'';
    begin_quote = sqindex;
  } else {
    quote = '"';
    begin_quote = dqindex;
  }

  left_line.Remove(0, begin_quote+1);
  end_quote = left_line.First(quote);
  if (end_quote == -1) {  // unmatched quote
    return std::make_pair (begin_quote+start_pos, -1);
  } else {
    return std::make_pair (begin_quote+start_pos, start_pos+begin_quote+1+end_quote);
  }
}

std::vector <TString> strparse::split(TString line,TString delim) {
  std::vector <TString> v; // return value

  TString left_line = line;
  int l = delim.Length(); // may be multi-char delimiter

  while(left_line.BeginsWith(delim)) {  // removing all beginning delimiters
    left_line.Remove(0, l);
  }
  while(left_line.EndsWith(delim)) {  // removing all ending delimiters
    left_line = left_line(0, left_line.Length()-l);
  }

  int i = left_line.Index(delim);
  while (i != -1) {
    v.push_back(left_line(0, i));
    left_line.Remove(0,i+l);
    while(left_line.Index(delim) == 0) {  // continuous delimiters
      left_line.Remove(0,l);
    }
    i = left_line.Index(delim);
  }
  v.push_back(left_line);

  return v;
}
