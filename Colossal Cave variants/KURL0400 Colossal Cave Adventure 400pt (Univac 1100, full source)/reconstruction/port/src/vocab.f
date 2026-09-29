       INTEGER FUNCTION VOCAB(ID,INIT)

C  LOOK UP ID IN THE VOCABULARY (ATAB) AND RETURN ITS  DEFINITION  (KTAB), OR
C  -1 IF NOT FOUND.  IF INIT IS POSITIVE, THIS IS AN INITIALISATION CALL SETTING
C  UP A KEYWORD VARIABLE, AND NOT FINDING IT CONSTITUTES A BUG.  IT ALSO MEANS
C  THAT ONLY KTAB VALUES WHICH TAKEN OVER 1000 EQUAL INIT MAY BE CONSIDERED.
C  (THUS       STEPS   , WHICH IS A MOTION VERB AS WELL AS AN OBJECT, MAY BE LOC
C  AS AN OBJECT.)  AND IT ALSO MEANS THE KTAB VALUE IS TAKEN MOD 1000.

      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comvoc.fi'
      character*5 id, hashed

       hashed = id
       call hash(hashed)

       DO 1 I=1,TABSIZ
       IF(KTAB(I).EQ.-1)GOTO 2
       IF(INIT.GE.0.AND.KTAB(I)/1000.NE.INIT)GOTO 1
       if (atab(i).eq.hashed) goto 3
1      CONTINUE
       CALL BUG(21)

2      VOCAB=-1
       IF(INIT.LT.0)RETURN
       CALL BUG(5)

3      VOCAB=KTAB(I)
       IF(INIT.GE.0)VOCAB=MOD(VOCAB,1000)
       RETURN
       END



C      Vocabulary hashing subroutine

       subroutine hash(word)

       implicit integer (a-z)
       character*5 word
       character*27 alph1, alph2
       data alph1/' abcdefghijklmnopqrstuvwxyz'/
       data alph2/'fjaqnbwsgcxkrud yohtlezupim'/

       do 20 i=1,5
       do 10 j=1,27
       if (word(i:i) .eq. alph1(j:j)) goto 15
10     continue
       goto 20
15     word(i:i) = alph2(j:j)
20     continue
       return
       end
