# Examples

`stand/` is the set the test stand runs with: one object trader on the north-west
airfield, eleven goods of vanilla classes in four categories (the trader deals in three
of them, so the fourth is "not taken here"), and the stand's money: the notes and the
wallet of the phase 0 probe, worth 5, 50 and 500. Copy the three files into
`<profile>\OpenZone\` of a server that also loads `@OpenZone_TraderProbe`.

`Pos` of a trader is the object's own position, exactly as the game reports it for a
placed object and as an editor exports it: for a building that is the middle of its
model, so the stall's `Pos` is 1.77 m above the runway it stands on. `Radius` is measured
from `Pos`; this stall's body lies south of its origin, hence the 8 metres.

They are not a starting point for a real server: the note classes exist on the stand only.
