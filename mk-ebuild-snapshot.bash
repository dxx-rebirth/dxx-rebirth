#!/bin/bash

# This file is part of the DXX-Rebirth project.
#
# It is copyright by its individual contributors, as recorded in the
# project's Git history.  See COPYING.txt at the top level for license
# terms and a link to the Git history.

set -eu
set -o pipefail

cd "$(dirname "$0")"

parse_version_from_SConstruct() {
	local version
	unset descent_version_MAJOR descent_version_MINOR descent_version_MICRO
	# Read the blob SConstruct from the chosen $git_commit.  For lines that
	# assign a decimal number to a variable matching VERSION_\w\+, print a=b
	# where a is the suffix of the variable and b is the decimal value.
	# Discard all other lines.  Printed lines are iterated to assign shell
	# variables of matching names.
	for version in $( git show "$git_commit:SConstruct" | sed -n -e 's/^\s*VERSION_\(\w\+\)\s*=\s*\([0-9]\+\)/\1=\2/p' ); do
		printf -v "descent_version_${version%=*}" '%s' "${version#*=}"
	done
	# Test that the three expected values were assigned.
	[[ -n "$descent_version_MAJOR" ]]
	[[ -n "$descent_version_MINOR" ]]
	[[ -n "$descent_version_MICRO" ]]
}

set_git_commit_information() {
	declare -a git_commit_information
	# Retrieve the date and hash of the chosen $git_commit.  Store them to
	# shell variables for later use.
	git_commit_information=( $(git log -1 '--pretty=%ad %H' '--date=format:%Y%m%d' "$git_commit" ) )
	git_commit_date="${git_commit_information[0]}"
	git_commit_hash="${git_commit_information[1]}"
}

generate_snapshot_ebuild() {
	[[ -n "$git_commit_date" ]]
	[[ -n "$git_commit_hash" ]]
	local live_ebuild snapshot_ebuild
	live_ebuild=contrib/gentoo/games-action/dxx-rebirth/dxx-rebirth-9999.ebuild
	snapshot_ebuild="${live_ebuild%/*}/dxx-rebirth-${descent_version_MAJOR}.${descent_version_MINOR}.${descent_version_MICRO}_pre${git_commit_date}.ebuild"
	# Read the blob of the live ebuild from the chosen $git_commit.  Pipe the
	# blob to a sed that inserts the commit hash into the assignment to
	# `MY_COMMIT`.  In dry-run mode, stdout remains directed to where the
	# caller set it.  In non-dry-run mode, stdout is redirected to the non-live
	# file named by $snapshot_ebuild.
	git show "$git_commit:$live_ebuild" | \
		(
			[[ -z "$dry" ]] && exec > "$snapshot_ebuild"
			exec sed -e "s/MY_COMMIT=''/MY_COMMIT='$git_commit_hash'/"
		)
	# If the caller requested to add the newly created ebuild to git, do so.
	if [[ -z "$dry" && -n "$add_snapshot" ]]; then
		git add "$snapshot_ebuild"
	fi
	# If the caller requested to remove all older snapshot ebuilds from git, do
	# so.
	if [[ -n "$remove_snapshot" ]]; then
		# List snapshot ebuilds, but tell git not to show the just-created
		# snapshot ebuild.  This prevents the `git rm` from removing the
		# just-created snapshot ebuild.
		git ls-files -z 'contrib/gentoo/games-action/dxx-rebirth/dxx-rebirth-*.*_pre*.ebuild' ':!'"$snapshot_ebuild" | xargs -0 git rm
	fi
}

add_snapshot=
dry=
remove_snapshot=

while getopts anr OPTNAME; do
	case "$OPTNAME" in
		a)
			add_snapshot=1
			;;
		n)
			dry=1
			;;
		r)
			remove_snapshot=1
			;;
	esac
done

# If not otherwise specified, read the most recent commit.
if [[ "$#" -lt $OPTIND ]]; then
	git_commit=HEAD
else
	git_commit="$1"
fi

# In dry-run mode, the snapshot ebuild will not be created, so it cannot be
# added to git.
if [[ -n "$dry" ]]; then
	add_snapshot=
fi

parse_version_from_SConstruct
set_git_commit_information
generate_snapshot_ebuild
