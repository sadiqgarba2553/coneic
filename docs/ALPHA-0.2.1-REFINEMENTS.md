# Coneic 0.2.1 Alpha — local refinements

This is a second local build of the same Alpha version. It must not be pushed or published without the owner's explicit instruction. Existing Moment Cards, Study Mode and Smart Collections remain available.

## Media relinking and library backup

Open **Your space → Library tools**. Missing local media appears in a list. Select an entry and choose **Locate selected file**. Confirm the replacement to update saved notes, moments, tags and the media reference. Neither source nor replacement media is edited. Old references are recorded as aliases so Coneic's personal library can find the replacement.

**Create backup** exports a portable JSON file containing library references, notes, moments, tags, collection rules, viewing metadata and Home choices. The actual videos, audio files and VLC's general configuration are not included.

**Restore backup** merges records into the existing library. Existing metadata is retained, tags are combined, and different notes with the same identifier are kept separately. Repeating the same restore does not duplicate identical records. A recovery snapshot is written next to the active personal-library file before restoration. Invalid or unsupported backup formats are refused. Restoration also refuses to overwrite an unreadable or structurally invalid active library; preserve that file before manual recovery.

## Command palette and shortcuts

Use the search icon in the header, the main menu, or **Ctrl+Shift+P**. Search commands, saved media, moments and native VLC settings. Use the arrow keys to choose a result and Enter to run it. Selecting a setting opens the real preferences page and focuses the matching option search. The palette sizes itself to its results rather than keeping a large empty panel.

| Shortcut | Action |
| --- | --- |
| Ctrl+Shift+P | Command palette |
| Ctrl+Shift+M | Save the current moment |
| Ctrl+Shift+N | Open Study notes |
| Ctrl+Shift+H | Switch the main window between library and player |

Existing VLC playback keys remain in place. A new Coneic shortcut is not assigned when the same sequence is already configured for a VLC key. Search **Keyboard shortcuts** in the palette to inspect assigned Coneic keys and the current playback keys. Shortcuts are scoped to the Coneic application, not registered as operating-system global hotkeys.

## Customisable Home

Use the customise icon on Home or search **Customise Home** in the palette. Check the sections to show, drag them into order (or use Move up/down), and choose collections to pin. Save applies the layout; Restore Defaults returns to the initial layout. Hiding sections never removes their files or notes. Saved moments and pinned collections open their matching personal-library destinations.

## Control bar and company credit

The transport retains native VLC action handlers but uses a wider orange play/pause target, larger vector icons, clearer spacing and a simple volume track. The seek row remains below the transport. An earlier default toolbar layout is migrated for display; explicitly customised toolbar arrangements are retained. Stop remains available through the existing playback key, original menus and the command palette.

Settings includes a clickable **Made by BLUECLOUD** credit linking to https://bluecloudai.online. Orange accents, the supplied logo and the embedded Inter typeface are retained. The requested Apple design guidance informs restraint, immediate feedback, hierarchy, spacing and keyboard focus; native Windows/Qt interaction is preserved.

## Limits and validation

The source remains a VLC 3.0.24 fork. Existing device, codec, standalone embedding and display-coverage limits remain documented in VALIDATION.md. Backups refer to file paths; media must be available or relinked on the destination PC. These tools do not add a cloud service or upload library data.

Native normal/high-DPI evidence, backup/relink regression results, source/package checks and actual screenshots accompany this local build. Checks exercise real native settings navigation, library edits, repeat restoration, invalid-input protection, Home choices, shortcut registration, original playback and the selected personal features. Bounded tests do not certify every media file, device or display.

### Observed results

Both normal and 200% native runs exited with code 0. Each passed 23 native interface checks (including the personal-feature aggregate), 19 persistence checks, 16 live personal/refinement checks and 14 library-tool regression checks. Source and package comparisons retained all baseline declarations and 360 plugins, with no protected-scope flags or unresolved imports. Machine-readable evidence and actual captures are in the adjacent alpha-0.2.1-r2 evidence/screenshots directories.
