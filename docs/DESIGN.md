# Design

The requested [Apple design skill](https://github.com/emilkowalski/skills/blob/main/skills/apple-design/SKILL.md) informed hierarchy, typography, restrained surfaces, immediate feedback and accessible interaction. The implementation uses native Windows/Qt widgets.

- Graphite dark surfaces, orange accents drawn from the supplied art, and a light alternative through the existing palette preference.
- Segoe UI Variable Text with Segoe UI fallback; minimum 10-point text while honoring larger existing font settings.
- Immediate hover, pressed and focus states. Sliders, keyboard controls and accessibility remain native Qt.
- Windows high-contrast mode bypasses the dark/custom palette. Reduced-motion settings disable optional Qt menu/combo/tooltip effects.
- Original VLC workflows, menus and advanced settings remain accessible.

Artwork SHA-256: `90fd204077e6d96fe22f4644cd3d24bd324bd35fda7fb539478a82a5da2c5b93`.

The original SVG is preserved byte-for-byte. PNG exports span 16–1024 pixels; the Windows ICO contains nine sizes from 16–256. REA's native executable resource inspection and payload hashing confirm that all nine executable icon images exactly match those PNG exports. Static resource matching does not certify Windows Explorer's icon cache.
