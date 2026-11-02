/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef HNUBL_KBL_DRIVER_ADAPTER_H
#define HNUBL_KBL_DRIVER_ADAPTER_H

#include "../runtime/native/kbl_native.h"
#include "../runtime/uefi/kbl_usb.h"
#include "../runtime/uefi/kbl_usb_native.h"
#include "../runtime/uefi/kbl_service.h"

#define KBL_DRIVER_PORT_ABI 1u
enum { KBL_DRIVER_ARGUMENT=-20, KBL_DRIVER_ABI=-21, KBL_DRIVER_STATE=-22,
       KBL_DRIVER_UNSUPPORTED=-23 };

/* Extensible adapter descriptor, not a replacement for any runtime ABI.
 * platform_bytes describes readable bytes at platform. Complete known fields
 * are copied; absent/partial optional fields are zero. Unknown tails are ignored.
 */
struct kbl_driver_port {
    uint32_t abi,bytes;
    const struct kbl_native_platform *platform;
    size_t platform_bytes;
};

/* Caller-owned, initially zero, non-copyable storage. It and provider opaque
 * state outlive the runtime, readers, USB DMA and optional firmware service.
 * The actual native backend is bound to this platform copy, not to a temporary
 * descriptor on the board's stack.
 */
struct kbl_driver_adapter {
    uint64_t cookie;
    struct kbl_driver_adapter *self;
    struct kbl_native_platform platform;
    struct kbl_backend backend;
    struct kbl_session session;
    struct kbl_usb_session *usb;
    struct kbl_native_usb_binding *native_usb;
    struct kbl_service_protocol *service;
    size_t service_bytes;
    struct kbl_usb_parent_facts parent;
    unsigned have_parent,service_used,usb_bound;
};

/* init and USB prepare have no provider calls, allocation, timer or I/O.
 * Separate explicit descriptor lengths prevent reading a new tail from an old
 * descriptor. init does not start KBL or alter the ordinary boot path.
 */
int kbl_driver_init(struct kbl_driver_adapter *,size_t adapter_bytes,
                    const struct kbl_driver_port *,size_t port_bytes);
const struct kbl_backend *kbl_driver_backend(struct kbl_driver_adapter *);
/* Uses the real kbl_handle controller. Unknown Fastboot requests return 0 and
 * are untouched. Do not combine this local world with a firmware service world.
 */
int kbl_driver_handle(struct kbl_driver_adapter *,const unsigned char *,size_t,
                      const struct kbl_output *);

/* Existing USB ABIs are used directly. The board owns these zeroed objects.
 * Preparing does not observe events, claim TX or change protocol slots. The
 * caller explicitly uses the original observe/ACK/fence/send APIs thereafter.
 */
int kbl_driver_prepare_usb(struct kbl_driver_adapter *,struct kbl_usb_session *,
    const struct kbl_usb_ops *,size_t,uint64_t,uint64_t,unsigned);
int kbl_driver_prepare_native_usb(struct kbl_driver_adapter *,struct kbl_native_usb_binding *,
    const struct kbl_native_usb_environment *,size_t,uint64_t,uint64_t,unsigned);

/* Optional alternative: invoke an already loaded, owned EFI service instance.
 * bind only records checked descriptors. Parent facts remain semantic resolver
 * inputs, never model/RVA tables. Only explicit START/ADB calls bind_usb.
 */
int kbl_driver_bind_service(struct kbl_driver_adapter *,struct kbl_service_protocol *,size_t,
                            const struct kbl_usb_parent_facts *,size_t);
int kbl_driver_firmware_request(struct kbl_driver_adapter *,unsigned,
    const unsigned char *,size_t,const void *,size_t,
    const struct kbl_firmware_link *,struct kbl_service_reply *);

/* 1 iff adapter-owned/borrowed capabilities are quiescent. No force free:
 * destroy returns KBL_AGAIN while a native context, upload, USB resource or
 * previously used service still retains ownership. It never sends kbloff,
 * abort, poweroff, ResetSystem, USB Stop or a repeated write.
 */
int kbl_driver_can_destroy(const struct kbl_driver_adapter *);
int kbl_driver_destroy(struct kbl_driver_adapter *);

/* Reference implementation is a real CurrentEL instruction on AArch64; on
 * other architectures zero means unavailable, not a fabricated EL1/EL2.
 */
unsigned kbl_driver_current_el(void *unused);
/* Optional example factory, implemented by board_template.c. A board may use
 * its own factory instead; the adapter itself does not reference this symbol. */
int kbl_board_port(struct kbl_driver_port *);

#endif
