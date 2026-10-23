# CardputerZero AppStore

Device-side APPLaunch application for browsing CardputerZero registries and installing apps from online registry metadata.

AppStore installs only Debian `.deb` packages whose registry review status is
`approved`. The backend downloads the `.deb` to the local cache first, verifies
the registry `download.md5`, then installs the local file with the Raspberry Pi
OS/Debian package tools. Installed apps can be launched through their APPLaunch
`.desktop` file and removed with the package manager.

## Runtime

The LVGL UI calls the native backend compiled into the AppStore executable for
registry, install, upgrade, and uninstall operations. No Python runtime or
backend service is required.

Useful backend environment variables:

```bash
M5APPSTORE_STATE_DIR=/root/.local/share/cardputerzero-appstore
M5APPSTORE_APP_ROOT=/usr/share/APPLaunch
```

Default registry:

```text
https://cardputerzero.github.io/generated/registry.json
```

CN region registry:

```text
https://cardputer-zero-repo.oss-cn-shenzhen.aliyuncs.com/packages/cn/registry.json
```

AppStore performs a fresh registry sync when it starts. Registry HTTP requests
send `Cache-Control: no-cache` and a timestamp query parameter so GitHub Pages
or intermediate CDN cache does not hide newly published entries. If a registry
cannot be loaded, the UI shows the load failure; if a previous catalog exists,
AppStore keeps showing the cached app list and marks the registry as cached.

Registry metadata and artwork use libhv asynchronous networking. Artwork is
downloaded in bounded batches of six requests so synchronization remains
responsive without exhausting device sockets. When an HTTP proxy is configured,
AppStore uses libhv's asynchronous thread pool with curl because libhv's native
async client does not implement HTTPS CONNECT proxy tunneling. Debian package
downloads continue to use curl so interrupted downloads can be resumed safely.

## Backend Commands

```bash
./M5CardputerZero-AppStore --summary
./M5CardputerZero-AppStore --registries
./M5CardputerZero-AppStore --regions
./M5CardputerZero-AppStore --set-region CN
./M5CardputerZero-AppStore --sync
./M5CardputerZero-AppStore --plan <app-id>
./M5CardputerZero-AppStore --install <app-id>
./M5CardputerZero-AppStore --upgrade <app-id>
./M5CardputerZero-AppStore --uninstall <app-id>
./M5CardputerZero-AppStore --prepare-package install <app-id>
./M5CardputerZero-AppStore --finalize-package install <app-id> <transaction-id>
```

`<app-id>` accepts either the app UUID or its `share_code` from registry metadata.

The LVGL application uses a three-stage package transaction. The unprivileged
`--prepare-package` stage downloads and verifies the package and records a
pending transaction. The UI then runs the emitted package helper command with
privileges. Finally, `--finalize-package` checks the real package state through
`dpkg-query` before updating AppStore's installed-app records. The original
`--install`, `--reinstall`, `--upgrade`, and `--uninstall` commands remain
available and use the same verification and record-update behavior.

Pending transactions are stored in the AppStore state directory using atomic
file replacement. AppStore reconciles them against the dpkg database on backend
startup and during summary refresh, so an interrupted finalize stage can be
completed after a restart. An interrupted transaction that provably left the
installed package state unchanged is cleared so it cannot block later
operations. A transaction remains pending when its outcome cannot be determined;
its diagnostic is reported again on the next reconciliation.

Package commands produce tab-separated, machine-readable status records.
`PACKAGE_JOB` describes the privileged step, `PACKAGE_RESULT` is the verified
terminal result, `WARNING` reports a non-fatal issue such as desktop-entry
repair, and `ERROR` reports a failed operation. Consumers must determine success
from the process exit code and `PACKAGE_RESULT`, not by searching free-form
package-manager output for words such as `ERROR`.

Desktop-entry repair is best effort. Failure to find or rewrite an APPLaunch
desktop entry is reported as `WARNING` after a successful package operation and
does not change the verified package result.

Settings offers three server modes for the same catalog: **Auto**, **Default**
(GitHub), and **CN** (Alibaba Cloud OSS). `Z/C` (left/right) changes the mode;
`4` goes back and `8` syncs after the pending selection is applied.
The screen retains its dark styling, segmented selector, and read-only active
server information. Auto tries GitHub first and falls back to OSS if it cannot
retrieve a usable catalog; explicit Default and CN choices stay pinned to that
server. Cached data does not count as a successful connectivity check. Only one
server supplies the catalog at a time; custom registries and their editor are
not supported. Existing saved custom sources are normalized to the selected
built-in server, and the mode and effective server are persisted across restarts.

On the home screen, use `Z/C` (left/right) to change category and `F/X` (up/down)
to select an app. Categories include All, Installed, and the registry's app
categories; there is no Recommended category. Installed filters catalog apps by
their actual package installation state and refreshes after package operations.
The Installed category is hidden when empty; removing its last app returns the
selection to All. Long names bounce like the selected APPLaunch title: they
pause for two seconds, scroll horizontally at 40 pixels per second to the end,
pause again, and scroll back. Status-bar updates keep the current reading
position, and labels never scroll vertically. In search results only the
selected name scrolls; the others end with an ellipsis. The home screen keeps
its original layout: the app icon on the left, name and metadata on the right,
and plain-text number-key shortcuts along the bottom. Permission icons appear
only on the app details page.
Press `4` for settings, `5` for share code, `6` for search, `7` for
install/reinstall, and `8` or Enter for details. When an installed app has an
available update, the list indicates it and shortcut `7` becomes **update**,
opening the upgrade confirmation directly without visiting details.
AppStore syncs the registry
automatically when it starts. The startup screen names the current step
(connecting, updating the catalog, loading artwork, preparing apps) above a
progress bar with the registry's host, and `Tab` opens settings. If no
registry can be reached, an alert offers Exit or Settings; `Z/C` moves between
them and Enter confirms.

On the home and startup screens, a short `Esc` press opens the centered
"Hold ESC for 3s to exit." hint. Press `Esc` again or another key to dismiss
the hint without activating the page underneath. On other screens, short
`Esc` keeps its back/cancel behavior. Hold `Esc` for three seconds to exit
from any screen, including while the hint is open.

Keys play the launcher's system sounds: arrows and `F/X/Z/C` play the switch
sound on every step, including repeats while held, and Enter, Tab, a short
`Esc`, and shortcut keys play the enter sound. Typing in the share-code and
search fields is silent. A held key repeats no faster than the screen redraws,
and its sound keeps playing while the redraw waits, so the view keeps up and
stops when the key is released.

The detail page shows the app name, color-coded version, maintainer, size,
and update fields, the install state, and the complete description. Screenshots
follow the description in vertical rows, one image per row at nearly full screen
width (300 pixels), with aspect ratio preserved. Package and category information
follow the screenshots, then a matching PERMISSIONS caption and permission icons.
`F/X` scrolls the whole content area with a short glide, 24 pixels per press;
`Z/C` and `K/L` page up and down, and `N/M` jump to the start and end. Holding
a key scrolls continuously. The system
bar and bottom actions stay fixed, and status messages appear just above the
actions at any scroll position. Descriptions wrap without a height limit or
automatic scrolling. Apps without screenshots have no screenshot area. Detail actions are `4` back,
`5` screens (the full-screen viewer), `6` install/reinstall, `7` update, and
`8` remove, shown when applicable. After a successful removal the Store returns
to the home list.
The existing full-screen screenshot viewer remains accessible through `5`, where
`Z/C` cycles images. On the details page itself those keys page through the
content.

Registry entries must provide a Debian package download:

```yaml
download:
  type: deb
  package: lofibox
  url: https://github.com/CardputerZero/packages/raw/main/pool/main/lofibox/lofibox_0.2.0-1~lofibox23_arm64.deb
  md5: 6a9798c1208e6d8bf5a9f28914160483
```

## Share Code Flow

On the AppStore home screen, press `5` to open the share code input page. Type
the code from CardputerZero Hub, then press `Enter` to jump to that app's detail
page. From there, use the normal install/reinstall flow.

## Fonts

The UI uses DejaVu Sans as its primary western font. Its glyph fallback is
Noto Sans CJK, which covers Chinese, Japanese, and Korean text while DejaVu
continues to render Latin, Greek, Cyrillic, and other western scripts. The
corresponding serif pair is also initialized for consumers that request it:
DejaVu Serif with Noto Serif CJK.

The default device paths are:

```text
/usr/share/APPLaunch/share/font/DejaVuSans.ttf
/usr/share/APPLaunch/share/font/DejaVuSerif.ttf
/usr/share/APPLaunch/share/font/NotoSansCJK-Regular.ttc
/usr/share/APPLaunch/share/font/NotoSerifCJK-Regular.ttc
```

Override paths when running on a system with a different font layout using
`M5APPSTORE_LATIN_SANS_FONT`, `M5APPSTORE_LATIN_SERIF_FONT`,
`M5APPSTORE_CJK_SANS_FONT`, and `M5APPSTORE_CJK_SERIF_FONT`.
