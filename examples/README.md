# Examples

`stand/` is the set the test stand runs with: one object trader on the north-west
airfield, eleven goods of vanilla classes in four categories (the trader deals in three
of them, so the fourth is "not taken here"), and the stand's money: the notes and the
wallet of the phase 0 probe, worth 5, 10, 20, 50, 100 and 500. Copy the three files into
`<profile>\OpenZone\` of a server that also loads `@OpenZone_TraderProbe`.

`Pos` of a trader is the object's own position, exactly as the game reports it for a
placed object and as an editor exports it: for a building that is the middle of its
model, so the stall's `Pos` is 1.77 m above the runway it stands on. `Radius` is measured
from `Pos`; this stall's body lies south of its origin, hence the 8 metres.

`Exchange` of a trader is the change tab: 1 (the default) changes the server's notes
at face value, a 100 into five 20s; 0 leaves the tab out. Only item money has notes,
and it takes two kinds of them.

They are not a starting point for a real server: the note classes exist on the stand only.
