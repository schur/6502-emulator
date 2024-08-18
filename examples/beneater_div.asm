*=$8000

value = $0000
mod10 = $0002
message = $0004 ; 6 bytes

  ; initialise output string
  lda #0
  sta message

  ; initialise value to the number to be converted
  lda number
  sta value
  lda number+1
  sta value+1

divide:
  ; initialise remainder to zero
  lda #0
  sta mod10
  sta mod10+1
  clc

  ldx #16
divloop:
  ; rotate quitient and remainder
  rol value
  rol value+1
  rol mod10
  rol mod10+1

  ; a,y, = dividend - divisor
  sec
  lda mod10
  sbc #10
  tay ; save low byte in Y
  lda mod10+1
  sbc #0
  bcc ignore_result ; branch if dividend < divisor
  sty mod10
  sta mod10+1

ignore_result:
  dex
  bne divloop

  rol value  ; shift in the last bit of quotient
  rol value+1

  lda mod10
  clc
  adc #$30 ; convert to ascii
  jsr push_char

  ; if value != 0, continue dividing
  lda value
  ora value+1
  bne divide ; branch if value !=0 

  nop
  nop
  nop
  brk

number: .word 1729

; Add chracter in A to beginning of message
push_char:
  pha ; push new character onto stack
  ldy #0

char_loop:
  lda message,y ; get char in string and put into X
  tax
  pla
  sta message,y ; pull char off stack and add to string
  iny
  txa
  pha           ; push char from string onto stack
  bne char_loop

  pla
  sta message,y ; pull null off stack and add to end of string  
  rts

