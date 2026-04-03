# Custom Backend Integration

Runtime layers are separated at the repository level:

- `client` contains the Telegram Desktop fork and custom adapter layer
- the backend runtime lives in a separate root-level folder

The first adapter slice lives in `Telegram/SourceFiles/custom_backend` and provides:

- DTO parsing for `User`, `Chat`, `Message`, `Channel`, `Presence`, `Attachment`, `Reaction`
- HTTP client methods for `login`, `refresh`, `fetchChats`, `fetchMessages`, `sendMessage`, `fetchChannels`
- WebSocket realtime client for `session.ready`, `message.created`, `message.updated`, `message.deleted`, `reaction.updated`, `presence.updated`, `typing.updated`, `sync.required`

Suggested next hook points in the desktop fork:

- replace auth flow entrypoints with `CustomBackend::HttpClient::login`
- map dialog loading to `fetchChats`
- map history loading to `fetchMessages`
- map send flow to `sendMessage`
- feed realtime updates into session/dialog/history state from `RealtimeClient`

No client code reads files from the backend runtime folder, and no backend code imports or references `client`.
