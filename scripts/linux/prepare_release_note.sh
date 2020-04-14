#!/bin/bash
set -x

RELEASE_NOTES_FILE="release_note.md"
CHANGELOG_FILE="release_notes.md"

# Prepare Release Notes in case this is a release.
rm -rf $RELEASE_NOTES_FILE

found=0;
while read -r line
do
  match=$(echo "$line" | sed -E -e 's/## Release version [0-9]+.[0-9]+.[0-9]+/1/')
  if [ $found = 0 ]
  then
	if [ "$match" = "1" ]
	then
		found=1
		echo "$line" >> $RELEASE_NOTES_FILE
	fi
  else
    if [ "$match" = "1" ]
	then
		break
	else
	  echo "$line" >> $RELEASE_NOTES_FILE
	fi
  fi
done < ${CHANGELOG_FILE}
