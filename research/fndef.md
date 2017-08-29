Anonymous (lambda) functions.

    fn -> x + y // single expression
    fn x y -> x + y // single expression
    fn x y -> : foo(); x + y; end // code block
    fn x y -> int : foo(); x + y; end // code block

The desugared function definition uses these forms:

let myfun = fn x y -> x + y // single expression
let myfun = fn x y -> : foo(); x + y; end // code block
let myfun = fn x y -> int : foo(); x + y; end // code block


The sugared versions:

let myfun x y = x + y // single expression
let myfun x y =: foo(); x + y; end // code block
let myfun x y -> int =: foo(); x + y; end // code block


let myfun x y: foo(); x + y; end // code block
let myfun x y -> int: foo(); x + y; end // code block



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

