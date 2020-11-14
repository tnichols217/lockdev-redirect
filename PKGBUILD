pkgname=lockdev-redirect
pkgver=$(sed -rn "s/.* version ([^']+)'\$/\1/p" lockdev-redirect)
pkgrel=1
pkgdesc="Tool for redirecting /var/lock to a user-writable directory"
url="https://www.github.com/M-Reimer/lockdev-redirect"
arch=('x86_64')
license=('LGPL3')

build() {
  cd "$startdir"
  make
}

package() {
  cd "$startdir"
  make DESTDIR=$pkgdir install
}
