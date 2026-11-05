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
  - `src/engraving/CMakeLists.txt`: `set_source_files_properties(... SKIP_UNITY_BUILD_INCLUSION ON)` listesine eklenir (`editahenk.cpp`, `makamtuner.cpp` gibi).
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

## Sıradaki işler

1. **Sesle nota girişi**:
   - İki kip: Bona (do-re-mi heceleri tek perdede okunur, hece tanıma) ve Ezgili solfej (perde takibi).
   - Ortak altyapı: kayıt, metronom, nota başı algılama, nicemleme.
2. Makam akordu için "Bu esere kaydet" ve makam başına "▶ Dinle".
3. Sağ sütunda Claude paneli (sonra).
4. Daha sonrası:
   - THM (halk müziği) donanımları
   - Program adı ve simgesi
   - Kurulum paketi
   - Çeviriler
   - Upstream MuseScore güncellemelerini alma düzeni
