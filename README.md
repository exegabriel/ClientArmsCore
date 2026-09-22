### ClientArmsCore

Custom fork focused on client-side arms and wearable refresh for bot skin management.

This project keeps the original arms-refresh behavior but is adapted to the custom workflow used by the local SourceMod setup:

- maintain stable viewmodel refresh
- handle glove / wearable state for custom skins
- support client-side update logic without coupling the server plugin to visual internals

### Responsibilities

- Server plugin decides what should be applied
- ClientHook manages client state
- ClientArmsCore applies the final refresh to the viewmodel / arms state

### Notes

This fork is project-owned and intentionally diverges from the upstream ArmsFix+ identity to avoid confusion during local testing and future plugin work.