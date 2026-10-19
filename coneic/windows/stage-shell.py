"""Add Windows shell integration to a staged Coneic app (requires Pillow).

Artwork uses Coneic's supplied logo, with a format label and a category glyph.
Never edits player binaries or user settings. Run before signing/archiving.
"""
import argparse
import json
import shutil
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent


def stage(app):
    if not (app / 'Coneic.exe').is_file():
        raise ValueError('Coneic.exe is missing')
    formats = json.loads((ROOT / 'file-types.json').read_text())
    destination = app / 'Coneic-shell'
    icons = destination / 'icons'
    icons.mkdir(parents=True, exist_ok=True)
    logo = Image.open(ROOT / '../../share/coneic/coneic-256.png').convert('RGBA')
    font_path = Path('C:/Windows/Fonts/seguisb.ttf')
    if not font_path.exists():
        font_path = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf')
    colors = {'audio': '#f28339', 'video': '#568ee8', 'playlist': '#a679e0'}
    for item in formats:
        extension, kind = item['extension'], item['kind']
        image = Image.new('RGBA', (256, 256))
        draw = ImageDraw.Draw(image)
        draw.rounded_rectangle((22, 6, 234, 250), radius=24, fill='#202936')
        draw.rounded_rectangle((22, 6, 234, 18), radius=6, fill=colors[kind])
        mark = logo.copy()
        mark.thumbnail((116, 116), Image.Resampling.LANCZOS)
        image.alpha_composite(mark, ((256 - mark.width)//2, 30))
        if kind == 'audio':
            for x, height in zip(range(83, 174, 15), [8, 16, 24, 32, 24, 16, 8]):
                draw.rounded_rectangle((x, 170-height//2, x+5, 170+height//2), radius=2, fill=colors[kind])
        elif kind == 'video':
            draw.polygon([(115, 154), (115, 184), (144, 169)], fill=colors[kind])
        else:
            for y in (157, 169, 181):
                draw.ellipse((90, y, 95, y+5), fill=colors[kind])
                draw.rounded_rectangle((104, y, 165, y+5), radius=2, fill=colors[kind])
        label = extension[1:].upper()
        size = 40
        font = ImageFont.truetype(str(font_path), size)
        while draw.textbbox((0, 0), label, font=font)[2] > 190:
            size -= 1
            font = ImageFont.truetype(str(font_path), size)
        draw.text((128, 215), label, font=font, anchor='mm', fill='#ffffff')
        stem = extension[1:]
        image.save(icons / (stem + '.png'))
        image.save(icons / (stem + '.ico'), sizes=[(16,16),(20,20),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
    for name in ('file-types.json', 'register-file-associations.ps1'):
        shutil.copyfile(ROOT / name, destination / name)
    for name, action, extra in [('Set Coneic as default.cmd', 'Register', ' -OpenSettings'),
                                ('Remove Coneic file associations.cmd', 'Unregister', '')]:
        (app / name).write_bytes(('@echo off\r\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File '
            '"%~dp0Coneic-shell\\register-file-associations.ps1" -Action ' + action + extra +
            '\r\nif errorlevel 1 pause\r\n').encode('ascii'))
    return len(formats)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    args = parser.parse_args()
    print(f'Staged {stage(args.app.resolve())} format icons and Windows registration helpers.')
