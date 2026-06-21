# Assets

Place the following files here before building the installer:

- `icon.ico` — Windows application icon (256×256, multi-resolution)
- `icon.png` — Standalone app icon (1024×1024 PNG)

You can generate these from a single SVG/PNG using any icon converter, e.g.:

```
# Using ImageMagick
convert -background none -define icon:auto-resize=256,128,64,48,32,16 icon.png icon.ico
```

If no icon is provided, the Inno Setup compiler will fail. Either:
1. Drop a real `icon.ico` here, OR
2. Remove the `SetupIconFile=..\Assets\icon.ico` line from `Installer/installer.iss` to use the Inno Setup default icon.
