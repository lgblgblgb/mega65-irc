; Copyright (C)2026 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

.SETCPU	"4510"

; Payload (eth.bin)
PAYLOAD_TADDR	= $2000
PAYLOAD_TBANK	= 4
; Do not set bit 4, otherwise the first 8K of lower 32K becomes mapped which alters ZP / stack too!
MAP_X_ETHBIN	= $E0 + PAYLOAD_TBANK
; Upper 32K memory config: not mapped, expect for the last 8K where it's from the second BANK, so the 2K colour RAM is at $F800 now
MAP_Z_GLOBAL	= $81

.SEGMENT	"LOADADDR"
	.WORD	$2001

.DEFINE SELF_BANK	0


.SEGMENT	"EXEHDR"
.SCOPE
	BASTOK_BANK	= $02FE
	BASTOK_BLOAD	= $11FE
	BASTOK_POKE	= $97
	BASTOK_SYS	= $9E
	BASTOK_TRAP	= $D7
	BASTOK_EQU_SIGN	= $B2
	.WORD	line2, 1
	.BYTE	"X",BASTOK_EQU_SIGN,"$1F700:",BASTOK_POKE,"X,0:",BASTOK_TRAP,"3"
	.BYTE	0
line2:
	.WORD	line3, 2
	.WORD	BASTOK_BLOAD
	.BYTE	34,"IRC.CFG,S",34,",P(X),R"
	.BYTE	0
line3:
	.WORD	nonextline, 3
	.BYTE	BASTOK_TRAP,":"
	.WORD	BASTOK_BANK
	.BYTE	"0:"
	.BYTE	BASTOK_SYS
	.BYTE	$30+.LOBYTE((stub_main .MOD 10000)/1000)
	.BYTE	$30+.LOBYTE((stub_main .MOD  1000)/ 100)
	.BYTE	$30+.LOBYTE((stub_main .MOD   100)/  10)
	.BYTE	$30+.LOBYTE( stub_main .MOD    10)
	.BYTE	0
nonextline:
	.WORD   0
.ENDSCOPE

.SEGMENT	"STARTUP"

;; Must be exactly here to easy to edit!
; Do not put anything before this in the STARTUP segment!
;.BYTE	192, 168, 0, 153
;.WORD	6667

stub_main:
	SEI
	CLD
	LDA	#0
	TAX
	TAY
	LDZ	#MAP_Z_GLOBAL
	MAP
	EOM
	LDA	#7
	STA	0
	LDA	#5
	STA	1
	LDA	#$47
	STA	$D02F
	LDA	#$53
	STA	$D02F
	LDA	#0
	STA	$D030
	JMP	sys_init	; at this point we are safe to accedss code at highter memory addressess too, I guess

.SEGMENT	"INIT"
.SEGMENT	"ONCE"

.BSS
	.RES	1	; be sure BSS segment is at least one byte long

; ----------------------------------------------------------------------------
.CODE
; ----------------------------------------------------------------------------


sys_init:
	LDA	#.LOBYTE(nmi_handler)
	STA	$FFFA
	LDA	#.HIBYTE(nmi_handler)
	STA	$FFFB
	LDA	#.LOBYTE(irq_handler)
	STA	$FFFE
	LDA	#.HIBYTE(irq_handler)
	STA	$FFFF
	; ---
	LDA	#$1B
	STA	$D011
	LDA	#$24
	STA	$D018
	LDA	#$C9
	STA	$D016
	LDA	#$E0
	STA	$D031		; H640 + FAST(C65-FAST) + ATTR
	LDA	#$40
	TSB	$D054		; M65 speed (M65 CPU speed, it seems to need C65-speed to be set first, but it's done with writing $D031 above)
	LDA	#$10
	TSB	$D07A		; 16 pixel font

	; NOTE: it seems VIC-IV allows 16 pixel height font only with the "char RAM" so we need to copy
	; later (see later, the DMA list), not only using this code to set precise address!
	;LDA	#.LOBYTE(vgafont)
	;STA	$D068
	;LDA	#.HIBYTE(vgafont)
	;STA	$D069
	;LDA	#0
	;STA	$D06A

	STA	$D707		; triggers in-line enhanced mode DMA
	.BYTE	$A		; enhanced option: F018A DMA list (shorter)
;	; ---- Clear screen ----
;	.BYTE	0		; end of enhanced DMA options
;	.BYTE	3+4		; command (fill) + CHAINED
;	.WORD	80*25
;	.WORD	$2020		; source, but here (fill): fill byte
;	.BYTE	0
;	.WORD	2048
;	.BYTE	0
;	.WORD	0		; modulo, not used
	; ---- Copy character set ----
	; (char RAM: FF-7-E000)
	.BYTE	$81, $FF	; target megabyte slice
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0+4		; command (copy) + etc, last one + CHAINED!!
	.WORD	vgafont_size	; DMA length
	.WORD	vgafont		; source DMA address
	.BYTE	0		; upper byte of source
	.WORD	$E000		; target
	.BYTE	7
	.WORD	0		; modulo, not used
	; ---- Copy possible payload after the program to $42000 ----
	.BYTE	$81, $00	; make sure to restore target megabyte slice DMA option
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0+4		; command (copy) + etc + CHAINED!!
	.WORD	payload_size	; DMA length
	.WORD	payload		; source DMA address
	.BYTE	0		; upper byte of source
	.WORD	PAYLOAD_TADDR	; target
	.BYTE	PAYLOAD_TBANK
	.WORD	0		; modulo, not used
	; ---- Copy HICODE segment ----
.IMPORT __HICODE_SIZE__
.IMPORT __HICODE_LOAD__
.IMPORT __HICODE_RUN__
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0+4		; command (copy) + etc + CHAINED!!
	.WORD	__HICODE_SIZE__	; DMA length
	.WORD	__HICODE_LOAD__	; source DMA address
	.BYTE	0		; upper byte of source
	.WORD	__HICODE_RUN__	; target
	.BYTE	1		; $E000 is in the mapped last 8K! (see comment at "HICODE" segment)
	.WORD	0		; modulo, not used
	; ---- Clear BSS segment ----
.IMPORT __BSS_SIZE__
.IMPORT __BSS_RUN__
	.BYTE	0		; end of enhanced DMA options
	.BYTE	3		; command (fill) + NOT CHAINED!!
	.WORD	__BSS_SIZE__
	.WORD	0		; source, but here (fill): fill byte
	.BYTE	0
	.WORD	__BSS_RUN__
	.BYTE	0
	.WORD	0		; modulo, not used
	; --- Initialize CC65's own things what CRT0 would do (probably ...) ---
.IMPORT	__MAIN_START__
.IMPORT	__MAIN_SIZE__
	LDA	#<(__MAIN_START__ + __MAIN_SIZE__)
	LDX	#>(__MAIN_START__ + __MAIN_SIZE__)
.SETCPU "6502"			; dirty hack! 4510 mode does not allow "sp" as identifier
	.IMPORTZP sp
	STA	sp
	STX	sp+1		; Set argument stack ptr
.SETCPU	"4510"
;.IMPORT	initlib
;	JSR	initlib
@kbdemptyloop:			; empty key buffer
	LDA	$D610
	STA	$D610
	BNE	@kbdemptyloop
	; --- Start my compiled C code at this point --
.IMPORT	_main_entry
	LDZ	#0		; make sure register Z is zero, for 65C02 code!
	TZA
	TAX
	TAY
	CLI			; *** WARNING: interrupts are enabled now ***
	JSR	_main_entry	; call the C entry point
@halt:
	INC	$D020
	JMP	@halt




.EXPORT	_arch_exit
_arch_exit:
	SEI
	LDA	#$10
	TRB	$D07A
	LDA	#0
	TAX
	TAY
	TAZ
	MAP
	EOM
	LDA	#6
	STA	1
	JMP	($FFFC)


; ---- KEYBOARD RELATED STUFF TO BE CALLED FROM C ----

.EXPORT	_arch_getkey
_arch_getkey:
	LDA	$D610
	BEQ	@ret
	STA	$D610
@ret:
	RTS

; ---- VIDEO RELATED STUFF TO BE CALLED FROM C ----

; Much faster than doing it in C with memmove/memcpy/memset stuff ...
.EXPORT	_mega65_scroller
_mega65_scroller:
	STA	$D707		; triggers in-line enhanced mode DMA
	.BYTE	$A		; enhanced option: F018A DMA list (shorter)
	; ---- move screen RAM ----
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0 + 4		; command (copy) + etc, last one + CHAINED
	.WORD	22 * 80		; DMA length
	.WORD	$0800 + 80	; source DMA address
	.BYTE	0		; upper byte of source
	.WORD	$0800		; target
	.BYTE	0
	.WORD	0		; modulo, not used
	; ---- clear one line  ----
	.BYTE	0		; end of enhanced DMA options
	.BYTE	3 + 4		; command (fill) + etc, last one + CHAINED
	.WORD	80		; DMA length
	.WORD	$2020		; source DMA address, but here (fill): fill byte
	.BYTE	0		; upper byte of source
	.WORD	$0800 + 22*80	; target
	.BYTE	0
	.WORD	0		; modulo, not used
	; ---- move colour RAM ----
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0		; command (copy) + etc, last one, NOT CHAINED!
	.WORD	22 * 80		; DMA length
	.WORD	$F800 + 80	; source DMA address
	.BYTE	1		; upper byte of source ($1F800, colour RAM)
	.WORD	$F800		; target
	.BYTE	1
	.WORD	0		; modulo, not used
	RTS
; memmove(screen, screen + 80, 22 * 80);
; memmove(colour, colour + 80, 22 * 80);
; memset(screen + 22 * 80, 32, 80);


; ----------------------------------------------------------------------------
.SEGMENT "ONCE"
; ----------------------------------------------------------------------------
; This segment will be overwritten by BSS!
; Must be copied first!
; ----------------------------------------------------------------------------


vgafont:
	.INCBIN	"assets/font-mega65.bin"
vgafont_size = * - vgafont

.IF vgafont_size <> $1000
	.ERROR "font.bin has wrong size!"
.ENDIF

payload:
	.INCBIN "megaip/eth.bin"
payload_size = * - payload

; Payload must be fit into the lower 32K because of "MAP" rules!
.IF PAYLOAD_TADDR + payload_size > $8000
	.ERROR "Payload is too large!"
.ENDIF
.IF .LOBYTE(PAYLOAD_TADDR) <> 0
	.ERROR "PAYLOAD_TADDR must be 256 byte aligned!"
.ENDIF


; ----------------------------------------------------------------------------
.SEGMENT "HICODE"
.UNDEF SELF_BANK
.DEFINE SELF_BANK 1
; ----------------------------------------------------------------------------
; This segment will be overwritten by BSS!
; Must be copied first!
; ----------------------------------------------------------------------------
; Code/data which should work always (even if the low 32K is mapped to bank 4
; for eth.bin) should be here. For real it's $E000 and mapped to bank 1,
; for easy non-I/O space 2K colour RAM access, so must be copied to $E000
; in bank 1, not bank 0, even though CPU will see it normally after the
; initial MAP at $E000.
; ----------------------------------------------------------------------------


rotchrs:
	.BYTE "-\|/"


nmi_handler:
	RTI

.PROC	irq_handler
	PHA
	PHX
	PHY
	PHZ
	ASL	$D019	; Acknowledge VIC interrupt
	LDA	$D7FA
	LSR	A
	LSR	A
	AND	#3
	TAX
	LDA	rotchrs, X
	STA	$800 + 24*80 - 1
	PLZ
	PLY
	PLX
	PLA
	RTI
.ENDPROC






.PROC	map_do
	LDA	#0
	TAY
	; X MUST BE PROVIDED BY THE CALLER OF THIS FUNCTION!
	LDZ	#MAP_Z_GLOBAL
	MAP
	EOM
	RTS
.ENDPROC


; ---- ETH.BIN related calls, "bridge" ----


; 64tass's labels. NOTE: these stuff lives in its own memory bank (PAYLOAD_TBANK) so either need
; DMA to access or to change memory mapping from HICODE segment to really access them!
; All original 64tass labels are prefixed with "megaip_" though to avoid any possible collusion
; with my own symbols

.INCLUDE "megaip/eth.i65"



.PROC	_eth_call
.EXPORT _eth_call, _eth_call_id, _eth_call_f,  _eth_call_a,  _eth_call_x,  _eth_call_y,  _eth_call_z
	PHA
	PHX
	PHY
	PHZ
	LDX	#MAP_X_ETHBIN		; eth.bin's memory mapping
	JSR	map_do
_eth_call_f = * + 1
	LDA	#$FF
	AND	#$C3			; make sure we count only for NVZC flags
	STA	_eth_call_f
	PHP
	PLA
	AND	#$3C
	ORA	_eth_call_f
	PHA				; push to-be-used flags in
_eth_call_a = * + 1
	LDA	#$FF
_eth_call_x = * + 1
	LDX	#$FF
_eth_call_y = * + 1
	LDY	#$FF
_eth_call_z = * + 1
	LDZ	#$FF
	PLP				; now active the flags
_eth_call_id = * + 1
	JSR	PAYLOAD_TADDR
	STA	_eth_call_a
	STX	_eth_call_x
	STY	_eth_call_y
	STZ	_eth_call_z
	PHP
	PLA
	STA	_eth_call_f
	LDX	#0			; native memory mapping for our program
	JSR	map_do
	PLZ
	PLY
	PLX
	PLA
	RTS
.ENDPROC


; TODO: Since it uses DMA, it can be moved out of HICODE segment in theory
; ... however if it's done, the length copy source bank will be 0 not 1!! (SELF_BANK should be OK?)
; Input:
; _copy_to_megaip_length byte must be set, DO NOT USE ZERO!!! it will break DMA (0 means 64K ...)
; _copy_to_megaip_source word must be set
.PROC	_copy_to_megaip_
.EXPORT	_copy_to_megaip_, _copy_to_megaip_length, _copy_to_megaip_source
	STA	$D707		; triggers in-line enhanced mode DMA
	.BYTE	$A		; enhanced option: F018A DMA list (shorter)
	; ---- Copy data ----
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0 + 4		; command (copy) + etc, CHAINED!!
_copy_to_megaip_length:
	.WORD	0		; DMA length
_copy_to_megaip_source:
	.WORD	0		; source DMA address
	.BYTE	0		; upper byte of source
	.WORD	megaip_TCP_DATA_PAYLOAD; target
	.BYTE	PAYLOAD_TBANK
	.WORD	0		; modulo, not used
	; --- Copy length info itself (only 2 bytes) ---
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0		; command (copy), NOT CHAINED!!!
	.WORD	2		; only 2 bytes to copy
	.WORD	_copy_to_megaip_length	; the source is the length info!!!
	.BYTE	SELF_BANK			; ... which is in BANK 1
	.WORD	megaip_TCP_DATA_PAYLOAD_SIZE	; target
	.BYTE	PAYLOAD_TBANK
	.WORD	0		; modulo, not used
	RTS
.ENDPROC


.PROC	_copy_ip_info
.EXPORT	_copy_ip_info, _copy_ip_info_addr
;	28 bytes!
;0	LOCAL_IP:               .byte 192, 168, 1, 75
;4	LOCAL_PORT:             .byte $c0, $00              ; ephemeral port 49152
;6	REMOTE_IP:              .byte 192, 168, 1, 1
;10	REMOTE_PORT:            .byte $00, $17
;12	GATEWAY_IP:             .byte 192, 168, 1, 1
;16	SUBNET_MASK:            .byte $ff, $ff, $ff, $00
;20	PRIMARY_DNS:            .byte 8, 8, 8, 8
	STA	$D707		; triggers in-line enhanced mode DMA
	.BYTE	$A		; enhanced option: F018A DMA list (shorter)
	; ---- Copy data ----
	.BYTE	0		; end of enhanced DMA options
	.BYTE	0		; command (copy) + etc, NOT CHAINED!
	.WORD	24		; DMA length
	.WORD	megaip_LOCAL_IP		; source DMA address
	.BYTE	PAYLOAD_TBANK		; upper byte of source
_copy_ip_info_addr:
	.WORD	0			; target
	.BYTE	0
	.WORD	0		; modulo, not used
	RTS
.ENDPROC
