# BR Snowfall v0.1.0 Beta

GPU-accelerated transparent snowfall source for Resolume Arena/Avenue on Windows.

Original plugin by **BELAJAR RESOLUME**  
Credit shown in Resolume: **Effect by Belajar Resolume**

## Features

- Round, Crystal, and Mixed snow
- Adjustable amount, size, size random, speed, and speed random
- Wind direction, wind strength, horizontal drift, and rotation
- Three depth layers
- Snow color, opacity, glow, and softness
- Low, Medium, and High quality modes
- Start, Pause, Resume, and Reset controls
- Transparent FFGL source; no image assets required
- Parameters can be mapped in Resolume to MIDI/OSC/Shortcuts

## Build with GitHub Actions

1. Create a new empty GitHub repository.
2. Upload **all contents** of this folder, including the `.github` folder.
3. Open the repository's **Actions** tab.
4. Open **Build BR Snowfall** and click **Run workflow**.
5. When the job is green, download artifact `BR-Snowfall-Windows-v0.1.0-beta`.

Do not choose one of GitHub's suggested workflow templates. This package already contains `.github/workflows/build-windows.yml`.

## Install in Resolume

1. Extract the downloaded GitHub artifact.
2. Copy `BR Snowfall.dll` to:
   `Documents\Resolume Arena\Extra Effects`
3. Restart Resolume.
4. Open **Sources** and search for `BR Snowfall`.
5. Drag the source to an empty clip slot.

## Performance recommendation

- Use **High** quality for the main 1080p output when the GPU is comfortable.
- Use **Medium** for most live shows.
- Use **Low** when running many effects or multiple outputs.
- Begin with the default amount and increase gradually while checking FPS.

## Notes

- Windows 64-bit only in this beta.
- The output is transparent and is intended for use on a separate layer above other visuals.
- Built against a pinned revision of the official Resolume FFGL SDK for repeatable builds.

