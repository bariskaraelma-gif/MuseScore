# Makam Nota: Türk müziği nota yazım programı (MuseScore 4.7.6 çatalı)

Bu depo, MuseScore Studio 4.7.6'dan çatallanmış bir Türk makam müziği nota programıdır (GPL-3.0).
Çalışma adı "Makam Nota"; asıl ad sonra konacak. "MuseScore" adı ve logosu marka olduğu için kullanılamaz.

- Çatal: github.com/bariskaraelma-gif/MuseScore
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
  gh workflow run build_windows.yml -R bariskaraelma-gif/MuseScore --ref makam-4.7.6 -f platforms=windows_portable -f build_mode=devel
  ```
  - Paket şöyle indirilir: `gh run download <id> -n <artifact> -D <klasör>`.
  - MSI paketi gizli GUID istediği için yalnız taşınabilir paket üretilir.
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
  - `usul`: 2 çizgili vurmalı dizek. Düm = 36 (alt), tek = 43 (üst).
- **Donanım paleti** (17 AEU donanımı) ve **usul paleti** (17 usul):
  - Veri: `src/engraving/dom/makamdata.h`.
  - Palet: `src/palette/internal/palettecreator.cpp`.
  - Usul kimliği: TimeSig kiriş gruplarının son düğümü `pos = 10000 + usul kodu` (`USUL_MARKER_BASE`).
  - Usul bırakılınca usul dizeği düm/tek ile dolar: `makamFillUsulStaves`, `src/engraving/editing/edit.cpp`.
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
- **Türk arıza ↑/↓ merdiveni:** `makam::turkishStep()` (`makamtuner.cpp`), çağrıldığı yer `NotationInteraction::movePitch`.
- **Usullü metronom:** usul ölçülerinde metronom düm/tek çalar, sayım bir usul ölçüsüdür. Kod: `src/engraving/playback/playbackeventsrenderer.cpp`.
- **Görüntü yakalama** (kamera düğmesi): `share/plugins/imagecapture/` içine gömülü eklenti, komut `image-capture`.
- Paletlerden Batı'ya özgü olanlar gizlendi: anahtar, perde, klavye paletleri ve İtalyanca tempo terimleri.
- **Palet düzeni** (`palettecreator.cpp`):
  - Donanım hücrelerinde yalnız makam adları var. Ölçü göstergeleri paletinin adı "Usûl".
  - Metin paletinde legato, arco, distort, overdrive, caz tonu ve dizek/sistem metin çizgileri yok. 1.–4. Hane, Teslim, Saz, Ara Saz sistem metni olarak eklendi.
  - Varsayılan palette yalnız p, mf, f ve kreşendo/dekreşendo var. Büyük mücennep diyez/bemol yalnız "Daha fazla" bölümünde.
  - Kayıtlı paletler çalışma alanında (`Default.mws`) durur. Palet tanımı değişince `paletteworkspacesetup.cpp` içindeki `MAKAM_PALETTE_VERSION` bir artırılır; kayıtlı paletler bir kez varsayılana döner.
- **Nota araç çubuğu:** çift diyez/bemol, marcato, accent, tenuto varsayılan olarak gizli (`defaultNoteInputBarConfig`).
- **Ahenk listesi:** en üstte "Yazıldığı gibi (Batı)", sonra Bolâhenk'ten Dâvut Mâbeyni'ne 12 ney ahengi, yarımşar ses aşağı. Varsayılan Kız (4 ses), `DEFAULT_AHENK`.
- **Tempo:**
  - Yeni eserin başında ♩ = 80 var. İlk ölçüye 8'lik birimli usul bırakılınca ♪ = 90 olur (`makamSetDefaultTempo`, `edit.cpp`; değerler `makamdata.h`). Elle değişmiş işarete dokunulmaz.
  - Çalma araç çubuğunda hız % değil tempo sayısıdır (`tempoBpm`, `playbacktoolbarmodel`). Gösterge, işaretin birimini kullanır (`NotationPlayback::tempoUnit`).
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

- **Finale benzeri arayüz** (Tercihler > Görünüm > Arayüz; ayar `makam/interface` = `musescore` | `finale`, varsayılan MuseScore):
  - Notanın solunda iki sütunlu, gruplu araç çubuğu: `src/appshell/qml/MuseScore/AppShell/NotationPage/MakamToolStrip.qml`. `NotationPage.qml` içinde `central` bir Item oldu: araç çubuğu + NotationView.
  - Gruplar: Seçim | Nota girişi — Makam | Usul, Ölçü çizgisi | Yerleşim — Mızrap ↓ | Mızrap ↑, Artikülasyon | Nüans, Metin | Söz, Tekrar — Özellikler | Bütün paletler. Anahtar, Tempo, Süsleme aracı yok (tempo üstteki çalma çubuğunda).
  - Her araç çubuğun yanında dikey bir sütun açar (açılır kutu yok): Nota girişi → AEU arızaları (üstte naturel, solda 4 diyez, sağda 4 bemol; `MakamToolsModel::applyAccidental`, nota çubuğundaki arıza düğmeleri gibi). Öteki araçlar → kendi paleti, iki hücre genişliğinde, "Daha fazla" ile tam palet (`ToolPaletteColumn.qml`, palette modülü). THM arızaları sonra.
  - Araç → palet eşlemesi: `PaletteRootModel::makamToolPalettes()` (`src/palette/qml/MuseScore/Palette/internal/paletterootmodel.cpp`). Kullanıcının kendi (sadeleştirilmiş) paletleri gösterilir.
  - Mızrap: `MakamToolsModel::addMizrap` seçili notalara `guitarStrumDown/Up` artikülasyonu koyar (üstte, `ArticulationAnchor::TOP`; Leland'de yok, Bravura'dan gelir). İkinci tık kaldırır.
  - Simgeler: Makam = ters bemol (koma, U+E280) solda aşağıda + çizgili bemol (bakiye, U+E442) sağda yukarıda; Usul = 5 üstte 8 altta, çizgisiz, palettekiyle aynı rakamlar; Metin = "Metin" yazısı. Nota yazı tipiyle (`ui.theme.musicalFont`); ötekiler MuseScore simge yazı tipinden.
  - Paneller (`NotationPageModel`, `src/appshell/qml/MuseScore/AppShell/notationpagemodel.cpp`): `applyMakamInterface` Finale kipinde açılışta ve kip seçilince (Tercihler kapandıktan sonra, zamanlayıcıyla) Paletler, Yerleşim, Özellikler'i kapatır; MuseScore seçilince açar ve Özellikler'i yerine takar. `makam-properties` komutu (`onMakamProperties`) Özellikler'i yüzen küçük pencere olarak açar: Özellikler düğmesi açar/kapatır, Söz ve Metin yalnız açar.
  - Model: `makamtoolsmodel.*` (`src/notationscene/qml/MuseScore/NotationScene/`). Notadaki etkileşime (NotationInteraction) dokunulmadı.
  - Durum: derlendi ve kuruldu, açılışta QML hatası yok; bu sürümü kullanıcı henüz denemedi.

## Sıradaki işler

1. **Sesle nota girişi**:
   - Bona 1. aşama (ritim) yapıldı; yukarıya bakın. Sırada: "Gecikmeyi ölç" düğmesi, 2. aşama hece tanıma (do-re-mi → perde).
   - Sonra Ezgili solfej (perde takibi), aynı kayıt ve nicemleme altyapısıyla.
2. Makam akordu için "Bu esere kaydet" ve makam başına "▶ Dinle".
3. Sağ sütunda Claude paneli (sonra).
4. Daha sonrası:
   - THM (halk müziği) donanımları
   - Program adı ve simgesi
   - Kurulum paketi
   - Çeviriler
   - Upstream MuseScore güncellemelerini alma düzeni
