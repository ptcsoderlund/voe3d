# 02 — The shot light flashes under the tank

## Seen
When I shoot, the light that flashes is now under the tank, not at the barrel.

## Expected
The shot's light flashes where the muzzle explosion's particles appear, at the end of the barrel,
and stays there as the tank drives and the turret turns. The same goes for an enemy tank's shots.
Wrecks and explosions still light up where they are, as now.

## How to reproduce
1. Open `examples/tank_game` and press Play.
2. Shoot. The ground lights up under the tank instead of around the muzzle explosion.
3. After the fix, the light flashes at the muzzle explosion, at the end of the barrel. Turn the turret
   and shoot again: the light follows the barrel.
4. Let an enemy tank shoot at you: its light flashes at its own barrel too.
