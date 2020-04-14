#!/bin/bash -eu

### Config
DEV_BRANCH=develop
MAIN_BRANCH=master
REPO=preesm/spider2

# git root dir
DIR=$(cd `dirname $0` && echo `git rev-parse --show-toplevel`)


if [ "$#" -ne "1" ]; then
  CURRENT_VERSION=$(cat ${DIR}/VERSION)
  echo -e "usage: $0 <new version>\nNote: current version = ${CURRENT_VERSION}"
  exit 1
fi

# First check access on git (will exit on error)
echo "Testing Github permission"
git ls-remote git@github.com:${REPO}.git > /dev/null

# then test github oauth access token
echo "Testing Github OAuth token validity"
if [ ! -e ~/.ghtoken ]; then
  echo "Error: could not locate '~/.ghtoken' for reading the Github OAuth token"
  echo "Visit 'https://github.com/settings/tokens/new' and generate a new token with rights on 'repo',"
  echo "then create the file '~/.ghtoken' with the token value as its only content."
  exit 1
fi
OAUTH_TOKEN=$(cat ~/.ghtoken)
STATUS=$(curl -s -i -H "Authorization: $OAUTH_TOKEN to" https://api.github.com/repos/${REPO}/releases | grep "^Status:" | cut -d' ' -f 2)
if [ "$STATUS" != "200" ]; then
  echo "Error: given token in '~/.ghtoken' is invalid or revoked."
  echo "  STATUS = $STATUS"
  echo "Visit 'https://github.com/settings/tokens/new' and generate a new token with rights on 'repo',"
  echo "then replace the content of '~/.ghtoken' with the token value."
  exit 1
fi

#warning
echo "Warning: this script will delete ignored files and remove all changes in $DEV_BRANCH and $MAIN_BRANCH"
read -p "Do you want to conitnue ? [NO/yes] " ANS
LCANS=`echo "${ANS}" | tr '[:upper:]' '[:lower:]'`
[ "${LCANS}" != "yes" ] && echo "Aborting." && exit 1

NEW_VERSION=$1

CURRENT_BRANCH=$(cd `dirname $0` && echo `git branch`)
ORIG_DIR=`pwd`
TODAY_DATE=`date +%Y.%m.%d`
#change to git root dir
cd $DIR

#move to dev branch and clean repo
git stash -u
git checkout $MAIN_BRANCH
git pull
git checkout $DEV_BRANCH
git reset --hard
git clean -xdf

#update version in code and stash changes
./releng/update-version.sh $NEW_VERSION
sed -i -e "s/X\.Y\.Z/$NEW_VERSION/g" release_notes.md
sed -i -e "s/XXXX\.XX\.XX/$TODAY_DATE/g" release_notes.md

RELEASE_LINES=$(cat release_notes.md | grep -n Release | head -n 2 | cut -d':' -f 1 | xargs)
NEW_RELEASE_LINE=$(echo $RELEASE_LINES | cut -d' ' -f 1)
PREV_RELEASE_LINE=$(echo $RELEASE_LINES | cut -d' ' -f 2)
RELEASE_BODY=$(cat release_notes.md | head -n $((PREV_RELEASE_LINE - 1)) | tail -n +${NEW_RELEASE_LINE} | tr '\n' '\r' | sed 's/\r/\\r\\n/g')

git stash

# Fix headers
./releng/fix_header_copyright_and_authors.sh
# commit fixed headers (if any)
NBCHANGES=`git status --porcelain | wc -l`
if [ $NBCHANGES -ne 0 ]; then
  git add -A
  git commit -m "[RELENG] Fix headers"
fi

#commit new version in develop
git stash pop
git add -A
git commit -m "[RELENG] Prepare version $NEW_VERSION"

#merge in master, add tag
git checkout $MAIN_BRANCH
git merge --no-ff $DEV_BRANCH -m "merge branch '$DEV_BRANCH' for new version $NEW_VERSION"
git tag v$NEW_VERSION

sleep 1

#move to snapshot version in develop and push
git checkout $DEV_BRANCH
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
git add -A
git commit -m "[RELENG] Move to snapshot version"

sleep 0.2

#deploy from master
git checkout $MAIN_BRANCH
./releng/deploy.sh

#push if everything went fine
git push --tags
git push
git checkout $DEV_BRANCH
git push



TAG=v$NEW_VERSION
GENERATE_POST_BODY() {
  cat <<EOF
{
  "tag_name": "${TAG}",
  "name": "${TAG}",
  "body": "${RELEASE_BODY}",
  "draft": false,
  "prerelease": false
}
EOF
}

# create GitHub release using API
echo ""
echo "Create release"
API_RESPONSE=$(curl --silent -H "Authorization: token ${OAUTH_TOKEN}" --data "$(GENERATE_POST_BODY)" -X POST "https://api.github.com/repos/${REPO}/releases")

echo ""
echo "Extract Upload URL"
UPLOAD_URL=$(echo ${API_RESPONSE} | \
  sed '0,/{/s/{//' | rev | tac | sed '0,/}/s/}//' | rev | tac | \
  perl -0777 -pe 's/\[[^\]]*\]//igs' | \
  perl -0777 -pe 's/\{[^\{\}]*\}//igs' | \
  perl -0777 -pe 's/\{[^\{\}]*\}//igs' | \
  perl -0777 -pe 's/.*"upload_url": "([^,]+)",?.*/\1/g')

echo "   => ${UPLOAD_URL}"

# TODO add assets to upload
for FILE in spider-$NEW_VERSION.zip; do
  echo " * upload $FILE"
  curl -H "Authorization: token $OAUTH_TOKEN" -H "Content-Type: $(file -b --mime-type $FILE)" -X POST "$UPLOAD_URL?name=$(basename $FILE)" --upload-file $FILE > /dev/null
done
