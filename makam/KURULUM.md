# Makam Nota: yeni bilgisayarda kurulum, düzenleme, derleme

Bu klasör (`makam/`) projeyi başka bir Windows bilgisayarda kurup yerelde derlemek için gerekenleri içerir.
Hedef makine: Windows 10/11 64 bit, 16 GB RAM, SSD'de en az 40 GB boş yer.

| Ne | Yaklaşık yer |
|---|---|
| Visual Studio 2022 Build Tools (C++) | 6–8 GB |
| Qt 6.10.2 | 3 GB |
| Kaynak kod | 1,5 GB |
| Derleme klasörleri (`build.release`, `build.install`) | 15–20 GB |

## 1. Git'i kur ve kodu indir

PowerShell'i aç (Başlat → "PowerShell"):

```powershell
winget install --id Git.Git -e
```

Kurulum bitince PowerShell'i kapatıp yeniden aç, sonra:

```powershell
git clone --branch makam-4.7.6 --single-branch --depth 50 https://github.com/bariskaraelma-gif/MuseScore.git C:\MakamNota\src
```

Yol kısa tutulmalı (`C:\MakamNota\src`). Uzun klasör yollarında derleyici dosya adı sınırına takılabiliyor.

## 2. Derleme araçlarını kur

```powershell
cd C:\MakamNota\src
powershell -ExecutionPolicy Bypass -File makam\kurulum.ps1
```

Betik eksik olanları kurar:
- Visual Studio 2022 Build Tools (C++ derleyici, Windows SDK, CMake, Ninja)
- Python 3
- GitHub CLI (`gh`)
- Qt 6.10.2 (`C:\Qt`). GitHub derlemesiyle aynı sürüm, Qt hesabı gerekmez.

winget her paket için lisans onayı sorar, `Y` ile onaylanır. Visual Studio kurulumu 15–30 dakika sürer.

Bitince yeni bir terminal açılır ve GitHub'a bir kez giriş yapılır (tarayıcı açılır):

```powershell
gh auth login
git -C C:\MakamNota\src config user.name "ADINIZ"
git -C C:\MakamNota\src config user.email "E-POSTA@ADRESINIZ"
```

Son iki satırda tırnak içine bu bilgisayardaki commit'lerde görünecek ad ve e-posta yazılır. Fork herkese açık olduğu için bu adres GitHub'da görülebilir.

## 3. Derle ve çalıştır

```powershell
cd C:\MakamNota\src
makam\derle.bat
makam\calistir.bat
```

İlk derleme her şeyi baştan derler. i7 10. nesilde:
- Masaüstü işlemcide (8 çekirdek, ör. i7-10700) 45–75 dakika sürer.
- Dizüstü işlemcide (4 çekirdek, ör. i7-10510U) 1,5–2,5 saat sürer.

Sonraki derlemeler yalnızca değişen dosyaları derler:
- Tek bir `.cpp` değişikliği: 1–3 dakika
- Çok yerde kullanılan bir başlık dosyası (`.h`) değişikliği: 10–30 dakika

Derleme yarıda kesilirse `makam\derle.bat` yeniden çalıştırılır, kaldığı yerden devam eder.

- **Bellek:** derleme varsayılan olarak 6 iş parçacığıyla çalışır (16 GB RAM için). "out of heap space" ya da "C1060" hatası çıkarsa:
  ```bat
  set JOBS=4
  makam\derle.bat
  ```
- **`makam\derle.bat yeniden`:** CMake yapılandırmasını baştan yapar.
- **`makam\derle.bat temiz`:** her şeyi siler ve sıfırdan derler.

Program **taşınabilir** yapılandırmada derlenir:
- Ayarları `build.install\Data\settings` içinde durur.
- Bilgisayardaki MuseScore'a ve onun ayarlarına dokunmaz.
- `build.install` klasörü bütün olarak başka yere kopyalanıp oradan da çalıştırılabilir.

## 4. Düzenleme (Claude ile)

1. O bilgisayara Claude masaüstü uygulaması kurulur ve aynı hesapla giriş yapılır.
2. Code sekmesinde klasör olarak `C:\MakamNota\src` seçilir.
3. Depo kökündeki `CLAUDE.md` projeyi Claude'a tanıtır: yapılanlar, dosyaların yeri, kurallar. Eski sohbetler ve bu bilgisayardaki Claude hafızası oraya taşınmaz; gerekenler `CLAUDE.md`'de.

Döngü:
1. Claude kodu değiştirir.
2. `makam\derle.bat` ile derlenir, `makam\calistir.bat` ile denenir.
3. Beğenilince commit atılıp GitHub'a gönderilir:
   ```powershell
   git push
   ```

## 5. GitHub'da derleme (isteğe bağlı)

Yerel derleme gerekmiyorsa ya da paket (.paf.exe) isteniyorsa derleme eskisi gibi GitHub Actions'ta yapılır (yaklaşık 40 dakika):

```powershell
gh workflow run build_windows.yml -R bariskaraelma-gif/MuseScore --ref makam-4.7.6 -f platforms=windows_portable -f build_mode=devel
```

## 6. İki bilgisayar arasında çalışma

- Çalışmaya başlamadan önce:
  ```powershell
  git pull
  ```
- Bitirince:
  ```powershell
  git push
  ```
- Aynı dosya iki bilgisayarda gönderilmeden değiştirilirse `git pull` çakışma bildirir. Claude'a "çakışmayı çöz" denebilir.

## Sorun giderme

- **Qt bulunamadı:** `makam\kurulum.ps1` yeniden çalıştırılır. Qt başka bir klasördeyse derlemeden önce yolu verilir:
  ```bat
  set QT_DIR=D:\Qt\6.10.2\msvc2022_64
  ```
- **"cl.exe bulunamadı" ya da "Visual Studio bulunamadı":** Visual Studio Installer açılır. Build Tools 2022'de "C++ ile masaüstü geliştirme" seçili olmalı.
- **Derleme hatası:** çıktıda "error C" ile başlayan ilk satır Claude'a yapıştırılır.
- **Yeni bir `.cpp` dosyası eklendiğinde garip "ambiguous symbol" hataları:** dosya ilgili `CMakeLists.txt`'te `SKIP_UNITY_BUILD_INCLUSION ON` listesine eklenmeli (ayrıntı `CLAUDE.md`'de).
