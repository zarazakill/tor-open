#!/usr/bin/env python3
"""
fix_compile.py — исправляет ошибки компиляции в mainwindow.cpp и mainwindow.h
Запуск: python3 fix_compile.py /путь/к/mainwindow.cpp /путь/к/mainwindow.h
"""
import sys, re

def fix_mainwindow_cpp(path):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    changed = False

    # ── 1. Убираем дублирующиеся определения resolve*-методов ──────────────
    # Находим ВСЕ вхождения каждого определения и оставляем только первое
    for pattern in [
        r'(// ─+[^\n]*поиск ta\.key[^\n]*\n[^\n]*easy-rsa[^\n]*\n(?:.*\n)*?^QString MainWindow::resolveTaKeyPath\(\)\n\{[^}]*?\})',
        r'(QString MainWindow::resolveTaKeyPath\(\)\s*\{[^}]+?\})',
        r'(QString MainWindow::resolveClientCertPath\(const QString &cn\) const\s*\{[^}]+?\})',
        r'(QString MainWindow::resolveClientKeyPath\(const QString &cn\) const\s*\{[^}]+?\})',
    ]:
        matches = list(re.finditer(pattern, content, re.MULTILINE | re.DOTALL))
        if len(matches) > 1:
            # Удаляем все кроме первого
            for m in reversed(matches[1:]):
                content = content[:m.start()] + content[m.end():]
                print(f"  Removed duplicate: {m.group()[:60].strip()}...")
                changed = True

    # ── 2. resolveTaKeyPath() — убедиться что возвращает QString ───────────
    old = 'void MainWindow::resolveTaKeyPath()'
    new = 'QString MainWindow::resolveTaKeyPath()'
    if old in content:
        content = content.replace(old, new)
        print(f"  Fixed: {old} -> {new}")
        changed = True

    # ── 3. Тело resolveTaKeyPath должно возвращать QString ─────────────────
    # Если тело было сгенерировано как void (без return), исправим
    body_pattern = r'(QString MainWindow::resolveTaKeyPath\(\)\s*\{)(.*?)(^\})'
    def fix_body(m):
        body = m.group(2)
        if 'return' not in body:
            # Добавляем return taKeyPath в конец
            body = body.rstrip() + '\n    return taKeyPath;\n'
            print("  Fixed: added return to resolveTaKeyPath body")
        return m.group(1) + body + m.group(3)
    content, n = re.subn(body_pattern, fix_body, content, flags=re.MULTILINE|re.DOTALL)
    if n: changed = True

    # ── 4. Вызовы resolveTaKeyPath() как void там где нужен QString ─────────
    # "resolveTaKeyPath(); // Найти ta.key..." -> просто вызов, не присваивание — OK
    # "resolveTaKeyPath();" на отдельной строке тоже OK (side effect — обновляет taKeyPath)
    # Проблема только если: void f() {...} а вызывается как QString x = resolveTaKeyPath()
    # Это уже исправлено выше (изменили возвращаемый тип на QString)

    if changed:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"✅ {path} fixed")
    else:
        print(f"ℹ️  {path} — no changes needed")

def fix_mainwindow_h(path):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    changed = False

    # Убедиться что resolveTaKeyPath объявлен как QString, не void
    if 'void resolveTaKeyPath()' in content:
        content = content.replace('void resolveTaKeyPath()', 'QString resolveTaKeyPath()')
        print(f"  Fixed header: void -> QString resolveTaKeyPath()")
        changed = True

    # Убрать дублирующиеся объявления
    for decl in [
        'QString resolveTaKeyPath();',
        'QString resolveClientCertPath(const QString &cn) const;',
        'QString resolveClientKeyPath(const QString &cn) const;',
        'QString sanitizeClientName(const QString &name);',
    ]:
        occurrences = [i for i in range(len(content)) if content[i:i+len(decl)] == decl]
        if len(occurrences) > 1:
            # Удаляем дубли (оставляем первое)
            for pos in reversed(occurrences[1:]):
                # Удаляем строку целиком (включая пробелы и \n)
                line_start = content.rfind('\n', 0, pos) + 1
                line_end = content.find('\n', pos) + 1
                content = content[:line_start] + content[line_end:]
                print(f"  Removed duplicate declaration: {decl}")
                changed = True

    if changed:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"✅ {path} fixed")
    else:
        print(f"ℹ️  {path} — no changes needed")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 fix_compile.py mainwindow.cpp mainwindow.h")
        sys.exit(1)
    print("Fixing compilation errors...")
    fix_mainwindow_cpp(sys.argv[1])
    fix_mainwindow_h(sys.argv[2])
    print("Done. Try building again.")
