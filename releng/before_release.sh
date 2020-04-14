#!/bin/bash -eu

[ "$#" -ne "1" ] && echo "usage: $0 <new version>" && exit 1

NEW_VERSION=$1

# git root dir
DIR=$(cd `dirname $0` && echo `git rev-parse --show-toplevel`)
CURRENT_BRANCH=$(cd `dirname $0` && echo `git branch`)
ORIG_DIR=`pwd`
TODAY_DATE=`date +%Y.%m.%d`

#change to git root dir
cd $DIR

#update version in code and stash changes
./releng/update-version.sh $NEW_VERSION
sed -i -e "s/X\.Y\.Z/$NEW_VERSION/g" release_notes.md
sed -i -e "s/XXXX\.XX\.XX/$TODAY_DATE/g" release_notes.md

RELEASE_LINES=$(cat release_notes.md | grep -n Release | head -n 2 | cut -d':' -f 1 | xargs)
NEW_RELEASE_LINE=$(echo $RELEASE_LINES | cut -d' ' -f 1)
PREV_RELEASE_LINE=$(echo $RELEASE_LINES | cut -d' ' -f 2)
RELEASE_BODY=$(cat release_notes.md | head -n $((PREV_RELEASE_LINE - 1)) | tail -n +${NEW_RELEASE_LINE} | tr '\n' '\r' | sed 's/\r/\\r\\n/g')

# Fix headers
./releng/fix_header_copyright_and_authors.sh

# commit fixed headers (if any)
NBCHANGES=`git status --porcelain | wc -l`
if [ $NBCHANGES -ne 0 ]; then
  git add -A
  git commit -m "[RELENG] Fix headers"
fi

./releng/update-version.sh $NEW_VERSION-SNAPSHOT
cat release_notes.md | tail -n +3 > tmp
cat > release_notes.md << EOF
Spider2 Changelog
================

## Release version X.Y.Z
*XXXX.XX.XX*

### New Feature

### Changes

### Bug fix

EOF
cat tmp >> release_notes.md
rm tmp
