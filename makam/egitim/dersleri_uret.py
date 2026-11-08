"""Makam Eğitimi derslerini üretir (Öğrenin > Makam Eğitimi sekmesi).

Kullanım: python -I -X utf8 makam/egitim/dersleri_uret.py [--kaynak]

- share/makam-egitim/<bölüm>/<ders>.txt beste metinlerinin her birinden aynı adlı .mscx yapar
  (beste2mscx.py). Program kartın başlığını ve yazılarını .txt'nin "# title:", "# makam:",
  "# form:", "# usul:", "# altbaslik:" (yoksa makam · form · usul), "# notlar:" satırlarından alır.
- --kaynak: önce dizi ve usul derslerinin .txt kaynaklarını baştan yazar (aşağıdaki DIZILER ve
  beste2mscx.USULS'tan). Elle düzeltilmiş dizi / usul .txt'leri varsa bunu kullanmayın.

Yeni ders: .txt'yi (ya da doğrudan bir .mscx / .mscz'yi) bir bölüm klasörüne koyun, bu betiği
çalıştırın, programı yeniden derleyin (kurulum klasörünü günceller).
"""
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "share", "makam-egitim"))
sys.path.insert(0, HERE)
import beste2mscx  # noqa: E402

DIZI_BOLUM = "1-Makam Dizileri"
USUL_BOLUM = "2-Usuller"

# dosya, makam, donanım, altyazı, çıkış (2 ölçü), iniş (2 ölçü), notlar
DIZILER = [
    ("01-rast", "Rast", "B-1 F4", "Durak Rast (Sol) · Güçlü Nevâ (Re)",
     "Sol4:24 La4:24 Si4b1:24 Do5:24 | Re5:24 Mi5:24 Fa5#4:24 Sol5:24",
     "Sol5:24 Fa5:24 Mi5:24 Re5:24 | Do5:12 Si4b1:12 La4:24 Sol4:48",
     "Rast beşlisi (Rast perdesinde) ve Rast dörtlüsü (Nevâ'da). Durak Rast (Sol), güçlü Nevâ (Re). "
     "Çıkışta Eviç (Fa diyez), inişte çoğunlukla Acem (Fa natürel)."),
    ("02-ussak", "Uşşak", "B-1", "Durak Dügâh (La) · Güçlü Nevâ (Re)",
     "La4:24 Si4b1:24 Do5:24 Re5:24 | Mi5:24 Fa5:24 Sol5:24 La5:24",
     "La5:24 Sol5:24 Fa5:24 Mi5:24 | Re5:12 Do5:12 Si4b1:24 La4:48",
     "Uşşak dörtlüsü (Dügâh'ta) ve Bûselik beşlisi (Nevâ'da). Durak Dügâh (La), güçlü Nevâ (Re). "
     "Segâh (Si koma bemol) Uşşak'ın karakter perdesidir."),
    ("03-huseyni", "Hüseynî", "B-1 F4", "Durak Dügâh (La) · Güçlü Hüseynî (Mi)",
     "La4:24 Si4b1:24 Do5:24 Re5:24 | Mi5:24 Fa5#4:24 Sol5:24 La5:24",
     "La5:24 Sol5:24 Fa5#4:24 Mi5:24 | Re5:12 Do5:12 Si4b1:24 La4:48",
     "Hüseynî beşlisi (Dügâh'ta) ve Uşşak dörtlüsü (Hüseynî'de). Durak Dügâh (La), güçlü Hüseynî (Mi). "
     "Eviç (Fa diyez) çıkışta tam, inişte biraz pes basılır."),
    ("04-hicaz", "Hicaz", "B-4 C4", "Durak Dügâh (La) · Güçlü Nevâ (Re)",
     "La4:24 Si4b4:24 Do5#4:24 Re5:24 | Mi5:24 Fa5#4:24 Sol5:24 La5:24",
     "La5:24 Sol5:24 Fa5:24 Mi5:24 | Re5:12 Do5#4:12 Si4b4:24 La4:48",
     "Hicaz dörtlüsü (Dügâh'ta) ve Rast beşlisi (Nevâ'da). Durak Dügâh (La), güçlü Nevâ (Re). "
     "Si bakiye bemol ile Do diyez arasındaki artık ikili Hicaz'ın rengidir; çıkışta Eviç, inişte Acem."),
]

# usul dersi temposu (sekizlik / dk)
TEMPO = {4: 160, 8: 200}
TEMPO_BY_NAME = {"Yürük Semâî": 240, "Aksak": 240, "Türk Aksağı": 240, "Curcuna": 240}

RESTS = [96, 72, 48, 36, 24, 18, 12, 9, 6]


def rests(total):
    out = []
    for r in RESTS:
        while total >= r:
            out.append("Es:%d" % r)
            total -= r
    assert total == 0
    return " ".join(out)


def write(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    io.open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")


def kaynak():
    for f, makam, key, sub, up, down, notes in DIZILER:
        u1, u2 = up.split(" | ")
        d1, d2 = down.split(" | ")
        write(os.path.join(ROOT, DIZI_BOLUM, f + ".txt"), [
            "# title: %s Dizisi" % makam,
            "# makam: %s" % makam,
            "# altbaslik: %s" % sub,
            "# key: %s" % key,
            "# source: TurkScore eğitim",
            "# notlar: %s" % notes,
            "",
            "m1 [section=Çıkış, meter=4/4, tempo=160] : " + u1,
            "m2 : " + u2,
            "m3 [section=İniş] : " + d1,
            "m4 [final] : " + d2,
        ])

    usuls = sorted(beste2mscx.USULS.items(), key=lambda kv: (kv[1][1] * 96 // kv[1][2], kv[1][0]))
    for i, (name, (code, n, d, strokes)) in enumerate(usuls, 1):
        unit = "dörtlük" if d == 4 else "sekizlik" if d == 8 else "on altılık"
        darb = " · ".join("%s (%d)" % ("Düm" if dum else "Tek", ln) for dum, ln in strokes)
        tempo = TEMPO_BY_NAME.get(name, TEMPO.get(d, 200))
        bar = rests(n * 96 // d)
        fname = "%02d-%s" % (i, name.lower().translate(str.maketrans("çğıöşüâîû ", "cgiosuaiu-")))
        write(os.path.join(ROOT, USUL_BOLUM, fname + ".txt"), [
            "# title: %s Usulü" % name,
            "# altbaslik: %d/%d · %d zamanlı" % (n, d, n),
            "# source: TurkScore eğitim",
            "# notlar: Vuruşlar (%s cinsinden): %s. Usul dizeğinde alt çizgi düm, üst çizgi tek. "
            "Oynatırken usullü metronom da bu vuruşları çalar." % (unit, darb),
            "",
            "m1 [usul=%s, tempo=%d] : %s" % (name, tempo, bar),
            "m2 : " + bar,
            "m3 : " + bar,
            "m4 [final] : " + bar,
        ])


def main():
    if "--kaynak" in sys.argv:
        kaynak()
    count = 0
    for bolum in sorted(os.listdir(ROOT)):
        folder = os.path.join(ROOT, bolum)
        if not os.path.isdir(folder):
            continue
        for f in sorted(os.listdir(folder)):
            if f.endswith(".txt"):
                src = os.path.join(folder, f)
                out = src[:-4] + ".mscx"
                sys.argv = ["beste2mscx.py", src, out]
                beste2mscx.main()
                count += 1
    print("%d ders" % count)


if __name__ == "__main__":
    main()
