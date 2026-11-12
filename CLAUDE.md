# Makam Nota: Türk müziği nota yazım programı (MuseScore 4.7.6 çatalı)

Bu depo, MuseScore Studio 4.7.6'dan çatallanmış bir Türk makam müziği nota programıdır (GPL-3.0).
Çalışma adı "Makam Nota"; asıl ad sonra konacak. "MuseScore" adı ve logosu marka olduğu için kullanılamaz.

- Çatal: github.com/bariskaraelma/TurkScore
- Dal: `makam-4.7.6`
- Eski `makam` dalı (4.7.5 tabanlı) kullanılmıyor.

Önceki eklenti çalışması "Makam" eklentisiydi (MuseScore 4 eklentisi, `Makam.qml` v0.23.2; ayrıca TurkAriza ↑/↓ eklentileri).
Bu program o eklentilerin yaptığını yerleşik yapar.

## Kullanıcı tercihleri

- Kullanıcıyla Türkçe konuşulur.
- **"anlat" / "kod yazma anlat sadece"** denince yalnız açıklama yapılır, kod yazılmaz. Kodlamaya "başla", "ekle", "Evet, başla" gibi açık onayla geçilir.
- **Kurulu orijinal MuseScore'a ve onun AppData ayarlarına dokunulmaz.** Okumak serbesttir. Değişiklikler yalnız bu programda yapılır.
- Kullanıcının başka bir uygulamaya yapıştıracağı metin:
  - düz kod bloğunda verilir;
  - alıntı bloğu (`>`) kullanılmaz;
  - metnin içinde tablo ve satır başı `>` olmaz.
- Commit ve push, bu dalda iş akışının parçası olarak yapılır. Commit mesajları İngilizce, `Makam: ...` önekiyle yazılır.

## Derleme

- **Yerel:** `makam\derle.bat`, ardından `makam\calistir.bat`. Ayrıntı `makam/KURULUM.md`'de.
  - Taşınabilir yapılandırma (`app-portable`); ayarlar `build.install\Data\settings` içinde durur.
  - Kullanılan sürümler: Qt 6.10.2 msvc2022_64 (`C:\Qt`), Visual Studio 2022 Build Tools.
- **GitHub Actions** (yaklaşık 40 dk):
  ```
  gh workflow run build_windows.yml -R bariskaraelma/TurkScore --ref makam-4.7.6 -f platforms=windows_portable -f build_mode=devel
  ```
  - Paket şöyle indirilir: `gh run download <id> -n <artifact> -D <klasör>`.
  - MSI paketi gizli GUID istediği için yalnız taşınabilir paket üretilir.
  - GitHub 2026-11-12'de bu çatalda Actions'ı "kullanım çok büyük" diye durdurdu; kullanıcı yeniden açtı. `build_macos.yml` iş akışı ayrıca açıldı (`gh api -X PUT .../actions/workflows/<id>/enable`). Mac derlemesi (evrensel, imzasız .dmg) yaklaşık 55 dk; başlatma: `gh workflow run build_macos.yml -R bariskaraelma/TurkScore --ref makam-4.7.6 -f build_mode=devel`.
- **Yayın (GitHub Releases, hesapsız indirilir):** `turkscore-0.1` (yalnız Windows) ve `turkscore-0.2` (Windows + Mac, 81f08aae). Windows paketi yerel derlemeden: `build.install` içinden `App`, `Other`, `help.html`, boş `Data` ve `OKUBENI.txt` bir `TurkScore` klasörüne konur, `tar -a -cf` ile zip'lenir. Kişisel ayarlar (`Data\settings`) pakete girmez. Mac paketi Actions'tan (`gh run download`), Mac'te henüz denenmedi. 0.2 etiketi 22a503ad'de kaldı (zorla taşımaya izin yoktu).
- 2026-11-12: kullanıcı adı `bariskaraelma-gif` → `bariskaraelma`, depo adı `MuseScore` → `TurkScore` oldu (remote, notlar ve belgeler güncellendi; GitHub eski adresleri yönlendirir). Depo hâlâ musescore/MuseScore çatalı olarak görünür.
- **Unity build açık** (`MUSE_COMPILE_USE_UNITY=ON`). Yeni bir `.cpp` dosyası unity gruplarını kaydırır, upstream dosyalarında "ambiguous symbol" hataları çıkar. Bu yüzden her yeni `.cpp` dosyası unity'den çıkarılır:
  - `src/engraving/CMakeLists.txt`: `set_source_files_properties(... SKIP_UNITY_BUILD_INCLUSION ON)` listesine eklenir (`editahenk.cpp`, `makamtuner.cpp`, `editstaffspacing.cpp` gibi).
  - Çok yerde kullanılan başlıklara (`actionicon.h`, `system.h` gibi) dokunmak derlemeyi çok uzatır; mümkünse kaçınılır.
  - QML modellerinde de aynısı yapılır (`src/notationscene/qml/MuseScore/NotationScene/CMakeLists.txt`, `makamtuningmodel.cpp`).
  - Yeni `.cpp` içinde `using namespace mu::engraving` kullanılmaz.
- Çalışma kopyası CRLF (`core.autocrlf=true`).

## Yapılanlar ve kodun yeri

Bizim eklediğimiz her şey `Makam:` yorumuyla işaretlidir. Aramak için: `git grep -n "Makam:"`.

- **Yeni eser** (sihirbaz yok, doğrudan boş sayfa): `src/project/internal/projectactionscontroller.{h,cpp}`.
  - Komutlar: `file-new`, `file-new-with-usul`, `file-new-usul`.
  - `BlankScoreKind`: tek çalgı / çalgı + usul / yalnız usul.
  - Başlık çerçevesi var, ilk sistemde girinti yok, varsayılan ahenk "4 ses".
  - Başlangıç sayfası düğmeleri: `src/project/qml/MuseScore/Project/ScoresPage.qml` ve `scorespagemodel`.
  - Başlangıç: `src/appshell/internal/startupscenario.cpp`.
- **Çalgılar** (`share/instruments/instruments.xml`, "turkish-music" grubu):
  - `turk-muzigi`: sol anahtarı.
  - `usul`: 2 çizgili vurmalı dizek. Düm = 36 (güçlü vuruş, üst çizgi, sap yukarı), tek = 43 (zayıf, alt çizgi, sap aşağı). Kullanıcı istedi (2026-11-13); önceden tersiydi. Eski eserde usul bırakılınca `makamFillUsulStaves` partinin drumset'ini çevirir (`ChangeDrumset`, geri alınabilir).
- **Donanım paleti** (17 AEU donanımı) ve **usul paleti** (17 usul):
  - Veri: `src/engraving/dom/makamdata.h`.
  - Palet: `src/palette/internal/palettecreator.cpp`.
  - Usul kimliği: TimeSig kiriş gruplarının son düğümü `pos = 10000 + usul kodu` (`USUL_MARKER_BASE`).
  - Usul bırakılınca usul dizeği düm/tek ile dolar: `makamFillUsulStaves`, `src/engraving/editing/edit.cpp`.
  - Kiriş (sap bağlantısı) grupları kullanıcının tanımıyla (2026-11-13), `Usul::beams` (1/d) ve `Usul::breaks16` (yalnız 16'lık/32'lik kesilen yerler, düğüm 0x110): Sofyan 2+2+2+2 (sekizlik), Türk Aksağı 2+2+1, Devr-i Hindi 3+2+2, Devr-i Turan 2+2+3, Düyek 4+2+2 (1+2+1 tek kiriş; 16'lıklar 2 sekizlikte bir), Müsemmen 3+2+3, Aksak 2+2+2+3 (16'lıklar 2+2+2+2+1: son vuruştaki iki 16'lık ayrı), Evfer 2+2+2+1+2 (7. sekizlik tek başına), Raks Aksağı 2+3+2+2, Oynak 3+2+2+2, Cengi Harbi 2+2+3+3. Darblar da kullanıcının hecelerine göre düzeltildi: Sofyan, Aksak, Evfer'de ikinci vuruş tek ("düüm te ke"). Kayıtlı paletlerdeki usul hücreleri `makamRefreshUsulCells` ile yenilenir (palet sürümü 9). Eğitim dersleri aynı tabloyu kullanır (`USUL_BEAMS`, `beste2mscx.py`).
- **Ahenk** (durum çubuğunda açılır menü; Yerinden / 1 ses / 4 ses / 5 ses / Yazıldığı gibi):
  - Kod: `src/engraving/editing/editahenk.{h,cpp}`. Ahenk = partinin çalgı transpozisyonu.
  - Arayüz: `src/appshell/qml/MuseScore/AppShell/notationstatusbarmodel.*` ve `NotationStatusBar.qml`.
  - Palet donanımları yazılı ton olarak yorumlanır: `writtenKeySigEvent`.
- **Akort motoru:** `src/engraving/editing/makamtuner.{h,cpp}`.
  - `tuneScore()` her komutun sonunda `Score::endCmd` içinde çalışır (`src/engraving/editing/cmd.cpp`). Böylece ilk ses de akortlu çıkar, Ctrl+Z tek adımda geri alır.
  - `Note::tuning` = kalibre sent − arıza işaretinin yerleşik sent değeri.
  - Makam tablosu `makam/araclar/makams.json`'dan üretildi (`gen_makam_table.py`). Bundan sonra doğrudan `.cpp` düzenlenir.
- **Makam akordu penceresi** (perdelerin çıkış/iniş/durak değerleri, mandal ya da sent):
  - `src/notationscene/qml/MuseScore/NotationScene/makamtuningmodel.*` ve `MakamTuningDialog.qml`.
  - Komut `makam-tuning`; araç çubuğunda düğme ve Araçlar menüsünde öğe var.
  - Ayarlar: `makam/calibration`, `makam/yedenSegah`, `makam/unit`.
- **Diziyi dinle** (Makam akordu penceresinde düğme, "örnek seyirle" kutusu; eklentideki "Diziyi dinle"):
  - Partisyon: `src/engraving/editing/makamdizi.{h,cpp}`. Dizi tabloları ve 27 SymbTr örnek seyiri eklentiden aktarıldı. Karar ve donanım `makamBasics()` ile `makamtuner.cpp`'den gelir.
  - `.mscx` metni Bolahenk'te ve akortsuz yazılır. Başlık "<Makam> dizisi" olduğu için akort motoru makamı başlıktan bulur.
  - Komut `makam-dizi` (argümanlar: makam kimliği, seyirli mi): `ProjectActionsController::listenMakamDizi`.
    - Dosyayı arka planda yükler, açık eserin ahengini (`setAhenk`) ve akordu (boş komut) uygular.
    - `%TEMP%\MakamNota\<id>[-seyir]-dizi.mscz` olarak kaydeder ve açar (eser açıksa yeni pencerede).
  - Açık eser o dizi ise (meta etiket `makamDizi`) yalnız baştan çalar. Yeni pencerede kendiliğinden çalmaz.
- **Türk arıza ↑/↓ merdiveni:** `makam::turkishStep()` (`makamtuner.cpp`), çağrıldığı yer `NotationInteraction::movePitch`. THM donanımında ya da THM arızalı notada ↑/↓ 12 eşit aralıklı yarım ses gider, yolda donanımın THM perdesine uğrar (kullanıcı istedi; Si♭2 donanımında: La♯ ↑ Si♭2 ↑ Si ↑ Do, Si ↓ Si♭2 ↓ Si♭ ↓ La; `thmTemperedStep`, `temperedNeighbour`). THM perdeleri merdivende `THM_STEP_BASE + koma` basamağıdır, böylece yanına gereken naturel/♯/♭ yazılır.
- **Usullü metronom:** usul ölçülerinde metronom düm/tek çalar, sayım bir usul ölçüsüdür. Kod: `src/engraving/playback/playbackeventsrenderer.cpp`.
- **Yükleniyor ekranı** "TurkScore" (kullanıcı seçti; programın öteki yerlerinde ad henüz değişmedi): arka plan `src/appshell/resources/LoadingScreen.svg` (lacivert, altın geometrik süsleme, soluk dizek çizgileri), yazılar `src/appshell/widgets/splashscreen/loadingscreenview.cpp` içinde UI yazı tipiyle çizilir (başlık, alt yazı, "MuseScore Studio 4.7.6 tabanlı · GPL-3.0", "Sürüm …"). Soluk dizeğin üstünde (çizgiler SVG'de kalır) `drawStaffSigns`: sol anahtarı, Hüzzam donanımı (Si ters bemol, Mi bakiye bemol, Fa diyez; Leland), ince ölçü çizgisi, THM düğmesinin arızaları (Si ♭2, Fa ♯3; Bravura). Yazı tipleri engraving modülünün Qt kaynaklarından yüklenir. MuseScore logosu ve web adresi kaldırıldı.
- **Logo / program simgesi** (kullanıcı B taslağını seçti): lacivert zeminde altın ay yıldız, bayrak oranlarında, ağzı 45° sağ yukarı. Kaynak `share/icons/AppIcon/turkscore_logo.svg` (ince altın çemberli, 48 px ve üstü) ve `turkscore_logo_small.svg` (çembersiz, daha büyük ay yıldız; 16–32 px). `MS4_AppIcon_*.png` ve `MS4_AppIcon.ico` bunlardan üretildi (dosya adları CMake değişmesin diye aynı kaldı; .ico içinde PNG'ler). macOS `.icns` ve `AppIcon.icon` değişmedi. Belge simgeleri (`MsczIcon`, `MscxIcon`) ve `mu_logo.svg` henüz MuseScore'un.
- **Görüntü yakalama** (kamera düğmesi): `share/plugins/imagecapture/` içine gömülü eklenti, komut `image-capture`.
  - Quick Copy (diyalogsuz panoya kopyalama): komut `image-capture-quick`, Dosya menüsünde, varsayılan kısayol Ctrl+Alt+C (`src/app/configs/data/shortcuts*.xml`). İki komut da eklenti yöneticisinde etkin olmasa da çalışır (`ProjectActionsController::openImageCapture`). Kullanıcı denedi, çalışıyor (2026-11-10).
- Paletlerden Batı'ya özgü olanlar gizlendi: anahtar, perde, klavye paletleri ve İtalyanca tempo terimleri.
- **Palet düzeni** (`palettecreator.cpp`):
  - Donanım hücrelerinde yalnız makam adları var. Ölçü göstergeleri paletinin adı "Usûl".
  - Metin paletinde legato, arco, distort, overdrive, caz tonu ve dizek/sistem metin çizgileri yok. 1.–4. Hane, Teslim, Saz, Ara Saz sistem metni olarak eklendi.
  - Varsayılan palette yalnız p, mf, f ve kreşendo/dekreşendo var. Büyük mücennep diyez/bemol yalnız "Daha fazla" bölümünde.
  - Kayıtlı paletler çalışma alanında (`Default.mws`) durur. Palet tanımı değişince `paletteworkspacesetup.cpp` içindeki `MAKAM_PALETTE_VERSION` bir artırılır. `MAKAM_PALETTE_RESET_VERSION` da o sürüme getirilirse kayıtlı paletler bir kez varsayılana döner (kullanıcının "Oluştur" hücreleri de gider). Yalnız donanım adı değiştiyse RESET sürümüne dokunulmaz: `makamRenameDonanimCells` kayıtlı paletlerdeki yerleşik donanım hücrelerine `makamdata.h`'deki adı verir, gerisi kalır (sürüm 5: "Hicaz, Hicaz Uzzal, Sultaniyegâh").
- **Nota araç çubuğu:** çift diyez/bemol, marcato, accent, tenuto varsayılan olarak gizli (`defaultNoteInputBarConfig`).
- **Ahenk listesi:** en üstte "Yazıldığı gibi (Batı)", sonra Bolâhenk'ten Dâvut Mâbeyni'ne 12 ney ahengi, yarımşar ses aşağı. Varsayılan Kız (4 ses), `DEFAULT_AHENK`.
- **Tempo:**
  - Yeni eserin başında ♩ = 80 var. İlk ölçüye 8'lik birimli usul bırakılınca ♪ = 90 olur (`makamSetDefaultTempo`, `edit.cpp`; değerler `makamdata.h`). Elle değişmiş işarete dokunulmaz.
  - Çalma araç çubuğunda hız % değil tempo sayısıdır (`tempoBpm`, `playbacktoolbarmodel`). Gösterge, işaretin birimini kullanır (`NotationPlayback::tempoUnit`).
  - Kutuya yazılan sayı Enter'da ya da kutudan çıkınca uygulanır (`onValueEditingFinished`). Önce her tuşta uygulanıyordu: 120'nin üstüne "80" yazarken önce 8 olup alt sınıra (×0,1) çarpıyor, kutu atlıyordu.
- **Elle aralık düzenleme** (`src/engraving/editing/editstaffspacing.{h,cpp}`, etkileşim `NotationInteraction`, fare `NotationViewInputController`):
  - Dizek aralığı: dizeğin solundaki tutamak (⇕) sürüklenir. Ctrl ile yalnız o satır değişir (sabit spacer); Ctrl'siz bütün satırlar değişir (dizek `userDist` ya da `minSystemDistance`). Klavye: Alt+PgUp/PgDown, Ctrl+Alt+PgUp/PgDown. İlk kullanımda dikey yayma kapanır.
  - Satır genişliği: son ölçünün sağındaki tutamak (⇔) satırın sonuna yatay çerçeve + satır sonu koyar ve çerçevenin genişliğini değiştirir. Çift tıklama ya da Yerleşim paletindeki "Satır sonu (yaslamasız)" (`makam-line-end`) satırı doğal genişliğe getirir. Hepsi MuseScore'un kendi öğeleri, dosya uyumlu kalır.
  - Notasyon görünümü hover olaylarını her kipte alır (tutamak imleci için).
- **Bona (sesle nota girişi), 1. aşama: yalnız ritim, bütün notalar La:**
  - Komut `makam-bona`. Araç çubuğunda ve Araçlar menüsünde. Pencere: `BonaDialog.qml`, model `bonamodel.*` (`src/notationscene/qml/MuseScore/NotationScene/`).
  - Akış: ölçü seçilir → Kaydet. Metronom ve sayım geçici açılır, çalma seçili ölçüden başlar. Dur'a basınca heceler bulunur, ızgaraya (8'lik/16'lık) oturtulur ve tek geri alma adımında yazılır.
  - Mikrofon: `bonarecorder.*`, miniaudio 0.11.22 (`thirdparty/miniaudio/`, tek başlık). 16 kHz mono. Aygıt kendi iş parçacığında açılıp kapanır, çünkü miniaudio COM'u dengesiz kapatıyor.
  - Çözümleme: `bonaanalysis.*`. Enerji eşiği + seslilik (otokorelasyon) + legato çukurları; `quantize()` nicemleme.
  - Zaman eşleme: mikrofon zamanı − çalma konumu farklarının ortancası, sonra `secToTick`. "Gecikme" (varsayılan 190 ms, ayar `makam/bonaLatencyMs`) ses kartı gecikmesini düşer. Vuruştan az önce söylenen hece eksiye taşar ve vuruşa yuvarlanır.
  - Son kayıt ve çözümlemesi: `%TEMP%\makam-bona-son.wav` / `.txt` (hata ayıklama için).
  - Kullanıcı testi: ilk denemede notalar 16'lık geç yazıldı (gecikme 60 → 190 yapıldı), sonra ilk hece düştü (düzeltildi). Son düzeltme kurulu ama mikrofonla henüz denenmedi.

- **Finale benzeri arayüz** (Tercihler > Görünüm > Arayüz; ayar `makam/interface` = `musescore` | `finale`, varsayılan Finale: ayar yoksa Finale, yalnız `musescore` yazılıysa MuseScore):
  - Notanın solunda iki sütunlu, gruplu araç çubuğu: `src/appshell/qml/MuseScore/AppShell/NotationPage/MakamToolStrip.qml`. `NotationPage.qml` içinde `central` bir Item oldu: araç çubuğu + NotationView.
  - Gruplar: Seçim | Nota girişi — Makam | Usul, Ölçü çizgisi | Yerleşim — Artikülasyon | Nüans, Metin | Söz, Tekrar — Özellikler | Bütün paletler. Anahtar, Tempo, Süsleme aracı yok (tempo üstteki çalma çubuğunda).
  - Her araç çubuğun yanında dikey bir sütun açar (açılır kutu yok): Nota girişi → AEU arızaları (üstte naturel, solda 4 diyez, sağda 4 bemol; `MakamToolsModel::applyAccidental`, nota çubuğundaki arıza düğmeleri gibi). Öteki araçlar → kendi paleti, iki hücre genişliğinde, "Daha fazla" ile tam palet (`ToolPaletteColumn.qml`, palette modülü). THM donanımında THM arızaları (aşağıya bakın).
  - **Oluştur…** (Makam ve Usul sütunlarında, `ToolPaletteColumn.qml` içinde açılır pencere): kullanıcı kendi donanımını (ad + nota/arıza satırları) ya da usulünü (ad + birim + düm/tek darbları; ölçü = darb toplamı/birim) yapar. `PaletteRootModel::makamCreateDonanim` / `makamCreateUsul` hücreyi kullanıcının paletinin sonuna, palete eleman bırakılmış gibi (`dropMimeData`, özel hücre) ekler ve ölçü seçiliyse uygular. Paletler çalışma alanıyla kaydedilir.
    - Kullanıcı usulü kendini ölçü göstergesinde taşır (`makamdata.h`): işaret kodu `USUL_CUSTOM_CODE` (1000), işaretten sonra darblar (`pos = 20000 + i`, `action = len*2 + düm`) ve UTF-8 ad (`pos = 30000 + i`). Böylece dosya başka bilgisayarda da usulü bilir (usul dizeği, metronom, ad yazısı). `usulOfGroups` artık `std::optional<Usul>` döndürür, `Usul::name` `std::string`.
    - Durum: kullanıcı yeni makam ve usul oluşturmayı denedi, çalışıyor (2026-11-08).
  - Araç → palet eşlemesi: `PaletteRootModel::makamToolPalettes()` (`src/palette/qml/MuseScore/Palette/internal/paletterootmodel.cpp`). Kullanıcının kendi (sadeleştirilmiş) paletleri gösterilir.
  - Simgeler: Makam = ters bemol (koma, U+E280) solda aşağıda + çizgili bemol (bakiye, U+E442) sağda yukarıda; Usul = 5 üstte 8 altta, çizgisiz, palettekiyle aynı rakamlar; Metin = "Metin" yazısı. Nota yazı tipiyle (`ui.theme.musicalFont`); ötekiler MuseScore simge yazı tipinden.
  - Paneller (`NotationPageModel`, `src/appshell/qml/MuseScore/AppShell/notationpagemodel.cpp`): `applyMakamInterface` Finale kipinde açılışta ve kip seçilince (Tercihler kapandıktan sonra, zamanlayıcıyla) Paletler, Yerleşim, Özellikler'i kapatır; MuseScore seçilince açar ve Özellikler'i yerine takar. `makam-properties` komutu (`onMakamProperties`) Özellikler'i yüzen küçük pencere olarak açar: yalnız Özellikler düğmesi açar/kapatır. Metin ve Söz açmaz (kullanıcı istedi).
  - Model: `makamtoolsmodel.*` (`src/notationscene/qml/MuseScore/NotationScene/`). Notadaki etkileşime (NotationInteraction) dokunulmadı.
  - Durum: derlendi ve kuruldu; kullanıcı deniyor.
- **THM donanımları** (TRT halk müziği; kullanıcının "trt thm donanimlari.docx" listesinden, 2026-11-11):
  - Finale araç çubuğunda Makam'ın yanında THM düğmesi (simge: sol altta Si ♭2, sağ üstte Fa ♯3; Bravura, `MakamToolsModel::thmFontFamily` yazı tipini yükler); yan sütunda "THM donanımları" paleti (Daha fazla / Oluştur yok). Sıra kullanıcının isteğiyle: 6. hücre "Si ♭, Do ♯".
  - Glifler SMuFL "Turkish folk music accidentals" (Bravura E450–E457: ♯1 ♯2 ♯3 ♯5, ♭1 ♭2 ♭3 ♭4). ♭ yalnız = 5 koma, ♯ yalnız = 4 koma (♯4 ayrı glif değil). Leland'da yoklar; MuseScore Bravura'dan çizer.
  - Veri `thmDonanimlar()` / `thmKomaSym()` (`makamdata.h`), 29 farklı donanım (belgedeki 36 görselden, aynı arızalılar birleşti; 2569 "hatalı" atlandı). Hücre adı arızalar + belgedeki örnek numaraları.
  - Palet `newThmKeySigPalette()` (`palettecreator.cpp`), türü KeySig; AEU donanım paletinden adıyla ayrılır (`THM_PALETTE_NAME`, `makamToolPalettes("thm")`). Kayıtlı paletlere `makamAddThmPalette` ekler ya da THM paletini yenisiyle değiştirir (`MAKAM_THM_PALETTE_VERSION`, şimdi 8; THM sırası değişince ikisi birden artırılır; öteki paletler ve kullanıcının hücreleri kalır).
  - Akort: `symKoma` / `accidentalKoma` 1–4 koma bemol, 1/2/3/5 koma diyez okur. THM perdesi (`NoteInfo::thm`: THM arızası ya da THM donanımındaki rakamlı arıza) makam kalibrasyonu ve yedenli Segah kuralı olmadan, kullanıcının tablosundaki sabit değeri çalar: koma × 1200/53 (♭1 −22.64, ♭2 −45.28, ♭3 −67.92, ♭4 −90.57, ♯1 22.64, ♯2 45.28, ♯3 67.92, ♯5 113.2), 0,01 sente yuvarlı. Notanın `tuning` değeri, MuseScore'un arızaya verdiği sentin (22,2'nin katları) üstüne eklenen farktır.
  - Nota girişi sütunu THM donanımında THM arızalarını gösterir (naturel; ♯1 ♯2 ♯3 ♯ ♯5 | ♭1 ♭2 ♭3 ♭4 ♭): `MakamToolsModel::updateThm` nota girişi konumundaki, yoksa seçimin başındaki donanıma bakar (`makam::isThmKeySig`). THM paletinin donanımları `THM_KEY_MODE` (KeyMode::IONIAN) ile işaretli, çünkü düz "Si ♭", "Fa ♯" AEU'dakilerle aynı; THM glifli donanımlar işaretsiz de tanınır. Palet sürümü 8.
- **Mızrap okları** (`guitarStrumDown/Up` artikülasyonu; dosya standart MuseScore dosyası kalır, asıl MuseScore'da kısa ok görünür):
  - Artikülasyon paletinin en başında (Bağ'dan önce): "Mızrap aşağı", "Mızrap yukarı" (`palettecreator.cpp`). İkinci tık kaldırır.
  - Uzun ince sap + küçük açık uç, ~3 aralık: kodla çizilir. Ölçüler `src/engraving/rendering/score/makammizrap.h`; şekil `TLayout::fillArticulationShape`, çizim `TDraw::draw(const Articulation*)`. Boşluk boyu `mag() * style().spatium()` (magS() değil: o oran).
  - Yerleşim `ChordLayout`: ANCHOR AUTO iken hep üstte; aynı akorda iki ok yan yana, konulma sırasıyla, çift ortalanmış (`layoutArticulations2`).
  - Komut satırından deneme: `MuseScore4.exe -o out.png test.mscx` (kurulu program). Kurulumdan sonra programı açıp zorla kapatma: palet sürümü sıfırlanmış sayılır ama palet kaydedilmez (`MAKAM_PALETTE_VERSION` bu yüzden 4).
- **Makam Eğitimi** (Başlangıç > Öğrenin, ilk sekme; kullanıcının seçtiği "hazır ders notaları" yolu, ileride usuller için sayfa içi vuruş çizimi eklenebilir):
  - Dersler `share/makam-egitim/<n-Bölüm>/<ders>.mscx|.mscz`, kurulumda `<program>/makam-egitim`. Klasör = bölüm, dosya = kart; baştaki numara gösterilmez. Aynı adlı `.txt` (beste metni) kartın başlığını (`# title:`), altyazısını (`# altbaslik:`, yoksa makam · form · usul) ve notunu (`# notlar:`, ipucunda tamamı) verir.
  - Kod: `LearnPageModel::makamLessons` / `openMakamLesson` (`src/framework/learn/qml/Muse/Learn/learnpagemodel.cpp`), görünüm `internal/MakamLessonsPage.qml`, sekme `LearnPage.qml` ("makam" sayfası).
  - Ders `file-open` ile Nota sekmesinde açılır, programın çalıcısıyla dinlenir. `NotationProject::load` yolunda `/makam-egitim/` olan dosyayı yeni eser sayar: Kaydet yer sorar, ders dosyası değişmez.
  - Üretim: `python -I -X utf8 makam/egitim/dersleri_uret.py` her `.txt`'den `.mscx` yapar (`makam/egitim/beste2mscx.py`, kullanıcının dönüştürücüsünün kopyası: bütün usuller, `# altbaslik:`, x/4'te dörtlüklü tempo yazısı). `--kaynak` dizi (Rast, Uşşak, Hüseynî, Hicaz) ve 16 usul dersinin `.txt`'lerini baştan yazar.
  - İlk içerik: 4 makam dizisi, 16 usul, Örnek Eserler'de Hicaz Saz Semâîsi. Sırada: kullanıcının kendi besteleri (çoğu kendi bestesi) ve SymbTr örnekleri (lisansı CC BY-NC-SA 4.0: kaynak gösterilir, ticari olmayan kullanım).
  - Durum: derlendi; dersler kart olarak görünüyor, tıklanınca açılıp çalıyor (2026-11-08).

- **SymbTr MusicXML açma** (2026-11-12; `src/engraving/editing/makamsymbtr.{h,cpp}`, çağrı `NotationProject::doImport` içinde, içe aktarmadan hemen sonra, undo geçmişi dışında):
  - Tanıma: ilk ölçüdeki "Makam: X, Form: Y, Usul: Z" yazısı (SymbTr'nin dönüştürücüsü burakuyar/MusicXMLConverter bunu metronomla aynı metne koyar). Yazı alt başlığa "X · Y · Z" olarak taşınır (ayarlayıcı makamı oradan bulur); "?" yazılı güftekâr/besteci silinir.
  - Usul: adı ve n/d'si TurkScore usullerinden biriyle tutan ölçü göstergelerine usul işareti konur (Aksak, Aksak Semâî, Düyek…). Ağır Aksak, Fahte, Devr-i Kebîr gibi listede olmayanlar düz kalır.
  - Arızalar: SymbTr her notaya kendi perdesini verir (işaret yoksa ve `<alter>` yoksa naturel; bakiye bemol, küçük mücennep diyez gibi işaretlerde `<alter>` hiç yazılmaz, perde işaretten okunur). Süsleme notalarında dönüştürücü işareti de düşürür: orada ölçüdeki önceki aynı nota, yoksa donanım. Yazılı arızalar silinir, sonra Türk okumasına (donanım + ölçü içi taşıma, `noteInfo` ile aynı) göre gereken yere arıza/naturel konur; sonda `readKomas` ile denetlenir, `tuneScore` çalışır. Ayarlayıcıdan açılan yardımcılar: `accidentalKomaShift`, `accidentalOfKoma`, `keySigKomas`, `readKomas` (`makamtuner.h`).
  - Görünüm: SymbTr'nin kirişleri silinir (BeamMode AUTO); usulü tanınan göstergeler usul gruplamasını, x/4 göstergeleri (Ağır Aksak 9/4, Ağır Aksak Semâî 10/4, Fahte 20/4…) her çeyrekte kiriş kesen gruplamayı alır. Not: MuseScore'un `Groups::endings` işlevi tablosunda olmayan bir gösterge için boş gruplama döndürüyor (boş kopyayı listeye ekleyip düğümleri yerel kopyaya yazıyor), bu yüzden bütün ölçü tek kiriş oluyordu; ortak koda dokunulmadı. "Music" adlı bölüm `turk-muzigi` çalgısına çevrilir (ad, kısa ad T.M., gitar sesi).
  - Kullanıcı isteğiyle (2026-11-12): x/8 usullerde (kendi kiriş tanımı yoksa) sekizlikler 2'şerli, tek sayıda sonu 3 (Aksak 9/8 = 2+2+2+3), onaltılıklar 2 sekizlikte bir kesilir (9/8 = 2+2+2+2+1; düğüm 0x110); Düyek 8/8 hep 2'şerli (`aksakNodes`). 2026-11-13'ten beri bütün x/8 usullerin kendi kiriş tanımı var; `aksakNodes` yalnız usul işareti tanınmayan x/8 göstergelerde kullanılır. Güftedeki "." (saz yerleri) silinir; "ARANAĞME", "SAZ" gibi büyük harfli bölüm adları dizek metnine ("Aranağme") çevrilir. Başlıktaki izafet "-I" → "-i" ("Bîgâne-i").
  - Deneme: MTG/SymbTr'den 17 eser (13 makam), txt ile nota nota karşılaştırma: 8.831 notadan 8.823'ü doğru perdeden okunuyor. Kalan 8: dönüştürücünün düşürdüğü ama aslında naturel olan süsleme notaları.
  - Lenovo'nun makale raporu (2.200 eser): 59 makamda donanım boş, ötekilerde makama göre sabit (ör. Hicaz'da Fa diyez); tekrar/dolap işaretleri hiç yok. Bunlar veride yok; perdeler yine doğru çıkar (boş donanımda her notada arıza).

## Sıradaki işler

1. **Sesle nota girişi**:
   - Bona 1. aşama (ritim) yapıldı; yukarıya bakın. Sırada: "Gecikmeyi ölç" düğmesi, 2. aşama hece tanıma (do-re-mi → perde).
   - Sonra Ezgili solfej (perde takibi), aynı kayıt ve nicemleme altyapısıyla.
2. Makam akordu için "Bu esere kaydet". (Makam başına "Diziyi dinle" yapıldı; sırada: yeni pencerede kendiliğinden çalma.)
3. Sağ sütunda Claude paneli (sonra).
4. Daha sonrası:
   - Program adı ve simgesi
   - Kurulum paketi
   - Çeviriler
   - Upstream MuseScore güncellemelerini alma düzeni
