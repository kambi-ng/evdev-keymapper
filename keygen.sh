#!/usr/bin/env bash

inputFile="/usr/include/linux/input-event-codes.h"
outputFile="src/keycodemap.cpp"

if [[ -n "$IN_NIX_SHELL" ]]; then
   inputFile="$(fd input-event-codes.h /nix/store --one-file-system --max-results=1)" 
fi

cat <<EOF
#ifndef KEYMAP_H
#define KEYMAP_H
#include <linux/uinput.h>
#include <unordered_map>
#include <string>


EOF

echo "static std::unordered_map<std::string, int> keycodeMap = {"

grep "#define KEY_" $inputFile | while IFS= read -r line
do
    keyName=$(echo "$line" | awk '{print $2}' )
    keyName="${keyName#KEY_}"
    # Output the map entry
    if [[ "$keyName" == "RESERVED" ]] || [[ "$keyName" == "MAX" ]]; then
        continue
    fi

    echo "    {\"$keyName\", "KEY_$keyName"},"
done


cat <<EOF
};

#endif // KEYMAP_H
EOF
