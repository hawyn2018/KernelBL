/* SPDX-License-Identifier: AGPL-3.0-only */
#include "adapter.h"

/* Deliberately unimplemented pool provider. Replace with the board's actual
 * private aligned RW allocation/release pair; do not cast executable firmware
 * pools or manufacture handles. Returning NULL reports a real unavailable port.
 */
static void*board_allocate(void*opaque,size_t bytes,size_t alignment){(void)opaque;(void)bytes;(void)alignment;return 0;}
static void board_release(void*opaque,void*memory,size_t bytes){(void)opaque;(void)memory;(void)bytes;}
static const struct kbl_native_platform platform={
    .allocate=board_allocate,.release=board_release,.current_el=kbl_driver_current_el,
    /* now_us: actual monotonic provider, or NULL for checkpoint-yield fallback.
     * elf: private RW/NX pages + real protect/icache; NULL means no ELF loading.
     * block_open/list, efi_run(_full), display, poweroff and uefi_environment:
     * leave NULL until a real corresponding capability has been implemented.
     */
};

/* This function only describes providers. It does not start KBL or touch USB. */
int kbl_board_port(struct kbl_driver_port*out){
    if(!out)return KBL_DRIVER_ARGUMENT;
    *out=(struct kbl_driver_port){KBL_DRIVER_PORT_ABI,sizeof(*out),&platform,sizeof(platform)};
    return KBL_OK;
}
