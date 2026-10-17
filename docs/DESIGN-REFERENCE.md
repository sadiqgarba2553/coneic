# Coneic visual reference

The requested reference is [I Redesigned VLC After 25 Years](https://www.youtube.com/watch?v=ERFeOgqk324)
by Juxtopposed, published October 12, 2026. The creator links the
[original design](https://www.figma.com/community/file/1691465999134370980/vlc-redesign),
licensed CC BY 4.0. Coneic adapts the design and preserves the orange accent,
Coneic name and BLUECLOUD parent-company branding.

Evidence inspected: video description and exported English auto-caption transcript;
original Figma General Settings frame 3046:9282; icon export node references in
modules/gui/qt/pixmaps/coneic-reference/README.md. The SVG import is pinned and
hashed in design-reference-icons.json. Third-party reference implementations
are evidence of their own work, not proof that Coneic implements any feature.

Reference decisions: compact left-hand previous/play/next (1:47â€“2:14), tools
on the right (2:15â€“2:55), thick seek bar below the transport (3:10â€“3:24),
two-tone menu icons and monochrome action icons (0:59â€“1:14), soft rectangular
shapes (1:17â€“1:32), 13 global settings categories (17:35â€“18:27), searchable
settings and advanced disclosures (16:04â€“16:36), information icons (18:47â€“18:53).

This is an adaptation to VLC 3's native Qt interface. The current preview implements local video thumbnails and seek previews through isolated native VLC worker processes. It is not a claim of pixel-for-pixel completion: subtitle timelines, folder presets and the video's music artist/album views are not implemented. Some retained dialogs use the original layout with adapted styling. The original full preferences and action handlers remain accessible.
