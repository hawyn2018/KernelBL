/* SPDX-License-Identifier: AGPL-3.0-only */
#include "adapter.h"

#define COOKIE UINT64_C(0x314154524f50424b)
#define END(type,field) (offsetof(type,field)+sizeof(((type *)0)->field))
static void zero(void *p,size_t n){unsigned char *d=p;while(n--)*d++=0;}
static void copy(void *p,const void *q,size_t n){unsigned char*d=p;const unsigned char*s=q;while(n--)*d++=*s++;}
static int valid(const struct kbl_driver_adapter*a){return a&&a->cookie==COOKIE&&a->self==a;}
static int service_valid(const struct kbl_driver_adapter*a){return a->service&&a->service->magic==KBL_SERVICE_MAGIC&&
    a->service->abi==KBL_SERVICE_ABI&&a->service->bytes<=a->service_bytes&&
    a->service->bytes>=END(struct kbl_service_protocol,request)&&a->service->request;}

unsigned kbl_driver_current_el(void *unused){
    (void)unused;
#if defined(__aarch64__)
    uint64_t value;__asm__ volatile("mrs %0, CurrentEL":"=r"(value));return (unsigned)((value>>2)&3);
#else
    return 0;
#endif
}

int kbl_driver_init(struct kbl_driver_adapter*a,size_t a_bytes,const struct kbl_driver_port*source,size_t readable){
    struct kbl_driver_port port;struct kbl_native_platform p;
    if(!a||a_bytes<sizeof(*a)||!source||readable<8)return KBL_DRIVER_ARGUMENT;
    if(a->cookie)return KBL_DRIVER_STATE;
    zero(&port,sizeof(port));copy(&port,source,8);
    if(port.abi!=KBL_DRIVER_PORT_ABI)return KBL_DRIVER_ABI;
    if(port.bytes>readable||port.bytes<END(struct kbl_driver_port,platform_bytes))return KBL_DRIVER_ARGUMENT;
    copy(&port,source,sizeof(port));
    if(!port.platform||port.platform_bytes<END(struct kbl_native_platform,current_el))return KBL_DRIVER_ARGUMENT;
    zero(&p,sizeof(p));
    /* Never copy half a pointer, nor read a field beyond the supplied prefix. */
#define FIELD(name) do {if(port.platform_bytes>=END(struct kbl_native_platform,name)) \
    copy(&p.name,(const unsigned char*)port.platform+offsetof(struct kbl_native_platform,name),sizeof(p.name));}while(0)
    FIELD(opaque);FIELD(allocate);FIELD(release);FIELD(current_el);FIELD(now_us);
    FIELD(elf);FIELD(stack_bytes);FIELD(output_bytes);FIELD(max_file_bytes);FIELD(max_ramfs_bytes);FIELD(max_nodes);
    FIELD(uefi_environment);FIELD(block_open);FIELD(block_list);FIELD(efi_run);FIELD(display);FIELD(poweroff);FIELD(efi_run_full);
#undef FIELD
    if(!p.allocate||!p.release||!p.current_el)return KBL_DRIVER_ARGUMENT;
    zero(a,sizeof(*a));a->platform=p;
    if(kbl_native_backend(&a->backend,&a->platform)!=KBL_OK){zero(a,sizeof(*a));return KBL_ERROR;}
    a->self=a;a->cookie=COOKIE;return KBL_OK;
}

const struct kbl_backend*kbl_driver_backend(struct kbl_driver_adapter*a){return valid(a)&&!a->service?&a->backend:0;}
int kbl_driver_handle(struct kbl_driver_adapter*a,const unsigned char*line,size_t bytes,const struct kbl_output*out){
    if(!valid(a))return KBL_DRIVER_STATE;
    if(a->service)return KBL_DRIVER_STATE;
    return kbl_handle(&a->session,&a->backend,line,bytes,out);
}

int kbl_driver_prepare_usb(struct kbl_driver_adapter*a,struct kbl_usb_session*s,const struct kbl_usb_ops*ops,
    size_t max,uint64_t timeout,uint64_t abort_timeout,unsigned polls){
    int result;if(!valid(a)||!s)return KBL_DRIVER_ARGUMENT;
    if(a->usb&&a->usb!=s)return KBL_DRIVER_STATE;
    result=kbl_usb_init(s,ops,max,timeout,abort_timeout,polls);if(!result)a->usb=s;return result;
}
int kbl_driver_prepare_native_usb(struct kbl_driver_adapter*a,struct kbl_native_usb_binding*b,
    const struct kbl_native_usb_environment*env,size_t max,uint64_t timeout,uint64_t abort_timeout,unsigned polls){
    int result;if(!valid(a)||!b)return KBL_DRIVER_ARGUMENT;
    if(a->native_usb&&a->native_usb!=b)return KBL_DRIVER_STATE;
    result=kbl_native_usb_init(b,env,max,timeout,abort_timeout,polls);if(!result)a->native_usb=b;return result;
}

int kbl_driver_bind_service(struct kbl_driver_adapter*a,struct kbl_service_protocol*s,size_t readable,
    const struct kbl_usb_parent_facts*parent,size_t parent_bytes){
    if(!valid(a)||!s||readable<END(struct kbl_service_protocol,request))return KBL_DRIVER_ARGUMENT;
    if(a->session.context||a->session.phase!=KBL_STOPPED||a->service_used||a->service)return KBL_DRIVER_STATE;
    if(s->magic!=KBL_SERVICE_MAGIC||s->abi!=KBL_SERVICE_ABI||s->bytes>readable||
        s->bytes<END(struct kbl_service_protocol,request)||!s->request)return KBL_DRIVER_ABI;
    if(parent){
        if(parent_bytes<sizeof(*parent)||parent->abi!=KBL_SERVICE_ABI||parent->bytes<sizeof(*parent)||parent->bytes>parent_bytes)
            return KBL_DRIVER_ARGUMENT;
        copy(&a->parent,parent,sizeof(a->parent));a->parent.bytes=sizeof(a->parent);a->have_parent=1;
    }else if(parent_bytes)return KBL_DRIVER_ARGUMENT;
    a->service=s;a->service_bytes=s->bytes;return KBL_OK;
}
int kbl_driver_firmware_request(struct kbl_driver_adapter*a,unsigned operation,const unsigned char*argument,size_t bytes,
    const void*download,size_t download_bytes,const struct kbl_firmware_link*link,struct kbl_service_reply*reply){
    int result;if(!valid(a)||!a->service)return KBL_DRIVER_UNSUPPORTED;
    if(!service_valid(a))return KBL_DRIVER_ABI;
    if(!reply)return KBL_DRIVER_ARGUMENT;
    if(operation<KBL_S_START||operation>KBL_S_SCREEN_STATUS)return KBL_DRIVER_UNSUPPORTED;
    a->service_used=1;
    if((operation==KBL_S_START||operation==KBL_S_ADB)&&a->have_parent&&!a->usb_bound){
        if(a->service->bytes<END(struct kbl_service_protocol,bind_usb)||!a->service->bind_usb)return KBL_DRIVER_UNSUPPORTED;
        result=a->service->bind_usb(a->service,&a->parent);if(result!=KBL_S_OK)return result;
        a->usb_bound=1;
    }
    return a->service->request(a->service,operation,argument,bytes,download,download_bytes,link,reply);
}

int kbl_driver_can_destroy(const struct kbl_driver_adapter*a){
    if(!valid(a))return 0;
    if(a->session.context||a->session.phase!=KBL_STOPPED||a->session.has_upload||a->session.dynamic_line)return 0;
    if(a->usb&&(a->usb->pending||a->usb->claimed||a->usb->buffer))return 0;
    if(a->native_usb&&!kbl_native_usb_can_release(a->native_usb))return 0;
    if(a->service_used){
        if(!service_valid(a))return 0;
        if(a->service->bytes<END(struct kbl_service_protocol,can_unload)||!a->service->can_unload)return 0;
        if(!a->service->can_unload(a->service))return 0;
    }
    return 1;
}
int kbl_driver_destroy(struct kbl_driver_adapter*a){
    if(!valid(a))return KBL_DRIVER_STATE;
    if(!kbl_driver_can_destroy(a))return KBL_AGAIN;
    zero(a,sizeof(*a));return KBL_OK;
}
