#!/bin/bash

if [[ ! -e .git/hooks/pre-commit ]]; then
    # install pre-commit hook for this (presumably) new clone
    cp githooks/pre-commit .git/hooks
else
    diff githooks/pre-commit .git/hooks/pre-commit > /dev/null 2>&1
    if [[ "$?" != "0" ]]; then
        echo "ERROR there are differences between [githooks/pre-commit] and [.git/hooks/pre-commit], please investigate before proceeding" >&2
        exit 1
    fi
fi

echo "CURRENT_TIME $(date +'%Y%m%d-%H%M%S')"
echo "STABLE_GIT_COMMIT $(git rev-parse HEAD)"
echo "STABLE_USER_NAME ${USER}"
exit 0
