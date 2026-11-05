"""Generates the C++ makam table of makamtuner.cpp from makams.json (dumped from Makam.qml)."""
import json, sys

d = json.load(open(sys.argv[1], encoding='utf-8'))


def cstr(s, utf8=False):
    assert utf8 or all(ord(c) < 128 for c in s), s
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def num(x):
    s = ('%.2f' % x).rstrip('0').rstrip('.')
    return s if '.' in s else s + '.0'


lines = []
for m in d['makams']:
    keys = ', '.join(cstr(k) for k in m['keys'])
    calib = ', '.join('{ %s, { %s, %s, %s } }' % (cstr(p), num(v[0]), num(v[1]), num(v[2]))
                      for p, v in m['calib'].items())
    ksl = [m['ks']] + m['alt']
    ks = ', '.join('{ ' + ', '.join(cstr(x) for x in lst) + ' }' for lst in ksl)
    lines.append('        { %s, %s,\n          { %s },\n          { %s },\n          { %s },\n          %s, %s },'
                 % (cstr(m['id']), cstr(m['name'], True), keys, calib, ks, cstr(m['karar']), num(m['seyir'])))
open(sys.argv[2], 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
