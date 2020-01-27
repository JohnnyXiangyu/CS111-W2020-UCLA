#! /bin/bash

git filter-branch --force --index-filter \
  'git rm --cached --ignore-unmatch Lab_0/README' \
  --prune-empty --tag-name-filter cat -- --all