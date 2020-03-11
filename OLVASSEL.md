USBImager
=========

<img src="https://gitlab.com/bztsrc/usbimager/raw/master/src/misc/icon32.png">
Az [USBImager](https://gitlab.com/bztsrc/usbimager) egy igen igen faék egyszerűségű ablakos alkalmazás, amivel
tömörített lemezképeket lehet USB meghajtókra írni és lementeni. Elérhető Windows, MaxOSX és Linux rendszereken. A felülete
annyira egyszerű, amennyire csak lehetséges, teljesen salang mentes.

| Platform     | Felület      | Leírás                       |
|--------------|--------------|------------------------------|
| Windows      | [GDI](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager-i686-win-gdi.zip) | natív interfész |
| MacOSX       | [Cocoa](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager-intel-macosx-cocoa.zip) | natív interfész |
| Ubuntu LTS   | [X11](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager_0.0.1_amd64.deb) | ua. mint a Linux PC X11 verzió, csak .deb formátumban |
| Linux PC     | [X11](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager-x86_64-linux-x11.zip)<br>[GTK+](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager-x86_64-linux-gtk.zip)  | javalott<br>kompatíbilitás (van egy kis biztonsági kockázat a nyers lemezelérések engedélyezésekor) |
| Raspbian     | [X11](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager_0.0.1_armv7l.deb) | ua. mint a Raspberry Pi X11 verzió, csak .deb formátumban |
| Raspberry Pi | [X11](https://gitlab.com/bztsrc/usbimager/raw/master/usbimager-armv7l-linux-x11.zip) | natív interfész |

Telepítés
---------

1. töltsd le a megfelelő `usbimager-*.zip` csomagolt fájlt a gépedhez (kevesebb, mint 192 Kilobájt mind)
2. csomagold ki: `C:\Program Files` (Windows), `/Applications` (MacOSX) vagy `/usr` (Linux) mappába
3. Használd!

A csomagban lévő futtathatót egyből használhatod, nem kell telepíteni, és a többi fájl is csak azért van, hogy beillessze az asztali
környezetedbe (ikonok és hasonlók). Automatikusan érzékeli az operációs rendszeredben beállított nyelvet, és ha talál hozzá szótárat, akkor
a saját nyelveden köszönt (természetesen tud magyarul).

Ubuntu LTS és Raspbian rendszeren letöltheted a deb csomagot is, amit aztán a `sudo dpkg -i usbimager-*.deb` paranccsal telepíthetsz.

Fícsörök
--------

- Nyílt Forráskódú és MIT licenszű
- Szállítható futtatható, nem kell telepíteni, csak csomagold ki
- Kicsi. Nagyon kicsi, csupán pár kilobájt, mégsincs függősége
- Az etch*r-el ellentétben nem kell aggódnod a privát szférád vagy a reklámok miatt, garantáltan GDPR kompatíbilis
- Minimalista, többnyelvű, natív interfész minden platformon
- Igyekszik hülyebiztos lenni, és nem engedi véletlenül felülírni a rendszerlemezed
- Szinkronizáltan ír, azaz minden adat garantáltan a lemezen lesz, amikorra a csík a végére ér
- Képes ellenőrizni az írást visszaolvasással és az eredeti lemezképpel való összevetéssel
- Képes nyers lemezképeket olvasni: .img, .bin, .raw, .iso, .dd, stb.
- Képes futási időben kitömöríteni: .gz, .bz2, .xz
- Képes csomagolt fájlokat kitömöríteni: .zip (PKZIP és ZIP64) (*)
- Képes lemezképeket készíteni, nyers és bzip2 tömörített formátumban
- Képes mikrokontrollerek számára soros vonalon leküldeni a lemezképeket

(* - a több fájlt is tartalmazó csomagolt fájlok esetén a csomagolt fájl legelső fájlját használja bemenetnek)

Összehasonlítás
---------------

| Leírás                          | balenaEtcher  | WIN32 Disk Imager | USBImager |
|---------------------------------|---------------|-------------------|-----------|
| Többplatformos                  | ✔             | ✗                 | ✔         |
| Minimum Windows                 | Win 7         | Win XP            | Win XP    |
| Minimum MacOSX                  | ?             | ✗                 | 10.14     |
| Elérhető Raspbian-on            | ✗             | ✗                 | ✔         |
| Program mérete (1)              | 130 Mb        | ✗                 | 256 Kb    |
| Függőségek                      | sok, ~300 Mb  | Qt, ~8 Mb         | ✗ nincs   |
| Kémkedés- és reklámmentes       | ✗             | ✔                 | ✔         |
| Natív interfész                 | ✗             | ✗                 | ✔         |
| Garantált kiírás (2)            | ✗             | ✗                 | ✔         |
| Kiírt adatok ellenőrzése        | ✔             | ✗                 | ✔         |
| Tömörített lemezképek           | ✔             | ✗                 | ✔         |
| Nyers kiírási idő (3)           | 23:16         | 23:28             | 24:05     |
| Tömörített kiírás (3)           | 01:12:51      | ✗                 | 30:47     |

(1) - a szállítható futtatható mérete Windowson. A WIN32 Disk Imagerhez nem tudtam letölteni előre lefordított hivatalos csomagokat, csak forrást.

(2) - USBImager csak nem-bufferelt IO utasításokat használ, hogy a fizikális lemezreírás biztos legyen

(3) - a méréseket @CaptainMidnight végezte Windows 10 Pro alatt egy SanDisk Ulta 32GB A1 kártyával. A nyers lemezkép mérete 31,166,976 Kb volt, míg a bzip2 tömörítetté 1,887,044 Kb. WIN32 Disk Imager nem kezel tömörített lemezképeket, így a végeredménye nem volt bebootolható.

Képernyőképek
-------------

<img src="https://gitlab.com/bztsrc/usbimager/raw/master/usbimager.png">

Használat
---------

Ha nem tudod írni a céleszközt (folyton "hozzáférés megtagadva" hibaüzenetet kapsz), akkor:


__Windows__: jobbklikk az usbimager.exe-n és használd a "Futtatás rendszergazdaként" opciót.

__MacOSX__: menj a rendszerbeállításokhoz "System Preferences", aztán "Security & Privacy" és "Privacy". Add hozzá az USBImager-t a
"Full Disk Access" listához. Alternatívaként indíthatod Terminálból a *sudo /Applications/USBImager.app/Contents/Mac/usbimager* paranccsal.

__Linux__: ez valószínűleg nem fordulhat elő, mivel az USBImager setgid bittel érkezik. Ha mégsem, akkor a *sudo chgrp disk usbimager && sudo chmod g+s usbimager*
parancs beállítja. Alternatívaként add hozzá a felhasználódat a "disk" csoportokhoz (az "ls -la /dev|grep -e ^b" parancs kiírja, melyik csoportban vannak az oprendszered
alatt a lemezeszközök). __Elvileg nincs szükség__ a *sudo /usr/bin/usbimager*-re, csak győzödj meg róla, hogy a felhasználódnak van írási
hozzáférése az eszközökhöz, ez a Legalacsonyabb Privilégium Elve (Principle of Least Privilege).

### Interfész

1. sor: lemezkép fájl
2. sor: műveletek, írás és olvasás ebben a sorrendben
3. sor: eszköz kiválasztás
4. sor: opciók: írás ellenőrzése, kimenet tömörítése és buffer méret rendre

Az X11 esetén mindent a nulláról írtam meg, hogy elkerüljem a függőségeket. A kattintás és a billentyűnavigáció a megszokott: <kbd>Tab</kbd>
és <kbd>Shift</kbd> + <kbd>Tab</kbd> váltogat a mezők között, <kbd>Enter</kbd> a kiválasztás. Plusz a fájl tallózásakor a <kbd>Bal nyíl</kbd>
/ <kbd>BackSpace</kbd> (törlés) feljebb lép egy könyvtárral, a <kbd>Jobbra nyíl</kbd> / <kbd>Enter</kbd> pedig beljebb megy (vagy kiválaszt, ha
az aktuális elem nem könyvtár). A sorrendezést a <kbd>Shift</kbd> + <kbd>Fel nyíl</kbd> / <kbd>Le nyíl</kbd> kombinációkkal tudod változtatni.
A "Legutóbb használt" fájlok listája szintén támogatott (a freedesktop.org féle [Desktop Bookmarks](https://freedesktop.org/wiki/Specifications/desktop-bookmark-spec/)
szabvány alapján).

### Lemezkép kiírása eszközre

1. kattints a "..." gombra az első sorban és válassz lemezkép fájlt
2. kattints a harmadik sorra és válassz eszközt
3. kattints a második sor első gombjára (Kiír)

Ennél a műveletnél a fájl formátuma és a tömörítése automatikusan detektálásra kerül. Kérlek vedd figyelembe, hogy a hátralévő idő becsült.
Bizonyos tömörített fájlok nem tárolják a kicsomagolt méretet, ezeknél a státuszban "x MiB ezidáig" szerepel. A hátralévő idejük nem lesz pontos,
csak egy közelítés a becslésre a tömörített pozíció / tömörített méret arányában (magyarán a mértékegysége sacc/kb).

Ha az "Ellenőrzés" be van pipálva, akkor minden kiírt blokkot visszaolvas, és összehasonlít az eredeti lemezképpel.

Az utolsó opció, a legördülő állítja, hogy mekkora legyen a buffer. Ekkora adagokban fogja a lemezképet kezeli. Vedd figyelembe, hogy a
tényleges memóriaigény ennek háromszorosa, mivel van egy buffer a tömörített adatoknak, egy a kicsomagolt adatoknak, és egy az ellenőrzésre
visszaolvasott adatoknak.

### Lemezkép készítése eszközről

1. kattints a harmadik sorra és válassz eszközt
2. kattints a második sor második gombjára (Beolvas)
3. a lemezkép az Asztalodon fog létrejönni, a fájlnév pedig megjelenik az első sorban

A generált lemezkép neve "usbimager-(dátum)T(idő).dd" lesz, a pontos időből számítva. Ha a "Tömörítés" be volt pipálva, akkor a fájlnév
végére egy ".bz2" kiterszejtést biggyeszt, és a lemezkép tartalma bzip2 tömörített lesz. Ennek sokkal jobb a tömörítési aránya, mint
a gzipé. Nyers lemezképek esetén a hátralévő idő pontos, tömörítés esetén nagyban ingadozik a tömörítés műveletigényétől, ami meg az
adatok függvénye, ezért csak egy becslés.

Megjegyzés: Linuxon ha nincs ~/Desktop (Asztal), akkor a ~/Downloads (Letöltések) mappát használja. Ha az sincs, akkor a lemezkép a
home mappába lesz lementve. A többi platformon mindig van Asztal, ha mégse találná, akkor az aktuális könyvtárba ment.

### Haladó funkciók

| Kapcsoló  | Leírás                  |
|-----------|-------------------------|
| -v/-vv    | Részletes kimenet       |
| -Lxx      | Nyelvkód kikényszerítés |
| -1..9     | Buffer méret beállítása |
| -s/-S     | Soros portok használata |
| --version | Kiírja a verziót        |

Windows felhasználóknak: jobb-klikk az usbimager.exe-n, majd választd a "Parancsikon létrehozása" menüt. Aztán jobb-klikk az újonnan
létrejött ".lnk" fájlra, és válaszd a "Tulajdonságok" menüt. A "Parancsikon" fülön, a "Cél" mezőben tudod hozzáadni a kapcsolókat.
Ugyancsak itt, a "Biztonság" fülön be lehet állítani, hogy rendszergazdaként futtassa, ha problémáid lennének a direkt lemezhozzáférésekkel.

A kapcsolókat külön-külön (pl. "usbimager -v -s -2") vagy egyben ("usbimager -2vs") is megadhatod, a sorrend nem számít. Azon kapcsolók
közül, amik ugyanazt állítják, csak a legutolsót veszi figyelembe (pl "-124" ugyanaz, mint a "-4").

A '-v' és '-vv' kapcsolók szószátyárrá teszik az USBImager-t, és mindenféle részletes infókat fog kiírni a konzolra. Ez utóbbi a szabvány
kimenet (stdout) Linux és MacOSX alatt (szóval terminálból használd), míg Windowson egy külön ablakot nyit az üzeneteknek.

A '-Lxx' kapcsoló utolsó két karaktere "en", "es", "de", "fr" stb. lehet. Ez a kapcsoló felülbírája a detektált nyelvet, és a megadott
szótárat használja. Ha nincs ilyen szótár, akkor angol nyelvre vált.

A szám kapcsolók a buffer méretét állítják a kettő hatványa Megabájtra (0 = 1M, 1 = 2M, 2 = 4M, 3 = 8M, 4 = 16M, ... 9 = 512M). Ha nincs
megadva, a buffer méret alapértelmezetten 1 Megabájt.

Ha az USBImager-t '-s' (kisbetű) kapcsolóval indítod, akkor a soros portra is engedi küldeni a lemezképeket. Ehhez szükséges, hogy a
felhasználó az "uucp" illetve a "dialout" csoport tagja legyen (disztribúciónként eltérő, használd a "ls -la /dev|grep tty" parancsot).
Ez esetben a kliensen:
1. tetszőleges ideig várakozni kell az első bájtra, majd lementeni azt a bufferbe
2. ezután a többi bájtot időtúllépéssel (mondjuk 250 mszekkel vagy 500 mszekkel) kell olvasni, és lerakni azokat is a bufferbe
3. ha az időtúllépés bekövetkezik, a lemezkép megérkezett.

A '-S' (nagybetű) kapcsoló hasonló, de ekkor az USBImager a [raspbootin](https://github.com/bztsrc/raspi3-tutorial/tree/master/14_raspbootin64)
kézfogást fogja alkalmazni a soros vonalon:
1. USBImager várakozik a kliensre
2. a kliens három bájtot küld, '\003\003\003' (3-szor <kbd>Ctrl</kbd>+<kbd>C</kbd>)
3. USBImager leküldi a lemezkép méretét, 4 bájt kicsi-elöl (little-endian) formátumban (méret = 4.bájt * 16777216 + 3.bájt * 65536 + 2.bájt * 256 + 1.bájt)
4. a kliens két bájttal válaszol, vagy 'OK' vagy 'SE' (size error, méret hiba)
5. ha a válasz OK volt, akkor az USBImager leküldi a lemezképet, méret bájtnyit
6. amikor a kliens fogadta a méretedik bájtot, a lemezkép megérkezett.

Mindkét esetben a soros port 115200 baud, 8 adatbit, nincs paritás, 1 stopbit módra kerül felkonfigurálásra. A soros vonali átvitelek esetében
az USBImager nem tömöríti ki a lemezképet, hogy csökkentse az átviteli időt, így a kicsomagolást a kliensen kell elvégezni. Ha egy egyszerű
rendszerbetöltőre vágysz, ami kompatíbilis az USBImager-el, akkor javalom az [Image Receiver](https://gitlab.com/bztsrc/imgrecv)-t
(elérhető RPi1, 2, 3, 4 és IBM PC BIOS gépekre).

Fordítás
--------

### Windows

Függőségek: csak szabvány Win32 DLL-ek, és MinGW a fordításhoz.

1. telepítsd a [MinGW](https://osdn.net/projects/mingw/releases)-t, ezáltal lesz "gcc" és "make parancsod Windows alatt
2. nyisd meg az MSYS terminált, és az src könyvtárban add ki a `make` parancsot
3. a csomagolt fájl létrehozásához futtasd a `make package` parancsot

### MacOSX

Függőségek: csak szabvány keretrendszerek (CoreFoundation, IOKit, DiskArbitration és Cocoa),
valamint a parancssori eszközök (nem kell az egész XCode, csak a CLI eszközei).

1. Terminálban futtasd a `xcode-select --install` parancsot, majd a felugró ablakban kattints a "Telepítés"-re. Ezáltal lesz "gcc" és "make parancsod MacOSX alatt
2. az src könyvtárban add ki a `make` parancsot
3. a csomagolt fájl létrehozásához futtasd a `make package` parancsot

Alapból az USBImager natív Cocoa támogatással fordul, libui használatával (mellékelve). Lefordíthatod azonban az X11 verziót is
(ha van XQuartz-od telepítve) az `USE_X11=yes make` paranccsal.

### Linux

Függőségek: libc, libX11 és szabvány GNU eszköztár.

1. az src könyvtárban add ki a `make` parancsot
2. a csomagolt fájl létrehozásához futtasd a `make package` parancsot
3. a Debian archívum létrehozásához futtasd a `make deb` parancsot
4. a telepítéshez add ki a `sudo make install` parancsot

Lefordíthatod GTK+ támogatással is az `USE_LIBUI=yes make` paranccsal. Ez libui-t fog használni (mellékelve), ami cserébe rengeteg
függőséget tartalmaz (pthread, X11, wayland, gdk, harfbuzz, pango, cairo, freetype2 stb.) Fontos továbbá, hogy a GTK verzió nem fog futni
setgid bittel, így az írási hozzáférés a lemezekhez nem garantált. Az X11 verzió automatikusan bekerül a "disk" csoportba futtatáskor.
A GTK esetén kézzel hozzá kell adnod a felhasználód ehhez a csoporthoz, vagy sudo-val kell indítanod az USBImager-t, különben "hozzáférés
megtagadva" hibaüzenetet fogsz kapni.

Forrás hackelése
----------------

Debuggoláshoz fordítsd a `DEBUG=yes make` paranccsal. Ez extra debuggoló szimbólumokat és forrásfájl hivatkozásokat fog a futtathatóba
rakni, amit mind a valgrind, mind a gdb tud értelmezni.

Szerkeszd a Makefile-t, és állítsd a `DISKS_TEST`-et 1-re, hogy egy speciális `test.bin` "eszköz" jelenjen meg a listában. Minden
platformon egységesen elérhető, ezzel a kicsomagolást tudod leteszteni.

Az X11 csak alacsony szintű hívásokkal operál (nem használ Xft, Xmu vagy más bővítményeket), így könnyű más POSIX rendszerekre portolni (pl.
BSD-kre vagy Minixre). Nem kezel lokalizációt, de a fájlnevekben támogatja az UTF-8 kódolást (ez csak a megjelenítésnél számít, a fájlműveletek
bármilyen kódlapot lekezelnek). Ha ezt ki akarod kapcsolni, akkor a main_x11.c fájl elején állítsd a `USEUTF8` define-t 0-ára.

A forrás jól elkülöníthetően 4 rétegre van bontva:
- stream.c / stream.h dolga a fájlok belolvasása, kicsomagolása, valamint tömörítése és kiírása
- disks_*.c / disks.h az a réteg, ami beolvassa és kiírja a lemezszektorokat, minden platformhoz külön van
- main_*.c / main.h az, ahol a main() (vagy WinMain) függvényt találod, ebben vannak a felhasználói felület cuccai
- lang.c / lang.h szolgál a nemzetköziesítésre és ebben vannak a platformfüggetlen szótárak

Ismert bugok
------------

Nincs. Ha belefutnál egybe, használd kérlek az [issue tracker](https://gitlab.com/bztsrc/usbimager/issues)-t.

Szerzők
-------

- libui: Pietro Gagliardi
- bzip2: Julian R. Seward
- xz: Igor Pavlov és Lasse Collin
- zlib: Mark Adler
- zip kezelés: bzt (semmilyen PKWARE függvénykönyvtár vagy forrás nem lett felhasználva)
- usbimager: bzt

Hozzájárulások
--------------

Szeretnék köszönetet mondani a következő felhasználóknak: @mattmiller, @MisterEd, @the_scruss, @rpdom, @DarkElvenAngel, és különösen
@tvjon-nak és @CaptainMidnight-nak amiért több különböző platformon és számos különböző eszközzel is letesztelték az USBImager-t.

Köszönet a fordítások ellenőrzéséért és javításáért: @mline-nak (német), @epoch1970-nek és @JumpZero-nak (francia), @hansotten-nek és @zonstraal-nak (holland), @ller (orosz), @zaval (ukrán), @lmarmisa (spanyol).

Legjobbakat,

bzt
