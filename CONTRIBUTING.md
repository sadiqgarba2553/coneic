# Contributing

Keep upstream license/credit notices and identify the exact VLC baseline. Prefer changes to Coneic presentation/build files. Do not drop modules, APIs, options or advanced dialogs to make a build pass.

Run the source audit and static package comparison, then test the actual Windows executable. Reconfigure and invalidate affected objects when feature flags change. Document negative tests and device/network limits; a successful compilation or plugin filename is not runtime proof.

See docs/BUILD.md and docs/VALIDATION.md for commands and the open embedding scenario. Generated portable binaries and local validation output are excluded from source commits; use release assets for binaries.
