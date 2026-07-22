# Core Library Architecture

This page describes how the core routing library in `src/` is put together, and the contract that a firmware application (see `examples/`) needs to fulfil to build a node on top of it. For the on-the-wire packet layout itself, see [Packet Format](./packet_format.md) and [Payloads](./payloads.md) — this page is about the C++ classes and control flow, not the byte layout.

## Layers

```
examples/<app>/MyMesh   <- one flashable firmware app; implements the app-specific virtuals
src/helpers/BaseChatMesh  <- optional layer: contact/channel table lookups for chat-style apps
src/Mesh                  <- protocol layer: payload types, encryption, routing decisions
src/Dispatcher             <- scheduler layer: radio I/O, CAD/duty-cycle, airtime budget
src/Packet                 <- fixed-size wire struct shared by every layer
```

`Dispatcher` (`src/Dispatcher.h/.cpp`) knows nothing about what a packet *means* — it owns the `Radio` driver, the `PacketManager` (packet pool + queues), and the clock, and is responsible purely for when to transmit/receive and how to pace retransmissions. `Mesh : public Dispatcher` (`src/Mesh.h/.cpp`) is where payload semantics live: it owns the node's `Identity`, `RNG`, `RTCClock` and `MeshTables` (dedup store), decrypts/interprets packets, and decides whether/how to forward them.

`Mesh` is abstract. Firmware apps subclass it in one of two ways:

- **Directly**, for apps that don't need a contact/channel address book: `simple_repeater`, `simple_room_server`, `simple_sensor`.
- **Via `BaseChatMesh : public mesh::Mesh`** (`src/helpers/BaseChatMesh.h`), which adds a `ContactInfo[]` table and implements the peer/channel lookup virtuals for chat-style apps: `companion_radio`, `simple_secure_chat`.

## Packet lifecycle

1. `Dispatcher::loop()` (`src/Dispatcher.cpp`) drains the radio and, on receipt, allocates a `Packet` from the `PacketManager` pool and parses the header.
2. Flood packets get a small SNR/airtime-based random delay (collision avoidance for simultaneous rebroadcasters) before being queued; direct packets are processed immediately.
3. `Mesh::onRecvPacket()` demultiplexes on route type + payload type, decrypts the payload if applicable (see Identity & encryption below), and calls the relevant `onXxxRecv()` virtual (`onPeerDataRecv`, `onAdvertRecv`, `onAckRecv`, `onTraceRecv`, …) that the app overrides.
4. `Mesh::routeRecvPacket()` appends this node's identity-hash to the packet's path and returns an action telling `Dispatcher` whether to forward it: release (drop), hold (app keeps it, e.g. a room server storing an anonymous request while it authenticates), or retransmit-delayed (continue the flood/direct route), gated by the app's `allowPacketForward()` override (forwarding is **off by default** — only repeater/transport-style apps enable it).
5. On the way out, apps call helpers like `createAdvert`, `createDatagram`, `createAnonDatagram`, `createAck`, `createTrace`, then `sendFlood`/`sendDirect`/`sendZeroHop`. `Dispatcher::checkSend()` enforces duty-cycle/CAD rules and hands the serialized bytes to the radio driver.

There is no hop-limit/TTL field on the wire — flood termination relies on `MAX_PATH_SIZE` (path buffer full) and per-node dedup tables, not a decrementing counter.

## Roles are a build-time choice, not a runtime concept

`Mesh`/`Dispatcher` have no notion of "this node is a repeater" vs "this node is a sensor." A node's role is simply which `examples/` app it was compiled and linked against — each app subclasses `Mesh` (or `BaseChatMesh`) and only implements the behaviour appropriate for its role (e.g. `simple_repeater` overrides `allowPacketForward()` to return true; `companion_radio` does not).

The `ADV_TYPE_CHAT` / `_REPEATER` / `_ROOM` / `_SENSOR` values (`src/helpers/AdvertDataHelpers.h`) carried inside a signed advert payload are purely descriptive metadata that *other* nodes read to display an icon/label for a discovered peer — core routing treats every node identically regardless of this field.

## Identity and encryption

`Identity` / `LocalIdentity` (`src/Identity.h/.cpp`) wrap an Ed25519 keypair used two ways:

- **Signing**: node adverts (`PAYLOAD_TYPE_ADVERT`) are signed with Ed25519 and sent as signed plaintext — anyone can read an advert, but not forge one.
- **Key exchange**: the same Ed25519 keypair is transposed into an X25519 curve to derive a per-peer ECDH shared secret (`LocalIdentity::calcSharedSecret`), used to encrypt everything else.

Every other payload type that carries app data (`REQ`, `RESPONSE`, `TXT_MSG`, `GRP_TXT`, `GRP_DATA`, `ANON_REQ`, `PATH`) is AES-encrypted then HMAC-SHA256 authenticated with a truncated tag (`Utils::encryptThenMAC` / `MACThenDecrypt`, `src/Utils.cpp`), keyed by that shared secret (or a channel pre-shared key for group payloads).

Node/path identity hashes are deliberately short (`PATH_HASH_SIZE` = 1 byte by default, see [Packet Format](./packet_format.md) for the 2/3-byte extension), so hash collisions between different peers are expected. Decrypt paths therefore loop over every candidate returned by `searchPeersByHash()` / `searchChannelsByHash()` and accept the first one whose MAC check succeeds, rather than assuming a hash uniquely identifies a peer.

## The extension contract (`Mesh.h`)

A concrete `Mesh` subclass is constructed with every hardware/platform dependency injected as an abstract interface — `Radio`, `MillisecondClock`, `RNG`, `RTCClock`, `PacketManager`, `MeshTables` — so the core library itself never depends on a specific board or radio chip. The board/example layer supplies concrete implementations (e.g. `StaticPoolPacketManager` for `PacketManager`, `SimpleMeshTables` for `MeshTables`, a RadioLib wrapper under `src/helpers/radiolib/` for `Radio`).

Virtuals an app typically overrides:

| Category | Examples |
|---|---|
| Forwarding policy | `allowPacketForward`, `filterRecvFloodPacket`, `getRetransmitDelay`, `getDirectRetransmitDelay` |
| Peer/channel lookup | `searchPeersByHash`, `getPeerSharedSecret`, `searchChannelsByHash` (stubbed in `Mesh`, implemented by `BaseChatMesh` against its `ContactInfo` table) |
| Data arrival | `onPeerDataRecv`, `onAnonDataRecv`, `onGroupDataRecv`, `onAdvertRecv`, `onAckRecv`, `onPeerPathRecv`/`onPathRecv`, `onTraceRecv`, `onControlDataRecv`, `onRawDataRecv` |
| Diagnostics/tuning | `logRx`/`logTx`/`logTxFail` (inherited from `Dispatcher`), `getAirtimeBudgetFactor`, `getCADEnabled`, `getDutyCycleWindowMs` |

Outbound composition helpers (`createAdvert`, `createDatagram`, `createAnonDatagram`, `createGroupDatagram`, `createAck`, `createPathReturn`, `createTrace`, `createControlData`) plus the send primitives (`sendFlood`, `sendDirect`, `sendZeroHop`) are the app-facing API for producing traffic.

`BaseChatMesh` (`src/helpers/BaseChatMesh.h`) is the reference implementation of the peer/channel lookup virtuals for contact-based apps: it maintains a `ContactInfo[]` table, implements `searchPeersByHash`/`getPeerSharedSecret`/`onAdvertRecv`/`onPeerDataRecv`/`onAckRecv`, and exposes a further set of app-specific pure virtuals (`onDiscoveredContact`, `onMessageRecv`, `onContactRequest`, `calcFloodTimeoutMillisFor`, …) for the concrete example to implement.

## Supporting subsystems in `src/helpers/`

- **`SimpleMeshTables`** — fixed ring buffer of recently-seen content hashes; gates both flood-rebroadcast suppression and direct-route double-forward suppression, and tracks dup counters used by the stats CLI.
- **`StaticPoolPacketManager`** — the concrete `PacketManager`. Allocates a fixed pool of `Packet` objects once at construction (the one legitimate dynamic allocation the "no dynamic allocation" rule refers to); after that, `allocNew()`/`free()` only move pointers between fixed-capacity queues. A single packet is owned by exactly one place at a time (unused pool, outbound queue, inbound queue, or held by the app via a manual-hold action) — there's no refcounting.
- **`ClientACL`** — maps a peer `Identity` to a role (`GUEST`/`READ_ONLY`/`READ_WRITE`/`ADMIN`) plus a stored return-path and shared secret; used by repeater/room-server apps to gate CLI/login commands, independent of core routing.
- **`TransportKeyStore`** — derives the 16-bit transport codes carried by `ROUTE_TYPE_TRANSPORT_*` packets, letting bridge/transport nodes (see `src/helpers/bridges/`, e.g. RS232 or ESP-NOW bridges) scope flood domains across a bridge link without polluting the normal LoRa mesh.
- **`CommonCLI`** — the shared serial/BLE admin command-line implementation (get/set prefs, stats, region/radio params) that every example app wires in via the `CommonCLICallbacks` interface. Not part of `Mesh` itself, but the de facto standard admin UI across all role examples; see [CLI Commands](./cli_commands.md) for the command reference.

## Practical guidance for changing core code

- Treat `Dispatcher` as "radio scheduling only" and `Mesh` as "protocol semantics only" — a change that needs both usually belongs as a new virtual/hook rather than blurring the two.
- Keep wire-format changes backward compatible with `PAYLOAD_VER_1` where possible; the payload-version bits exist specifically to allow future formats without breaking older firmware on the same mesh.
- Remember identity/path hashes are intentionally short and ambiguous — any new lookup-by-hash code should loop over candidates and verify via MAC/signature rather than assuming uniqueness.
- `src/` must stay buildable under the native (`env:native`) test target, which does not define `ARDUINO` — new core code that needs an Arduino-only API must guard it with `#ifdef ARDUINO`, following the existing pattern in `src/Utils.cpp` and `src/MeshCore.h`.
