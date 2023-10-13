#!/bin/bash

echo "--------- Begin ---------"
git status
read -p "Input commit: " msg
git add .
git commit -m "$msg"
git pull
git push
echo "Push success: [$msg]"
echo "---------- End ----------"
read -p "Press Enter to continue..."

# cp -r ../RaOS/ ../RaOS.bak/
