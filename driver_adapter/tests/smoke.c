/* SPDX-License-Identifier: AGPL-3.0-only */
#include "../adapter.h"
#define CHECK(x) do{++checks;if(!(x)){if(out_checks)*out_checks=checks;return __LINE__;}}while(0)
static unsigned checks,touches,el_reads,finals,okay,requests,bindings,quiet;
static void clear(void*p,size_t n){unsigned char*d=p;while(n--)*d++=0;}
static void*allocate(void*o,size_t n,size_t a){(void)o;(void)n;(void)a;++touches;return 0;}
static void release(void*o,void*p,size_t n){(void)o;(void)p;(void)n;++touches;}
static unsigned measured(void*o){++el_reads;return kbl_driver_current_el(o);}
static int display(void*o,unsigned op,char*b,size_t n){(void)o;(void)op;(void)b;(void)n;++touches;return -KBL_ENOSYS;}
static void info(void*o,const char*s){(void)o;(void)s;}
static void final(void*o,int success,const char*s){(void)o;(void)s;++finals;okay=(unsigned)success;}
static int claim(void*o){(void)o;++touches;return 0;}
static void leave(void*o){(void)o;++touches;}
static int dma(void*o,size_t n,void**p){(void)o;(void)n;(void)p;++touches;return -1;}
static void free_dma(void*o,void*p){(void)o;(void)p;++touches;}
static int send_usb(void*o,uint8_t ep,size_t n,void*p){(void)o;(void)ep;(void)n;(void)p;++touches;return -1;}
static int event_usb(void*o,struct kbl_usb_event*e){(void)o;(void)e;++touches;return -1;}
static uint64_t clock_usb(void*o){(void)o;++touches;return 0;}
static int forward(void*o,const struct kbl_usb_event*e){(void)o;(void)e;++touches;return -1;}
static kefi_status native_send(uint8_t ep,size_t n,void*p){(void)ep;(void)n;(void)p;++touches;return 1;}
static kefi_status native_event(uint32_t*t,size_t*n,void*p){(void)t;(void)n;(void)p;++touches;return 1;}
static kefi_status native_allocate(size_t n,void**p){(void)n;(void)p;++touches;return 1;}
static kefi_status native_free(void*p){(void)p;++touches;return 1;}
static int command_rx(void*o,const struct kbl_usb_outcome*t){(void)o;(void)t;++touches;return 0;}
static void native_final(void*o,int good,const char*s){(void)o;(void)good;(void)s;++touches;}
static int service_request(struct kbl_service_protocol*s,unsigned op,const unsigned char*a,size_t n,const void*d,size_t z,
    const struct kbl_firmware_link*l,struct kbl_service_reply*r){(void)s;(void)a;(void)n;(void)d;(void)z;(void)l;++requests;r->bytes=sizeof(*r);r->response_sent=op;return 0;}
static int service_bind(struct kbl_service_protocol*s,const struct kbl_usb_parent_facts*f){(void)s;if(f->abi!=1)return -1;++bindings;return 0;}
static int service_quiet(struct kbl_service_protocol*s){(void)s;return (int)quiet;}

#if defined(_WIN32)
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
int kbl_adapter_smoke(unsigned*out_checks){
    struct kbl_driver_adapter a={0},moved;
    struct kbl_native_platform p={0};p.allocate=allocate;p.release=release;p.current_el=measured;p.display=display;
    struct kbl_driver_port port={1,sizeof(port),&p,sizeof(p)};
    struct kbl_output output={0,info,final};
    checks=touches=el_reads=finals=okay=requests=bindings=quiet=0;
    CHECK(kbl_driver_init(&a,sizeof(a),&port,7)==KBL_DRIVER_ARGUMENT);
    port.abi=2;CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==KBL_DRIVER_ABI);port.abi=1;
    port.bytes=sizeof(port)+1;CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==KBL_DRIVER_ARGUMENT);port.bytes=sizeof(port);
    port.platform_bytes=offsetof(struct kbl_native_platform,current_el);CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==KBL_DRIVER_ARGUMENT);
    /* A physical object containing only the legacy required prefix. */
    struct prefix{void*opaque;void*(*allocate)(void*,size_t,size_t);void(*release)(void*,void*,size_t);unsigned(*current_el)(void*);} prefix={0,allocate,release,measured};
    port.platform=(const struct kbl_native_platform*)&prefix;port.platform_bytes=sizeof(prefix);
    CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==0);
    CHECK(!a.platform.display&&!a.platform.block_open&&!a.platform.elf.allocate&&!a.platform.efi_run&&!a.platform.poweroff);
    CHECK(!touches&&!el_reads);CHECK(kbl_driver_backend(&a)==&a.backend);CHECK(a.backend.opaque==&a.platform);
    void *host_context=(void*)(uintptr_t)1;
    CHECK(a.backend.start(a.backend.opaque,&host_context)==KBL_ERROR);CHECK(!host_context&&!touches);
    CHECK(kbl_driver_handle(&a,(const unsigned char*)"getvar:product",14,&output)==0);CHECK(!touches&&!el_reads&&!finals);
    moved=a;CHECK(!kbl_driver_backend(&moved));CHECK(kbl_driver_destroy(&moved)==KBL_DRIVER_STATE);
    CHECK(kbl_driver_handle(&a,(const unsigned char*)"oem kblstart",12,&output)==1);
    CHECK(finals==1&&!okay&&!touches);CHECK(a.session.phase==KBL_STOPPED&&!a.session.context);
    CHECK(kbl_driver_destroy(&a)==0);
    port.platform=&p;port.platform_bytes=sizeof(p);
    struct extension{struct kbl_driver_port base;uint64_t future[2];} extended={port,{1,2}};
    extended.base.bytes=sizeof(extended);CHECK(kbl_driver_init(&a,sizeof(a),&extended.base,sizeof(extended))==0);CHECK(kbl_driver_destroy(&a)==0);
    port.platform_bytes=offsetof(struct kbl_native_platform,display)+sizeof(p.display)-1;
    CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==0);CHECK(!a.platform.display);CHECK(kbl_driver_destroy(&a)==0);
    port.platform_bytes=sizeof(p);CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==0);
    CHECK(a.platform.display==display);CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==KBL_DRIVER_STATE);
    struct kbl_usb_session usb={0};struct kbl_usb_ops ops={0,claim,leave,dma,free_dma,send_usb,event_usb,0,clock_usb,0,forward};
    CHECK(kbl_driver_prepare_usb(&a,&usb,&ops,4096,1000,1000,4)==0);CHECK(!touches);
    usb.pending=1;CHECK(kbl_driver_destroy(&a)==KBL_AGAIN);usb.pending=0;
    struct kbl_native_usb_binding binding={0};struct kbl_usb_device_protocol proto={0};
    proto.send=native_send;proto.handle_event=native_event;proto.allocate=native_allocate;proto.free=native_free;
    struct kbl_usb_device_protocol*slot=&proto;unsigned char ack[8]={0};
    struct kbl_native_usb_environment env={0,&slot,ack,claim,leave,command_rx,native_final,clock_usb,0};
    CHECK(kbl_driver_prepare_native_usb(&a,&binding,&env,4096,1000,1000,4)==0);
    CHECK(!touches&&slot==&proto&&!binding.attached);CHECK(kbl_driver_can_destroy(&a)==1);CHECK(kbl_driver_destroy(&a)==0);
    CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==0);
    struct kbl_service_protocol service={KBL_SERVICE_MAGIC,1,sizeof(service),service_request,service_quiet,service_bind};
    struct kbl_usb_parent_facts parent={0};parent.abi=1;parent.bytes=sizeof(parent);
    CHECK(kbl_driver_bind_service(&a,&service,sizeof(service),&parent,sizeof(parent))==0);CHECK(!requests&&!bindings&&!touches);
    CHECK(!kbl_driver_backend(&a));CHECK(kbl_driver_handle(&a,(const unsigned char*)"oem kblstart",12,&output)==KBL_DRIVER_STATE);
    struct kbl_service_reply reply={0};CHECK(kbl_driver_firmware_request(&a,KBL_S_STATUS,0,0,0,0,0,&reply)==0);CHECK(requests==1&&!bindings);
    CHECK(kbl_driver_firmware_request(&a,KBL_S_START,0,0,0,0,0,&reply)==0);CHECK(requests==2&&bindings==1);
    CHECK(kbl_driver_firmware_request(&a,KBL_S_ADB,0,0,0,0,0,&reply)==0);CHECK(bindings==1&&requests==3);
    CHECK(kbl_driver_destroy(&a)==KBL_AGAIN);quiet=1;CHECK(kbl_driver_destroy(&a)==0);
    CHECK(kbl_driver_init(&a,sizeof(a),&port,sizeof(port))==0);
    service.bind_usb=0;CHECK(kbl_driver_bind_service(&a,&service,sizeof(service),&parent,sizeof(parent))==0);
    CHECK(kbl_driver_firmware_request(&a,KBL_S_START,0,0,0,0,0,&reply)==KBL_DRIVER_UNSUPPORTED);CHECK(requests==3);
    CHECK(kbl_driver_destroy(&a)==0);CHECK(!touches);
    if(out_checks)*out_checks=checks;return 0;
}
