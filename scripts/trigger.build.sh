#!/bin/bash
curl -X POST \
    --header 'Content-Type: application/json' \
    --header "Authorization: apiToken $3" \
    -d "{\"branchName\": \"$2\"}" \
    "https://api.shippable.com/projects/$1/newBuild"