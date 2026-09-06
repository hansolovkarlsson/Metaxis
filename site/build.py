#!/usr/bin/env python3
"""build.py -- the website, generated from the documents the suite checks.

Every page under site/out/ is rendered from a markdown file in docs/ (or from
site/index.md, or from examples/ and their recorded outputs). Nothing is
written for the site alone, so the site cannot say something the tree does
not: tests/docs.sh runs every transcript in docs/, and this only renders them.

    python3 site/build.py            # writes site/out/
"""
import re, html, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'site', 'out')
GH = 'https://github.com/hansolovkarlsson/Metaxis'

PAGES = [  # (output name, nav title, source)
    ('index',     'Introduction', 'site/index.md'),
    ('tutorial',  'Tutorial',     'docs/tutorial.md'),
    ('glossary',  'Glossary',     'docs/glossary.md'),
    ('reference', 'Reference',    'docs/REFERENCE.md'),
    ('examples',  'Examples',     None),
    ('notation',  'Notation',     'docs/notation.md'),
    ('direction', 'Direction',    'docs/direction.md'),
    ('prior-art', 'Prior art',    'docs/prior-art.md'),
    ('languages', 'Languages',    'docs/languages.md'),
    ('changelog', 'Changelog',    'docs/CHANGELOG.md'),
    ('roadmap',   'Roadmap',      'docs/ROADMAP.md'),
]
LINKMAP = {'REFERENCE.md': 'reference.html', 'tutorial.md': 'tutorial.html',
           'glossary.md': 'glossary.html', 'notation.md': 'notation.html',
           'direction.md': 'direction.html', 'prior-art.md': 'prior-art.html',
           'languages.md': 'languages.html',
           'CHANGELOG.md': 'changelog.html', 'ROADMAP.md': 'roadmap.html',
           'COMPLETED.md': GH + '/blob/main/docs/COMPLETED.md',
           'POSTMORTEM.md': GH + '/blob/main/docs/POSTMORTEM.md',
           'work-journal/': GH + '/tree/main/docs/work-journal',
           'README.md': 'index.html'}

KW = r'\b(emit|if|else|for|in|sep|not|and|or|contribute|terminated|override|as|eol|indent|right|left|join|matched|count|at|num|level|group|replace|drop|fresh|splice|expression|text)\b'

def esc(s): return html.escape(s, quote=False)

def slug(s):
    s = re.sub(r'<[^>]+>', '', s)
    s = re.sub(r'[`*_]', '', s).lower()
    s = re.sub(r'[^\w\s-]', '', s, flags=re.UNICODE)
    return re.sub(r'[\s]+', '-', s.strip())

def link(target):
    if target.startswith('#') or target.startswith('http'): return target
    base, _, anchor = target.partition('#')
    for k, v in LINKMAP.items():
        if base == k or base.endswith('/' + k):
            return v + ('#' + anchor if anchor else '')
    if base.startswith('examples/') or base.startswith('lib/') or base.startswith('tests/') or base.startswith('.github') or base.startswith('docs/'):
        return GH + '/blob/main/' + base
    return target

def inline(t):
    t = esc(t)
    codes = []
    def stash(m):
        codes.append('<code>' + m.group(1) + '</code>'); return '\x00%d\x00' % (len(codes) - 1)
    t = re.sub(r'`([^`]+)`', stash, t)
    t = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', t)
    t = re.sub(r'(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])', r'<em>\1</em>', t)
    t = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', lambda m: '<a href="' + link(m.group(2)) + '">' + m.group(1) + '</a>', t)
    return re.sub(r'\x00(\d+)\x00', lambda m: codes[int(m.group(1))], t)

def hl_header_line(line):
    if line.lstrip().startswith(';'):
        return '<span class="cm">' + esc(line) + '</span>'
    out = []; i = 0
    while i < len(line):
        c = line[i]
        if c == '"':
            j = i + 1
            while j < len(line):
                if line[j] == '\\': j += 2; continue
                if line[j] == '"': break
                j += 1
            out.append('<span class="st">' + esc(line[i:j+1]) + '</span>'); i = j + 1; continue
        if c == '@' and (i == 0 or line[i-1] == ' '):
            m = re.match(r'@\w+', line[i:])
            if m: out.append('<span class="dr">' + esc(m.group()) + '</span>'); i += len(m.group()); continue
        if line.startswith('=>', i): out.append('<span class="kw">=&gt;</span>'); i += 2; continue
        if c in '[]{}': out.append('<span class="kw">' + c + '</span>'); i += 1; continue
        if c == ';': out.append('<span class="cm">' + esc(line[i:]) + '</span>'); break
        m = re.match(KW, line[i:])
        if m and (i == 0 or not (line[i-1].isalnum() or line[i-1] == '_')):
            out.append('<span class="kw">' + m.group() + '</span>'); i += len(m.group()); continue
        out.append(esc(c)); i += 1
    return ''.join(out)

def render_code(body, label=None, lang=''):
    lines = body.split('\n')
    while lines and lines[-1] == '': lines.pop()
    if lines and lines[0].startswith('$ '):
        rows = []
        for l in lines:
            if l.startswith('$ '): rows.append('<span class="pr"><span class="ps">$</span> ' + esc(l[2:]) + '</span>')
            elif l.startswith('mx: '): rows.append('<span class="er">' + esc(l) + '</span>')
            else: rows.append(esc(l))
        return '<figure class="tx"><figcaption>run</figcaption><pre>' + '\n'.join(rows) + '</pre></figure>'
    is_mx = lang == 'mx' or (lang == '' and any(l.startswith('@') for l in lines))
    cap = f'<figcaption>{esc(label)}</figcaption>' if label else ''
    if not is_mx:
        return f'<figure class="cd">{cap}<pre>' + esc('\n'.join(lines)) + '</pre></figure>'
    rows = []; in_body = False
    has_end = any(l.strip() == '@end' for l in lines)
    for l in lines:
        if in_body: rows.append('<span class="bd">' + esc(l) + '</span>')
        else:
            rows.append(hl_header_line(l))
            if l.strip() == '@end': in_body = True
    return f'<figure class="{"mx split" if has_end else "mx"}">{cap}<pre>' + '\n'.join(rows) + '</pre></figure>'

def render_md(src):
    out = []; toc = []; lines = src.split('\n'); i = 0; para = []; pending_label = None; title = ''
    in_terms = False   # a section whose bold-led paragraphs are entries: the glossary's Part two
    def flush():
        nonlocal para
        if not para: return
        text = ' '.join(para)
        m = re.match(r'\*\*(.+?)\.?\*\*', text) if in_terms else None
        if m:
            term = m.group(1); anchor = 'term-' + slug(term)
            toc.append(('', inline(term), anchor, 3))
            out.append(f'<p class="term" id="{anchor}">' + inline(text) + '</p>')
        else:
            out.append('<p>' + inline(text) + '</p>')
        para = []
    while i < len(lines):
        l = lines[i]
        if l.startswith('```'):
            flush(); lang = l[3:].strip(); j = i + 1; buf = []
            while j < len(lines) and not lines[j].startswith('```'): buf.append(lines[j]); j += 1
            out.append(render_code('\n'.join(buf) + '\n', pending_label, lang)); pending_label = None
            i = j + 1; continue
        if l.startswith('# '):
            flush(); title = l[2:]; i += 1; continue
        m = re.match(r'^(#{2,4}) (.*)', l)
        if m:
            flush(); depth = len(m.group(1)); text = m.group(2)
            mm = re.match(r'(\d+(?:\.\d+)?) · (.*)', text)
            num, name = (mm.group(1), mm.group(2)) if mm else ('', text)
            anchor = slug(text)
            if depth in (2, 3): toc.append((num, inline(name), anchor, depth))
            if depth == 2: in_terms = name.startswith('Part two')
            numspan = f'<span class="num">{num}</span>' if num else ''
            out.append(f'<h{depth} id="{anchor}">{numspan}<span class="t">{inline(name)}</span></h{depth}>')
            i += 1; continue
        if l.startswith('---'): flush(); i += 1; continue
        if l.startswith('> '):
            flush(); buf = []
            while i < len(lines) and lines[i].startswith('>'): buf.append(lines[i][1:].strip()); i += 1
            out.append('<blockquote><p>' + inline(' '.join(buf)) + '</p></blockquote>'); continue
        if l.startswith('|'):
            flush(); rows = []
            while i < len(lines) and lines[i].startswith('|'): rows.append(lines[i]); i += 1
            cells = lambda r: [c.strip().replace('\\|', '|') for c in re.split(r'(?<!\\)\|', r.strip())[1:-1]]
            head = cells(rows[0]); body = [cells(r) for r in rows[2:]]
            t = '<div class="tw"><table><thead><tr>' + ''.join('<th>' + inline(c) + '</th>' for c in head) + '</tr></thead><tbody>'
            for r in body: t += '<tr>' + ''.join('<td>' + inline(c) + '</td>' for c in r) + '</tr>'
            out.append(t + '</tbody></table></div>'); continue
        m = re.match(r'^(\s*)(-|\d+\.) (.*)', l)
        if m:
            flush(); ordered = m.group(2) != '-'; items = []
            while i < len(lines):
                mm = re.match(r'^(-|\d+\.) (.*)', lines[i])
                if mm: items.append(mm.group(2)); i += 1
                elif lines[i].startswith('  ') and items: items[-1] += ' ' + lines[i].strip(); i += 1
                else: break
            tag = 'ol' if ordered else 'ul'
            out.append(f'<{tag}>' + ''.join('<li>' + inline(x) + '</li>' for x in items) + f'</{tag}>'); continue
        if l.startswith('*') and not l.startswith('**') and i < 20 and not out:
            flush(); buf = [l]; i += 1
            while i < len(lines) and lines[i].strip() and not lines[i].startswith('*'): buf.append(lines[i]); i += 1
            t = ' '.join(buf).strip().strip('*').strip()
            out.append('<p class="note">' + inline(t) + '</p>'); continue
        if l.strip() == '': flush(); i += 1; continue
        m = re.match(r'`([\w./-]+\.mx)`(?:, whole)?:$', l.strip())
        if m: flush(); pending_label = m.group(1); i += 1; continue
        para.append(l.strip()); i += 1
    flush()
    return title, ''.join(out), toc

def examples_page():
    readme = open(os.path.join(ROOT, 'README.md')).read()
    rows = re.findall(r'^\| \[([\w.-]+)\]\(examples/[\w.-]+\) \| (.*) \|$', readme, re.M)
    out = ['<p class="note">Every file here is run by <code>make check</code> against the output recorded beside it, and five of them are compiled and executed. The descriptions are the README\'s; the sources and outputs are the files in the tree, exactly.</p>']
    toc = []
    for name, desc in rows:
        base = name[:-3]
        src = open(os.path.join(ROOT, 'examples', name)).read()
        anchor = slug(base)
        toc.append(('', base, anchor, 2))
        out.append(f'<h2 id="{anchor}"><span class="t">{esc(base)}</span></h2><p>' + inline(desc) + '</p>')
        out.append(render_code(src, 'examples/' + name, 'mx'))
        for outname in sorted(f for f in os.listdir(os.path.join(ROOT, 'examples')) if f.startswith(base + '.out') or f.startswith(base + '-') and f.endswith('.out')):
            got = open(os.path.join(ROOT, 'examples', outname)).read()
            out.append(render_code(got, 'examples/' + outname + ('' if outname == base + '.out' else '  (mx -b ' + outname[len(base)+1:-4] + ')'), 'txt'))
    return 'The examples', ''.join(out), toc

CSS = open(os.path.join(ROOT, 'site', 'style.css')).read()

def page(name, navtitle, title, body, toc):
    nav = ''.join(f'<a href="{n}.html"{" class=\"here\"" if n == name else ""}>{t}</a>' for n, t, _ in PAGES)
    rail = ''
    if toc and len(toc) > 2:
        rail = '<nav class="rail" aria-label="On this page"><p class="eyebrow">On this page</p><ol>' + ''.join(
            f'<li class="d{d}"><a href="#{a}">' + (f'<span class="num">{n}</span>' if n else '') + f'<span class="t">{t}</span></a></li>' for n, t, a, d in toc) + '</ol></nav>'
    return f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{esc(re.sub(r"<[^>]+>", "", title))} · Metaxis</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:ital,wght@0,400;0,500;1,400&family=IBM+Plex+Sans:wght@500;600&family=IBM+Plex+Serif:ital,wght@0,400;0,600;1,400&display=swap">
<link rel="icon" type="image/png" href="logo.png">
<style>{CSS}</style></head>
<body><header class="top"><a class="brand" href="index.html"><img class="mark" src="logo.png" alt="">Metaxis</a><nav class="site" aria-label="Site">{nav}<a href="{GH}">GitHub</a></nav></header>
<div class="wrap{"" if rail else " norail"}"><header class="masthead{" hero" if name == "index" else ""}">{"<img class=\"hero-mark\" src=\"logo.png\" alt=\"The Metaxis mark: a solid block and a wireframe cube, interlocked\">" if name == "index" else ""}<div><p class="eyebrow">{esc(navtitle)}</p><h1>{title}</h1></div></header>{rail}<main>{body}</main></div>
<footer class="foot"><p>Generated from the documents in the tree by <code>site/build.py</code>. Every transcript on these pages is run by the test suite.</p></footer></body></html>'''

def main():
    os.makedirs(OUT, exist_ok=True)
    import shutil
    shutil.copy(os.path.join(ROOT, 'site', 'logo.png'), os.path.join(OUT, 'logo.png'))
    for name, navtitle, srcpath in PAGES:
        if srcpath is None: title, body, toc = examples_page()
        else:
            text = open(os.path.join(ROOT, srcpath)).read()
            # The marker says, in the source, why an em dash stays; the page
            # does not need telling. See tests/hygiene.sh, the prose rule.
            text = text.replace('<!-- as written -->', '')
            title, body, toc = render_md(text)
            title = inline(title)
        open(os.path.join(OUT, name + '.html'), 'w').write(page(name, navtitle, title, body, toc))
        print(f'{name}.html  {os.path.getsize(os.path.join(OUT, name + ".html")) // 1024} KB')

if __name__ == '__main__': main()
