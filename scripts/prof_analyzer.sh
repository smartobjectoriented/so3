#! /bin/bash

if [ $# -lt 1 -o $# -gt 3 ]; then
	echo "Usage: $0 <logFile> [elfFile] [disassembly_symbol]"
	exit 1
fi

log_file=$1

declare -a fn_tags
declare -a exec_times
declare -a entry_timestamps

if ! [ -r "$log_file" ]; then
	echo "Error: file $log_file doesn't exist or can't be read"
	exit 2
fi

# Analyse log file of program execution to recover profiling timestamps
# Profiling timestamps have the format: prof >/< <fn_tag> <timestamp>
# prof marks a profiling timestamp
# >/< tells if the timestamp is for entry/exit of the profiled function
# <fn_tag> is the return address of the profiled function
# <timestamp> is the time returned by clock_gettime at entry/exit of the function
# Resulting execution times are stored in fn_tags and exec_times arrays, sorted by fn_tags
while read log_line; do
	if [ -z "$log_line" ]; then continue; fi
	if [[ "$log_line" == "prof "* ]]; then
		prof_data=${log_line#prof }
	else continue
	fi
	#echo "DBG Prof data 1: $prof_data"

	if [[ "$prof_data" == "> "* ]]; then
		fn_entry=true
		prof_data=${prof_data#> *}
		#echo "DBG function entry timestamp"
	elif [[ $prof_data == "< "* ]]; then
		fn_entry=false
		prof_data=${prof_data#< *}
		#echo "DBG function exit timestamp"
	else
		echo "Malformed prof data: $prof_data"
		continue
	fi
	#echo "DBG Prof data 2: $prof_data"

	fn_tag=${prof_data%%: *}
	#echo "DBG fn_tag: $fn_tag"

	timestamp=$(tr -d $'\r' <<< "${prof_data##*: }")
	#echo "DBG timestamp: $timestamp"

	if $fn_entry; then
		entry_timestamps+=("$timestamp")
	else
		last_entry=$((${#entry_timestamps[@]} - 1))
		#echo "DBG array: $last_entry -> ${entry_timestamps[@]}"
		last_timestamp=${entry_timestamps[$last_entry]}
		#echo "DBG last timestamp: $last_timestamp"
		fn_time=$(bc <<< "$timestamp - $last_timestamp")
		unset entry_timestamps[$last_entry]
		#echo "DBG entry_timestamps: ${entry_timestamps[@]}"
		fn_tags+=("$fn_tag")
		exec_times+=("$fn_time")
		#echo "DBG fn_tag, fn_time: $fn_tag, $fn_time"
		#echo "DBG fn_tags, exec_times: ${fn_tags[@]}, ${exec_times[@]}"

		# Insertion sort of new fn_tag and execution time
		curr_idx=$((${#fn_tags[@]} - 1))
		while [ $curr_idx -gt 0 ]; do
			prev_idx=$((curr_idx - 1))
			curr_tag=${fn_tags[$curr_idx]}
			prev_tag=${fn_tags[$prev_idx]}
			if [[ "0x$curr_tag" -lt "0x$prev_tag" ]]; then
				# Swap tags
				fn_tags[$prev_idx]=$curr_tag
				fn_tags[$curr_idx]=$prev_tag

				# Swap corresponding execution times
				exec_swap=${exec_times[$prev_idx]}
				exec_times[$prev_idx]=${exec_times[$curr_idx]}
				exec_times[$curr_idx]=$exec_swap

				((curr_idx--))
			else
				break
			fi
		done

		#echo "DBG fn_tags, exec_times sorted: ${fn_tags[@]}, ${exec_times[@]}"
	fi
done < $log_file

if [ $# -lt 2 ]; then #No ELF file given, simply display tags and execution time
	echo "Function tag | Elapsed Time"
	for idx in ${!fn_tags[@]}; do
		#echo "DBG idx: $idx"
		echo "${fn_tags[$idx]} | ${exec_times[$idx]}"
	done
	exit 0
fi

elf_file=$2
objdump=/usr/bin/aarch64-none-elf-objdump #For now, the profiling feature is only expected to work on arm64 platforms

if ! [ -r "$elf_file" ]; then
	echo "Error: file $elf_file doesn't exist or can't be read"
	exit 2
fi

if ! [ -x "$objdump" ]; then
	echo "Error: Desired objdump ($objdump) doesn't exist"
	exit 3
fi

declare -a fn_names
caller_fn="none"
callee_fn="none"
curr_addr=0
fn_idx=0
disassembly=false

#echo "DBG pwd|user: $(pwd)|$USER"

if [ $# -eq 3 ]; then disassembly="$objdump --disassemble=$3 $elf_file"
else disassembly="$objdump -d $elf_file"; fi

while read line; do

	# Ignore lines preceding disassembly
	if ! [[ $disassembly ]]; then 
		if [[ $line == "Disassembly of section"* ]]; then disassembly=true; continue; fi
	fi

	if ! [[ $line == *":"* ]]; then continue; fi
	line_head=${line%%:*}
	line_data=${line#*:}
	if [ -z "$line_data" ]; then
		caller_fn=${line_head##*<}
		caller_fn=${caller_fn%%>*}
		#echo "DBG line -> caller_fn: $line -> $caller_fn"
	else 
		curr_addr=$(tr -d '[:space:]' <<< "$line_head")
		if [[ "0x$curr_addr" == "0x${fn_tags[$fn_idx]}" ]]; then
			fn_name="$caller_fn/$callee_fn"
			#echo "DBG curr_addr, fn_name: $curr_addr, $fn_name"
			fn_names+=("$fn_name")
			fn_idx=$((fn_idx + 1))
		fi

		if [[ "$line_data" == *"bl"*"<"*">"* ]]; then
			callee_fn=${line_data##*<}
			callee_fn=${callee_fn%>*}
			#echo "DBG line -> callee_fn: $line ->$callee_fn"
		fi
	fi
done < <($disassembly)
#echo "DBG fn_names: ${fn_names[@]}"

echo "Function name (Function tag) | Elapsed Time (s)"
for idx in ${!fn_tags[@]}; do
	echo "${fn_names[$idx]} (${fn_tags[$idx]}) | ${exec_times[$idx]}"
done
