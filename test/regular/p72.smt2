(set-logic QF_S)
(declare-fun A () String)
(assert (str.in_re (str.++ "higmnihzjuhe" A ) (re.* (re.union (re.* (str.to_re "")) (re.* (str.to_re "higmnihzjuhext"))))))
(check-sat)