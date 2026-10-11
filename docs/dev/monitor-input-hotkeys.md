# LG monitor input hotkeys

On the Windows server, open **Configure Server → Hotkeys** and click **Add HDMI /
USB-C hotkeys**. The presets are editable:

- **Ctrl+Alt+1** selects HDMI 1 and sends keyboard/mouse input to the server.
- **Ctrl+Alt+2** selects USB-C and sends keyboard/mouse input to the first client.

Existing shortcuts with those keys are kept. Each new hotkey uses the **Switch
monitor input (LG)** action. Edit the action to choose the monitor index (starting
at 0), input, optional keyboard/mouse destination, and control utility path. A
blank destination changes only the monitor input. You can also create this action
on any existing hotkey, on press or release.

The action invokes `writeValueToDisplay.exe` with the proprietary LG command
verified on a 27BK67U connected to an NVIDIA GPU:

```
HDMI 1: <index> 0x90 0xF4 0x50
USB-C:  <index> 0xD1 0xF4 0x50
```

This helper is an external dependency and is not distributed in this repository.
Place your copy beside `deskflow.exe`, or select its location in the action editor.
The local preset also checks `~/tools/monitor-control/writeValueToDisplay.exe`.
The command runs without a console window as the active Windows session user,
including when Deskflow's core runs through its SYSTEM service. It does not run
through a shell. Helper paths are hex-encoded in exported configuration to preserve
spaces and configuration punctuation.

Hotkeys use the Windows server's physical keyboard. They continue to work while
that keyboard controls the Mac. The Mac's own keyboard does not send these
hotkeys back to Windows. Clients need no changes.

This is an LG-specific input action, not a general monitor DDC implementation.
Successful process launch is logged; the helper's detached completion and monitor
acknowledgement are not checked. The computer destination changes immediately
after launching the helper, without waiting for the monitor's input transition.

Validation target: `MonitorInputTests`, covering verified helper arguments,
path encoding, action persistence, core parser round trips, release-trigger
serialization, and editor validation/save behavior.
