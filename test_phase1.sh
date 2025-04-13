#!/bin/bash

# Clean up previous runs
rm -rf HuntTest logged_hunt-HuntTest
echo "== Starting Phase 1 Test =="

# Compile the program
gcc -o treasure_manager treasure_manager.c

# Add a treasure
echo -e "treasure1\nplayer1\n45.12\n-93.45\nFind me under the rock.\n100" | ./treasure_manager --add HuntTest

# Add a second treasure
echo -e "treasure2\nplayer2\n44.98\n-93.27\nLook behind the tree.\n250" | ./treasure_manager --add HuntTest

# List treasures
echo -e "\n== List Treasures =="
./treasure_manager --list HuntTest

# View specific treasure
echo -e "\n== View Treasure treasure1 =="
./treasure_manager --view HuntTest treasure1

# Remove one treasure
echo -e "\n== Remove Treasure treasure1 =="
./treasure_manager --remove_treasure HuntTest treasure1

# List again to confirm removal
echo -e "\n== List Treasures After Removal =="
./treasure_manager --list HuntTest

# Remove the entire hunt
echo -e "\n== Remove Hunt =="
./treasure_manager --remove HuntTest

# Final cleanup
rm -f treasure_manager

echo -e "\n== Phase 1 Test Complete =="

