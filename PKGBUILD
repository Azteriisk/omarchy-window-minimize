# Maintainer: Azteriisk <https://github.com/Azteriisk>
pkgname=omarchy-plugin-window-minimize-git
pkgver=1.1.0
pkgrel=1
pkgdesc="Window control, minimization, CSD titlebar interceptor hook, and status badge for Omarchy and Hyprland"
arch=('x86_64')
url="https://github.com/Azteriisk/omarchy-window-minimize"
license=('MIT')
depends=('hyprland' 'quickshell')
makedepends=('git' 'gcc' 'make' 'pkgconf' 'hyprland-headers')
provides=('omarchy-plugin-window-minimize')
conflicts=('omarchy-plugin-window-minimize')
_commit="1a88115f8a0089ac0f6509447d0062a2a787c222"
source=("${pkgname}::git+https://github.com/Azteriisk/omarchy-window-minimize.git#commit=${_commit}")
sha256sums=('SKIP')

pkgver() {
  cd "$srcdir/${pkgname}"
  if tag=$(git describe --long --tags --abbrev=7 2>/dev/null); then
    echo "$tag" | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
  else
    printf "1.1.0.r%s.%s\n" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
  fi
}

build() {
  cd "$srcdir/${pkgname}/hyprland-plugin"
  make
}

package() {
  cd "$srcdir/${pkgname}"

  # 1. Install CLI helper to /usr/bin
  install -Dm755 scripts/omarchy-minimize "$pkgdir/usr/bin/omarchy-minimize"

  # 2. Install Omarchy plugin files
  install -d "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize"
  install -Dm644 manifest.json "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/manifest.json"
  install -Dm644 BarWidget.qml "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/BarWidget.qml"
  install -Dm644 Panel.qml "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/Panel.qml"
  install -Dm644 Service.qml "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/Service.qml"
  install -Dm644 README.md "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/README.md"
  install -Dm755 install.sh "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/install.sh"
  install -Dm755 uninstall.sh "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/uninstall.sh"

  # 3. Install compiled Hyprland C++ plugin hook
  install -Dm755 hyprland-plugin/minimize-hook.so "$pkgdir/usr/lib/hyprland/minimize-hook.so"
  install -d "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/hyprland-plugin"
  install -Dm755 hyprland-plugin/minimize-hook.so "$pkgdir/usr/share/omarchy/plugins/azterisk.minimize/hyprland-plugin/minimize-hook.so"

  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
