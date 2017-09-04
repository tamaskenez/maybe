# Source files

Source files must be UTF-8 documents with an optional BOM in the first 3 bytes:

    0xEF 0xBB 0XBF

Allowed characters outside comments and string literals:

- SPC (0x20) and TAB (0x09) as whitespace
- 0x0D 0x0A (CRLF), 0x0A (LF) or 0x0D (CR) as newline
- any other non-whitespace UTF-8 sequences

Character classes (where a character is a Unicode code-point, encoded by a UTF-8
sequence):

NEWLINE: CR and LF OR CRLF
UCNZC: (Unicode no-ZC) Unicode except the Z? and C? categories
Z?: Unicode Zs, Zl, Zp categories
C?: Unicode Cc, Cf, Cs, Co, Cn categories

# Lines

Each line is one of these:

Shell comment:
    '#' (UCNZC|Zs|TAB|Cf|Cs)* NEWLINE

Blank line:
    (TAB|SPACE)* NEWLINE

Code line:
    TAB* UCNZC (UCNZC|TAB|SPACE)* NEWLINE
    SPACE* UCNZC (UCNZC|TAB|SPACE)* NEWLINE

# Indentation

Code block hierarchies, which are constructed with curly braces in
C-descendants, are created mostly with indentation. The system is similar to
Python.

Each line is assigned an indentation level which is the number of spaces or
tabs in the indentation. Two consecutive code lines (blank/comment lines
ignored, they do not separate) must have the same type of indentation character,
either tabs or spaces (unlike in Python, where tabs are converted to spaces).

# Logical and physical lines

The code lines counts consecutive even if there are blank or comment lines
between them.

By default each physical line is a logical line, a separate unit. Consecutive
physical lines must have the same indentation level and they will be joined
with a token equivalent with the semicolon in the C-language.

Consecutive physical lines will be joined in the following cases:

1. If the last token of a line is an ellipsis (...) it will be joined with the
   following line:

    var a = ...
        3 * 4

2. If the first token of a line after the indentation is an ellipsis it will be
   joined to the preceding line:

    var a = 1 + 2
        ... + 3 * 4

These two options can be combined.

3. Until there's an unclosed '[', '{' or '(' the next line will be
   interpreted as part of the same logical line.

   var a = 1 + 2 + (3 *
       4 + 5)
   var b = [
       1, 2, 3,
       4, 5
   ] 

In all cases, the physical lines forming a single logical line must have an
indentation level greater or equal then the first line.

A new code block will be if the last token of the line is colon and the next
line has greater indentation. The code block ends at correctly aligned keywords
(`end`, `else`) or when the indentation level drops to or below of the level
of the line that started the block:

loop:
    if a == b:
        if_branch_expr1()
        if_branch_expr2()
    else:
        else_branch_expr1()
        else_branch_expr2()
foo() // implicit end of `if` and `loop`

loop:
    if a == b:
        if_branch_expr1()
        if_branch_expr2()
    else:
        else_branch_expr1()
        else_branch_expr2()
end // implicit end of `if` and explicit end of `loop`


New blocks can be started on continuing lines, too:

    a = foo(1, if a > b:
                   2
               else:
                   3
               end, 4, 5,
        6, 7)

Inline blocks can be created by continuing after the colon on the same line.
These blocks can be closed either by explicit `else` or `end` keywords or
implicitly at the end of the line:

    if a > b: print(1); if a > c: print(2)
    if a > b: print(1); if a > c: print(2) end end

The example from the Python documentation:

// Compute the list of all permutations of l
let perm l: [a] -> [[a]] =:
    if #l <= 1:
        ret [clone l]
    mut r : [[]]
    do i <- 0 :< #l:
        let s = l@(:<i) ++ l@(i+1:<end)
        let p = perm(s)
        do x <- p:
            r ++= [clone l@i] ++ x
    ret r


// Same function annotated:

The function perm

    let perm ... 

takes one parameter, an `Ixable a` where a is a type variable

      l: [a] ...

and returns an `Ixable Ixable a` which is not a concrete type
(it's an interface). Since the default materialization of Ixable is
List, it will be List(List(a))

      -> [[a]] =:

If number of elements in l is less than 1

        if #l <= 1:

`clone l` results in either a `List(a)` or at least an `Ixable` which can be
turned int `List(a)`

            ret [clone l]

`r` is a mutable, empty `Ixable`, that is, a `List` of something. The something
can be derived from the rest of the function, it's `a`

        mut r : [[]]

Execute the next block for all values of `i` in the interval [0 .. length(l))

        do i <- 0 :< #l:

`s` is the concatentation of List of the first `i` elements of l, leaving out l@i
then the rest.

            let s = [l@(:<i)] ++ l@(i+1:<end)

Since `s` is immutable and used at exactly one expression, it won't be materialized
into `List a` but stays `[a]`, that is, a view.
Recurse on s which returns list of lists of `a`

            let p = perm(s)

For each of them

            do x <- p:

append `r` with `l@i` and `x`. The right side evaluates to an Ixable, because
both arguments to `++` are Ixables. RHS is lazy-evaluated and `++=` can
query the length of RHS beforehand then iterate over it and append to `r`.
Since `x` is not referenced after this expression, elements of `x` can be\
moved into `r`. `l@i` may or may not be copyable, that's why we need the
`clone` which clones only if needed.

                r ++= [clone l@i] ++ x
        ret r

# Trying to find alterative, simpler indentation rules

- two successive lines, same indentation generates instruction separator. There
  must be no unclosed {[( since the first line in a row of indentical
  indentation levels. Unclosed {[('s are compile error.
- two successive lines, increased indentation, no unfinished {[( from first
  line, last char colon: start new block
- two successive lines, increased indentation, unfinished {[( from first line,
  no colon at end: continue line
- two successive lines, increased indentation, no unfinished {[( and no colon:
  compile error
- two successive lines, increased indentation, unfinished {[( and colon: could
  be handled but for no compiler error
- two successive lines, decreased indentation, closes all all colons and
  unfinished ({[ since back to the line which has the same indentation level.


Algorithm:

- Start of line: remember current line indentation level
- opening ([{: push on stack with indentation level
- Closing )]}: 
- Colon at end of line: push on stack with indentation level, emit 


opening: ( [ {
closing: ) ] }

last token of prev line:
  ellipsis
  colon
  other

first token of next line:
  ellipsis
  )]
  }
  } else
  else
  end
  other

indentation level change between two lines:
  less
  same
  more


1. Handle ellipses

- Examine the `newlines,indent` sections
- If there's an ellipse on any side of the the section:
  - If indentation decresases, ERROR
  - if indentation increases, replace the newlines,indent with an indent token
    which keeps the previous logical line and introduces the new physical
  - if indentation is same:
    - if logline == physline: ERROR
      else emit indent token with new physical line, same logical line

2. Then

In curly mode, (default and after `{` or block starting `:`) newlines mean `;`,
the sequencing operator
In paren mode, (after `(` and `[`) newlines are whitespace

When encountering ([{: a stack is maintained.

`:` is allowed only when the current logical line has no unfinished ([{. After
it, the indentation must be increased in relative to the last physical line.

When ([{, )]} encountered, stack and curly/paren mode is updated.

