const netlinknfc_base = require('bindings')('netlinknfc');
const EventEmitter = require('events').EventEmitter;

class NetlinkNFC extends EventEmitter {
}

module.exports = new NetlinkNFC();
Object.assign(module.exports, netlinknfc_base);
netlinknfc_base.emit = module.exports.emit.bind(module.exports);