/* automatically generated: Wed May  9 17:32:33 CEST 2012 by edu@ubuntu, do not edit
 *
 * Built by relaytool, a program for building delay-load jumptables
 * relaytool is (C) 2004 Mike Hearn <mike@navi.cx>
 * See http://autopackage.org/ for details.
 */

/* GCC 4 generates bad code with optimizations on here...*/
#if ((__GNUC__ ==  4 && __GNUC_MINOR__ >=  4) || __GNUC__ >  4)
#  pragma GCC optimize "O0"
#endif

#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/mman.h>

#ifdef __cplusplus
    extern "C" {
#endif

static void **ptrs;
static const char *functions[] = {
    "mpg123_add_string",
    "mpg123_add_substring",
    "mpg123_clip",
    "mpg123_close",
    "mpg123_copy_string",
    "mpg123_current_decoder",
    "mpg123_decode",
    "mpg123_decode_frame",
    "mpg123_decode_frame_64",
    "mpg123_decoder",
    "mpg123_decoders",
    "mpg123_delete",
    "mpg123_delete_pars",
    "mpg123_enc_from_id3",
    "mpg123_encodings",
    "mpg123_eq",
    "mpg123_errcode",
    "mpg123_exit",
    "mpg123_feature",
    "mpg123_feed",
    "mpg123_feedseek",
    "mpg123_feedseek_64",
    "mpg123_fmt",
    "mpg123_fmt_all",
    "mpg123_fmt_none",
    "mpg123_fmt_support",
    "mpg123_format",
    "mpg123_format_all",
    "mpg123_format_none",
    "mpg123_format_support",
    "mpg123_framebyframe_decode",
    "mpg123_framebyframe_decode_64",
    "mpg123_framebyframe_next",
    "mpg123_free_string",
    "mpg123_geteq",
    "mpg123_getformat",
    "mpg123_getpar",
    "mpg123_getparam",
    "mpg123_getstate",
    "mpg123_getvolume",
    "mpg123_grow_string",
    "mpg123_icy",
    "mpg123_icy2utf8",
    "mpg123_id3",
    "mpg123_index",
    "mpg123_index_64",
    "mpg123_info",
    "mpg123_init",
    "mpg123_init_string",
    "mpg123_length",
    "mpg123_length_64",
    "mpg123_meta_check",
    "mpg123_new",
    "mpg123_new_pars",
    "mpg123_open",
    "mpg123_open_64",
    "mpg123_open_fd",
    "mpg123_open_fd_64",
    "mpg123_open_feed",
    "mpg123_open_handle",
    "mpg123_open_handle_64",
    "mpg123_outblock",
    "mpg123_par",
    "mpg123_param",
    "mpg123_parnew",
    "mpg123_plain_strerror",
    "mpg123_position",
    "mpg123_position_64",
    "mpg123_rates",
    "mpg123_read",
    "mpg123_replace_buffer",
    "mpg123_replace_reader",
    "mpg123_replace_reader_64",
    "mpg123_replace_reader_handle",
    "mpg123_replace_reader_handle_64",
    "mpg123_reset_eq",
    "mpg123_resize_string",
    "mpg123_safe_buffer",
    "mpg123_scan",
    "mpg123_seek",
    "mpg123_seek_64",
    "mpg123_seek_frame",
    "mpg123_seek_frame_64",
    "mpg123_set_filesize",
    "mpg123_set_filesize_64",
    "mpg123_set_index",
    "mpg123_set_index_64",
    "mpg123_set_string",
    "mpg123_set_substring",
    "mpg123_store_utf8",
    "mpg123_strerror",
    "mpg123_supported_decoders",
    "mpg123_tell",
    "mpg123_tell_64",
    "mpg123_tell_stream",
    "mpg123_tell_stream_64",
    "mpg123_tellframe",
    "mpg123_tellframe_64",
    "mpg123_timeframe",
    "mpg123_timeframe_64",
    "mpg123_tpf",
    "mpg123_volume",
    "mpg123_volume_change",
    0
};

/* 1 if present, 0 if not */
int libmpg123_is_present = 0;
int libmpg123_is_using_64 = 0;

static void *handle = 0;

#if defined(__PIC__)
  #if defined( __i386__ )
    /* NOTE: Assumes GOT is in ebx. Probably the case since the function
  is unlikely to be called from outside the module. */
    #define ASM_VAR(x)      #x "@GOTOFF(%ebx)"
    #define ASM_VAR_EPILOG
    #define ASM_FUNC(x)     #x "@PLT"
    #define ASM_FIXUPVAR(x) #x "@GOTOFF(%%ebx)"
  #elif defined( __x86_64__ )
    #define ASM_VAR(x)      #x "@GOTPCREL(%rip)"
    #define ASM_VAR_EPILOG  "movq (%r11), %r11\n"
    #define ASM_FUNC(x)     #x "@PLT"
    #define ASM_FIXUPVAR(x) #x "@GOT(%%rbx)"
  #endif
#endif

#ifndef ASM_VAR
  #define ASM_VAR(x)    #x
#endif
#ifndef ASM_VAR_EPILOG
  #define ASM_VAR_EPILOG
#endif
#ifndef ASM_FUNC
  #define ASM_FUNC(x)   #x
#endif
#ifndef ASM_FIXUPVAR
  #define ASM_FIXUPVAR(x) #x
#endif

/* 1 if present, 0 if not, 0 with warning to stderr if lib not present or symbol not found */
int libmpg123_symbol_is_present(char *s)
{
    int i;

    if( !libmpg123_is_present ) {
        fprintf(stderr, "%s: relaytool: libmpg123.so not present so cannot check for symbol %s.\n", getenv("_"), s);
        fprintf(stderr, "%s: relaytool: This probably indicates a bug in the application, please report.\n", getenv("_"));
        return 0;
    }

    i = 0;
    while (functions[i++])
      if (!strcmp( functions[i - 1], s ))
        return ptrs[i - 1] != NULL ? 1 : 0;

    fprintf( stderr, "%s: relaytool: %s is an unknown symbol in libmpg123.so.\n", getenv("_"), s );
    fprintf( stderr, "%s: relaytool: If you are the developer of this program, please correct the symbol name or rerun relaytool.\n", getenv("_") );
    return 0;
}

__attribute__((noreturn)) void _relaytool_stubcall_libmpg123(int offset)
{
    fprintf( stderr, "%s: relaytool: stub call to libmpg123.so:%s, aborting.\n", getenv("_"),
             functions[offset / sizeof(void*)] );

    /* give the app a chance to save a crashbackups of the current document... */
    kill(0, SIGABRT);

    /* don't return (in case that kill is not caught) */
    exit(1);
}

#if defined( __i386__ )
    #define FIXUP_GOT_RELOC(sym, addr) \
        asm("\tmovl %0, %%eax\n" \
            "\tmovl %%eax, " ASM_FIXUPVAR (sym) "\n" : : "r" (addr));
#elif defined( __powerpc__ )

    /* The PowerPC ELF ABI is a twisted nightmare. Until I figure it out,
       for now we don't support GOT fixup on this architecture */

    #error Variables are not currently supported on PowerPC

#elif defined( __x86_64__ )
    #define FIXUP_GOT_RELOC(sym, addr) \
        asm("\tmovq %0, %%rax\n" \
            "\tmovq %%rax, " ASM_FIXUPVAR (sym) "\n" : : "r" (addr));
#else
    #error Please define FIXUP_GOT_RELOC for your architecture
#endif

void __attribute__((constructor)) _relaytool_init_libmpg123()
{
    int i = 0;

    ptrs = malloc( sizeof(functions) );
    memset( ptrs, 0, sizeof(functions) );

    handle = dlopen( "libmpg123.so.0", RTLD_LAZY );
    if (!handle) handle = dlopen( "libmpg123.so.0.20.1", RTLD_LAZY );
    if (!handle) handle = dlopen( "libmpg123.so.0.2.4", RTLD_LAZY );

    if (!handle) return;
    
    libmpg123_is_present = 1;

    void* testfunc = dlsym(handle, "mpg123_open_64");
    if (dlerror() == 0 && testfunc != 0)
    {
      libmpg123_is_using_64 = 1;
    }

    /* build function jumptable */
    while (functions[i++]) ptrs[i - 1] = dlsym( handle, functions[i - 1] );
    
}

#if defined( __i386__ )

#define JUMP_SLOT(name, index)  \
    asm(".section .text." name ", \"ax\", @progbits\n" \
        ".globl  " name "\n" \
        ".hidden " name "\n" \
        "        .type " name ", @function\n"  \
        name ":\n" \
        "        movl " ASM_VAR (ptrs) ", %eax\n" \
                        ASM_VAR_EPILOG \
        "        movl " #index "(%eax), %eax\n" \
        "        test %eax, %eax\n" \
        "        jnz  JS" #index "\n" \
        "        push $" #index "\n" \
        "        call " ASM_FUNC (_relaytool_stubcall_libmpg123) "\n" \
        "JS" #index ":    jmp *%eax\n");


#elif defined( __x86_64__ )

#define JUMP_SLOT(name, index)  \
    asm(".section .text." name ", \"ax\", @progbits\n" \
        ".globl  " name "\n" \
        ".hidden " name "\n" \
        "        .type " name ", @function\n"  \
        name ":\n" \
        "        movq " ASM_VAR (ptrs) ", %r11\n" \
                        ASM_VAR_EPILOG \
        "        movq " #index "(%r11), %r11\n" \
        "        test %r11, %r11\n" \
        "        jnz  JS" #index "\n" \
        "        push $" #index "\n" \
        "        call " ASM_FUNC (_relaytool_stubcall_libmpg123) "\n" \
        "JS" #index ":    jmp *%r11\n");

#elif defined( __powerpc__ )

#define JUMP_SLOT(name, index) \
    asm(".section .text." name ", \"ax\", @progbits\n" \
        ".globl  " name "\n" \
        ".hidden " name "\n" \
        "        .type " name ", @function\n" \
        name ":\n" \
        "        lis r11, ptrs@ha\n" \
        "        lwz r11, " #index "(r11)\n" \
        "        cmpi cr0,r11,0\n" \
        "        beq- 1f\n" \
        "        mtctr r11\n" \
        "        bctr\n" \
        "1:      li r3, " #index "\n" \
        "        b _relaytool_stubcall_libmpg123\n" \
        );

#else
    #error Please define JUMP_SLOT for your architecture
#endif

/* define placeholders for the variable imports: their type doesn't matter,
   however we must restrict ELF symbol scope to prevent the definition in the imported
   shared library being bound to this dummy symbol (not all libs are compiled -Bsymbolic)
 */

/* define each jump slot in its own section. this increases generated code
   size, but it means unused slots can be deleted by the linker when
   --gc-sections is used.
 */


#if defined( __x86_64__ )

JUMP_SLOT("mpg123_add_string", 0);
JUMP_SLOT("mpg123_add_substring", 8);
JUMP_SLOT("mpg123_clip", 16);
JUMP_SLOT("mpg123_close", 24);
JUMP_SLOT("mpg123_copy_string", 32);
JUMP_SLOT("mpg123_current_decoder", 40);
JUMP_SLOT("mpg123_decode", 48);
JUMP_SLOT("mpg123_decode_frame", 56);
JUMP_SLOT("mpg123_decode_frame_64", 64);
JUMP_SLOT("mpg123_decoder", 72);
JUMP_SLOT("mpg123_decoders", 80);
JUMP_SLOT("mpg123_delete", 88);
JUMP_SLOT("mpg123_delete_pars", 96);
JUMP_SLOT("mpg123_enc_from_id3", 104);
JUMP_SLOT("mpg123_encodings", 112);
JUMP_SLOT("mpg123_eq", 120);
JUMP_SLOT("mpg123_errcode", 128);
JUMP_SLOT("mpg123_exit", 136);
JUMP_SLOT("mpg123_feature", 144);
JUMP_SLOT("mpg123_feed", 152);
JUMP_SLOT("mpg123_feedseek", 160);
JUMP_SLOT("mpg123_feedseek_64", 168);
JUMP_SLOT("mpg123_fmt", 176);
JUMP_SLOT("mpg123_fmt_all", 184);
JUMP_SLOT("mpg123_fmt_none", 192);
JUMP_SLOT("mpg123_fmt_support", 200);
JUMP_SLOT("mpg123_format", 208);
JUMP_SLOT("mpg123_format_all", 216);
JUMP_SLOT("mpg123_format_none", 224);
JUMP_SLOT("mpg123_format_support", 232);
JUMP_SLOT("mpg123_framebyframe_decode", 240);
JUMP_SLOT("mpg123_framebyframe_decode_64", 248);
JUMP_SLOT("mpg123_framebyframe_next", 256);
JUMP_SLOT("mpg123_free_string", 264);
JUMP_SLOT("mpg123_geteq", 272);
JUMP_SLOT("mpg123_getformat", 280);
JUMP_SLOT("mpg123_getpar", 288);
JUMP_SLOT("mpg123_getparam", 296);
JUMP_SLOT("mpg123_getstate", 304);
JUMP_SLOT("mpg123_getvolume", 312);
JUMP_SLOT("mpg123_grow_string", 320);
JUMP_SLOT("mpg123_icy", 328);
JUMP_SLOT("mpg123_icy2utf8", 336);
JUMP_SLOT("mpg123_id3", 344);
JUMP_SLOT("mpg123_index", 352);
JUMP_SLOT("mpg123_index_64", 360);
JUMP_SLOT("mpg123_info", 368);
JUMP_SLOT("mpg123_init", 376);
JUMP_SLOT("mpg123_init_string", 384);
JUMP_SLOT("mpg123_length", 392);
JUMP_SLOT("mpg123_length_64", 400);
JUMP_SLOT("mpg123_meta_check", 408);
JUMP_SLOT("mpg123_new", 416);
JUMP_SLOT("mpg123_new_pars", 424);
JUMP_SLOT("mpg123_open", 432);
JUMP_SLOT("mpg123_open_64", 440);
JUMP_SLOT("mpg123_open_fd", 448);
JUMP_SLOT("mpg123_open_fd_64", 456);
JUMP_SLOT("mpg123_open_feed", 464);
JUMP_SLOT("mpg123_open_handle", 472);
JUMP_SLOT("mpg123_open_handle_64", 480);
JUMP_SLOT("mpg123_outblock", 488);
JUMP_SLOT("mpg123_par", 496);
JUMP_SLOT("mpg123_param", 504);
JUMP_SLOT("mpg123_parnew", 512);
JUMP_SLOT("mpg123_plain_strerror", 520);
JUMP_SLOT("mpg123_position", 528);
JUMP_SLOT("mpg123_position_64", 536);
JUMP_SLOT("mpg123_rates", 544);
JUMP_SLOT("mpg123_read", 552);
JUMP_SLOT("mpg123_replace_buffer", 560);
JUMP_SLOT("mpg123_replace_reader", 568);
JUMP_SLOT("mpg123_replace_reader_64", 576);
JUMP_SLOT("mpg123_replace_reader_handle", 584);
JUMP_SLOT("mpg123_replace_reader_handle_64", 592);
JUMP_SLOT("mpg123_reset_eq", 600);
JUMP_SLOT("mpg123_resize_string", 608);
JUMP_SLOT("mpg123_safe_buffer", 616);
JUMP_SLOT("mpg123_scan", 624);
JUMP_SLOT("mpg123_seek", 632);
JUMP_SLOT("mpg123_seek_64", 640);
JUMP_SLOT("mpg123_seek_frame", 648);
JUMP_SLOT("mpg123_seek_frame_64", 656);
JUMP_SLOT("mpg123_set_filesize", 664);
JUMP_SLOT("mpg123_set_filesize_64", 672);
JUMP_SLOT("mpg123_set_index", 680);
JUMP_SLOT("mpg123_set_index_64", 688);
JUMP_SLOT("mpg123_set_string", 696);
JUMP_SLOT("mpg123_set_substring", 704);
JUMP_SLOT("mpg123_store_utf8", 712);
JUMP_SLOT("mpg123_strerror", 720);
JUMP_SLOT("mpg123_supported_decoders", 728);
JUMP_SLOT("mpg123_tell", 736);
JUMP_SLOT("mpg123_tell_64", 744);
JUMP_SLOT("mpg123_tell_stream", 752);
JUMP_SLOT("mpg123_tell_stream_64", 760);
JUMP_SLOT("mpg123_tellframe", 768);
JUMP_SLOT("mpg123_tellframe_64", 776);
JUMP_SLOT("mpg123_timeframe", 784);
JUMP_SLOT("mpg123_timeframe_64", 792);
JUMP_SLOT("mpg123_tpf", 800);
JUMP_SLOT("mpg123_volume", 808);
JUMP_SLOT("mpg123_volume_change", 816);

#else

JUMP_SLOT("mpg123_add_string", 0);
JUMP_SLOT("mpg123_add_substring", 4);
JUMP_SLOT("mpg123_clip", 8);
JUMP_SLOT("mpg123_close", 12);
JUMP_SLOT("mpg123_copy_string", 16);
JUMP_SLOT("mpg123_current_decoder", 20);
JUMP_SLOT("mpg123_decode", 24);
JUMP_SLOT("mpg123_decode_frame", 28);
JUMP_SLOT("mpg123_decode_frame_64", 32);
JUMP_SLOT("mpg123_decoder", 36);
JUMP_SLOT("mpg123_decoders", 40);
JUMP_SLOT("mpg123_delete", 44);
JUMP_SLOT("mpg123_delete_pars", 48);
JUMP_SLOT("mpg123_enc_from_id3", 52);
JUMP_SLOT("mpg123_encodings", 56);
JUMP_SLOT("mpg123_eq", 60);
JUMP_SLOT("mpg123_errcode", 64);
JUMP_SLOT("mpg123_exit", 68);
JUMP_SLOT("mpg123_feature", 72);
JUMP_SLOT("mpg123_feed", 76);
JUMP_SLOT("mpg123_feedseek", 80);
JUMP_SLOT("mpg123_feedseek_64", 84);
JUMP_SLOT("mpg123_fmt", 88);
JUMP_SLOT("mpg123_fmt_all", 92);
JUMP_SLOT("mpg123_fmt_none", 96);
JUMP_SLOT("mpg123_fmt_support", 100);
JUMP_SLOT("mpg123_format", 104);
JUMP_SLOT("mpg123_format_all", 108);
JUMP_SLOT("mpg123_format_none", 112);
JUMP_SLOT("mpg123_format_support", 116);
JUMP_SLOT("mpg123_framebyframe_decode", 120);
JUMP_SLOT("mpg123_framebyframe_decode_64", 124);
JUMP_SLOT("mpg123_framebyframe_next", 128);
JUMP_SLOT("mpg123_free_string", 132);
JUMP_SLOT("mpg123_geteq", 136);
JUMP_SLOT("mpg123_getformat", 140);
JUMP_SLOT("mpg123_getpar", 144);
JUMP_SLOT("mpg123_getparam", 148);
JUMP_SLOT("mpg123_getstate", 152);
JUMP_SLOT("mpg123_getvolume", 156);
JUMP_SLOT("mpg123_grow_string", 160);
JUMP_SLOT("mpg123_icy", 164);
JUMP_SLOT("mpg123_icy2utf8", 168);
JUMP_SLOT("mpg123_id3", 172);
JUMP_SLOT("mpg123_index", 176);
JUMP_SLOT("mpg123_index_64", 180);
JUMP_SLOT("mpg123_info", 184);
JUMP_SLOT("mpg123_init", 188);
JUMP_SLOT("mpg123_init_string", 192);
JUMP_SLOT("mpg123_length", 196);
JUMP_SLOT("mpg123_length_64", 200);
JUMP_SLOT("mpg123_meta_check", 204);
JUMP_SLOT("mpg123_new", 208);
JUMP_SLOT("mpg123_new_pars", 212);
JUMP_SLOT("mpg123_open", 216);
JUMP_SLOT("mpg123_open_64", 220);
JUMP_SLOT("mpg123_open_fd", 224);
JUMP_SLOT("mpg123_open_fd_64", 228);
JUMP_SLOT("mpg123_open_feed", 232);
JUMP_SLOT("mpg123_open_handle", 236);
JUMP_SLOT("mpg123_open_handle_64", 240);
JUMP_SLOT("mpg123_outblock", 244);
JUMP_SLOT("mpg123_par", 248);
JUMP_SLOT("mpg123_param", 252);
JUMP_SLOT("mpg123_parnew", 256);
JUMP_SLOT("mpg123_plain_strerror", 260);
JUMP_SLOT("mpg123_position", 264);
JUMP_SLOT("mpg123_position_64", 268);
JUMP_SLOT("mpg123_rates", 272);
JUMP_SLOT("mpg123_read", 276);
JUMP_SLOT("mpg123_replace_buffer", 280);
JUMP_SLOT("mpg123_replace_reader", 284);
JUMP_SLOT("mpg123_replace_reader_64", 288);
JUMP_SLOT("mpg123_replace_reader_handle", 292);
JUMP_SLOT("mpg123_replace_reader_handle_64", 296);
JUMP_SLOT("mpg123_reset_eq", 300);
JUMP_SLOT("mpg123_resize_string", 304);
JUMP_SLOT("mpg123_safe_buffer", 308);
JUMP_SLOT("mpg123_scan", 312);
JUMP_SLOT("mpg123_seek", 316);
JUMP_SLOT("mpg123_seek_64", 320);
JUMP_SLOT("mpg123_seek_frame", 324);
JUMP_SLOT("mpg123_seek_frame_64", 328);
JUMP_SLOT("mpg123_set_filesize", 332);
JUMP_SLOT("mpg123_set_filesize_64", 336);
JUMP_SLOT("mpg123_set_index", 340);
JUMP_SLOT("mpg123_set_index_64", 344);
JUMP_SLOT("mpg123_set_string", 348);
JUMP_SLOT("mpg123_set_substring", 352);
JUMP_SLOT("mpg123_store_utf8", 356);
JUMP_SLOT("mpg123_strerror", 360);
JUMP_SLOT("mpg123_supported_decoders", 364);
JUMP_SLOT("mpg123_tell", 368);
JUMP_SLOT("mpg123_tell_64", 372);
JUMP_SLOT("mpg123_tell_stream", 376);
JUMP_SLOT("mpg123_tell_stream_64", 380);
JUMP_SLOT("mpg123_tellframe", 384);
JUMP_SLOT("mpg123_tellframe_64", 388);
JUMP_SLOT("mpg123_timeframe", 392);
JUMP_SLOT("mpg123_timeframe_64", 396);
JUMP_SLOT("mpg123_tpf", 400);
JUMP_SLOT("mpg123_volume", 404);
JUMP_SLOT("mpg123_volume_change", 408);

#endif


#ifdef __cplusplus
    }
#endif
