Anonymous (lambda) functions.

    fn -> x + y // single expression
    fn x y -> x + y // single expression
    fn x y -> : foo(); x + y; end // code block
    fn x y -> int : foo(); x + y; end // code block

    a = blabla(v, fn -> if x > 2 { ret x * 2 } else { ret x / 3 } )

second idea

    fn { x + y } // single expression
    fn x y { x + y } // single expression
    fn x y { foo(); ret x + y; } // code block
    fn x y -> int { foo(); ret x + y; } // code block

    a = blabla(v, fn { if x > 2 { ret x * 2 } else { ret x / 3 } } )

third idea

    $x + $y // single expression
    fn x y -> { foo(); ret x + y; } // code block
    fn x y -> int { foo(); ret x + y; } // code block

    a = blabla(v, $x > 2 ? $x * 2 : $x / 3 )

The desugared function definition uses these forms:

let myfun = fn x y -> x + y // single expression
let myfun = fn x y -> : foo(); x + y; end // code block
let myfun = fn x y -> int : foo(); x + y; end // code block

second idea:

let myfun = fn x y { x + y } // single expression
let myfun = fn x y { foo(); ret x + y; } // code block
let myfun = fn x y -> int { foo(); ret x + y; } // code block

third idea:

+ myfun = $x + $y // single expression
+ myfun = fn x y -> { foo(); ret x + y; } // code block
+ myfun = fn x y -> int { foo(); ret x + y; } // code block

The sugared versions:

let myfun x y = x + y // single expression
let myfun x y =: foo(); x + y; end // code block
let myfun x y -> int =: foo(); x + y; end // code block

let myfun x y: foo(); x + y; end // code block
let myfun x y -> int: foo(); x + y; end // code block

second idea:

let myfun x y = x + y // single expression
let myfun x y = { foo(); ret x + y; } // code block
let myfun x y -> int = { foo(); ret x + y; } // code block

let myfun x y -> int = {
    foo()
    ret x + y
} // code block

let myfun x y -> int =:
    foo()
    ret x + y
// code block

third idea:

+ fn myfun x y = x + y // single expression
+ fn myfun x y = { foo(); ret x + y; } // code block
+ fn myfun x y -> int = { foo(); ret x + y; } // code block

+ fn myfun x y -> int {
    foo()
    ret x + y
} // code block

+ fn myfun x y -> int
    foo()
    ret x + y


let myfun x y = {
    do x <- y {
        foo()
        if x > 0 {
            bela x y
        }
    }
}

let myfun x y = :
    do x <- y:
        foo()
        if x > 0:
            bela x y
    end do
end fn

