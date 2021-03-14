# duthomhas::utf8_console

## DESCRIPTION

Windows library that:

 -  Makes cin, cout, and cerr (and clog) UTF-8 enabled when attached to the Windows Console.
 -  Makes main()'s arguments UTF-8.
 -  Requires no changes to written code.

## USAGE

Use `BUILD.bat` to compile `utf8_console.o` / `utf8_console.obj` for your system.
Then simply link it to your project.

 - For **MSVC** and **Clang-CL**, life is good. We hijack the `wmain()` entry point to initialize and call `main()`.
 - For modern **MinGW-w64** and **Clang-w64** life is also good: Use the `-municode -mconsole` options when compiling your programs.
 - For older **MinGW**s you are stuck with a `.ctor` section function, positioned to construct last. (It works just fine, though.)
 - For everything else, you are SOL with likely-[SIOF](https://isocpp.org/wiki/faq/ctors#static-init-order) code.

## LIMITATIONS

  The Windows Console limits Unicode input to the [Basic Multilingual Plane](https://en.wikipedia.org/wiki/Plane_(Unicode)#Basic_Multilingual_Plane),
  which typically includes all the symbols you will need anyway. This is a limitation of
  the Windows Console itself. When redirected to or from a file there are no limitations.

## TECHNICAL NOTES

#### MSVC & Clang-CL

  Auto-magically changes the actual program entry point to the (totally valid) `wmain()`, which
  initializes and calls `main()` as if nothing happened.

#### MinGW-w64 & Clang-w64

  The most recent versions of **MinGW-w64** can also use `wmain()` as the entry point, just like **MSVC**,
  by declaring `-municode -mconsole` at the command line.
  Prefer this.

#### MinGW

  Initializes in the `.ctor` section, which is executed after the `.init` section.

  Initialization short-circuits the usual startup by treating itself as the new entry point,
  calling `main()`, and terminating the program.

  Compilation will tell you if you get this version.

#### Other Compilers

  There is no current specialization for other compilers. Instead, initialization is activated
  during globals initialization, meaning you have an even chance of experiencing the [Static
  Initialization Order Fiasco](https://isocpp.org/wiki/faq/ctors#static-init-order)!

  Either use one of **MSVC**, **Clang**, or **MinGW**, or read your compiler's documentation to
  properly adjust the main entry point through wmain(). If you do this, then tell this library by
  #defining `UTF8_CONSOLE_USE_WMAIN` somewhere.

#### Windows API

None of this makes Windows API functions UTF-8 enabled. Unicode Windows functions use UTF-16 (or, in
some cases, like the Console API, plain old UCS-2), so you must still convert stuff in order to use
the `wchar_t` versions of functions.

## LEGAL STUFF

Copyright 2019 Michael Thomas Greer<br>
Distributed under the Boost Software License, Version 1.0.<br>
(See accompanying file [LICENSE_1_0.txt](file://LICENSE_1_0.txt)
 or copy at https://www.boost.org/LICENSE_1_0.txt )
