game-tech
=========

Depending on available computer resources/optimization:
This game is either about: base building (and destroying!), or sniping stuff, or maybe both.


Current capacity of blocks/"atoms" we can simulate at 60fps is ~1728, or roughly a cube 
that is 12 * 12 * 12.

We could mess around with optimization by activating blocks and deactivating depending on how close our
projectile is.


We can also optimize calculations on an individual "cube" of these atoms by doing calculations on
differing segments of the block at different times. (so instead of messing with the whole cube each
frame, we mess with every other atom, or 1/3 of atoms, etc).


The only collisions we are checking are a few of these cubes, like the "core atom"


Weapons might include:
canons, sniper rifle (with upgrades).
sword (to slice through cubes), spear -- gets stuck in a cube if it collides with x amount of atoms in a cube, or just the center atom...
Grenade:
Basically you throw a grenade, then it causes a huge force in many directions towards enemies.
Explosive force:
You cause a cube to simply explode, from the inside, symmetrically. Oh snap.



Enemies:
Opponents base. While we aim at the oponent base, our base (made out of dynamic cubes/atoms) gets transformed into planes/static objects. so we can make this turn based.

--or--

Robots, who walk from A-B, are made of cube atoms, and you must snipe them! They might also shoot at you.
Implement: "desintegration" animation for a cube. (random/or sequence/wave of atoms will blink and disappear like reacting to a shockwave)
