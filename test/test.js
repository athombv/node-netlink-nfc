const nfc = require('..');

console.log(nfc);

const adapters = nfc.getAdapters();
console.log(adapters);

const adapter = Object.values(adapters)[0];

adapter.startPolling(nfc.constants.NFC_PROTO_MIFARE | nfc.constants.NFC_PROTO_ISO14443);


nfc.on('tag.read', console.log);
