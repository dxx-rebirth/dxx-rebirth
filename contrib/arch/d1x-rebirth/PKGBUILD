# Contributor: zico <zico@dxx-rebirth.com>
# Contributor: Dominic Radermacher <dominic.radermacher@gmail.com>
# Maintainer: SanskritFritz (gmail)

pkgname=d1x-rebirth
pkgver=20191121
pkgrel=2
pkgdesc="An enhanced engine to play with Descent1 data."
arch=('x86_64')
url="http://www.dxx-rebirth.com/"
license=('custom:D1x' 'LGPL' 'custom:as-is')
depends=('sdl' 'sdl_mixer' 'mesa' 'physfs')
makedepends=('scons' 'unzip')
install="$pkgname.install"
source=("https://www.dxx-rebirth.com/download/dxx/rebirth/dxx-rebirth_$pkgver-src.tar.xz"
        "http://www.dxx-rebirth.com/download/dxx/res/d1xr-hires.dxa"
        "http://www.dxx-rebirth.com/download/dxx/res/d1xr-sc55-music.dxa")
noextract=("d1xr-hires.dxa" "d1xr-sc55-music.dxa")
md5sums=('049cae350eab9da1c3bb2561cedadf27'
         '7d978ce3875995e54cbf426167e88b94'
         'bf54b45a648052b2531bbb95c001403b')

build() {
	cd "dxx-rebirth_$pkgver-src"
	scons sharepath="/usr/share/$pkgname" d1x=1
}

package() {
	mkdir -p "$pkgdir/usr/bin/"
	mkdir -p "$pkgdir/usr/share/$pkgname"
	mkdir -p "$pkgdir/usr/share/pixmaps"
	mkdir -p "$pkgdir/usr/share/applications"
	mkdir -p "$pkgdir/usr/share/licenses/$pkgname"

	install -m755 "dxx-rebirth_$pkgver-src/d1x-rebirth/d1x-rebirth" "$pkgdir/usr/bin/"

	install -m644 "dxx-rebirth_$pkgver-src/d1x-rebirth/d1x-rebirth.xpm" "$pkgdir/usr/share/pixmaps/"

	install -m644 "dxx-rebirth_$pkgver-src/d1x-rebirth/d1x-rebirth.desktop" "$pkgdir/usr/share/applications/"

	install -m644 "dxx-rebirth_$pkgver-src/COPYING.txt" "$pkgdir/usr/share/licenses/$pkgname/"
	install -m644 "d1xr-sc55-music.dxa" "$pkgdir/usr/share/$pkgname/"
	install -m644 "d1xr-hires.dxa" "$pkgdir/usr/share/$pkgname/"
} 
