# Simple Secure Chat

A minimal terminal-based reference implementation of end-to-end encrypted 1:1 and channel chat over the mesh — the simplest example of using `BaseChatMesh` directly, useful as a starting point for a custom chat-style app.

## What it does

- Everything lives in one file: `class MyMesh : public BaseChatMesh, ContactVisitor` (`main.cpp:73`).
- You type commands into a serial terminal (Serial Monitor in VS Code, or a Serial USB terminal app on Android); `MyMesh` manages the `ContactInfo`/`ChannelDetails` tables and prints replies as plain text/hex to the console.
- No binary companion protocol and no persistence layer beyond what `BaseChatMesh` provides in RAM — this is intentionally the smallest possible demonstration of the `BaseChatMesh` callback contract (`onDiscoveredContact`, `onMessageRecv`, `onContactRequest`, …).

Command reference: [Terminal Chat CLI](../../docs/terminal_chat_cli.md).

## Building

Environment names use the `terminal_chat` suffix:

```bash
pio run -e RAK_4631_terminal_chat
pio run -e Heltec_v3_terminal_chat
```

## Using it

Open the device's serial port at 115200 baud (VS Code Serial Monitor, `screen`, PuTTY, or a phone USB-serial terminal app) and use the commands in [Terminal Chat CLI](../../docs/terminal_chat_cli.md) (`set freq`, `advert`, `list`, `to`, `send`, …).
