# Maintainer: Azteriisk <https://github.com/Azteriisk>
pkgname=omarchy-plugin-window-minimize-git
pkgver=1.1.0.r0.g515fec4
pkgrel=1
pkgdesc="Window control, minimization, CSD titlebar interceptor hook, and status badge for Omarchy and Hyprland"
arch=('x86_64')
url="https://github.com/Azteriisk/omarchy-window-minimize"
license=('MIT')
depends=('hyprland' 'quickshell')
makedepends=('git' 'gcc' 'make' 'pkgconf' 'hyprland-headers')
provides=('omarchy-plugin-window-minimize')
conflicts=('omarchy-plugin-window-minimize')
source=("git+https://github.com/Azteriisk/omarchy-window-minimize.git")
md5sums=('SKIP')

pkgver() {
  cd "$srcdir/omarchy-window-minimize"
  printf "1.1.0.r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/omarchy-window-minimize/hyprland-plugin"
  make
}

package() {
  cd "$srcdir/omarchy-window-minimize"

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
}
