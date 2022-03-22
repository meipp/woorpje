(set-logic QF_S)

(declare-fun A () String)

(assert (str.in_re (str.++ "aasdb:" A "1") (str.to_re "aasdb:testtest1")))

(check-sat)