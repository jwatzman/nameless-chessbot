# nameless

A chess engine that I started back in the summer of 2008 in order to play with bitboards and alpha-beta search. I didn't intend to take it very far, just enough to play with bitboards, so I didn't think it was worth naming it.

For a long time, this note said that it's

> slow, buggy, and doesn't play chess very well, but was a ton of fun to write.

It turned out to be a ton of fun to tinker with too over the years, and so it slowly... seems to have become kind of a decent engine actually? It's certainly never going to win any awards or become notable in any way, but it's also climbed well out of trash-tier.

The code style and structure is still pretty bad though. (Weird factoring, global variables, and complex coupling galore!) This code should be considered neither a good example of how to write a decent chess engine nor a good example of C style.

## Compiling

If you just want to run the thing, make sure you have `meson` and `clang` installed, then:

```
./build.sh
```

will drop a binary in `./bin` which speaks the xboard protocol.

Otherwise, for tinkering, it builds via `meson` in the usual way. Setting `CC=clang` is *strongly* recommended --- as of this writing, the result is *dramatically* faster than what `gcc` produces. A release build is the default; `-Dbuildtype=debug` will get a debuggable build with asserts enabled etc.

It's known to work on Linux, macOS, and FreeBSD, on both x86 and ARM. (The very earliest development happened on PPC so it worked there too at some point, though I haven't had a machine to test on in a decade.) Things should work but are likely to be painful if you aren't running a 64-bit OS, or are using an old x86 processor without AVX2.

## Features

Features implemented in some form include:

- SEE
- History heuristic
- Killer and countermoves
- Late move reduction
- Null move pruning
- Futility and reverse futility pruning
- History and late move pruning
- NNUE evaluator with hand-written AVX2 and NEON SIMD

## License

Most of the code is licensed under the 3-clause BSD as in the LICENSE file. Other files may be under other licenses as indicated at the top of the file.
