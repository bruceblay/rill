# Ensemble sync — prototype proposal

Status: design only, not implemented in the current firmware. Start with two physical devices.

Devices need a compatible radio and firmware implementing the same protocol; ESP32 branding alone does not provide musical interoperability. ESP-NOW is the proposed transport. Different synthesizers could participate if they understand the shared musical messages. Transport support must be checked for each chip and SDK.

One elected conductor broadcasts versioned session ID, conductor ID, monotonic timestamp, tempo, future bar boundary, tonic, mode, harmony position and generation seed. Keep packets small, with explicit integer encoding and sequence numbers. Receivers schedule future events locally rather than triggering notes on packet arrival. Timestamp exchanges estimate clock offset and drift; corrections adjust future event deadlines gradually. Never jump the running audio sample clock.

Conductor election uses stable device IDs and a discovery interval; a session change takes effect on a bar boundary. Ignore stale, malformed or incompatible-version packets. After packet loss, continue the last tempo locally; after a longer timeout, fall back to solo at a phrase boundary. Rejoining should not retrigger or cut sounding notes.

Musical roles: conductor main motif, follower sparse answering voice; later devices can take lower support or occasional upper punctuation. Shared harmony alone is insufficient: coordinate density and registers so additional devices do not simply multiply the same crowded pattern. Local visual generations stay independent.

First engineering step: a separate two-device clock diagnostic, measuring scheduled-event skew, drift and packet loss while the current audio and display loads run. Then integrate future-bar scheduling into the score. Verify clean joins, conductor loss and simultaneous regeneration before expanding group size. No claims of sample-accurate audio phase or unlimited participants.
