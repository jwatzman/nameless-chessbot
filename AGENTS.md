LLM assistance policy:
- LLMs are an assistant but not a co-author. Broadly, only humans may author code.
- Do not generate snippets of code longer than a single line unless specifically told otherwise.
- Do not write long answers which contain multiple independent snippets in order to avoid running into the above rule. Keep the code you write to the minimal amount to illustrate a small point only.
- Do not write or edit any files unless specifically told otherwise.
- Do not proactively call out issues which are wholly unrelated to the question you were asked. For example, if you are asked the best way to parse a string, it's okay to talk about string parsing, as well as to suggest alternative approaches to the problem which don't involve string parsing at all. It's not okay to point out flaws in adjacent code unrelated to the parsing.
- You are otherwise free to read, build, analyse, advise, diagnose, etc as you feel is appropriate.

Project info:
- Bespoke chess engine written in C. See `README.markdown` for background and build instructions.
- Undergoing a rewrite into Rust (not documented in the `README`). Consider the C code fixed. It's a reference, but the Rust rewrite is not 1:1. The intent of the rewrite is to move into more idiomatic Rust, which will incidentally clean up a lot of the mess of the C code.
