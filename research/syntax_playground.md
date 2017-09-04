IF THEN ELSE

    a = blabla(v, fn { if x > 2 { x * 2 } else { x / 3 } } )
    a = blabla(v,
        fn {
            if x > 2:
                x * 2
            else
                x / 3
        }
    )

    a = blabla(v, [x>2 ? x*2 : x/3])
    a = blabla(v, ? x>2 [x*2 ; x/3])
    a = blabla(v, ? x>2 {x*2 ; x/3})
    a = blabla(v,
        fn {
            { x > 2 ? x * 2 : x / 3 }
        }
    )

    a = blabla(v,
        fn {
            ? x > 2 { x * 2 ; x / 3 }
        }
    )

    ? x>2 [
        qweqe
        qeq
        qweq
    ;
        qeq
        qweqwe
    ]
