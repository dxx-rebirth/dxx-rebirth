#!/bin/bash

# This file is part of the DXX-Rebirth project.
#
# It is copyright by its individual contributors, as recorded in the
# project's Git history.  See COPYING.txt at the top level for license
# terms and a link to the Git history.

set -eu

cd "$(dirname "$0")"

parse_version_from_SConstruct() {
	local version
	unset descent_version_MAJOR descent_version_MINOR descent_version_MICRO
	for version in $( git show "$git_commit:SConstruct" | sed -n -e 's/^\s*VERSION_\(\w\+\)\s*=\s*\([0-9]\+\)/\1=\2/p' ); do
		printf -v "descent_version_${version%=*}" '%s' "${version#*=}"
	done
	[[ -n "$descent_version_MAJOR" ]]
	[[ -n "$descent_version_MINOR" ]]
	[[ -n "$descent_version_MICRO" ]]
}

set_git_commit_information() {
	declare -a git_commit_information
	git_commit_information=( $(git log -1 '--pretty=%ad %H' '--date=format:%Y%m%d' "$git_commit" ) )
	git_commit_date="${git_commit_information[0]}"
	git_commit_hash="${git_commit_information[1]}"
}

generate_snapshot_ebuild() {
	[[ -n "$git_commit_date" ]]
	[[ -n "$git_commit_hash" ]]
	local live_ebuild
	live_ebuild=contrib/gentoo/games-action/dxx-rebirth/dxx-rebirth-9999.ebuild
	git show "$git_commit:$live_ebuild" | \
		sed -e "s/MY_COMMIT=''/MY_COMMIT='$git_commit_hash'/" \
		> "$(dirname "$live_ebuild")/dxx-rebirth-${descent_version_MAJOR}.${descent_version_MINOR}.${descent_version_MICRO}_pre${git_commit_date}.ebuild"
}

if [[ "$#" = 0 ]]; then
	git_commit=HEAD
else
	git_commit="$1"
fi

parse_version_from_SConstruct
set_git_commit_information
generate_snapshot_ebuild
