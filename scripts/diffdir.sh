#!/bin/bash

source_dir=$1
target_dir=$2
output_dir=$3

echo Generating multi-patches...

# Create the output directory if it doesn't exist
mkdir -p "$output_dir"

# Diff files
diff_files=$(diff -rq "$source_dir" "$target_dir")

while IFS= read -r line; do

    type=$(echo "$line" | awk '{print $1}')
    src=$(echo "$line" | awk '{print $2}')
    target=$(echo "$line" | awk '{print $4}')

    if [ "$type" == "Only" ]; then

        printf '%s' "."

        # File or directory only exists in the target

        path=$(echo "$line" | awk '{print substr($3, 1, length($3)-1) "/" $4}')

        if [[ -f "$path" ]]; then
         filename=$(awk '{idx = split(FILENAME, parts, "/"); print parts[idx]; nextfile}' $path)
         diff -Naur /dev/null "$path" > "$output_dir/$filename.patch"
        else

         filename=$(awk -F'/' '{print $NF}' <<< $path)

         subpath=$(echo "${path#$2}")

         diff -Naur "$1/$subpath" "$path" > "$output_dir/$filename.patch"
        fi


    elif [ "$type" == "Files" ]; then

        printf '%s' "."

        filename=$(awk '{idx = split(FILENAME, parts, "/"); print parts[idx]; nextfile}' $src)
        src=$(echo "$line" | awk '{print $2}')
        target=$(echo "$line" | awk '{print $4}')

        # Files are different
        diff -Naur "$src" "$target" > "$output_dir/$filename.patch"

    fi
done <<< "$diff_files"


echo ""
echo "Patch files generated in: $output_dir"
