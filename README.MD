<div align="center">

# Kingdom Enhanced

[![Release](https://img.shields.io/badge/Release-v0.6.3--alpha-gold.svg)](https://github.com/Zaykus/KingdomEnhanced/releases)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64%20(DirectX%2011%20%2F%2012)-blue.svg)](#)
[![Unity Engine](https://img.shields.io/badge/Unity-IL2CPP%2064--bit-lightgrey.svg)](#)
[![License](https://img.shields.io/badge/License-MIT%20%2F%20Educational-green.svg)](#legal-disclaimer--eula-compliance)

**Kingdom Enhanced** is a high-performance native C++ enhancement modification and quality-of-life suite for **Kingdom Two Crowns** (IL2CPP 64-bit, DirectX 11 & DirectX 12). 

Created by **Zaykus** as the official standalone continuation and complete native rewrite of the original Kingdom Enhanced project (also known as KTC-mod), engineered from the ground up to restore, stabilize, and dramatically expand the game's modding capabilities with zero external mod loader dependencies.

<br/>

<img width="750" alt="Kingdom Enhanced Banner" src="https://raw.githubusercontent.com/Zaykus/KingdomEnhanced/main/preview.png" />

</div>

---

<div align="center">

## What's New in v0.6.0-alpha

</div>

This release expands native performance, localization, construction QoL, and tester-reported stability across all campaigns:

- **100% Native C++ Architecture**: Unlike legacy mods requiring external C# Mono injectors or BepInEx loaders that cause freezes, Kingdom Enhanced runs via a native `version.dll` proxy hooked directly into the game's DirectX 11/12 swapchain and IL2CPP runtime.
- **Instant Build Mode**: Completes paid construction and wall upgrades immediately (`ConstructionBuildingComponent.AllAutoBuild` & `WorkableBuilding.CompleteConstruction`), distinct from **Hyper Builders**, automatically locking build speed sliders while active.
- **Full Arabic Localization & Right-to-Left (RTL) UI**: Complete Arabic dictionary with contextual shaping, bidirectional text reordering, mirrored sidebar/tab grids, right-aligned checkboxes/combos/sliders, localized minimap POIs, and RTL toasts.
- **7-Language Native Font Localization**: Full typography support for **English**, **中文 (Chinese)**, **한국어 (Korean)**, **Español (Spanish)**, **Deutsch (German)**, **Français (French)**, and **العربية (Arabic)**.
- **Customizable Top-Center Status Bar HUD (`F5` / `F4` Sync)**: Toggleable translucent status bar showing live time, phase, day/season, purse coins, treasury balance, and gems, with configurable hotkey (`F4` Monitor HUD sync or dedicated key) and background opacity (`0%`–`95%`).
- **Dual-Side Wall Forest Clearing & Render Safety**: Safely clears trees outward from both West and East perimeter walls in a single action with vagrant/merchant sanctuary protection and click cooldown.
- **Fire Arrows & Performance Optimization**: Standalone Flaming Arrows without forced fire-rate multiplier, low-overhead `hk_ArcherUpdate` fast-exit & cached tower checks, throttled Instant Build scans, and non-blocking snapshot UI telemetry.
- **Bottom-Right On-Screen Action Toasts**: Real-time visual feedback notifications for hotkey toggles, spawns, drops, banking actions, and config save/load.
- **Royal Banking & Treasury Sync**: Direct vault deposit/withdraw, automatic purse overflow & ground coin sweeper, bounds-checked balance sync, and snapshot-backed vault unlock state across islands.
- **Knight Squad Tactical Transfer & Boat Automation**: Rally knight squads cleanly between West and East walls, and complete/launch island shipwrecks automatically.

---

<div align="center">

## Features

</div>

### 1. Monarch, Economy & Royal Banking
- **God Mode & Infinite Stamina**: Complete invulnerability for your monarch against all Greed attacks, plus unlimited sprint for any mount without tiring or eating grass.
- **Speed Multipliers & Resizer**: Adjustable monarch move speed (1.0x to 5.0x) and dynamic visual scale adjustments.
- **Expanded Royal Purse**: Configurable big bag capacity presets (100, 250, 500, or 1000 coins) that persist cleanly across islands.
- **Royal Banker Vault Controls**: Deposit +10 and Withdraw -10 coins directly to/from the Banker without manual coin dropping.
- **Smart Ground Coin Sweeper**: Automatically collects coins dropped from hunting, farming, and water spills into the bank vault with payment animation immunity.
- **Auto-Bank Overflow**: Automatically sweeps excess coins into the vault when the purse is full to prevent water loss.

### 2. Military, Combat & Tactical Defense
- **Knight Squad Tactical Transfer**: Rally and transfer Knight squads to the Left or Right frontier wall with coordinated unit march logic.
- **Drop Tools Suite**: Spawn worker hammers, archer bows, pikes, katanas, and Norse round shields anywhere on the map with biome availability checks and idle peasant validation.
- **Norse Lands Round Shields**: Equips all citizen types (Archers, Builders, Farmers, Peasants) to form defensive shieldwalls effortlessly.
- **Standalone Flaming Arrows**: Toggles fire arrow upgrades on archers without altering attack speed.
- **Independent Fire Rate Sliders**: Fine-tune Ground Archer fire rate and Watchtower fire rate independently (1.0x to 5.0x).
- **Divine Artifacts & Relics**: Equip and fire legendary mythical weapons (Thor's Hammer, Hel's Scythe, Heimdall's Horn, Artemis Bow) with functional handsocket attachments.
- **Combat Utilities**: Instant Kill All Enemies, Destroy Portals, and Catapult boulder launch (`Spacebar`) factoring monarch facing direction.

### 3. Unit, Mount & Greed Spawner
- **Civilian & Combatant Spawner**: Spawn Peasants/Villagers, Vagrants/Beggars, Archers, Builders/Workers, Farmers, Pikemen, Knights, Berserkers, and Ninjas.
- **Mount & Hermit Spawner**: Spawn all 27+ mounts across all DLCs with duplicate prevention, plus Hermits and wildlife.
- **Greed Wave Spawner**: Spawn Greedlings, Masked Greed, Breeders, Crown Stealers, Floaters, Crushers, Greed Knights, and Greed Archers.
- **Unified Target Coordinates**: Select whether spawned units appear near the Monarch, at the Castle Center, at the Left Wall, or at the Right Wall.

### 4. Kingdom Maintenance & World Lab
- **Instant Wall Repair**: Restore all defensive fortifications across the island to 100% HP with a single click.
- **Complete & Launch Boat**: Automatically repairs unbuilt shipwrecks, purchases missing parts, stacks materials, mounts hull pieces, and pushes the vessel directly to the dock.
- **Smart Edge-Wall Forest Clearing**: Safely clears trees strictly outside outermost perimeter walls (West, East, or Both) while protecting vagrant camp and merchant sanctuaries (>18m clearance).
- **Weather & Season Controller**: Switch instantaneously between Spring, Summer, Autumn, or Winter, lock eternal summer, or set Clear, Rain, Fog, or Storm weather.
- **Time Controls**: Skip days, fast-forward to dawn or nightfall, and disable Blood Moons.

### 5. Symmetrical Panoramic Minimap (M)
- **Dead-Center Castle Projection**: Symmetrical dual-wing coordinate system pins the Castle/Campfire at the exact 50% dead-center of the screen regardless of island terrain asymmetry.
- **Full Island Tracking**: Real-time positions of your Monarch, Castle, Walls, Portals, Vagrant Camps, Hermits, Steeds, and Greed waves (`Ctrl+F` toggles Full Island Reveal).
- **Interactive POI Context Menu**: Right-click any minimap icon to Teleport Here, Recruit Vagrants, Destroy Portals, or Repair Walls.
- **Quick Filter Pills**: Toggle specific categories (All, Camps, Mounts, Shrines/Chests, Defense, Threats) with zero frame stutter.

### 6. Kingdom Monitor HUD (F4) & Top Status Bar (F5)
- **Real-Time Census**: Live demographic breakdown (Peasants, Archers, Builders, Knights, Squires, Pikemen, Ninjas, Farmers, Beggars).
- **Fortifications & Economy**: Fortified perimeter status, standing walls, portal counts, purse coins, vault coins, and gems.
- **Mount Telemetry**: Active steed name, mount feed status, and live stamina gauge (`N` toggles the Mount Stamina Bar).
- **Customizable Appearance & Top Bar**: Configurable F5 / F4-sync Top Status Bar hotkey, adjustable background opacity sliders, and full 7-language localization including Right-to-Left (RTL) Arabic.

---

<div align="center">

## Default Controls & Hotkeys

</div>

| Hotkey | Action |
|---|---|
| **F1** or **Insert** | Toggle Main Enhancement Menu |
| **F4** | Toggle Kingdom Monitor HUD |
| **F5** | Toggle Top-Center Status Bar HUD (configurable or sync with F4) |
| **M** | Toggle Symmetrical Panoramic Top Minimap |
| **N** | Toggle Mount Stamina Gauge |
| **Ctrl + F** | Toggle Minimap Full Island Reveal |
| **Spacebar** | Launch Catapult Boulder (in facing direction) |
| **C** | Drop Coins (Configurable burst amount) |
| **G** | Drop Gems (Configurable burst amount) |

*All hotkeys can be fully customized in-game via **Menu -> Settings** or directly in `KingdomEnhanced.ini`.*

---

<div align="center">

## Installation

</div>

1. Download the latest release archive (`KingdomEnhanced-v0.6.0-alpha.zip`).
2. Extract the archive contents (`version.dll` and `README.txt`).
3. Move `version.dll` directly into your Kingdom Two Crowns game folder (the directory containing `KingdomTwoCrowns.exe`):
   - *Example Steam path*: `C:\Program Files (x86)\Steam\steamapps\common\Kingdom Two Crowns\`
4. Launch the game normally and press **F1** to open the menu (`KingdomEnhanced.ini` is auto-generated on first launch).

### Uninstallation
Simply delete `version.dll` and `KingdomEnhanced.ini` from your game folder. The original game files remain completely untouched.

---

<div align="center">

## Configuration (`KingdomEnhanced.ini`)

</div>

The mod automatically generates and maintains `KingdomEnhanced.ini` in the game directory. Settings can be edited live via the **F1** in-game menu or manually with any text editor:

```ini
[General]
Language=English      ; English, Chinese, Korean, Spanish, German, French, Arabic

[Modules]
EnableMinimap=1       ; Panoramic top radar map
EnableStaminaBar=1    ; Mount stamina gauge
EnableHotkeys=1       ; Boulder/teleport hotkeys
UIScale=1.00          ; 0.75x - 2.50x display scale
BoldFont=0            ; 1 = Bolder text font
MonitorAlpha=0.55     ; Translucency of F4 HUD

[Cheats]
GodMode=0
InfiniteStamina=1
SpeedMultiplier=2.00
```

---

<div align="center">

## Frequently Asked Questions (FAQ)

</div>

#### Q: Does this mod work with the latest Steam / GOG release of Kingdom Two Crowns?
**A:** Yes. The mod is built against the 64-bit Unity IL2CPP runtime with native DirectX 11 and DirectX 12 renderers.

#### Q: Do I need BepInEx, MelonLoader, or Mono?
**A:** No. Kingdom Enhanced is completely standalone. It operates via a native proxy DLL without requiring any third-party mod loader or external runtime framework.

#### Q: Will this cause lag or game stutter?
**A:** No. All rendering is synchronized to the native swapchain present pipeline, and engine interactions run asynchronously on Unity's main thread to prevent frame drops, lag, or stutter.

#### Q: Can I use this on all DLCs?
**A:** Yes. Fully compatible with Medieval (Classic), Shogun, Dead Lands, Norse Lands, and Call of Olympus.

---

<div align="center">

## Legal Disclaimer & EULA Compliance

</div>

> [!IMPORTANT]
> **Please read this disclaimer carefully before using this software.**

1. **Non-Commercial & Fan-Made**: This modification is an independent, non-commercial, open-source fan project developed strictly for educational, research, and single-player entertainment purposes.
2. **Intellectual Property**: **Kingdom Two Crowns**, its assets, artwork, audio, trademarks, and associated intellectual property are the exclusive property of **Raw Fury**, **Fury Studios**, and **Coatsink**. The creators of this modification claim no ownership over any original game assets or trademarks.
3. **No Redistribution of Proprietary Assets**: This software does NOT contain, package, or distribute any copyrighted game binaries, proprietary art, sound, or game code. It operates solely as a dynamic runtime memory interop module (`version.dll`) linking user-owned game files on the user's local machine.
4. **Offline & Single-Player Intended**: This modification is designed exclusively for offline and single-player gameplay. It is not intended to bypass software copy protection, DRM, or gain unfair advantage in competitive network environments.
5. **Fair Use & Reverse Engineering**: Code analysis and runtime hooks are implemented in compliance with jurisdictional fair-use provisions for software interoperability and user customization.
6. **No Warranty**: This software is provided *"as-is"*, without warranty of any kind, express or implied. Users assume all responsibility for the use of this software.

---

<div align="center">

## Credits & Acknowledgments

</div>

- **Zaykus** — Lead Developer  
  Architecture, Native C++ Suite, Accessibility systems, DirectX 12 Hook & UI everything lol
- **Abevol** — Framework & Engine Research  
  Engine analysis and IL2CPP reverse engineering groundwork + open source mod ([https://github.com/abevol](https://github.com/abevol))
- **Community Testers and special thanks to Zeternal-1**  
  Accessibility validation, testing, profiling, and community feedback
- **Dear ImGui** (Omar Cornut) — Immediate mode graphical user interface library
- **MinHook** (Tsuda Kageyu) — The Minimalistic x86/x64 API Hooking Library
