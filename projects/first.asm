; my first assembly program

10	*=$1000
20 	LDY #0
30  LDA #0
40 LOOP CLC
50  INY
60  ADC #1
70  CPY #3
80  BNE LOOP
90 END
