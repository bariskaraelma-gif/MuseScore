"""Beste metnini (m<no> [işaretler] : notalar) Makam Nota için .mscx dosyasına çevirir.

Kullanım: python -I -X utf8 beste2mscx.py <beste.txt> <çıktı.mscx> [makam]

- Türk Müziği dizeği (ahenk 4 ses: yazılı La = Si3) ve usul dizeği (düm / tek).
- Donanım "# key:" satırından (ör. "B-4 C4"); arıza işareti yalnız donanımdan ya da ölçüde
  daha önce yazılandan farklı perdeye konur.
- Notalar Makam Nota'nın akort motorunun varsayılan değerleriyle akortlu yazılır (çıkış / iniş /
  durak); program ilk düzenlemede kendi kalibrasyonuyla yeniden akortlar.
- Usul, zaman işaretindeki usul işaretiyle (Groups, pos = 10000 + usul kodu) yazılır: usullü
  metronom ve usul paleti onu tanır.

Nota: SymbTr yazımı, "Si4b4:12" = Si 4 koma bemol, 4. oktav, 12/96 (sekizlik). "Es:12" = es.
İşaretler: section=<prova işareti>, usul=<ad>, meter=<n/d>, tempo=<sekizlik / dk>,
startRepeat, endRepeat, final.
"""
import io
import re
import sys

KOMA_CENT = 1200 / 53

# usuls of Makam Nota (src/engraving/dom/makamdata.h): code, n, d, strokes (düm?, length in 1/d)
USULS = {
    "Nim Sofyan": (1, 2, 4, [(True, 1), (False, 1)]),
    "Semâî": (2, 3, 4, [(True, 1), (False, 1), (False, 1)]),
    "Sofyan": (3, 4, 4, [(True, 2), (True, 1), (False, 1)]),
    "Türk Aksağı": (4, 5, 8, [(True, 2), (False, 2), (False, 1)]),
    "Yürük Semâî": (5, 6, 8, [(True, 1), (False, 1), (False, 1), (True, 1), (False, 2)]),
    "Devr-i Hindî": (6, 7, 8, [(True, 1), (False, 1), (False, 1), (True, 2), (False, 2)]),
    "Devr-i Turan": (7, 7, 8, [(True, 2), (False, 2), (False, 3)]),
    "Düyek": (8, 8, 8, [(True, 1), (False, 2), (False, 1), (True, 2), (False, 2)]),
    "Müsemmen": (9, 8, 8, [(True, 3), (False, 2), (False, 3)]),
    "Aksak": (10, 9, 8, [(True, 2), (True, 1), (False, 1), (True, 2), (False, 2), (False, 1)]),
    "Evfer": (11, 9, 8, [(True, 2), (True, 1), (False, 1), (True, 2), (False, 1), (False, 2)]),
    "Raks Aksağı": (12, 9, 8, [(True, 2), (False, 3), (True, 2), (False, 2)]),
    "Oynak": (13, 9, 8, [(True, 1), (False, 1), (False, 1), (True, 2), (False, 2), (False, 2)]),
    "Aksak Semâî": (14, 10, 8, [(True, 2), (True, 1), (False, 2), (True, 2), (False, 2), (False, 1)]),
    "Curcuna": (16, 10, 8, [(True, 2), (True, 1), (False, 2), (True, 2), (False, 2), (False, 1)]),
    "Cenk-i Harbî": (17, 10, 8, [(True, 1), (False, 1), (True, 1), (False, 1), (True, 1), (False, 1),
                                (False, 1), (True, 1), (False, 1), (False, 1)]),
}
# beam groups of the usuls in eighths, and the places (in eighths) where only the 16th and 32nd
# beams break: as in makamdata.h (Usul::beams, Usul::breaks16)
USUL_BEAMS = {
    "Sofyan": ([2, 2, 2, 2], []),
    "Türk Aksağı": ([2, 2, 1], []),
    "Devr-i Hindî": ([3, 2, 2], []),
    "Devr-i Turan": ([2, 2, 3], []),
    "Düyek": ([4, 2, 2], [2]),
    "Müsemmen": ([3, 2, 3], []),
    "Aksak": ([2, 2, 2, 3], [8]),
    "Evfer": ([2, 2, 2, 1, 2], []),
    "Raks Aksağı": ([2, 3, 2, 2], []),
    "Oynak": ([3, 2, 2, 2], []),
    "Aksak Semâî": ([2, 1, 2, 2, 2, 1], []),
    "Curcuna": ([2, 1, 2, 2, 2, 1], []),
    "Cenk-i Harbî": ([2, 2, 3, 3], []),
}
# melody beam groups in eighths, for the other time signatures
BEAMS = {(10, 8): [3, 2, 2, 3], (6, 8): [3, 3], (9, 8): [2, 2, 2, 3], (8, 8): [3, 3, 2], (4, 4): [2, 2, 2, 2],
         (5, 8): [2, 3], (7, 8): [3, 2, 2]}

# default calibration of the tuning engine (makamtuner.cpp): perde -> (up, down, stop) cents from
# the written equal-tempered note
CALIB = {
    "hicaz": {"B-4": (-83.33, -83.33, -83.33), "C4": (-16.67, -16.67, -16.67), "F4": (-33.33, -50.0, -33.33)},
    "huseyni": {"B-1": (-33.33, -50.0, -33.33), "F4": (-33.33, -50.0, -33.33)},
    "rast": {"B-1": (-16.67, -16.67, -16.67), "F4": (-16.67, -33.33, -16.67)},
    "ussak": {"B-1": (-50.0, -66.67, -50.0)},
}

AHENK = (-6, -10)          # Kız, 4 ses (makam::DEFAULT_AHENK)

PC = dict(C=0, D=2, E=4, F=5, G=7, A=9, B=11)
FIFTH = dict(F=-1, C=0, G=1, D=2, A=3, E=4, B=5)
LETTER = dict(Do="C", Re="D", Mi="E", Fa="F", Sol="G", La="A", Si="B")
SYM = {-1: "accidentalQuarterToneFlatStein", -4: "accidentalBakiyeFlat", -5: "accidentalFlat",
       -8: "accidentalBuyukMucennebFlat", 1: "accidentalQuarterToneSharpStein", 4: "accidentalSharp",
       5: "accidentalKucukMucennebSharp", 8: "accidentalBuyukMucennebSharp", 0: "accidentalNatural"}
BUILTIN = {-1: -50.0, -4: -44.0, -8: -89.0, 1: 50.0, 5: 56.0, 8: 89.0, 4: 0.0, -5: 0.0, 0: 0.0}
DUR = {6: ("16th", 0), 9: ("16th", 1), 12: ("eighth", 0), 18: ("eighth", 1), 24: ("quarter", 0),
       36: ("quarter", 1), 48: ("half", 0), 72: ("half", 1), 96: ("whole", 0)}


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def parse(path):
    meta, measures = {}, []
    for line in io.open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        m = re.match(r"#\s*(\w+):\s*(.*)", line)
        if m and not measures:
            meta.setdefault(m.group(1), m.group(2).strip())
            continue
        m = re.match(r"m(\d+)\s*(?:\[(.*?)\])?\s*:\s*(.*)", line)
        if not m:
            continue
        marks = {}
        for part in (m.group(2) or "").split(","):
            part = part.strip()
            if part:
                k, _, v = part.partition("=")
                marks[k.strip()] = v.strip() if v else True
        notes = []
        for tok in m.group(3).split():
            name, _, ln = tok.partition(":")
            ln = int(ln)
            if name == "Es":
                notes.append(None if False else {"rest": True, "len": ln})
                continue
            t = re.match(r"(Do|Re|Mi|Fa|Sol|La|Si)(\d)(?:([b#])(\d))?$", name)
            if not t:
                raise SystemExit("bilinmeyen nota: %s (m%s)" % (tok, m.group(1)))
            letter = LETTER[t.group(1)]
            koma = 0 if not t.group(3) else (-1 if t.group(3) == "b" else 1) * int(t.group(4))
            nat = (int(t.group(2)) + 1) * 12 + PC[letter]
            alter = 1 if koma == 4 else -1 if koma == -5 else 0
            notes.append({"rest": False, "len": ln, "letter": letter, "koma": koma, "nat": nat, "alter": alter,
                          "written": nat + alter, "tpc2": 14 + FIFTH[letter] + 7 * alter})
        measures.append({"no": int(m.group(1)), "marks": marks, "notes": notes})
    return meta, measures


def tune(notes, makam):
    """The tuning engine's default values (makamtuner.cpp, tuneScore) for one voice."""
    calib = CALIB.get(makam, {})
    seq = [n for n in notes if not n["rest"]]
    for n in seq:
        et = 4 if n["alter"] > 0 else -5 if n["alter"] < 0 else 0
        n["pos"] = n["written"] + (n["koma"] - et) / 10.0
    for i, n in enumerate(seq):
        perde = n["letter"] + str(n["koma"])
        builtin = BUILTIN.get(n["koma"], 0.0) if n.get("acc") else 0.0
        if n["len"] >= 48:
            d = 2
        else:
            d = None
            for j in range(i + 1, len(seq)):
                if abs(seq[j]["pos"] - n["pos"]) > 0.01:
                    d = 0 if seq[j]["pos"] > n["pos"] else 1
                    break
            if d is None:
                for j in range(i - 1, -1, -1):
                    if abs(seq[j]["pos"] - n["pos"]) > 0.01:
                        d = 0 if n["pos"] > seq[j]["pos"] else 1
                        break
            if d is None:
                d = 0
        et = 4 if n["alter"] > 0 else -5 if n["alter"] < 0 else 0
        if perde in calib:
            cents = calib[perde][d] - builtin
        elif n["koma"] != et:
            cents = n["koma"] * KOMA_CENT - n["alter"] * 100 - builtin
        else:
            cents = 0.0
        n["tuning"] = round(cents, 1)


def groups_xml(usul, n, d, ind):
    beams, breaks16 = BEAMS.get((n, d), []), []
    if usul in USUL_BEAMS and USULS[usul][1:3] == (n, d):
        beams, breaks16 = USUL_BEAMS[usul]
    actions, pos = {}, 0
    for g in beams[:-1]:
        pos += g * 4                      # 1/32
        actions[pos] = 0x111              # 8th, 16th and 32nd beams break
    for e in breaks16:
        actions.setdefault(e * 4, 0x110)  # only the 16th and 32nd beams
    nodes = sorted(actions.items())
    if usul:
        nodes.append((10000 + USULS[usul][0], 0))
    if not nodes:
        return ""
    x = ind + "<Groups>\n"
    for p, a in nodes:
        x += ind + '  <Node pos="%d" action="%d"/>\n' % (p, a)
    return x + ind + "  </Groups>\n"


def main():
    src, out = sys.argv[1], sys.argv[2]
    meta, measures = parse(src)
    makam = sys.argv[3] if len(sys.argv) > 3 else re.sub(r"[^a-z]", "", meta.get("makam", "").lower()
                                                         .translate(str.maketrans("çğıöşüâîû", "cgiosuaiu")))
    key = {}
    for p in meta.get("key", "").split():
        key[p[0]] = int(p[1:])

    # meters, usuls, accidentals per measure
    n, d, usul = 4, 4, None
    for m in measures:
        if "meter" in m["marks"]:
            n, d = map(int, m["marks"]["meter"].split("/"))
        if "usul" in m["marks"]:
            usul = m["marks"]["usul"]
            n, d = USULS[usul][1], USULS[usul][2]
            m["usulChange"] = True
        m["n"], m["d"], m["usul"] = n, d, usul
        total = sum(x["len"] for x in m["notes"])
        if total != n * 96 // d:
            raise SystemExit("m%d: süre %d/96, ölçü %d/%d = %d/96 olmalı" % (m["no"], total, n, d, n * 96 // d))
        shown = {}
        for x in m["notes"]:
            if x["rest"]:
                continue
            k = (x["letter"], x["nat"])
            expected = shown.get(k, key.get(x["letter"], 0))
            x["acc"] = x["koma"] != expected
            if x["acc"]:
                shown[k] = x["koma"]

    tune([x for m in measures for x in m["notes"]], makam)

    # key signature: written C, concert key moved by the ahenk (editahenk.cpp, writtenKeySigEvent)
    flats, sharps = "BEADGCF", "FCGDAEB"
    don = sorted(key.items(), key=lambda kv: (kv[1] > 0, (sharps if kv[1] > 0 else flats).index(kv[0])))
    keysig = "          <KeySig>\n            <concertKey>2</concertKey>\n            <actualKey>0</actualKey>\n"
    if don:
        keysig += "            <custom>1</custom>\n"
        for letter, k in don:
            keysig += ('            <CustDef>\n              <sym>%s</sym>\n              <def degree="%d" xAlt="0" octAlt="0"/>\n'
                       "              </CustDef>\n" % (SYM[k], "CDEFGAB".index(letter)))
    keysig += "            </KeySig>\n"

    title = meta.get("title", "Beste")
    composer = meta.get("beste", "")
    x = '<?xml version="1.0" encoding="UTF-8"?>\n<museScore version="4.70">\n  <Score>\n    <Division>480</Division>\n'
    x += '    <metaTag name="workTitle">%s</metaTag>\n    <metaTag name="composer">%s</metaTag>\n' % (esc(title), esc(composer))
    x += '    <metaTag name="source">%s</metaTag>\n' % esc(meta.get("source", ""))
    x += """    <Part>
      <Staff id="1">
        <StaffType group="pitched">
          <name>stdNormal</name>
          </StaffType>
        </Staff>
      <trackName>Türk Müziği</trackName>
      <Instrument id="turk-muzigi">
        <longName>Türk Müziği</longName>
        <shortName>T.M.</shortName>
        <trackName>Türk Müziği</trackName>
        <minPitchP>40</minPitchP>
        <maxPitchP>93</maxPitchP>
        <minPitchA>40</minPitchA>
        <maxPitchA>93</maxPitchA>
        <transposeDiatonic>%d</transposeDiatonic>
        <transposeChromatic>%d</transposeChromatic>
        <instrumentId>pluck.guitar.nylon-string</instrumentId>
        <Channel name="open">
          <program value="24"/>
          <synti>Fluid</synti>
          </Channel>
        </Instrument>
      </Part>
    <Part>
      <Staff id="2">
        <StaffType group="percussion">
          <name>perc2Line</name>
          <lines>2</lines>
          <lineDistance>4</lineDistance>
          </StaffType>
        <defaultClef>PERC</defaultClef>
        </Staff>
      <trackName>Usul</trackName>
      <Instrument id="usul">
        <longName>Usul</longName>
        <shortName>Usul</shortName>
        <trackName>Usul</trackName>
        <instrumentId>drum.group.set</instrumentId>
        <useDrumset>1</useDrumset>
        <Drum pitch="36">
          <head>normal</head>
          <line>0</line>
          <voice>0</voice>
          <name>Düm</name>
          <stem>1</stem>
          <shortcut>D</shortcut>
          </Drum>
        <Drum pitch="43">
          <head>normal</head>
          <line>2</line>
          <voice>0</voice>
          <name>Tek</name>
          <stem>2</stem>
          <shortcut>T</shortcut>
          </Drum>
        <Channel>
          <controller ctrl="0" value="1"/>
          <program value="0"/>
          <synti>Fluid</synti>
          </Channel>
        </Instrument>
      </Part>
""" % AHENK

    head = " ".join(p for p in (meta.get("makam", ""), meta.get("form", "")) if p)
    sub = " · ".join(p for p in (head, meta.get("usul", "")) if p)
    sub = meta.get("altbaslik", sub)  # TurkScore: a lesson may give its own subtitle
    x += "    <Staff id=\"1\">\n      <VBox>\n        <height>18</height>\n"
    x += "        <Text>\n          <style>title</style>\n          <text>%s</text>\n          </Text>\n" % esc(title)
    x += "        <Text>\n          <style>subtitle</style>\n          <text>%s</text>\n          </Text>\n" % esc(sub)
    if composer:
        x += "        <Text>\n          <style>composer</style>\n          <text>Beste: %s</text>\n          </Text>\n" % esc(composer)
    x += "        </VBox>\n"

    def measure_head(m, i):
        h = "      <Measure>\n"
        if m["marks"].get("startRepeat"):
            h += "        <startRepeat/>\n"
        if m["marks"].get("endRepeat"):
            h += "        <endRepeat>2</endRepeat>\n"
        return h

    def timesig(m, i, prev):
        s = ""
        if i == 0 or (m["n"], m["d"]) != (prev["n"], prev["d"]) or m.get("usulChange"):
            s += "          <TimeSig>\n            <sigN>%d</sigN>\n            <sigD>%d</sigD>\n" % (m["n"], m["d"])
            s += groups_xml(m["usul"], m["n"], m["d"], "            ")
            s += "            </TimeSig>\n"
        return s

    for i, m in enumerate(measures):
        prev = measures[i - 1] if i else None
        x += measure_head(m, i) + "        <voice>\n"
        if i == 0:
            x += keysig
        x += timesig(m, i, prev)
        if "tempo" in m["marks"]:
            bpm = int(m["marks"]["tempo"])
            # tempo = eighths per minute; a x/4 measure shows it in quarters
            sym, shown = ("metNoteQuarterUp", bpm // 2) if m["d"] == 4 else ("metNote8thUp", bpm)
            x += ("          <Tempo>\n            <tempo>%.6f</tempo>\n            <followText>1</followText>\n"
                  "            <text><sym>%s</sym> = %d</text>\n            </Tempo>\n" % (bpm / 120.0, sym, shown))
        if m.get("usulChange"):
            x += "          <SystemText>\n            <text>%s</text>\n            </SystemText>\n" % esc(m["usul"])
        if "section" in m["marks"]:
            x += "          <RehearsalMark>\n            <text>%s</text>\n            </RehearsalMark>\n" % esc(m["marks"]["section"])
        for nt in m["notes"]:
            t, dots = DUR[nt["len"]]
            if nt["rest"]:
                x += "          <Rest>\n" + ("            <dots>1</dots>\n" if dots else "") + \
                     "            <durationType>%s</durationType>\n            </Rest>\n" % t
                continue
            x += "          <Chord>\n" + ("            <dots>1</dots>\n" if dots else "")
            x += "            <durationType>%s</durationType>\n            <Note>\n" % t
            if nt["acc"]:
                x += "              <Accidental>\n                <role>1</role>\n                <subtype>%s</subtype>\n                </Accidental>\n" % SYM[nt["koma"]]
            x += "              <pitch>%d</pitch>\n              <tpc>%d</tpc>\n              <tpc2>%d</tpc2>\n" % (
                nt["written"] + AHENK[1], nt["tpc2"] + 2, nt["tpc2"])
            if nt["tuning"]:
                x += "              <tuning>%s</tuning>\n" % nt["tuning"]
            x += "              </Note>\n            </Chord>\n"
        if m["marks"].get("final"):
            x += "          <BarLine>\n            <subtype>end</subtype>\n            </BarLine>\n"
        x += "          </voice>\n        </Measure>\n"
    x += "      </Staff>\n"

    # usul staff
    x += "    <Staff id=\"2\">\n"
    for i, m in enumerate(measures):
        prev = measures[i - 1] if i else None
        x += measure_head(m, i) + "        <voice>\n"
        if i == 0:
            x += "          <KeySig>\n            <concertKey>0</concertKey>\n            </KeySig>\n"
        x += timesig(m, i, prev)
        u = USULS[m["usul"]] if m["usul"] else None
        if u and (u[1], u[2]) == (m["n"], m["d"]):
            for dum, ln in u[3]:
                t, dots = DUR[ln * 96 // u[2]]
                x += "          <Chord>\n" + ("            <dots>1</dots>\n" if dots else "")
                x += "            <durationType>%s</durationType>\n" % t
                x += "            <StemDirection>%s</StemDirection>\n            <Note>\n" % ("up" if dum else "down")
                x += "              <pitch>%d</pitch>\n              <tpc>%d</tpc>\n              </Note>\n            </Chord>\n" % (
                    36 if dum else 43, 14 if dum else 15)
        else:
            x += "          <Rest>\n            <durationType>measure</durationType>\n            <duration>%d/%d</duration>\n            </Rest>\n" % (m["n"], m["d"])
        if m["marks"].get("final"):
            x += "          <BarLine>\n            <subtype>end</subtype>\n            </BarLine>\n"
        x += "          </voice>\n        </Measure>\n"
    x += "      </Staff>\n    </Score>\n  </museScore>\n"
    io.open(out, "w", encoding="utf-8").write(x)
    print("%s: %d ölçü, makam %s" % (out, len(measures), makam))


if __name__ == "__main__":
    main()
