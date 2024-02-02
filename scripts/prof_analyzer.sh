#! /bin/bash

# if [ $# -ne 2 ]; then

# 	echo "Usage: $0 <logFile> <elfFile>"
# 	exit 1
# fi

log_file=$1
#elf_file=$2

toolchain=aarch64-none-linux-gnu- #For now, the profiling feature is only expected to work on arm64 platforms

declare -a fn_tags
declare -a exec_times
declare -a entry_timestamps


if ! [ -f $log_file  -a -r $log_file ]; then
	echo "Error: file $log_file doesn't exist or can't be read"
	exit 2
fi

# if ! [ -f $elf_file  -a -r $elf_file ]; then
# 	echo "Error: file $elf_file doesn't exist or can't be read"
# 	exit 2
# fi

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
		entry_timestamps+=$timestamp
	else
		last_entry=$((${#entry_timestamps[@]} - 1))
		#echo "DBG array: $last_entry -> ${entry_timestamps[@]}"
		last_timestamp=${entry_timestamps[$last_entry]}
		#echo "DBG last timestamp: $last_timestamp"
		fn_time=$(bc <<< "$timestamp - $last_timestamp")
		unset entry_timestamps[$last_entry]
		#echo "DBG entry_timestamps: ${entry_timestamps[@]}"
		echo "$fn_tag time elapsed: $fn_time"
		fn_tags+=$fn_tag
		exec_times+=$fn_time
	fi
	
done < $log_file
