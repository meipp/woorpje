(set-logic QF_S)
(declare-fun A () String)
(assert (str.in_re A 
        ((_ re.loop 4 5) (re.range "a" "z" ))
))