# wx-stock

Supplementary `wxstd.mo` catalogs — wxWidgets' own translations for its stock
strings (generic dialog buttons like "OK"/"Cancel", common file-picker text,
etc.), as opposed to `i18n/<lang>.po`, which are this project's own
`pgadmin3` UI translations.

Only 11 of the 45 shipped languages ship a `wxstd.mo` in `x64/Release/i18n/`
today (`ca_ES`, `cs_CZ`, `de_DE`, `es_ES`, `fr_FR`, `ja_JP`, `lv_LV`, `pl_PL`,
`ru_RU`, `sr_RS`, `zh_CN`). This directory vendors the missing 34, sourced
from the original [pgadmin-org/pgadmin3](https://github.com/pgadmin-org/pgadmin3)
project (same PostgreSQL Licence, same project lineage). They may lag behind
the exact wxWidgets version this project builds against, but wx's stock
strings (button labels, common dialog text) rarely change, so they're a
strict improvement over shipping no translation at all for these languages.

`windows/package_release.sh` layers these on top of `x64/Release/i18n/`
when assembling the Windows zip, filling in only the languages that don't
already have a `wxstd.mo`. Nothing here is copied into `x64/Release/` itself
— that folder stays exactly as committed.
