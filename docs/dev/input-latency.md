# Input latency diagnostics

Set `DESKFLOW_INPUT_TIMING=1` in the core process environment to enable aggregate
timing logs. Each active stage emits at most one INFO summary every five seconds:

- `windows-capture-hook`: processing in the Windows low-level mouse hook.
- `server-enqueue-absolute`: serialization and enqueueing of outgoing pointer positions.
- `client-receive-batch`: parsing and dispatching one batch of received messages.
- `client-inject-absolute` / `client-inject-relative`: applying motion through the
  client and platform injection path.

`mean_work_us` and `max_work_us` measure synchronous local work, not network transit
or compositor presentation. `max_queue_bytes` is the input stream's reported
buffer size at the start of a receive batch; it may be packet-oriented depending
on the stream filter. `max_start_gap_ms` includes intentional idle time, so compare
it only while continuously moving the mouse. Clocks on different computers are
not synchronized by these diagnostics. No pointer coordinates or key contents are
logged by this feature. Timing is off by default and does not change wire messages.

The receiver applies pending coalesced motion after at most 64 complete messages
or approximately 2 ms of processing, whichever comes first, and queues a
continuation if input is still buffered. The time limit is checked between
messages; it cannot interrupt an individual slow parser or OS call. This avoids
waiting indefinitely for a busy receive buffer to empty and gives native events
and timers another turn. Existing flushes before buttons, keys, wheel events and
screen leave remain in place; enter discards motion from the previous focus.
Absolute positions can be coalesced, while relative deltas must be accumulated.

Tests: `ServerProxyTests::handleData_busyStreamYieldsAndReschedules` checks bounded
work and continuation without a new socket notification.
`BidirectionalInputTests::receiveBatchFlushesMotionBeforeYieldAndPreservesClickPosition`
exercises the real parser, client and computer path, checking intermediate motion
delivery and the positions of a press and release spanning receive batches.

This update is wire-compatible with existing clients. Receiver improvements apply
only on updated clients. It does not eliminate Wi-Fi pauses, add prediction, or
replace TCP with a datagram transport.
