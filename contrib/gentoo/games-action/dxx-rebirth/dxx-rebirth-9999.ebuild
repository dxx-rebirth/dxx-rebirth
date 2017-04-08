# After release 0.58.1 and before beta release 0.59.100, upstream
# combined the source for the Descent 1 and Descent 2 engines into a
# single tree.  The combined tree builds common code into a static
# library, which is linked into both games, but not installed.  Users
# who want both engines benefit from this because they can build the
# common code once, rather than once per game.  This ebuild supports
# building one or both engines, depending on USE=d1x and USE=d2x.

EAPI=6

inherit eutils scons-utils toolchain-funcs
if [[ "$PV" = 9999 ]]; then
	inherit git-r3
	EGIT_REPO_URI="https://github.com/dxx-rebirth/dxx-rebirth"
else
	SRC_URI="https://github.com/dxx-rebirth/dxx-rebirth/archive/$PV.zip -> $PN-$PVR.zip"

	# Restriction only for use in private overlays.  When this is added to a
	# public tree, post the sources to a mirror and remove this restriction.
	RESTRICT='mirror'
fi

DESCRIPTION="Descent Rebirth - enhanced Descent engines"
HOMEPAGE="http://www.dxx-rebirth.com/"

LICENSE="D1X GPL-3"
SLOT="0"
# Other architectures are reported to work, but not tested regularly by
# the core team.
#
# Raspberry Pi support is tested by an outside contributor, and his
# fixes are merged into the main source by upstream.
#
# Cross-compilation to Windows is also supported.
KEYWORDS="amd64 x86"
# Default to building both game engines.  The total size is relatively
# small.
IUSE="+d1x +d2x debug editor ipv6 +opengl timidity tracker"

DEPEND="opengl? ( virtual/opengl virtual/glu )
	dev-games/physfs[hog,mvl,zip]
	media-libs/libsdl[sound,opengl?,video]
	media-libs/sdl-mixer[timidity?]"
# The build process does not use the game data, nor change how the game
# is built based on what game data will be used.  At startup, the game
# will search for both types of game data and use what it finds.  Users
# can switch between shareware/retail data at any time by
# adding/removing the appropriate data packages.  A rebuild is _not_
# required after swapping the data files.
RDEPEND="${DEPEND}
	d1x? (
		|| (
			games-action/descent1-data
			games-action/descent1-demodata
		)
	)
	d2x? (
		|| (
			games-action/descent2-data
			games-action/descent2-demodata
		)
	)
"

# This ebuild builds d1x-rebirth, d2x-rebirth, or both.  Building none
# would mean this ebuild installs zero files.
REQUIRED_USE="|| ( d1x d2x )"

dxx_scons() {
	# Always build profile `m`.  If use editor, also build profile `e`.
	# Set most variables in the default anonymous profile.  Only
	# `builddir` and `editor` are set in the named profiles, since those
	# must be different between the two builds.
	local scons_build_profile=m mysconsargs=(
		sdlmixer=1
		verbosebuild=1
		debug=$(usex debug 1 0)
		ipv6=$(usex ipv6 1 0)
		opengl=$(usex opengl 1 0)
		use_tracker=$(usex tracker 1 0)
		prefix=/usr
		m_builddir=build/main/
		m_editor=0
	)
	if use editor; then
		scons_build_profile+=+e
		mysconsargs+=( 
			e_builddir=build/editor/
			e_editor=1
		)
	fi
	# Add sharepath and enable build of selected games.  The trailing
	# comma after `$scons_build_profile` is required to cause scons to
	# search the anonymous profile.  If omitted, only settings from the
	# named profile would be used.
	use d1x && mysconsargs+=( d1x_sharepath="/usr/share/games/d1x" d1x="$scons_build_profile," )
	use d2x && mysconsargs+=( d2x_sharepath="/usr/share/games/d2x" d2x="$scons_build_profile," )
	escons "${mysconsargs[@]}" "$@"
}

src_compile() {
	tc-export CXX PKG_CONFIG
	dxx_scons register_install_target=0 build
}

src_install() {
	dxx_scons register_compile_target=0 register_install_target=1 DESTDIR="$D" "$D"
}
