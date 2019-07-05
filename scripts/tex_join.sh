#!/bin/bash
############################################
# Create tex-style table lines for pdf display
# by joinning all input files
# -- Weibin (2019-07-03)
############################################

ERR_MISSING_PARAMETER=1
ERR_FILE_EXIST=11
ERR_FILE_NOT_EXIST=12
ERR_BAD_FUN_CALL=22

TEMP_TXT=temp_txt.txt

function usage {
  echo "$0 txt1 txt2 [ txt3 ...]"
}

function check_file {
  if [ $# -eq 0 ]; then
    echo "Error: At least one argument needed. Quitting!"
    exit $ERR_BAD_FUN_CALL
  fi
  while [ $# -gt 0 ]; do
    filename="$1"
    if ! [ -a "$filename" ]; then
      echo "Error: File: $filename doesn't exist, please check the filename and execute the script again. Quitting! " 
      exit $ERR_FILE_NOT_EXIST
    fi
    shift
  done
}
function check_no_file {
  if [ $# -eq 0 ]; then
    echo "Error: At least one argument needed. Quitting!"
    exit $ERR_BAD_FUN_CALL
  fi
  while [ $# -gt 0 ]; do
    filename="$1"
    if [ -a "$filename" ]; then
      echo "Error: File: $filename already exist, please remove it and execute the script again. Quitting! " 
      exit $ERR_FILE_EXIST
    fi
    shift
  done
}

if [ $# -lt 2 ]; then
  echo "Error: At least 2 parameter needed. Quitting"
  usage;
  exit $ERR_MISSING_PARAMETER
fi

# extract data from each input file and join them

declare -a runNumbers
# the first input
check_file "$1"
file_index=0
runNumbers[0]=${1##*_} 
title=${runNumbers[$file_index]} && echo "run number set: $title";
check_no_file "$TEMP_TXT-$file_index"
cat "$1" | grep block | sed 's/.*.\(block[0-9]\)-.*.\(block[0-9]\)\s/\1-\2/g' | sed 's/mean:/\&/g' | sed 's/meanerror:/\&/g' | sed 's/ratio:/\&/g' > "$TEMP_TXT-$file_index";
shift
TEMP_TXT_old="$TEMP_TXT-$file_index"
let file_index++

# 2nd to 2nd last input
while [ $# -gt 1 ]; do
  check_file "$1"
  runNumbers[$file_index]=${1##*_} 
  title="$title+${runNumbers[$file_index]}" && echo "run number set: $title";
  check_no_file "$TEMP_TXT-$i";
  cat "$1" | grep block | sed 's/.*.\(block[0-9]\)-.*.\(block[0-9]\)\s/\1-\2/g' | sed 's/mean:/\&/g' | sed 's/meanerror:/\&/g' | sed 's/ratio:/\&/g' | join "$TEMP_TXT_old" - > "$TEMP_TXT-$file_index";
  rm -f "$TEMP_TXT_old" && TEMP_TXT_old="$TEMP_TXT-$file_index"
  let file_index++;
  shift
done

# the last input; add '\\' to the end of each line
check_file "$1";
runNumbers[$file_index]=${1##*_};
title="$title+${runNumbers[$file_index]}" && echo "run number set: $title";
check_no_file "$TEMP_TXT";
cat "$1" | grep block | sed 's/.*.\(block[0-9]\)-.*.\(block[0-9]\)\s/\1-\2/g' | sed 's/mean:/\&/g' | sed 's/meanerror:/\&/g' | sed 's/ratio:/\&/g' | sed 's/$/\t\\\\\\\\/g' | join "$TEMP_TXT_old" - > "$TEMP_TXT";
rm -f "$TEMP_TXT_old" && unset TEMP_TXT_old
let file_index++;
shift


# create tex output
var_index=0;
line_index=0
output=tex_$title;
check_no_file $output;
touch $output;
# variables=('inj\_laser\_table\_battery\_1' 'inj\_laser\_table\_battery\_2' 'inj\_battery\_2' 'ch\_battery\_1' 'ch\_battery\_2' 'phasemonitor'); # variables
variables=('bcm\_an\_ds3' 'bcm\_an\_us' 'bpm4aX' 'bpm4aY' 'bpm4eX' 'bpm4eY' 'usl' 'usr' 'dsl' 'dsr'); # variables

i=0
columns=1
while [ $i -lt $file_index ]; do
  tabularline="$tabularline| c   c   c "
  firsthead="$firsthead& \\\\multicolumn{3}{c}{${runNumbers[$i]}} "
  secondhead="$secondhead& Mean  & Meanerror & ratio "
  let columns+=3
  let i++
done
unset i
cat "$TEMP_TXT" | while read line; do
  if [ $line_index -eq 0 ]; then
cat << EOF >> "$output"


\\begin{frame}
  \\frametitle{$title: ${variables[$var_index]}}
  \\begin{adjustbox}{center}
      \\begin{tabular}{ c $tabularline}
          \\hline
          \\multirow{2}{*}{Blocks} &$firsthead\\\\
          \\cline{2-$columns}
          $secondhead\\\\
          \\hline
          $line
EOF
  elif [ $line_index -eq 2 ] || [ $line_index -eq 4 ]; then
cat << EOF >> "$output"
          \\hline
          $line
EOF
  elif [ $line_index -eq 5 ]; then
cat << EOF >> "$output"
          $line
          \\hline
      \\end{tabular}
  \\end{adjustbox}
\\end{frame}
EOF
  else
cat << EOF >> "$output"
          $line
EOF
  fi
  let line_index++
  let var_index+=line_index/6
  let line_index%=6
done
rm -f "$TEMP_TXT"
unset runNumbers
unset title
unset file_index columns var_index line_index
unset tabularline firsthead secondhead
unset output
