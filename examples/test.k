STEP = 1

start:
	; Set initial value in B
	ldb	%A

loop:
	; Output B
	lda	b
	int	$FF

	; Add STEP to B
	add	STEP
	ldb	a

	; Check if B is at end of alphabet
	lda	b
	eq	%Z+1
	if	end

	; Loop back to start
	jump	loop

end:
	; Print newline
	lda	10
	int	$FF

stop:
	; Enter infinite loop
	jump	stop