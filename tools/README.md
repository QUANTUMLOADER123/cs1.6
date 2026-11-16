# Tools — Ghidra для реверса

## Что здесь

- `download_ghidra.sh` — скрипт загрузки официального релиза Ghidra с GitHub.
- Бинарный ZIP (~570 МБ) и распакованная Ghidra (~1+ ГБ) в репозиторий не коммитятся — см. `.gitignore`.

## Почему бинарник не скачался прямо в песочнице

Песочница Arena разрешает исходящий трафик только на:
`github.com`, `codeload.github.com`, `api.github.com`, `registry.npmjs.org`, `pypi.org`, `files.pythonhosted.org`.

Официальные релизы Ghidra лежат на `release-assets.githubusercontent.com` — этот хост в allowlist отсутствует, поэтому `curl`/`gh release download` падают с `SSL_ERROR_SYSCALL` / `EOF`.

Что удалось сделать внутри песочницы:
- `git clone --depth 1 --branch Ghidra_12.2_build https://github.com/NationalSecurityAgency/ghidra.git` — исходники Ghidra 12.2 успешно склонированы (проверено, ~440 МБ), затем удалены, чтобы не превышать лимит снапшота (~128 МБ).

## Как получить рабочий Ghidra локально

1. Установи JDK 21+ 64-bit (для 12.2 — JDK 25, см. README Ghidra).
2. Скачай релиз вручную:
   https://github.com/NationalSecurityAgency/ghidra/releases — файл `ghidra_12.2_PUBLIC_20261016.zip`
3. Или запусти скрипт на машине с полным интернетом:

```bash
chmod +x tools/download_ghidra.sh
./tools/download_ghidra.sh
```

4. Запуск: `./ghidra_12.2_PUBLIC/ghidraRun`

## Сборка из исходников (альтернатива)

```bash
git clone --depth 1 --branch Ghidra_12.2_build https://github.com/NationalSecurityAgency/ghidra.git
cd ghidra
./gradlew buildGhidra -x test
```

Требуется JDK + Gradle, собирается долго, но работает без доступа к release-assets.
