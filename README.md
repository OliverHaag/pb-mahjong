# pb-mahjong
Mahjong puzzle for PocketBook eReaders
## Building
1. Clone <https://github.com/pocketbook/SDK_6.3.0/tree/5.19> and set it up as described
2. Configure with `cmake -DCMAKE_TOOLCHAIN_FILE=$SDK_DIR/SDK-$ARCHITECTURE/share/cmake/arm_conf.cmake -DCMAKE_BUILD_TYPE=Release`, replacing `$SDK_DIR` and `$ARCHITECTURE` accordingly
3. Build with `make`
4. Connect reader via USB and mount the internal storage
5. Copy pb-mahjong.app to applications/pb-mahjong.app
6. Extend your language file (e. g. system/language/en.txt) for a nicer menu entry:
```
English
@Pb-mahjong=Mahjong
```
