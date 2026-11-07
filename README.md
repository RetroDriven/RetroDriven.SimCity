# SimCity for Analogue Pocket

![SimCity](banner.png)

![Version](https://img.shields.io/badge/version-1.0.0-green) ![Platform](https://img.shields.io/badge/platform-Analogue%20Pocket-blue)

A native Pocket port of **SimCity Classic MS-DOS 2.00**, with the original MCGA presentation and simulation adapted for a controller. This is a buildable reverse-engineered reconstruction, not recovered Maxis source. It is an independent project.

## Pocket features

- **Placement preview:** an optional yellow footprint with tile divisions shows where the selected building or tool will land; enabled by default.
- **Controller pointer:** precise single-pixel taps and accelerating held movement, with access to menus, toolbar, reports and map controls.
- **Map scrolling:** automatic scrolling at map edges, plus manual B + D-pad panning.
- **More map space:** the top menu auto-hides to expose two extra tile rows; Select reveals it and moves the hand onto the first menu item.
- **City naming:** a themed on-screen keyboard, with physical keyboard and mouse support.
- **Display and audio options:** saved 4:3 / 5:4 view modes, separate music and sound-effects switches, and live volume control.
- **Original game systems:** 15 construction tools, terrain editing, eight scenarios, budgets, disasters, maps/graphs, six graphics themes and six city-save slots.

## Install

1. Download and Extract the latest Release to the SD card root, merging `Cores`, `Platforms` and `Assets`.
2. Download and run `SimCityAssetImporter.exe`.
3. Select your DOS game folder or ZIP and the SD root. Choose **Convert and Install Assets**.
4. Safely eject and start **SimCity**.

**Required game data:** your own **SimCity Classic MS-DOS 2.00 (1994, MCGA)** installation, including `SIMCITY.EXE` and the required files from `DATA`, `MCGA` and `SCENARIO`. Windows Classic and other revisions are not interchangeable. **[See the exact 35-file checklist and ten generated Pocket files below]**

The importer supports the recorded DOS Classic 2.00/1994 release only. It validates 35 inputs and verifies all ten converted outputs. It works offline, leaves originals and saves untouched, and backs up replaced assets.

Firmware 2.2+ is required by the core metadata. Identity: `Cores/RetroDriven.SimCity`, `Platforms/simcity.json`, `Assets/simcity`. Saves use `Saves/simcity/common/SimCity_0.sav` through `SimCity_5.sav`.

## Required game files

You need **SimCity Classic for MS-DOS, version 2.00 (1994, MCGA)**. Supply all **35 original files** below to the Windows importer, keeping the folder structure intact.

| Location in your DOS game folder | Required filenames | Count |
| --- | --- | ---: |
| Game folder root | `SIMCITY.EXE` | 1 |
| `DATA/` | `ASIA_MSG.PTF`<br>`BULLLOOP.VOC`<br>`COPTER.VOC`<br>`EXPLODE.VOC`<br>`FEUR_MSG.PTF`<br>`FUSA_MSG.PTF`<br>`HONK.VOC`<br>`MANGO.VOC`<br>`MEDI_MSG.PTF`<br>`MESSAGE.PTF`<br>`MOON_MSG.PTF`<br>`PLOP.VOC`<br>`POLICE.VOC`<br>`SCTUNE.XMI`<br>`TOOTIT.VOC`<br>`WEST_MSG.PTF` | 16 |
| `MCGA/` | `ASIAMCGA.PGF`<br>`FEURMCGA.PGF`<br>`FUSAMCGA.PGF`<br>`MCGADAT.PGF`<br>`MCGANTRO.PPF`<br>`MCGASCEN.PPF`<br>`MCGATBAR.PGF`<br>`MEDIMCGA.PGF`<br>`MOONMCGA.PGF`<br>`WESTMCGA.PGF` | 10 |
| `SCENARIO/` | `BERN.PSN`<br>`BOSTON.PSN`<br>`DETROIT.PSN`<br>`DULLSVIL.PSN`<br>`HAMBURG.PSN`<br>`RIO.PSN`<br>`SANFRAN.PSN`<br>`TOKYO.PSN` | 8 |

Select the game folder or ZIP in **SimCityAssetImporter.exe**, then select your Pocket SD card root and choose **Convert and Install Assets**. The importer checks the contents of every file; the filenames alone do not establish compatibility. Windows Classic and other DOS revisions are not supported.

The importer creates these **10 files** in `Assets/simcity/common/`:

| Graphics and interface | Game data | Scenarios and audio |
| --- | --- | --- |
| `simgfx.pgf` | `simcode.dat` | `scenarios.dat` |
| `simtitle.ppf` | `messages.dat` | `simaudio.dat` |
| `simscen.ppf` | `simthemes.dat` | |
| `cursor.dat` | | |
| `terrainbar.dat` | | |

## Controls

| Control | Action |
| --- | --- |
| D-pad | Move hand; one-pixel taps, accelerating while held |
| A | Click/place; hold to paint roads, wires, rails or bulldoze |
| X / Y | Next / previous construction tool |
| L / R | Bulldozer / road |
| Select | Open System and move hand onto the first item |
| B tap / Start | Open menu; B also closes dialogs or returns |
| B + D-pad | Pan without moving the hand |
| Hold into map edge | After 250 ms, scroll automatically; release stops |
| Release and press again at edge | Cross into the surrounding UI |

The menu auto-hides; the top edge reveals it. The enlarged map has two extra tile rows. Query is the toolbar `+`: hold A to inspect and release to close. In Maps, X/Y switches Density/Growth or Police/Fire for the selected layer. Terrain editor: select brush/Fill, A paints, L Undo, R Smooth, B returns.

City-name keyboard: D-pad selects keys, A types, X Backspace, Y case, L/R text caret, Start Done, B Cancel. Mouse and physical keyboard also work.

## Core Settings

Open the Pocket menu, then **Core Settings**. Settings persist between launches.

| Setting | Choices | Behavior |
| --- | --- | --- |
| View Mode | Original 4:3 / Pocket 5:4 | Choose the display aspect ratio. |
| Placement Preview | On / Off; default On | Yellow footprint with tile divisions before placement. Shows area, not placement validity. |
| Music | On / Off; default On | Master switch for music; in-game Options preferences still apply. |
| Sound Effects | On / Off; default On | Independent master switch for effects; in-game Options preferences still apply. |
| Core Volume | 100%, 75%, 50%, 25%, Mute; default 100% | Changes music and effects volume live. |

## Features and limitations

All 15 construction tools, terrain generation/editor, eight scenarios, budgets, disasters, reports, six graphics themes, window controls and six DOS-format city slots are integrated. Music uses a licensed **GeneralUser GS** adaptation, not original AdLib synthesis. The desktop development audio backend is silent. DOS printer/card/setup utilities are outside Pocket scope. Finite automated comparisons do not establish every city/branch or exhaustive reverse engineering.

Save before leaving the core. 

No original game data, private captures or test cities belong in a public source commit. Reconstructed code, supplied branding and prebuilt runtime dependencies require the review described in LICENSING.md. Asset separation is not a declaration of legal clearance.

[Credits](CREDITS.md)
[Licensing](LICENSING.md)	