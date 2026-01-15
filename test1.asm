.import print
.import init
.import new
.import delete
lis $4
.word 4
lis $11
.word 1
beq $0, $0, Pwain

Pp:
lis $3
.word 241


        ;begin Pp epilogue
jr $31

        ;begin wain prologue

Pwain:
sw $1, -4($30)
sub $30, $30, $4
sw $2, -4($30)
sub $30, $30, $4
add $2, $0, $0 
sw $31, -4($30) ; PUSH
sub $30, $30, $4
lis $3
.word init
jalr $3
add $30, $30, $4 ; POP
lw $31, -4($30)
sub $29, $30, $4
; declarations --> 
; statements -->
; return -->
sw $29, -4($30) ; PUSH
sub $30, $30, $4
sw $31, -4($30) ; PUSH
sub $30, $30, $4
lis $3
.word Pp
jalr $3
add $30, $30, $4 ; POP
lw $31, -4($30)
add $30, $30, $4 ; POP
lw $29, -4($30)

        ;begin Pwain epilogue
add $30, $30, $4
add $30, $30, $4
jr $31
;  Scope: 'Pwain'
;    Symbol: 'b', Value: 4
;    Symbol: 'a', Value: 8
;  Scope: 'Pp'
