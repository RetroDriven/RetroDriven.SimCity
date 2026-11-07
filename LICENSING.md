# Licensing and distribution review

## Original game material

Original executable, graphics, cursor, messages, music, effects, scenarios and converted copies retain their owners' rights. The importer requires the user's DOS 2.00 files and never downloads them. `simcode.dat` is an unchanged copy of the original executable, not independently redistributable new code.

## SDK/runtime

ThinkElastic-authored SDK source is Apache-2.0; musl is MIT. Preserve the included SDK NOTICE and licences. Prebuilt OS/FPGA binaries also incorporate Analogue Pocket Framework, Intel IP and Analogizer/MiSTer RTL under separate terms. Review redistribution terms and any corresponding-source obligations for the actual shipped runtime before publication.

- SDK: https://github.com/openfpgaOS/openfpgaSDK/tree/a408ddc12aed0dfaa4aa22c06af82f829db77126
- Runtime source revision recorded in SDK manifest: https://github.com/openfpgaOS/openfpgaOS/tree/618a3eb

## Music bank

Packages use **GeneralUser GS 2.0.3** by S. Christian Collins, converted to OFSF v3 at 22050 Hz and restricted to the supported soundtrack's programs. Its License v2.0 permits software-project use/modification and includes sample-provenance caveats. The full licence and conversion provenance are retained. The SDK's SC-55-derived development bank is not in the release packages.

- https://www.schristiancollins.com/generaluser.php
- https://github.com/mrbumpy409/GeneralUser-GS

## Windows importer

The EXE bundles CPython and Tcl/Tk under their respective licences; PyInstaller uses its GPL bootloader exception. Included notices apply. The EXE embeds converter code and hash metadata, not original game assets.