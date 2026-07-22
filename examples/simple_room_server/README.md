# Simple Room Server

A BBS-style server node: clients log in (optionally password-gated) and post/read messages from a shared bulletin board that persists across reboots.

## What it does

- `class MyMesh : public mesh::Mesh, public CommonCLICallbacks` (`MyMesh.h:91`) — subclasses `Mesh` directly like the repeater, but adds a persisted post log (`PostInfo` records) written to the board's filesystem.
- Client access is gated by `ROOM_PASSWORD`/`ADMIN_PASSWORD` and the same `ClientACL` role model (`GUEST`/`READ_ONLY`/`READ_WRITE`/`ADMIN`) used by the repeater, checked during the anonymous-request login flow.
- Exposes the same `CommonCLICallbacks` admin CLI as the repeater — see [CLI Commands](../../docs/cli_commands.md) — plus optional display/UI support.
- Reports itself with `FIRMWARE_ROLE "room_server"` (`MyMesh.h`).

## Building

Environment names follow `<Board>_room_server`:

```bash
pio run -e RAK_4631_room_server
pio run -e RAK_4631_room_server_ethernet
pio run -e Heltec_v3_room_server
```

## Configuring

Set the room/admin password and manage posts via the serial CLI ([CLI Commands](../../docs/cli_commands.md)), or connect with a companion app that supports room servers to read/post messages over the mesh.
