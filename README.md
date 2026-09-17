# rdbt v5

A modular automation and client utility for Hypixel SkyBlock on Minecraft 26.2 / 26.1.2.

[Releases](https://github.com/victormtz06/V5-Macro/releases) | [Documentation](https://rdbt.top/docs/features)

---

## Overview

rdbt v5 runs on a custom automation engine designed for modern Fabric loaders, featuring configurable smoothing algorithms, automated fail recovery, and an embedded scripting runtime[cite: 1].

| Core System | Technical Implementation |
| :--- | :--- |
| **Rotations** | Pitch/yaw spline curves designed to avoid linear camera snaps. |
| **Pathing** | 3D obstacle traversal, step compensation, and etherwarp coordinate sequences. |
| **Fail Recovery** | Aborts execution or disconnects upon server warps, proximity, lag spikes, or chat triggers. |
| **Session Manager** | Configurable playtime caps, randomized downtime intervals, and reconnect timers. |
| **Integrations** | Discord RPC support and webhook alerts with telemetry and automated screenshots. |
| **Security** | Session token theft protection and Mojang authentication rate-limiting. |

---

## Feature Modules

<details>
<summary><b>Mining</b></summary>
<br>

<details>
<summary><b>Commission Macro</b></summary>

Automated route traversal and objective completion across the Dwarven Mines[cite: 1]. Supports customizable pathing between assignment locations and automated mining target selection[cite: 1].
</details>

<details>
<summary><b>Ore Macro</b></summary>

Configurable route runner supporting custom walk nodes, etherwarp coordinates, deployable abilities, and prioritized block filters[cite: 1].
</details>

<details>
<summary><b>Nuker & Chest Opener</b></summary>

Adjustable-radius block breaking with integrated Crystal Hollows treasure chest opening and powder grinding support[cite: 1].
</details>

<details>
<summary><b>Fossil Excavator</b></summary>

Automated Suspicious Scrap routing inside the Fossil Excavator to maximize Glacite Powder gain[cite: 1].
</details>

<details>
<summary><b>Pingless Miner</b></summary>

Latency-compensated break sequencer for instant hardstone clearing in the Crystal Hollows[cite: 1].
</details>

<details>
<summary><b>Jasper Drill Exploit</b></summary>

Automated weapon and tool swap sequencing to maintain the Jasper Drill speed boost bonus[cite: 1].
</details>

<details>
<summary><b>Glowing Mushroom Cave</b></summary>

Automated pathing and farming loop designed for the Glowing Mushroom Cave[cite: 1].
</details>

</details>

<details>
<summary><b>Garden Automation</b></summary>

| Layout / Tool | Operational Target |
| :--- | :--- |
| `S-Shape Layouts` | Optimized for Cocoa, Nether Wart, Cane, and standard crops[cite: 1]. |
| `16thGarden Layouts` | Specialized sweeps for A/D Cactus/Cocoa, W/S Melons, Pumpkins, Mushrooms, and Sugar Cane[cite: 1]. |
| `Vertical & SDS` | Multi-tier vertical plots and SDS staircase layouts[cite: 1]. |
| `Pest Clearing` | Detects plot alerts, paths to targets, clears pests, and returns to active lane[cite: 1]. |
| `Visitor Handler` | Processes queue NPCs and purchases required items from Bazaar within set coin caps[cite: 1]. |
| `Drop Management` | Automatically dumps inventory to Bazaar orders or NPC trade menus at capacity thresholds[cite: 1]. |
| `Bonus Management` | Empties Philip vacuum bags automatically on buff expiration[cite: 1]. |

</details>

<details>
<summary><b>Combat & Slayer</b></summary>

| Routine | Functionality |
| :--- | :--- |
| `Voidgloom Helper` | Auto-chains Katana Soulcry casts and deployable flares (Plasmaflux, Overflux) via hotkeys[cite: 1]. |
| `Rift Sun Gecko` | Automated target tracking and route interaction for the Sun Gecko encounter[cite: 1]. |
| `Trapper Macro` | Entity detection, pathfinding, and combat handling for Trevor pelt missions[cite: 1]. |
| `Beachballer` | Trajectory calculation and auto-bouncing for summer beach balls[cite: 1]. |

</details>

<details>
<summary><b>Fishing & Skills</b></summary>

| Component | Functionality |
| :--- | :--- |
| `Fishing Assistant` | Automated casting, bite detection reeling, and dynamic pet swapping on catch[cite: 1]. |
| `Stridersurfer Route` | High-efficiency Strider fishing loop (~3M+ XP/hr)[cite: 1]. |
| `Auto Experiments` | Direct solver and auto-clicker for Chronomatron and Ultrasequencer minigames[cite: 1]. |
| `Chocolate Factory` | Auto-clicks production cookies, retrieves stray rabbits, and highlights hidden eggs[cite: 1]. |
| `Bulk Openers` | High-speed opener for Jerry boxes and inventory containers[cite: 1]. |

</details>

<details>
<summary><b>Foraging, ESP & Quality of Life</b></summary>

| Feature | Details |
| :--- | :--- |
| `Etherwarp Nukers` | Precision coordinate loops for Lushlilac flowers and Mudworms[cite: 1]. |
| `Auto Harp` | Latency-compensated packet clicker for Melody's Harp[cite: 1]. |
| `Structure ESP` | Bounding boxes for Crystal Hollows areas (Divan, Grotto, Jungle Temple, Khazad-dûm)[cite: 1]. |
| `Entity Highlights` | Chams and tracer lines for pests, rats, players, and custom targets[cite: 1]. |
| `HUD Elements` | Movable on-screen readouts for real-time TPS, ping, and session rates[cite: 1]. |
| `Player Utilities` | Freecam, Freelook, Inventory Walk, Left-Click Etherwarp, and Anvil Auto-Combine[cite: 1]. |

</details>

## Installation

### Prerequisites
- Minecraft Java Edition `26.2` or `26.1.2`
- [Fabric Loader](https://fabricmc.net/) (matching target version)
- [Fabric API](https://modrinth.com/mod/fabric-api) (required dependency)
- Java 21+ (64-bit)

### Setup
1. Verify Fabric Loader and **Fabric API** are installed in your Minecraft instance.
2. Download the latest `.jar` build from the [releases page](https://github.com/victormtz06/V5-Macro/releases).
3. Place the downloaded `.jar` into your `.minecraft/mods` directory.
4. Launch the game using your Fabric profile.
5. Press `Right Shift` or run `/rdbt` in chat to open the configuration interface.

---
