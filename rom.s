.org $41C0

FB_START    = $0200          ; Framebuffer base address
FB_SIZE     = 16320          ; Total bytes in framebuffer
PALETTE_MAX = 16             ; Number of colors in the palette

cock = 4*5

lda cock

ass:
    jmp ass

; inputs:
; X = coorinate (0-239)
; Y = coorinate (0-135)
; A = color (0-15)

set_pixel:
    tya
    ldx #120
