#! /bin/bash
#
# Usage:
#
#   misc/commit-clang-tidy.sh clang-tidy-file commitmsg
#
# The given .clang-tidy is copied in laf and in aseprite repos,
# creating a commit in main and beta branches, in both repos.

pwd=$(pwd)

if [[ ! -f "$pwd/INSTALL.md" ]] ; then
    echo "run this from the aseprite dir"
    exit 1
fi

file="$1"
commitmsg="$2"
if [[ ! -f "$file" || "$commitmsg" == "" ]] ; then
    echo "Usage: commit-clang-tidy.sh clang-tidy-file commitmsg"
    exit 1
fi

echo "Coping files..."
cp $file $pwd/laf/.clang-tidy
cp $file $pwd/.clang-tidy

echo "Updating laf main branch..."
cd $pwd/laf
git fetch origin || exit 1
git checkout main || exit 1
git merge --ff-only origin/main || exit 1
git add .clang-tidy || exit 1
if ! git commit -m "$commitmsg" ; then
    echo "same rules, nothing to update"
    exit 1
fi
echo "Updating laf beta branch..."
git checkout beta || exit 1
git merge --ff-only origin/beta || exit 1
git merge --commit --no-edit main || exit 1
git checkout main || exit 1

echo "Updating aseprite main branch..."
cd $pwd
git fetch origin || exit 1
git checkout main || exit 1
git merge --ff-only origin/main || exit 1
git add laf .clang-tidy || exit 1
if ! git commit -m "$commitmsg" ; then
    echo "same rules, nothing to update"
    exit 1
fi
echo "Updating aseprite beta branch..."
cd $pwd/laf && git checkout beta || exit 1
cd $pwd
git checkout beta || exit 1
git merge --ff-only origin/beta || exit 1
git merge --commit --no-edit main # conflict, laf must be merged
git add laf
git commit --no-edit
git merge --commit --no-edit main || exit 1

cd $pwd/laf && git checkout main
cd $pwd
git checkout main
echo "Done, aseprite and laf are in main now"
