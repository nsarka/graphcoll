# Collective Graph Parser (Flex + Bison)

This project implements a simple
domain-specific language (DSL) that describes custom collective communication
patterns. `MPI_Allgather` is used as an example collective because it is well known. 

The DSL format is shown in the sample file [`allgather.coll`](./allgather.coll):

```text
sources=
A: process 0, size 1 MB
B: process 1, size 1 MB
C: process 2, size 1 MB
D: process 3, size 1 MB

destinations=
E: process 0, [A,B,C,D]
F: process 1, [A,B,C,D]
G: process 2, [A,B,C,D]
H: process 3, [A,B,C,D]
```

Each Source Buffer specifies:

- A buffer name (A, B, etc.)
- The process rank that owns it
- The size

Each Destination Buffer specifies:

- A buffer name (E, F, etc.)
- The process rank that owns it
- A list of input buffer names that should be combined into it

# Dependencies

- `flex`
- `bison` (version ≥ 3.8 recommended)
- `gcc` or `clang`

# Build

`make`

This generates the parser (parser.tab.c/h), lexer (lexer.c), and final binary
collparse.

# Run

To parse the provided example:

`./collparse allgather.coll`

**Expected output:**

```
=== Source Buffers (4) ===
  A: process 0, size 1048576 bytes (1.00 MB)
  B: process 1, size 1048576 bytes (1.00 MB)
  C: process 2, size 1048576 bytes (1.00 MB)
  D: process 3, size 1048576 bytes (1.00 MB)

=== Destination Buffers (4) ===
  E: process 0, inputs [A, B, C, D]
  F: process 1, inputs [A, B, C, D]
  G: process 2, inputs [A, B, C, D]
  H: process 3, inputs [A, B, C, D]
```

# Files

`lexer.l` – token definitions (Flex)

`parser.y` – grammar and semantic actions (Bison)

`main.c` – driver program, prints parsed result

`Makefile` – build rules

`allgather.coll` – sample DSL input

# Extending

Possible extensions:

- Add datatype keywords (float, int32, etc.)
- Support sizes with KB, MB, GB
- Export the parse result as JSON

Hook into a backend implementation (e.g., UCX ucp_tag_send/recv)
to perform the actual collective communication described.

# Notes

Empty rules in the grammar use %empty (Bison 3.8+).

In lexer.l, comments use // inside the rules section;
block comments /* ... */ are only valid inside C code sections (%{ ... %}).
