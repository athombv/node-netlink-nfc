#include "netlink.h"

#include <node_api.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <map>

#include <netlink/errno.h>
#include <linux/socket.h>
#include <linux/nfc.h>

#include "buffer.hpp"

class JSConvertible {
  public:
    virtual napi_value ToJS(napi_env env) {
        return NULL;
    };
};


class NFC_Target : public JSConvertible {
  public:
    NFC_Target();
    NFC_Target(uint32_t idx, uint32_t target_idx,
			uint32_t protocols, uint16_t sens_res, uint8_t sel_res,
			uint8_t *nfcid, uint8_t nfcid_len,
			uint8_t iso15693_dsfid,
			uint8_t iso15693_uid_len, uint8_t *iso15693_uid);

    napi_value ToJS(napi_env env);

  private:
    uint32_t _adapter_idx;
    uint32_t _target_idx;
    uint32_t _protocols;
    uint16_t _sens_res;
    uint8_t _sel_res;
    buffer _nfcid;
    uint8_t _iso15693_dsfid;
    buffer _iso15693_uid;
};

class NFC_Adapter : public JSConvertible {
  public:
    NFC_Adapter();
    NFC_Adapter(uint32_t idx, const char *name, uint32_t protocols, bool powered);

    const char* GetName();
    void GetTargetsDone();

    static napi_value Enable(napi_env env, napi_callback_info info);
    static napi_value Disable(napi_env env, napi_callback_info info);

    //(uint8_t comm_mode, uint8_t rf_mode);
    //napi_value LinkUp(napi_env env, napi_callback_info info);
    //napi_value LinkDown(napi_env env, napi_callback_info info);

    //(uint32_t im_protocols, uint32_t tm_rotocols); //NL
    static napi_value StartPoll(napi_env env, napi_callback_info info);
    static napi_value StopPoll(napi_env env, napi_callback_info info);

    napi_value ToJS(napi_env env);

    std::map<uint32_t, NFC_Target> _targets;
  private:
    uint32_t _adapter_idx;
    std::string _name;
    uint32_t _protocols;
    bool _powered;

    uint32_t _active_protocols_i;
    uint32_t _active_protocols_t;
};

/*
class NFC_Device {
  public:
  private:
    buffer _nfcid;
};*/


class NFC_Manager {
  public:
    NFC_Manager();
    ~NFC_Manager();

    void Emit(std::string event, JSConvertible& arg);
    void Emit(std::string event, uint32_t idx);
    void Emit(std::string event, uint32_t idx1, uint32_t idx2);

    static napi_value GetAdapters(napi_env env, napi_callback_info info);
    //napi_value GetDevices(napi_env env, napi_callback_info info);

    napi_value Init(napi_env env);

    std::map<uint32_t, NFC_Adapter> _adapters;
    //std::map<uint32_t, NFC_Device> _devices;
  private:
    napi_env _env;
    napi_ref _self;
};


NFC_Manager _nfc_manager;

NFC_Adapter::NFC_Adapter() {

}

NFC_Adapter::NFC_Adapter(uint32_t idx, const char *name, uint32_t protocols, bool powered)
    : _adapter_idx(idx), _name(name), _protocols(protocols), _powered(powered) {
    _active_protocols_i = 0;
    _active_protocols_t = 0;
}

const char *NFC_Adapter::GetName() {
    return _name.c_str();
}

napi_value NFC_Adapter::Enable(napi_env env, napi_callback_info info) {
    uint32_t adapter_idx = 0;
    napi_get_cb_info(env, info, 0, NULL, NULL, (void**)&adapter_idx);
    int res = __nfc_netlink_adapter_enable(adapter_idx, true);

    if(res != 0) {
        nl_perror(res, NULL);
        //napi_throw_error(env, "", nl_geterror(res));
    }
    return nullptr;
}

napi_value NFC_Adapter::Disable(napi_env env, napi_callback_info info) {
    uint32_t adapter_idx = 0;
    napi_get_cb_info(env, info, 0, NULL, NULL, (void**)&adapter_idx);
    __nfc_netlink_adapter_enable(adapter_idx, false);
    return nullptr;
}

void NFC_Adapter::GetTargetsDone() {
    if(_active_protocols_i != 0 || _active_protocols_t != 0) {
        _targets.clear();
        __nfc_netlink_start_poll(_adapter_idx, _active_protocols_i, _active_protocols_t);
    }
}

napi_value NFC_Adapter::StartPoll(napi_env env, napi_callback_info info) {
    uint32_t adapter_idx = 0;

    napi_value argv[2];
    size_t argc = 2;
    napi_get_cb_info(env, info, &argc, argv, NULL, (void**)&adapter_idx);

    NFC_Adapter& adapter = _nfc_manager._adapters[adapter_idx];

    adapter._active_protocols_i = NFC_PROTO_MIFARE_MASK | NFC_PROTO_ISO14443_MASK;
    adapter._active_protocols_t = 0;

    napi_get_value_uint32(env, argv[0], &adapter._active_protocols_i);
    napi_get_value_uint32(env, argv[1], &adapter._active_protocols_t);

    adapter._active_protocols_i &= _nfc_manager._adapters[adapter_idx]._protocols;
    adapter._active_protocols_t &= _nfc_manager._adapters[adapter_idx]._protocols;

    if(!_nfc_manager._adapters[adapter_idx]._powered)
        __nfc_netlink_adapter_enable(adapter_idx, true);

    int res = __nfc_netlink_start_poll(adapter_idx, adapter._active_protocols_i, adapter._active_protocols_t);

    if(res != 0) {
        nl_perror(res, NULL);
        //napi_throw_error(env, "", nl_geterror(res));
    } else {
        adapter._targets.clear();
    }
    return nullptr;
}

napi_value NFC_Adapter::StopPoll(napi_env env, napi_callback_info info) {
    uint32_t adapter_idx = 0;
    napi_get_cb_info(env, info, 0, NULL, NULL, (void**)&adapter_idx);
    int res = __nfc_netlink_stop_poll(adapter_idx);

    NFC_Adapter& adapter = _nfc_manager._adapters[adapter_idx];

    adapter._active_protocols_i = 0;
    adapter._active_protocols_t = 0;

    if(res != 0) {
        nl_perror(res, NULL);
        //napi_throw_error(env, "", nl_geterror(res));
    }
    return nullptr;
}

napi_value NFC_Adapter::ToJS(napi_env env) {
    napi_value result, prop;

    napi_create_object(env, &result);

    napi_create_uint32(env, _adapter_idx, &prop);
    napi_set_named_property(env, result, "id", prop);

    napi_create_uint32(env, _protocols, &prop);
    napi_set_named_property(env, result, "protocols", prop);

    napi_create_string_utf8(env, _name.c_str(), -1, &prop);
    napi_set_named_property(env, result, "name", prop);

    napi_get_boolean(env, _powered, &prop);
    napi_set_named_property(env, result, "powered", prop);

    napi_create_function(env, NULL, 0, Enable, (void*)_adapter_idx, &prop);
    napi_set_named_property(env, result, "enable", prop);

    napi_create_function(env, NULL, 0, Disable, (void*)_adapter_idx, &prop);
    napi_set_named_property(env, result, "disable", prop);

    napi_create_function(env, NULL, 0, StartPoll, (void*)_adapter_idx, &prop);
    napi_set_named_property(env, result, "startPolling", prop);

    napi_create_function(env, NULL, 0, StopPoll, (void*)_adapter_idx, &prop);
    napi_set_named_property(env, result, "stopPolling", prop);

    napi_create_object(env, &prop);
    for (std::map<uint32_t, NFC_Target>::iterator it = _targets.begin(); it != _targets.end(); it++ ) {
        napi_set_element(env, prop, it->first, it->second.ToJS(env));
    }
    napi_set_named_property(env, result, "targets", prop);
    //TODO: tags

    return result;
}





NFC_Target::NFC_Target(uint32_t adapter_idx, uint32_t target_idx,
			uint32_t protocols, uint16_t sens_res, uint8_t sel_res,
			uint8_t *nfcid, uint8_t nfcid_len,
			uint8_t iso15693_dsfid,
			uint8_t iso15693_uid_len, uint8_t *iso15693_uid)
    : _adapter_idx(adapter_idx), _target_idx(target_idx), _protocols(protocols),
        _sens_res(sens_res), _sel_res(sel_res), _nfcid(nfcid, nfcid_len),
        _iso15693_dsfid(iso15693_dsfid), _iso15693_uid(iso15693_uid, iso15693_uid_len) {
}

NFC_Target::NFC_Target() {

}

napi_value NFC_Target::ToJS(napi_env env) {
    napi_value result, prop;

    napi_create_object(env, &result);

    napi_create_uint32(env, _target_idx, &prop);
    napi_set_named_property(env, result, "id", prop);

    napi_create_uint32(env, _adapter_idx, &prop);
    napi_set_named_property(env, result, "adapter", prop);

    napi_create_uint32(env, _protocols, &prop);
    napi_set_named_property(env, result, "protocols", prop);

    napi_create_buffer_copy(env, _nfcid.size(), _nfcid.data(), NULL, &prop);
    napi_set_named_property(env, result, "nfcid", prop);

    napi_create_buffer_copy(env, _iso15693_uid.size(), _iso15693_uid.data(), NULL, &prop);
    napi_set_named_property(env, result, "uid", prop);

    napi_create_uint32(env, _iso15693_dsfid, &prop);
    napi_set_named_property(env, result, "dsfid", prop);

    return result;
}





NFC_Manager::NFC_Manager() {
    __nfc_netlink_init();
    __nfc_netlink_get_adapters();
}

NFC_Manager::~NFC_Manager() {
    __nfc_netlink_cleanup();
}

napi_value NFC_Manager::GetAdapters(napi_env env, napi_callback_info) {
    napi_value result;

    napi_create_object(env, &result);
    for (std::map<uint32_t, NFC_Adapter>::iterator it = _nfc_manager._adapters.begin(); it != _nfc_manager._adapters.end(); it++ ) {
        napi_set_named_property(env, result, it->second.GetName(), it->second.ToJS(env));
    }

    return result;
}

napi_value NFC_Manager::Init(napi_env env) {
    napi_value result, constants, prop;

    napi_create_object(env, &result);
    napi_create_object(env, &constants);

    napi_create_function(env, NULL, 0, GetAdapters, NULL, &prop);
    napi_set_named_property(env, result, "getAdapters", prop);

    napi_create_uint32(env, NFC_PROTO_JEWEL_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_JEWEL", prop);
    napi_create_uint32(env, NFC_PROTO_MIFARE_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_MIFARE", prop);
    napi_create_uint32(env, NFC_PROTO_FELICA_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_FELICA", prop);
    napi_create_uint32(env, NFC_PROTO_ISO14443_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_ISO14443", prop);
    napi_create_uint32(env, NFC_PROTO_NFC_DEP_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_NFC_DEP", prop);
    napi_create_uint32(env, NFC_PROTO_ISO14443_B_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_ISO14443_B", prop);
    napi_create_uint32(env, NFC_PROTO_ISO15693_MASK, &prop);
    napi_set_named_property(env, constants, "NFC_PROTO_ISO15693", prop);

    napi_set_named_property(env, result, "constants", constants);

    napi_create_reference(env, result, 1, &_self);

    _env = env;

    return result;
}

void NFC_Manager::Emit(std::string event, JSConvertible& arg) {
    napi_handle_scope scope;
    napi_value self;
    napi_value func;
    napi_value n_event;
    napi_open_handle_scope(_env, &scope);

    napi_get_reference_value(_env, _self, &self);

    napi_get_named_property(_env, self, "emit", &func);

    napi_create_string_utf8(_env, event.c_str(), -1, &n_event);

    napi_value n_args[] = {
        n_event,
        arg.ToJS(_env),
    };
    napi_value res;

    napi_make_callback(_env, NULL, self, func, 2, n_args, &res);
    napi_close_handle_scope(_env, scope);
}

void NFC_Manager::Emit(std::string event, uint32_t idx) {
    napi_handle_scope scope;
    napi_value self;
    napi_value func;
    napi_value n_event;
    napi_open_handle_scope(_env, &scope);

    napi_get_reference_value(_env, _self, &self);

    napi_get_named_property(_env, self, "emit", &func);

    napi_create_string_utf8(_env, event.c_str(), -1, &n_event);

    napi_value n_args[] = {
        n_event
    };
    napi_value res;

    napi_make_callback(_env, NULL, self, func, 1, n_args, &res);
    napi_close_handle_scope(_env, scope);
}

void NFC_Manager::Emit(std::string event, uint32_t idx1, uint32_t idx2) {
    napi_handle_scope scope;
    napi_value self;
    napi_value func;
    napi_value n_event;
    napi_open_handle_scope(_env, &scope);

    napi_get_reference_value(_env, _self, &self);

    napi_get_named_property(_env, self, "emit", &func);

    napi_create_string_utf8(_env, event.c_str(), -1, &n_event);

    napi_value n_args[] = {
        n_event
    };
    napi_value res;

    napi_make_callback(_env, NULL, self, func, 1, n_args, &res);
    napi_close_handle_scope(_env, scope);
}








int __nfc_manager_adapter_add(uint32_t adapter_idx, const char *name,
			uint32_t protocols, bool powered) {
    _nfc_manager._adapters[adapter_idx] = NFC_Adapter(adapter_idx, name, protocols, powered);
    _nfc_manager.Emit("adapter.add", _nfc_manager._adapters[adapter_idx]);
    return 0;
}

void __nfc_manager_adapter_remove(uint32_t adapter_idx) {
    _nfc_manager._adapters.erase(adapter_idx);
    _nfc_manager.Emit("adapter.remove", adapter_idx);
}

int __nfc_adapter_add_target(uint32_t adapter_idx, uint32_t target_idx,
			uint32_t protocols, uint16_t sens_res, uint8_t sel_res,
			uint8_t *nfcid, uint8_t nfcid_len,
			uint8_t iso15693_dsfid,
			uint8_t iso15693_uid_len, uint8_t *iso15693_uid) {
    _nfc_manager._adapters[adapter_idx]._targets[target_idx] =
        NFC_Target(adapter_idx, target_idx, protocols,
            sens_res, sel_res, nfcid, nfcid_len, iso15693_dsfid,
            iso15693_uid_len, iso15693_uid);
    _nfc_manager.Emit("tag.read", _nfc_manager._adapters[adapter_idx]._targets[target_idx]);
    return 0;
}

int __nfc_adapter_get_targets_done(uint32_t adapter_idx) {
    NFC_Adapter& adapter = _nfc_manager._adapters[adapter_idx];
    if(adapter._targets.size() == 0) {
        __nfc_netlink_adapter_enable(adapter_idx, false);
        __nfc_netlink_adapter_enable(adapter_idx, true);
    }
    _nfc_manager.Emit("adapter.gotTargets", adapter_idx);
    adapter.GetTargetsDone();
    return 0;
}

int __nfc_adapter_remove_target(uint32_t adapter_idx, uint32_t target_idx) {
    _nfc_manager._adapters[adapter_idx]._targets.erase(target_idx);
    _nfc_manager.Emit("tag.gone", adapter_idx, target_idx);
    return 0;
}

int __nfc_adapter_add_device(uint32_t idx, uint8_t *nfcid, uint8_t nfcid_len) {
    return 0;
}
int __nfc_adapter_remove_device(uint32_t idx) {
    return 0;
}

int __nfc_adapter_set_dep_state(uint32_t idx, bool dep) {
    return 0;
}

napi_value Init(napi_env env, napi_value exports) {
    return _nfc_manager.Init(env);
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);

#include "netlink.c"
