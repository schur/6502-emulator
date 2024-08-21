*=$8000
; use ZP addresses to store variables
value = $00
mod10 = $02
message = $04 ; 6 bytes

  ; initialise output string
  ldx #6 ; last byte of output string
  lda #0
  sta message,x
  txa
  pha   ; push position in output string to stack

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

  pla   
  tax  ; pull position in output sring from stack into x
  lda mod10
  clc
  adc #$30 ; convert to ascii
  dex
  sta message,x   ; write string to message
  txa
  pha ; push position in output string to stack

  ; if value != 0, continue dividing
  lda value
  ora value+1
  bne divide ; branch if value !=0 
  pla ; pull pull position in output sring from stack

  nop
  nop
  nop
  brk


number: .word 1729
