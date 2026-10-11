# Bidirectional keyboard and mouse input (experimental fork feature)

Branch: `feature/bidirectional-input`.

Install builds of this branch on both Windows and macOS. In **Preferences → General**,
enable **Share keyboard and mouse from both computers (experimental)** on both,
then restart the core on both computers. The persisted setting is
`core/bidirectionalInput=true`. It defaults to false.

The Windows computer remains the network server and owns the layout and hotkeys.
Either computer's physical keyboard, mouse or trackpad controls the currently
selected desktop. Touching a different device changes the physical input source;
it does not jump to that device's own desktop. Use the configured edges or hotkeys
to select a desktop. This is a shared focus, not two independent users/cursors.

Configured keyboard hotkeys are registered on the Mac and forwarded to the server,
so the existing Ctrl+Alt+1 / Ctrl+Alt+2 LG monitor shortcuts work from either keyboard.
The monitor helper still runs on Windows. A shortcut conflicting with a macOS
system shortcut may fail to register; the core logs that failure.

## Implementation

This fork negotiates protocol 1.9. Ordinary clients still negotiate their existing
1.0–1.8 formats and use ordinary one-way sharing. A client with bidirectional input
enabled refuses an older server or a server without the option, before enabling
capture. This private 1.9 extension is not claimed to interoperate with future
upstream protocol 1.9 implementations.

- `DBCP`: server permission for reverse input, before handshake completion.
- `DBFC`: recipient epoch and whether this client's desktop has focus.
- `DBHK`: configured hotkey key/modifier pairs.
- `DBIN`: client physical input, including its recipient epoch and keyboard layout.

Clients use the existing primary capture path. They allow physical events locally
when their desktop is selected and consume/forward them when another desktop is
selected. The server routes reverse events to its local input injector or another
client. Locally delivered client events are never echoed to that client. Existing
layout mappings work for movement from either device.

Windows injections carry a `dwExtraInfo` marker; Quartz keyboard, mouse, scroll
and media injections carry `kCGEventSourceUserData`. Capture ignores these events.
The marker is per-event, so asynchronous injections do not rely on a time window.

A chord or drag belongs to one physical source until release. Focus changes and
source disconnections release recorded keys/buttons on their original recipient;
stale recipient epochs and unmatched releases are rejected. A paired hotkey release
is allowed after its press changed focus, to preserve actions configured on release.

## Validation and limits

`BidirectionalInputTests` exercises the real server, client encoder and server
decoder with simulated platform devices. It covers reverse keyboard injection,
source ownership, reverse cursor movement/edge switching, no echo, stale events,
focus changes, disconnect cleanup, malformed input, disabled mode, remote hotkeys
and client capture focus restoration. Existing protocol-format and GUI regression
tests also run. A branch workflow builds the Mac paths and runs these tests.

Physical Mac/Windows testing remains necessary: Quartz event-tap permissions,
secure input, trackpad behavior, modifier translation, cursor visibility, hotkey
registration, Windows elevation and the physical LG source switch are not proved
by simulated platform tests. macOS needs Accessibility/Input Monitoring permissions
for this build. Start with no keys or buttons held during connection.

Use one physical keyboard/mouse at a time. This is not simultaneous multi-user input:
another source's relayed input is ignored while a chord or drag is held. Physical
input on the selected machine can still be delivered by its OS. Keyboard broadcast
and mouse-button hotkey rules retain their ordinary server behavior and are not
extended to reverse input in this implementation. Linux reverse capture is not
enabled.
