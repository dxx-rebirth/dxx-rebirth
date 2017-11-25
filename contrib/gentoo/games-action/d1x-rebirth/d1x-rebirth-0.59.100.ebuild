# This is a meta-package to depend on games-action/dxx-rebirth-$PVR and
# require that it have the same USE flag settings as used on this
# package.  See the comments in that ebuild for an explanation.
#
# This meta-package is provided primarily to ease the upgrade path, so
# that users who previously installed a non-meta-package for d1x-rebirth
# or d2x-rebirth are offered an upgrade to the combined dxx-rebirth
# package.

EAPI=6

# Upstream's game engine number (Descent 1 vs Descent 2), not a release
# version number.
MY_ENGINE="${PN:1:1}"
DESCRIPTION="Descent Rebirth - enhanced Descent $MY_ENGINE engine"
HOMEPAGE="http://www.dxx-rebirth.com/"
SRC_URI=""

LICENSE="DXX-Rebirth GPL-3"
SLOT="0"
KEYWORDS="amd64 x86"
IUSE="debug editor ipv6 +opengl timidity tracker"

MY_USE_DEP="${IUSE//+} "
DEPEND="=games-action/dxx-rebirth-$PVR[${MY_USE_DEP// /=,}d${MY_ENGINE}x]"
RDEPEND="$DEPEND"
unset MY_USE_DEP
unset MY_ENGINE
