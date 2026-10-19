# Display arrangement

Server configuration has an optional **Display arrangement** tab. Add and name
computers in the Computers tab, then enable **Use display sizes and positions
instead of the grid**. Select each computer to edit its X, Y, width and height,
or drag its rectangle. Nearby edges snap together. Green segments show where
the cursor can cross. Positions may be negative.

Use a consistent unit for all rectangles. Pixels work for desktops with matching
pixel densities. Physical dimensions (for example millimetres) are more useful
for aligning differently sized or scaled displays on a desk. The initial sizes
are placeholders; enter the dimensions appropriate for your setup.

Aligned edges, including those separated by gaps, generate partial-edge links using Deskflow's existing
configuration format. Each edge segment links to the nearest screen in that direction, preserving its aligned entry position. Closer screens block farther screens only over their shared span. Corner-only contact, overlaps and disconnected
groups cannot be saved while this mode is enabled. Turning the mode off returns
to the grid links and retains the manual dimensions for later use.

Clients already send aggregate desktop X/Y/width/height in the DINF protocol
message. They do not report physical dimensions or a list of individual monitor
rectangles. The host forwards reported desktop dimensions to the editor; the
**Use reported pixel size** button applies them explicitly without overwriting
saved manual dimensions on reconnect. Reports are available after a connection;
reconnect after a monitor configuration change to refresh them. Each
rectangle represents one computer's whole desktop. Multiple monitors belonging
to one computer cannot be positioned separately with this feature.

The generated links work with stock Deskflow cores and clients. The custom host
core additionally forwards desktop size reports to the GUI; clients need no
changes. Build the GUI, core and daemon from the same checkout when
installing a custom host package because their local IPC version identifiers
must match.

Validation targets: `DisplayLayoutTests`, `ComputerTests`, `ServerConfigTests`.
The new tests cover asymmetric mappings, reciprocal cursor positions, multiple
neighbors on one edge, negative positions, invalid layouts, persistence and
editor cancellation/reset isolation.
