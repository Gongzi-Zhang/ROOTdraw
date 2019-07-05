#!/bin/bash
############################################
# Create tex-style table lines for pdf display
# for each input file
# -- Weibin (2019-07-02)
############################################

ERR_MISSING_PARAMETER=1
ERR_FILE_EXIST=11
ERR_FILE_NOT_EXIST=12
ERR_BAD_FUN_CALL=22

TEMP_TXT=temp_txt.txt

function usage {
  echo "$0 txt1 [ txt2 txt3 ...]"
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

if [ $# -eq 0 ]; then
  echo "Error: At least one parameter needed. Quitting"
  usage;
  exit $ERR_NO_PARAMETER
fi

while [ $# -gt 0 ]; do
  check_no_file $TEMP_TXT;
  runNumber=${1##*_};
  echo "run number = $runNumber"; echo;
  cat $1 | grep block | sed 's/.*.\(block[0-9]\)-.*.\(block[0-9]\)\s/\1-\2/g' | sed 's/mean:/\&/g' | sed 's/meanerror:/\&/g' | sed 's/ratio:/\&/g' | sed 's/$/\t\\\\\\\\/g' > $TEMP_TXT;
  var_index=0;
  line_index=0
  output=tex_$runNumber;
  check_no_file $output;
  touch $output;
  # variables=('inj\_laser\_table\_battery\_1' 'inj\_laser\_table\_battery\_2' 'inj\_battery\_2' 'ch\_battery\_1' 'ch\_battery\_2' 'phasemonitor'); # variables
  variables=('bcm\_an\_ds3' 'bcm\_an\_us' 'bpm4aX' 'bpm4aY' 'bpm4eX' 'bpm4eY' 'usl' 'usr' 'dsl' 'dsr'); # variables
  cat $TEMP_TXT | while read line; do
    if [ $line_index -eq 0 ]; then
cat << EOF >> "$output"


\\begin{frame}
    \\frametitle{Run $runNumber: ${variables[$var_indexi]}}
    \\begin{table}
        \\begin{tabular}{ c | c   c | c }
            \\hline
            Block & Mean  & Meanerror & ratio  \\\\
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
    \\end{table}
\\end{frame}
EOF
    else
cat << EOF >> "$output"
            $line
EOF
    fi
    let line_index++
    let var_indexi+=line_index/6
    let line_index%=6
  done
  rm $TEMP_TXT
  shift
done
unset runNumber
unset var_index line_index
unset output
