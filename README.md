# pb-mahjong
Mahjong Solitaire for PocketBook eReaders
## Building
1. Clone <https://github.com/pocketbook/SDK_6.3.0/tree/5.19> and set it up as described
2. Configure with `cmake -DCMAKE_TOOLCHAIN_FILE=$SDK_ROOT_DIR/SDK-B288/share/cmake/arm_conf.cmake -DCMAKE_BUILD_TYPE=Release`, replacing `$SDK_ROOT_DIR` accordingly
3. Build with `make`
4. Deploy to install folder with `make install`
## Installation
1. Connect reader via USB and mount the internal storage
2. Copy the applications and system folder from the install directory (or package) to the internal storage
3. Extend your language file (e. g. system/language/en.txt) for a nicer menu entry:
```
English
@Pb-mahjong=Mahjong Solitaire
```
