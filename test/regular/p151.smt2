(set-logic QF_S)
(declare-fun A () String)
(assert (str.in_re (str.++ "rxrxzbgmuulqqhpcm" (str.++ A "guig")) (re.* (re.* (re.++ (re.* (re.++ (re.union (re.* (str.to_re "rx")) (re.* (str.to_re ""))) (re.* (re.++ (str.to_re "") (str.to_re "zbgmuulqqhpcmti"))))) (str.to_re "uig"))))))
(check-sat)