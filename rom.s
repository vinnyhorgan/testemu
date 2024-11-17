.org $41C0

FB_START    = $0200

; addition params
num1lo = $62
num1hi = $63
num2lo = $64
num2hi = $65
reslo = $66
reshi = $67

; multiplication params
aux = $68
acc = $69

lda #$06
sta FB_START

lda FB_START
ora #$30
sta FB_START

lda #$36
sta FB_START + 120 * 1 + 1

loop:
    jmp loop

; x coord in x register
; y coord in y register
; color in a register
set_pixel:
    ; save x, y, and color in zero page
    stx $01
    sty $02
    sta $03

    ; calulate x / 2
    lda $01
    lsr
    sta $04 ; save x / 2

    ; perform 0x200 + x / 2 (already in $01)
    lda $04
    sta num1lo
    lda #0
    sta num1hi
    sta num2lo
    lda #2
    sta num2hi
    jsr add

    ; store result in $04 and $05
    lda reslo
    sta $04
    lda reshi
    sta $05

    ; y * 120
    lda $02
    sta acc
    lda #120
    sta aux
    jsr mult

    ; finally add y * 120 to our intermediate result in $04 and $05
    sta num1hi
    lda acc
    sta num1lo
    lda $04
    sta num2lo
    lda $05
    sta num2hi
    jsr add

    ; great, now reslo and reshi contain the address of the pixel we want to set

    ; indirect addressing => sta (reslo), y
    lda (reslo), y
    lsr a
    bcc even
    ora $03
    jmp write

even:
    and #$F0
    asl $03
    ora $03

write:
    sta (reslo), y

    rts

add:
    clc
    lda num1lo
    adc num2lo
    sta reslo
    lda num1hi
    adc num2hi
    sta reshi
    rts

; Multiply acc * aux
;
; On Exit:
;   acc (low byte)
;   A   (high byte)
mult:
    lda #0
    ldy #9      ; loop counter
    clc
-
    ror
    ror acc
    bcc +
    clc         ; decrement aux above to remove CLC
    adc aux
+
    dey
    bne -
    rts
