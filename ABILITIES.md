# Ability Research: Cross-Game Analysis for Mecha Bullet Hell

## Ability Archetypes Across MOBAs

### Damage Delivery
| Type | Description | In Your Game? |
|------|-------------|---------------|
| **Point Nuke** | Targeted single-hit burst | Sniper aimed shot |
| **Line Skillshot** | Directional projectile, hits first or pierces | Railgun (pierce all) |
| **AoE** | Damage in a zone/circle | Rocket explosion, grenade |
| **Cone/Fan** | Spread in front of caster | Shotgun, revolver fan |
| **DoT** | Sustained damage debuff | Sniper slow only — no DoT yet |
| **Channel** | Hold still for continuous damage | BFG lightning chain (semi) |
| **Bounce/Chain** | Jumps between targets | BFG chain, shotgun ricochet |
| **Execute** | Bonus damage on low HP targets | Not yet |

### Crowd Control (Deadlock's taxonomy is the best reference)
| CC Type | Effect | Your Game? |
|---------|--------|-----------|
| **Stun** | Full disable | No |
| **Slow** | Reduced movement | Sniper debuff only |
| **Knockback** | Forced displacement | Shotgun, spin, rocket |
| **Root/Immobilize** | Can't move, CAN still shoot | No |
| **Silence** | Can't use abilities, CAN shoot | No |
| **Disarm** | Can't shoot, CAN use abilities | No |
| **Fear** | Forces enemy to run away | No |
| **Blind** | Attacks miss / aim becomes inaccurate | No |

Key insight from Deadlock: **partial disables** create decision-making. Silence vs Disarm punishes different build types differently.

### Mobility
| Type | Description | Your Game? |
|------|-------------|-----------|
| **Dash** | Quick directional movement | Yes (3 charges) |
| **Blink/Teleport** | Instant reposition, no travel | No |
| **Leap/Slam** | Vertical + AoE on landing | No |
| **Charge** | Rush forward, hit first thing | Sword lunge (close) |
| **Hook/Pull** | Bring enemy to you | No |
| **Force Push** | Shove enemy away | Spin knockback (close) |

### Defensive
| Type | Description | Your Game? |
|------|-------------|-----------|
| **Shield/Barrier** | Absorb damage as separate HP | Yes (ABL_SHIELD) |
| **Invulnerability/Stasis** | Can't be hit, can't act | Dash iframes (partial) |
| **Damage Reflection** | Return damage to attacker | No |
| **Lifesteal/Drain** | Damage heals you | Spin lifesteal |
| **Cleanse/Dispel** | Remove debuffs | No |
| **Damage Reduction** | % damage reduction | No |
| **Evasion** | Chance to dodge attacks | No |

### Utility / Deployables
| Type | Description | Your Game? |
|------|-------------|-----------|
| **Turret** | Auto-attacking stationary entity | No |
| **Trap/Mine** | Trigger on proximity | No |
| **Zone Denial** | Persistent damaging area | No |
| **Stealth** | Become invisible | No |
| **Decoy** | Aggro distraction | Yes (super dash decoy) |
| **Tether** | Restrict enemy to radius | No |

---

## What Would Fit This Game's Design

Given 12 ability slots, mecha customization philosophy, and bullet hell context.

### High-fit abilities (simple to implement, high gameplay value)

**1. Root/Immobilize** — enemy can't move but CAN still shoot. In a bullet hell this is interesting: you pin a shooter in place (easier to dodge its pattern since it's stationary) but it becomes a turret. A root grenade or tether mine.

**2. Fear/Repel** — force all nearby enemies to flee outward. Like a reverse-aggro spin. Crowd control in the literal sense — buys breathing room.

**3. Damage Reflection / Return Fire** — like Deadlock's Return Fire item or Athena's Deflect in Hades. Spin already deflects enemy bullets. A dedicated "return fire" toggle that reflects projectiles for X seconds would be a clean slot ability.

**4. Blink/Teleport** — instant reposition (no travel time like dash). Higher risk because no iframes, but instant. Could replace a dash charge or be a separate slot.

**5. Execute threshold** — bonus damage to enemies below X% HP. Passive that rewards finishing enemies efficiently. Could be a "system upgrade" passive slot.

**6. Ground Slam / Stomp** — AoE damage + stun centered on player. The mecha lands hard, enemies in radius are stunned briefly. Channel or instant.

**7. Turret/Drone** — deploy a stationary auto-shooter. Fits the mecha fantasy (deploy a drone from your mech). Times out or has HP.

**8. Mine/Trap** — place on ground, detonates on enemy proximity. Area denial. The Explosive struct already exists for this.

### Medium-fit (more complex, worth considering)

**9. Tether** — link to an enemy, they can't move more than X distance from the tether point. If they try, they get pulled back. Good against fast chasers (RHOM, OCTA).

**10. Damage Amp debuff** — mark an enemy to take +50% damage from all sources for X seconds. Like Discord Orb (OW2) or Paradox's Pulse Grenade (Deadlock). Makes focus fire rewarding.

**11. Stealth** — become invisible for X seconds. Breaking stealth with an attack grants bonus damage/fire rate (Deadlock's Shadow Weave pattern). High skill expression.

**12. Healing field** — drop a zone that heals you while you stand in it. Trades mobility for sustain.

---

## Synergy / Combo Patterns (for the future item system)

The Hades duo boon model maps perfectly to the 12-slot system:
- Each ability/item applies a **theme** (fire, electric, ice, bleed, armor shred)
- Having two specific themes active unlocks a **combo bonus**
- Example: Slow + DoT = "Frostburn" — slowed enemies take bonus damage per tick
- Example: Knockback + Mine = mines trigger on knocked-back enemies hitting them

### Hades God Themes (reference)
| God | Theme | Status Effect | What It Does |
|-----|-------|--------------|--------------|
| **Zeus** | Lightning | Jolted | Enemy takes lightning damage when they attack |
| **Ares** | War/Doom | Doom | Delayed burst damage after a few seconds |
| **Athena** | Defense | Deflect | Reflects projectiles back at enemies |
| **Poseidon** | Water | Knockback | Pushes enemies away, bonus damage on wall collision |
| **Aphrodite** | Love | Weak | Enemy deals less damage |
| **Artemis** | Hunt | Critical | Chance for massive bonus damage |
| **Dionysus** | Wine | Hangover | Stacking DoT, slows at high stacks |
| **Demeter** | Cold | Chill/Freeze | Slow, eventually freeze solid at 10 stacks |

### Hades Duo Boon Examples (synergy reference)
- **Lightning Rod** (Zeus + Artemis): Cast stuck in enemies attracts lightning bolts
- **Curse of Longing** (Ares + Aphrodite): Doom repeatedly procs on Weakened enemies
- **Cold Fusion** (Zeus + Demeter): Jolted effect doesn't expire after one proc
- **Merciful End** (Ares + Athena): Doom triggers immediately when you Deflect
- **Hunting Blades** (Ares + Artemis): Blade Rifts seek out enemies
- **Smoldering Air** (Zeus + Aphrodite): Ultimate slowly charges automatically

---

## Risk-Reward Tradeoffs (from Dota 2 / Deadlock)

The game already has great examples (overheat QTE, rocket self-damage, ADS movement slow). More patterns:
- **Metal Skin** (Deadlock): bullet immune but movement slowed — could be a shield mode toggle
- **Ghost Form**: ability damage immune but take more bullet damage (or vice versa)
- **Berserker**: below 30% HP, gain massive damage boost — encourages risky play
- **Overcharge**: abilities cost HP instead of cooldown — glass cannon toggle
- **BKB degradation** (Dota 2): powerful effect that gets weaker each use — creates timing decisions

---

## Ultimate Charge Design (from Overwatch)

BFG already charges from damage dealt. This pattern could generalize:
- Each chassis gets a unique ultimate
- Charge from: damage dealt, damage taken, kills, or passive over time
- Once charged, dramatic transformation or massive effect
- Can't bank charge past 100% — use it or waste passive generation

### What makes ultimates feel impactful (OW2):
1. Change what the player can do (transformation)
2. Visible, dramatic effects (screen shake, big particles, sound)
3. Moments of vulnerability for the user (channeling) balanced against power
4. Enable combos with other abilities
5. Clear counter-play

---

## Deadlock Deep Dive (Closest Comparison)

### Deadlock's CC Hierarchy
1. **Stun** = full disable (hard CC)
2. **Sleep** = full disable but breaks on damage (soft CC, useful for setting up)
3. **Immobilize** = can't move but can shoot/cast (punishes positioning)
4. **Silence** = can't cast abilities but can shoot/move (punishes ability-reliant builds)
5. **Disarm** = can't shoot but can cast/move (punishes weapon-reliant builds)
6. **Curse** = can't shoot + can't cast + can't use items (ultimate disable)

Rock-paper-scissors: ability-heavy builds fear Silence, weapon-heavy builds fear Disarm, mobile builds fear Immobilize.

### Deadlock Active Items Worth Studying

**Mobility Actives**:
- Fleetfoot (1,250 souls): Sprint speed + bonus ammo for 4 seconds
- Warp Stone: Short-range teleport
- Majestic Leap: Launch high into the air, press again to slam down
- Phantom Strike (6,300 souls): Teleport TO an enemy, apply slow + disarm

**Defensive Actives**:
- Metal Skin: Bullet immunity but slowed
- Ethereal Shift: Near-immobile invulnerability, then grants speed + spirit power after
- Unstoppable: 6 seconds of CC immunity (cannot activate WHILE stunned)
- Debuff Remover: Instantly cleanse all negative effects
- Rescue Beam: Heal an ally and pull them to you

**Offensive Actives**:
- Silence Glyph (3,000): Skillshot that silences for 3 seconds
- Knockdown (3,000): Stuns target after 2-second delay
- Curse: Silence + Disarm + Mute, uncleansable
- Decay: -50% healing reduction + bleed DoT based on current HP
- Slowing Hex: Spirit damage + movement slow + silences movement abilities

**Utility Actives**:
- Shadow Weave: Stealth, breaking it grants fire rate + spirit power + melee damage
- Return Fire: Reflect incoming damage
- Heroic Aura: Sprint speed + bullet resist for you and nearby allies

### Deadlock Hero Ability Examples

**Seven**: Lightning Ball (AoE), Static Charge (delayed stun), Power Surge (bouncing shock on attacks), Storm Cloud ult (expanding channeled storm)

**Shiv**: Serrated Knives (thrown knife + bleed), Bloodletting (deferred damage passive + active clear), melee assassin kit

**Holliday**: Powder Keg (delayed explosion barrel), Spirit Lasso (pull + stun)

**Calico**: Gloom Bombs (cluster delayed detonation), Leaping Slash (dash + melee circle + heal on hero hit)

**Mirage**: Fire Scarabs + Tornado transformation (damage + lift + lifesteal)

### Deadlock Item System Structure
- 3 categories: Weapon (gun stats), Vitality (health/resist), Spirit (ability power)
- 16 item slots total (4 per category + 4 flex)
- Items are passive (stat sticks) or active (extra abilities on Z/X/C/V)
- Up to 4 active items at once

---

## Dota 2 Key Active Items (reference)

**Mobility**:
- **Blink Dagger**: 1200-unit instant teleport, 15s CD. Disabled for 3s after taking hero damage
- **Force Staff**: 600-unit push in facing direction. Can target allies or enemies

**Defensive**:
- **BKB**: 9 seconds spell immunity (degrades to 6 with use). 95s cooldown
- **Ghost Scepter**: Physical immune, 40% extra magic damage. Tradeoff item
- **Aeon Disk**: Auto-triggers below 70% HP: 2.5s of 0 damage dealt and received

**Offensive**:
- **Scythe of Vyse (Hex)**: Turn target into critter for 2.8s (silence + disarm + mute + slow)
- **Orchid Malevolence**: Silence 5s, then burst 30% of damage dealt during silence

**Utility**:
- **Refresher Orb**: Reset ALL ability cooldowns. Double-ultimate combos
- **Aghanim's Scepter**: Upgrades hero's ultimate, sometimes dramatically

---

## The 12-Slot Architecture (Synthesis)

Drawing from Deadlock's item categories + Hades' boon slots + existing design:

**Weapon Slots (modify M1/M2 primary weapon)**:
- Fire rate modifications
- Projectile modifications (pierce, bounce, split, explode on hit)
- On-hit effects (DoT, slow, lifesteal, damage amp)
- Magazine/heat modifications (existing overheat QTE system)

**Active Ability Slots (additional abilities on keybinds)**:
- Mobility (dash, blink, leap, charge)
- Offensive (grenade, railgun, BFG, shotgun blast)
- Defensive (shield, ethereal shift, unstoppable, cleanse)
- Utility (turret, mine, zone denial, tether)

**Passive System Slots (always-on modifications)**:
- Stat modifiers (speed, damage, health, resistance)
- On-condition triggers (on kill: heal, on dash: damage pulse, on low HP: speed boost)
- Build-defining passives (lifesteal on all damage, crit chance, damage reflection)

---

## Biggest Gaps in Current Ability Roster

1. **CC variety** — only have slow and knockback. No stun, root, fear, or blind
2. **Deployables** — no turrets, mines, or persistent zones
3. **DoT** — no bleed, burn, or poison effects
4. **Passive slot system** — all slots are active abilities, no stat modifiers or on-condition triggers

---

## Sources
- [TV Tropes MOBA Skill and Item Archetypes](https://tvtropes.org/pmwiki/pmwiki.php/Analysis/MOBASkillAndItemArchetypes)
- [Deadlock All Heroes and Abilities](https://www.sportskeeda.com/esports/all-deadlock-characters-abilities)
- [Deadlock CC Effects](https://gamerant.com/deadlock-all-crowd-control-effects-explained/)
- [Deadlock Wiki - Items](https://deadlock.wiki/Items)
- [Deadlock Wiki - Active Items](https://deadlock.wiki/Category:Active_Items)
- [Deadlock Active Items Guide](https://egamersworld.com/blog/deadlock-active-items-how-to-use-them-259Gi7vyoH)
- [Overwatch Ultimate Ability Wiki](https://overwatch.fandom.com/wiki/Ultimate_ability)
- [Hades Boons Wiki](https://hades.fandom.com/wiki/Boons)
- [Hades Duo Boons Wiki](https://hades.fandom.com/wiki/Duo_Boons)
- [Dota 2 Abilities Wiki](https://dota2.fandom.com/wiki/Abilities)
- [Dota 2 BKB](https://dota2.fandom.com/wiki/Black_King_Bar)
- [Valorant Abilities Guide](https://xreart.com/blogs/blog/agent-abilities-explained-mastering-every-role-in-valorant)
- [Risk of Rain 2 Item Synergies](https://steamcommunity.com/sharedfiles/filedetails/?id=1940344201)
