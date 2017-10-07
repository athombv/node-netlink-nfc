#ifndef NETLINK_H_INCLUDED
#define NETLINK_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <linux/socket.h>
#include <linux/nfc.h>

#define NFC_HEADER_SIZE 1

#define NFC_MAX_NFCID1_LEN 10
#define NFC_MAX_ISO15693_DSFID_LEN 1
#define NFC_MAX_ISO15693_UID_LEN 8

int __nfc_netlink_adapter_enable(int adapter_idx, bool enable);
int __nfc_netlink_dep_link_up(uint32_t adapter_idx, uint32_t target_idx,
				uint8_t comm_mode, uint8_t rf_mode);
int __nfc_netlink_dep_link_down(uint32_t adapter_idx);

int __nfc_netlink_start_poll(int adapter_idx,
				uint32_t im_protocols, uint32_t tm_protocols);
int __nfc_netlink_stop_poll(int adapter_idx);
int __nfc_netlink_get_adapters(void);

int __nfc_netlink_init(void);
void __nfc_netlink_cleanup(void);

#endif