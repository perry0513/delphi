(set-logic LIA)

(declare-var x Int)
(declare-var y Int)
(declare-var x! Int)
(decare-var y! Int)

(synth-fun inv-f ((x Int) (y Int)) Bool)

(declare-oracle-fun correctness iceoracle_all ((-> Int Int Bool)) Bool)

(define-fun pre-f ((x Int) (y Int)) Bool
    (and (<= x 5) (>= x 4) (<= y 5) (>= y 4)))
(define-fun trans-f ((x Int) (y Int) (x! Int) (y! Int)) Bool
    (or 
    (and (= x! (- x 1)) 
    (= y! y) (>= x 0) (>= y 0)) 
    (and (= x! x) (= y! (- y 1)) (< x 0) (>= y 0)) 
    (and (= x! (+ x 1)) (= y! (- y 1)) (< y 0))))
(define-fun post-f ((x Int) (y Int)) Bool
    (<= y 5))

(constraint (correctness inv-f))

(oracle-constraint iceoracle_pos ((inv-f (-> Int Int Bool))) ((B Bool)(xRes Int) (yRes Int))
(=> (not B)(= (inv-f xRes yRes) true)))

(oracle-constraint positive_unroll ((inv-f (-> Int Int Bool))) ((xRes Int) (yRes Int))
(inv-f xRes yRes))

(oracle-constraint iceoracle_neg ((inv-f (-> Int Int Bool))) ((B Bool)(xRes Int) (yRes Int))
(=> (not B)(= (inv-f xRes yRes) false)))

(oracle-constraint iceoracle_imp ((inv-f (-> Int Int Bool))) ((B Bool)(xRes Int) (yRes Int)(xRes! Int) (yRes! Int))
(=> (not B)(ite (inv-f xRes yRes) (inv-f xRes! yRes!)(not (inv-f xRes! yRes!)) )))


(check-synth)