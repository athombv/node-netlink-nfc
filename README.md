netlink-nfc
========

NodeJS library for accessing netlink NFC sockets (uses N-API).

This library only supports reading target IDs, and requires Node 8.6.0 or higher.

## Installation

### Step 1: Prerequisites
This module requires Linux with netlink support and NFC (kernel) drivers.
In order to compile the module you need to install libnl-3-dev and libnl-genl-3-dev.

On Linux, you want:

    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install libnl-3-dev libnl-genl-3-dev

### Step 2: Installation

To install it, use npm:

    npm install netlink-nfc


## Initialization and Information

```js
const nfc = require('netlink-nfc');

console.log(nfc);

const adapters = nfc.getAdapters();
console.log(adapters);

const adapter = Object.values(adapters)[0];

adapter.startPolling(nfc.constants.NFC_PROTO_MIFARE | nfc.constants.NFC_PROTO_ISO14443);

nfc.on('tag.read', console.log);
```

