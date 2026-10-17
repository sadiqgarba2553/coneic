# Coneic 0.2.1 Alpha

Local development build for review. **Do not push or publish this update without the owner's explicit instruction.** The published 0.2.0 preview remains unchanged.

Open **Your space** from the main header's bookmark icon, Home sidebar, or main menu.

## Moment Cards

Play a seekable video or audio file and choose **Save current moment**. The saved card includes its source, timestamp and a local video thumbnail where available. Use **Edit** to add a title or note. Search covers titles, notes and source names. **Play moment** opens the correct source and seeks to the saved time. Deleting a card never deletes its media file.

## Study Mode

In **Your space → Study**, write a note while playing a lecture or other media. The first text captures its timestamp. **Mark start** changes the start; **Mark end** defines a later endpoint in the same source. **Save note** keeps the text and section. Select a saved section and enable **Loop selected section** to repeat it through VLC's native seek controls. Closing Your space stops its loop. Unsaved drafts require confirmation before dismissal.

**Export notes** writes Markdown for the currently open source, including note text and timestamps. Saved notes are also visible and editable as Moment Cards.

## Smart Collections

Built-in collections include **All media**, **Unfinished**, **Added this week**, and **Under 20 minutes**. Media comes from added folders, recent files and played sources. Local files are scanned for duration using VLC's background metadata preparser; this does not play them. Unsupported or unknown durations are excluded from duration rules. Network sources are not automatically scanned.

Select media and choose **Edit tags**. **New** creates a collection based on a tag, recent addition, duration or unfinished progress. The contents update from the actual library metadata. **Delete** removes a custom collection without removing media, tags or notes. Built-in collections remain available.

## Interface changes

The Home screen uses fewer primary actions, a single **Add media** menu, shorter navigation labels and responsive card sizing. Your space groups the three new features in one optional window. Compact pages can scroll instead of forcing an oversized window. SVG icons and embedded Inter typography retain high-DPI rendering; generated video thumbnails are now 640 pixels wide with a separate cache key from older previews.

## Local data and compatibility

`personal-library.json` is stored alongside Coneic's interface settings in the Coneic app-data directory. It stores versioned JSON with media references, viewing progress, tags, notes and collection rules. Writes use `QSaveFile` and a cross-process `QLockFile`; each mutation reads the latest saved state before changing it. Invalid or newer-schema data is refused rather than overwritten. Ordinary source media files are never modified by these features.

The prebuilt Qt 5.6/toolchain combination exhibited a crash through a JSON reference assignment path. Direct object insertion and array replacement avoid that path; regression checks exercise creation, editing, deletion and persistence. This is an observed compatibility workaround, not a claim that every Qt ABI issue is resolved.

VLC 3.0.24 remains the media engine. Existing codec/device/embedding limitations in VALIDATION.md still apply. This Alpha adds no cloud service, account requirement or data upload.

## Verified checks

The native runner exited cleanly at normal and 200% scaling. Each run passed 19 persistence checks and 10 live personal-feature checks, alongside 22 native UI checks (including the personal-feature aggregate). The custom collection editor, real section loop, local metadata parsing, on-disk Markdown export, and compact/large workspace sizes were exercised. Static source and package checks found no lost core declarations, missing baseline plugins or unresolved imports. Evidence is in `docs/evidence/alpha-0.2.1` and actual captures in `docs/screenshots/alpha-0.2.1`. These checks do not certify all devices, codecs or displays.
