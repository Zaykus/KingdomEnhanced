# Kingdom Enhanced (v0.2.0-alpha)

[![Release](https://img.shields.io/badge/Release-v0.1.0--alpha-gold.svg)](https://github.com/Zaykus/KTC-mod)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64%20(DirectX%2012)-blue.svg)](#)
[![Unity Engine](https://img.shields.io/badge/Unity-IL2CPP%2064--bit-lightgrey.svg)](#)
[![License](https://img.shields.io/badge/License-MIT%20%2F%20Educational-green.svg)](#legal-disclaimer--eula-compliance)

**Kingdom Enhanced** is a high-performance native C++ enhancement modification and quality-of-life suite for **Kingdom Two Crowns** (IL2CPP 64-bit, DirectX 12). 

Created by **Zaykus** as the official standalone continuation and complete rewrite of the original Kingdom Enhanced project also know as KTC-mod, engineered from the ground up to restore, stabilize, and dramatically expand the game's modding capabilities.

---

## What's New & What Was Ported / Fixed

This project revitalizes the legacy mod into a standalone, crash-free, zero-dependency release:

- **100% Native C++ & Zero-Freeze Engine**: Unlike older injection mods that caused picture freezes or required heavy C# mono loaders, Kingdom Enhanced runs via a native `version.dll` proxy hooked directly into the game's DirectX 12 swapchain and IL2CPP runtime.
- **Fixed Picture & Render Freezes**: Replaced deprecated blocking command queue waits with a dedicated asynchronous main-thread action pipeline.
- **DirectX 12 ImGui Overlay**: High-fidelity medieval fantasy UI styled after parchment and gold leaf that scales seamlessly from 1080p to 4K displays.
- **Fixed Boulder Launching Direction**: The catapult boulder launch hotkey (`Spacebar`) now accurately factors monarch facing direction (Left & Right) rather than firing only to the right.
- **Fixed Kill All Enemies & Utility Cheats**: Rewritten to target native IL2CPP `Damageable.DestroyNow()` and `Enemy.Despawn()`, resolving previous silent failures.
- **Abevol Interactive Minimap (M)**: Real-time top radar tracking kingdom walls, portal locations, vagrant camps, greed wave clusters, and player position with an optional full reveal (`Ctrl+F`).
- **Kingdom Monitor HUD (F4)**: A customizable, translucent on-screen heads-up display showing active citizens, builders, archers, coins, gems, and steed stamina.
- **Trilingual & European Localization**: Full native font rendering for **English**, **中文 (Chinese)**, **한국어 (Korean)**, **Español (Spanish)**, **Deutsch (German)**, and **Français (French)**, complete with bold typography support.
- **Live In-Menu Action Logger**: Real-time diagnostic terminal inside the mod menu (`F1 -> Logs`) and file logging (`KingdomEnhanced.log`) providing instant feedback on every user action.

---

## Features

### 1. Main & Player Cheats
- **God Mode**: Complete invulnerability for your monarch against all Greed attacks.
- **Infinite Mount Stamina**: Unlimited sprint for any mount without tiring or eating grass.
- **Speed Multipliers**: Adjustable monarch move speed (1x, 1.5x, 2x, 4x, or custom slider).
- **Player & Mount Resizer**: Dynamically scale your monarch's visual size.
- **Drop Tool & Resources**: Spawn worker hammers, archer bows, coins (10, 50, 100, or fill wallet), and gems anywhere on the map.
- **Combat Utilities**: Instant Kill All Enemies, Destroy Portals, and Recruit All Vagrants.

### 2. Unit & Greed Spawner
- **Civilian & Military Units**: Spawn Peasants/Villagers, Vagrants/Beggars, Archers, Builders/Workers, Farmers, Pikemen, Knights, Norse Berserkers, and Shogun Ninjas.
- **Hermits & Specialists**: Spawn Bakery, Ballista, Stable, Horn, Fire, Warrior, and Berserker Hermits on demand.
- **Greed Spawner**: Spawn Greedlings, Masked Greed, Breeders, Crown Stealers, Floaters/Squids, Crushers, Greed Knights, and Greed Archers for custom wave defense challenges.
- **Spawn Positioning**: Toggle between spawning at the kingdom campfire or directly at your monarch's coordinates.

### 3. Lab & World Experiments
- **Season Controller**: Switch instantaneously between Spring, Summer, Autumn, or Winter, or lock eternal summer.
- **Time Controls**: Skip time phases (day/night transition) by 6-hour increments.
- **Weather & Blood Moon**: Clear rainy/snowy weather and disable blood moon events.
- **Defense & Nature Boosts**: Self-repairing walls, tree regrowth accelerator, and buoyant currency that floats instead of sinking into water.
- **Combat Modifiers**: Rapid-fire archer/tower boost, ballista/catapult reload speed and projectile speed multipliers, berserker rage mode, and steed buff aura duration.

### 4. Hard Mode & Difficulty Director
- **Wave Multiplier**: Scale enemy wave counts from 0.1x to 10.0x for colossal survival sieges.
- **Enemy Speed Multiplier**: Adjust enemy movement speed from 0.5x to 3.0x.
- **Portal Spawn Rate & Greed Queen HP**: Customize portal defense intensity and boss hitpoints.
- **Director Threat Scaling**: Fine-tune the AI director's threat evaluation logic.
- **No Crown Stealing**: Protect your crown from being snatched by Greed snatchers.

### 5. Abevol Kingdom Minimap (M)
- Toggleable top screen radar showing:
  - Monarch position & orientation
  - Campfire / Town center
  - Left and Right outermost defensive walls
  - Active and destroyed Portals
  - Beggar / Vagrant tree camps with living vagrant counters
  - Greed wave spawn clusters marching toward your walls
- Press `Ctrl + F` while the map is active to reveal all fog of war and off-screen island landmarks.

### 6. Kingdom Monitor HUD (F4)
- Floating status dashboard displaying:
  - Island day counter & reign metrics
  - Citizen census (total citizens, vagrants, archers, builders, farmers)
  - Fortifications (standing walls, active portals)
  - Wealth & Mount info (held coins, gems, mount name, stamina percentage)
- Translucent background with dynamic alpha adjustment (35%, 55%, 75%, 95%) and 4 aesthetic themes (Classic, Neon, Light, Ghost).

---

## Default Controls & Hotkeys

| Hotkey | Action |
|---|---|
| **F1** | Toggle Main Enhancement Menu |
| **F4** | Toggle Translucent Kingdom Monitor HUD |
| **M** | Toggle Abevol Kingdom Minimap |
| **N** | Toggle Compact Mount Stamina Gauge |
| **Spacebar** | Launch Catapult Boulder (in facing direction) |
| **C** | Drop Coins |
| **G** | Drop Gems |
| **T** | Teleport Monarch to Mouse Cursor |
| **Ctrl + F** | Toggle Minimap Full Island Reveal |

*All hotkeys and keybinds can be customized inside the mod menu under **Settings** or directly in `KingdomEnhanced.ini`.*

---

## Installation

1. Download the latest release archive (`KingdomEnhanced-v0.1.0-alpha.zip`).
2. Extract the archive contents:
   - `version.dll`
   - `KingdomEnhanced.ini` (optional default configuration)
3. Move `version.dll` and `KingdomEnhanced.ini` directly into your Kingdom Two Crowns game folder (the directory containing `KingdomTwoCrowns.exe`).
   - *Example Steam path*: `C:\Program Files (x86)\Steam\steamapps\common\Kingdom Two Crowns\`
4. Launch the game normally. Press **F1** once in-game to open the mod menu.

### Uninstallation
Simply delete `version.dll` and `KingdomEnhanced.ini` from your game folder. The original game files remain completely untouched.

---

## Configuration (`KingdomEnhanced.ini`)

The mod automatically generates and maintains `KingdomEnhanced.ini` in the game directory. Settings can be edited live via the **F1** in-game menu or manually with any text editor:

```ini
[General]
Language=English      ; English, Chinese, Korean, Spanish, German, French

[Modules]
EnableMinimap=1       ; Top radar map
EnableStaminaBar=1    ; Mount stamina bar
EnableHotkeys=1       ; Boulder/teleport hotkeys
UIScale=1.00          ; 1.00x - 2.50x display scale
BoldFont=0            ; 1 = Bolder text font
MonitorAlpha=0.55     ; Translucency of F4 HUD

[Cheats]
GodMode=0
InfiniteStamina=1
SpeedMultiplier=2.00
```

---

## Frequently Asked Questions (FAQ)

#### Q: Does this mod work with the latest Steam / GOG release of Kingdom Two Crowns?
**A:** Yes. The mod is built against the 64-bit Unity IL2CPP runtime and DirectX 12 graphics engine.

#### Q: Do I need BepInEx, MelonLoader, or Mono?
**A:** No. Kingdom Enhanced is completely standalone. It does not require any third-party mod loader or external runtime framework.

#### Q: Will this cause lag or game stutter?
**A:** Maybe. All rendering is synchronized to the native swapchain present pipeline, and engine interactions run asynchronously on Unity's main thread to prevent frame drops.

---

## Legal Disclaimer & EULA Compliance

> [!IMPORTANT]
> **Please read this disclaimer carefully before using this software.**

1. **Non-Commercial & Fan-Made**: This modification is an independent, non-commercial, open-source fan project developed strictly for educational, research, and single-player entertainment purposes.
2. **Intellectual Property**: **Kingdom Two Crowns**, its assets, artwork, audio, trademarks, and associated intellectual property are the exclusive property of **Raw Fury**, **Fury Studios**, and **Coatsink**. The creators of this modification claim no ownership over any original game assets or trademarks.
3. **No Redistribution of Proprietary Assets**: This software does NOT contain, package, or distribute any copyrighted game binaries, proprietary art, sound, or game code. It operates solely as a dynamic runtime memory interop module (`version.dll`) linking user-owned game files on the user's local machine.
4. **Offline & Single-Player Intended**: This modification is designed exclusively for offline and single-player gameplay. It is not intended to bypass software copy protection, DRM, or gain unfair advantage in competitive network environments.
5. **Fair Use & Reverse Engineering**: Code analysis and runtime hooks are implemented in compliance with jurisdictional fair-use provisions for software interoperability and user customization.
6. **No Warranty**: This software is provided *"as-is"*, without warranty of any kind, express or implied. Users assume all responsibility for the use of this software.

---

## Credits & Acknowledgments
- **Zaykus** — Architecture, IL2CPP reverse-engineering, D3D12 hooking, and modern rewrite.
- **Abevol** — Inspiration for the top kingdom radar map layout and telemetry mechanics.
- **Original Kingdom Enhanced Team** — Gratitude for pioneering Kingdom modding concepts.
- **Dear ImGui** (Omar Cornut) — Immediate mode graphical user interface library.
- **MinHook** (Tsuda Kageyu) — The Minimalistic x86/x64 API Hooking Library.
