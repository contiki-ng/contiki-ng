#!/bin/bash -e

# Copyright (c) 2022, Research Institutes of Sweden
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# How to use from $(CONTIKI_NG_TOP_DIR) on the develop branch:
#
# git checkout -b update-subrepo
# (cd <subrepo> && git checkout <branch/commit>)
# git add <subrepo>
# tools/git/update-submodule.sh <subrepo>
#
# Using the script inside other repositories can be done by disabling
# the verification of the Contiki-NG repository, to update mspsim in Cooja,
# run the script as:
#
# env NO_SAFETY=1 ../../tools/git/update-submodule.sh mspsim

error() {
    echo "$*"
    exit 1
}

[ ! -z "$NO_SAFETY" -o -f CODE_OF_CONDUCT.md ] || error "Run this script from top directory"
[ -z "$1" ] && error "No submodule path specified"

module=$1

[ -d "$module" ] || error "Not a directory: $module"
[ -f "$module/.git" ] || error "Not a git repository: $module"
git diff --staged --quiet -- "$module" && error "No commit staged"

old=$(git diff --staged -- "$module" | grep "\-Subproject commit" | awk '{print $3}')
new=$(git diff --staged -- "$module" | grep "+Subproject commit" | awk '{print $3}')

# We have identified the changes, create a commit message for them.
cd "$module"
commitfile=$(mktemp)
echo -e "Update $module submodule\n\nCommits:" >> "$commitfile"
git --no-pager log --oneline --no-decorate $old..$new >> "$commitfile"
# Filter out merge commits, they reference the Cooja PR number
# and create false references to the Contiki-NG PR/issue tracker.
perl -pi -e 's|^([^M]+)Merge pull request #(.+)\n||sg' "$commitfile"
cd -

# Commit message created, make the commit and remove the temporary file.
git commit -F "$commitfile"
rm $commitfile
