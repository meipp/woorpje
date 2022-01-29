(set-logic QF_S)

(declare-fun A () String)
(declare-fun B () String)

(assert (= A A))

(assert (str.in_re A
        (re.union (str.to_re "s") (str.to_re ""))
))

(check-sat)