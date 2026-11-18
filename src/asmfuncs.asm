; ============================================================================
;  asmfuncs.asm - x86-32 Linux low-level helpers for ASCII QUEST
;
;  All functions use the cdecl calling convention.  They talk to the kernel
;  directly through the 32-bit int 0x80 system call gate, so the C side can
;  keep its stdio buffering out of the game's render path.
;
;  assemble:  nasm -f elf32 -o build/asmfuncs.o src/asmfuncs.asm
; ============================================================================

BITS 32

section .data
align 4
rng_state:  dd 0x9E3779B9

section .text

; ----------------------------------------------------------------------------
; ssize_t asm_write(int fd, const void *buf, size_t count)
;   eax = 4 (sys_write), ebx = fd, ecx = buf, edx = count
; ----------------------------------------------------------------------------
global asm_write
asm_write:
    push ebx
.retry:
    mov eax, 4
    mov ebx, [esp + 8]      ; fd
    mov ecx, [esp + 12]     ; buf
    mov edx, [esp + 16]     ; count
    int 0x80
    cmp eax, -4             ; -EINTR: retry the syscall
    je .retry
    pop ebx
    ret

; ----------------------------------------------------------------------------
; ssize_t asm_read(int fd, void *buf, size_t count)
;   eax = 3 (sys_read)
; ----------------------------------------------------------------------------
global asm_read
asm_read:
    push ebx
.retry:
    mov eax, 3
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    mov edx, [esp + 16]
    int 0x80
    cmp eax, -4             ; -EINTR: retry the syscall
    je .retry
    pop ebx
    ret

; ----------------------------------------------------------------------------
; unsigned int asm_random(void)
;   xorshift32 PRNG.  Small, fast and good enough for a dungeon crawl.
;   The state is never zero (asm_srandom guarantees a non-zero seed).
; ----------------------------------------------------------------------------
global asm_random
asm_random:
    mov eax, [rng_state]
    mov edx, eax
    shl edx, 13
    xor eax, edx
    mov edx, eax
    shr edx, 17
    xor eax, edx
    mov edx, eax
    shl edx, 5
    xor eax, edx
    mov [rng_state], eax
    ret

; ----------------------------------------------------------------------------
; void asm_srandom(unsigned int seed)
; ----------------------------------------------------------------------------
global asm_srandom
asm_srandom:
    mov eax, [esp + 4]
    test eax, eax
    jnz .store
    mov eax, 0x6D2B79F5
.store:
    mov [rng_state], eax
    ret

; ----------------------------------------------------------------------------
; unsigned int asm_get_random_state(void)
; ----------------------------------------------------------------------------
global asm_get_random_state
asm_get_random_state:
    mov eax, [rng_state]
    ret

; ----------------------------------------------------------------------------
; void asm_set_random_state(unsigned int state)
; ----------------------------------------------------------------------------
global asm_set_random_state
asm_set_random_state:
    mov eax, [esp + 4]
    test eax, eax
    jnz .store
    mov eax, 0x6D2B79F5
.store:
    mov [rng_state], eax
    ret

; ----------------------------------------------------------------------------
; void asm_fill32(void *dst, unsigned int value, size_t count)
;   Fill count dwords with value (rep stosd).  Used for screen/map clearing.
; ----------------------------------------------------------------------------
global asm_fill32
asm_fill32:
    push edi
    mov edi, [esp + 8]
    mov eax, [esp + 12]
    mov ecx, [esp + 16]
    rep stosd
    pop edi
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
