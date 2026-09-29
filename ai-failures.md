# AI failures

Record of mistakes by any AI tool, corrections and rejected suggestions. Appended as they happen, never reconstructed later. Feeds the "AI use" section of the report.

## 2026-09-23: Google Benchmark failed to build under GCC 16

- **Asked:** write a `CMakeLists.txt` for the project, pulling in GoogleTest and Google Benchmark with FetchContent.
- **What went wrong:** Claude pinned Google Benchmark to v1.9.1 and left its default of building with `-Werror`. It never tried the build, because CMake was not installed at the time. Updating MSYS2 to install CMake also moved GCC from 14.2 to 16.2. GCC 16 raises a `-Wswitch` warning in Benchmark's `src/sysinfo.cc`, a missing `CacheUnknown` case, and `-Werror` turned that warning into a build failure.
- **How it was found:** a test build with placeholder source files, run by Claude after CMake was installed. The first attempt failed for an unrelated reason, a build path over Windows' 260-character limit caused by Claude's long scratch directory. The second attempt showed the real error.
- **What was done:** set `BENCHMARK_ENABLE_WERROR OFF` in `CMakeLists.txt`. The warning now appears but no longer stops the build. Configure, build, `ctest` and the benchmark binary all ran successfully afterwards.
- **Lesson:** a pinned dependency is only known to work with the compilers it was tested against. The build file should have been tested before being presented as done.

## 2026-09-23: Unsupported claim written into the README

- **Asked:** record the toolchain choice, MinGW GCC or MSVC, in the README.
- **What went wrong:** Claude's first draft justified GCC by saying results would "compare more directly with published Abseil and hashbrown numbers". There was no basis for this. hashbrown is Rust, compiled through LLVM, and Abseil's published benchmarks are commonly built with Clang, not GCC.
- **How it was found:** Claude's own re-read of the draft, before commit.
- **What was done:** the clause was removed. The README now gives only the supported reason, that flags and code generation carry over to a Linux GCC run.
- **Lesson:** plausible-sounding justifications get written in passing. Every factual claim in a document needs a source or a check.

## 2026-09-23: Miscounted references in the plan

- **Asked:** plan the day-1 tasks, including renaming the AI log.
- **What went wrong:** the plan said `CLAUDE.md` had three references to `AI_USE_LOG.md`. It had one. The count was stated without searching the file.
- **How it was found:** a search of `CLAUDE.md` during implementation.
- **What was done:** updated the single reference. No consequence beyond the wrong statement.

## 2026-09-24: Out-of-date description of Abseil's H1/H2 split

- **Asked:** how to choose a hash function, and which hash bits the table should use for H2.
- **What went wrong:** two errors, both presented as current fact.
  1. Claude said Abseil takes H2 from the low 7 bits and H1 from `hash >> 7`. That was true up to release 20250512. Since release 20250814, Abseil uses H1 = the whole hash and H2 = the top 7 bits, the same split as hashbrown. The author chose "Abseil's split" partly because Claude said it matched the Abseil design notes, and that reason rested on the out-of-date fact.
  2. Claude said H1 is masked to the number of groups. In old Abseil, current Abseil and hashbrown, H1 is masked to the number of slots. Probing starts at any slot and reads 16 control bytes from there.
- **How it was found:** the verification step written into the plan. It read `raw_hash_set.h` at Abseil master and at eight release tags, plus hashbrown's `src/control/tag.rs` and `src/raw.rs`. The Abseil commit that made the change explains it: "change H2 to use the most significant 7 bits - saving 1 cycle in H1. Using Mix instead of WeakMix means that the entire 64 bits of hash are expected to have good entropy."
- **What was done:** the split decision was reopened with the author before any code depending on it was written. The plan's colliding-key generator and hash-quality metrics were changed to use slot indices, not group indices.
- **Lesson:** knowledge of fast-moving libraries goes out of date. A claim about a library's internals needs a version attached, and it should be checked before a decision rests on it.

## 2026-09-24: Wrong high-confidence prediction about multiplicative hashing

- **Asked:** draft hypotheses for the hash-quality experiment before any measurement.
- **What went wrong:** Claude predicted, at high confidence, that the multiplicative hash's top bits, the H2 bits, would show low avalanche bias. The measured mean bias was 0.26 to 0.28, far from the 0 expected of a good hash. The reasoning error: "the top bits depend on every input bit" was treated as "the top bits respond randomly". Flipping input bit i adds the fixed constant 2^i·C to the product, so each output bit flips with a fixed probability, not a probability of 1/2. Averaged over input bits, that gives a bias of about 0.25.
- **How it was found:** the first run of `swiss_hash_quality`, checked against the hypotheses committed beforehand in `17ffa96`.
- **What was done:** the committed hypothesis was left unchanged. The divergence and its explanation are in `experiments/hash-quality-results.md`, finding 5. Two lower-confidence predictions also missed, about folded multiply and tabulation's worst avalanche cell. Those are recorded there as ordinary experimental outcomes.
- **Lesson:** "depends on" is not the same as "is mixed by". Committing hypotheses before measuring is what made this error visible rather than quietly rewritten.

## 2026-09-28: Code review reasoned about the intended design, not the code as written

- **Asked:** review `OA_Map` for remaining bugs before implementing `hash` and `rehash`.
- **What went wrong:** Claude reported that the load factor ignored tombstones and that the table could run out of empty slots, causing every probe loop to run forever. It proposed a separate `m_deleted` counter as the fix, and when questioned, gave a worked example in which `m_size` never exceeded 1 across repeated insert and erase cycles. That example was wrong for the actual code. `erase` never decremented `m_size`, so `m_size` already counted filled plus deleted slots, the load check already included tombstones, and the termination guarantee already held. The error came from reviewing against the design Claude had recommended earlier, where `erase` decrements `m_size`, rather than tracing the code that existed. The real issue was narrower: `m_size` meant "occupied slots", so `size()` over-reported after any erase.
- **How it was found:** pointing out that `erase` does not decrement `m_size`, after checking Claude's worked example against the code.
- **What was done:** [record the choice made: one counter renamed to `m_occupied` with `rehash` recounting live entries, or two counters `m_size` and `m_deleted` with the load check using their sum.]
- **Lesson:** a review has to trace the code line by line, not the reviewer's memory of what the code should be. A confident worked example is only as good as its assumptions, and the assumption here, that `erase` decrements the count, was never checked against the source.

## 2026-09-28: Correct code, wrong justification about name lookup

- **Asked:** whether the private `hash` member of `OA_Map` would clash with `std::hash`, given `using namespace std` in the header.
- **What went wrong:** Claude correctly explained that class-scope lookup finds the private member before `std::hash`, then added that this was why `SplitMix64` had to write `std::hash<K>` explicitly. That reasoning was false. `SplitMix64` is a separate struct defined outside `OA_Map`, so the map's private member has no effect on lookup inside it. With the struct named `SplitMix64`, a plain `hash<K>` there would have found `std::hash` without qualification. The rule was right, but it was applied to a scope it does not cover.
- **How it was found:** asking why `SplitMix64` would need `std::` when it sits outside the class.
- **What was done:** the code was left unchanged. `std::hash` stays fully qualified, with the correct reasons recorded: it cannot collide with any other `hash` visible at global scope in a file that includes `hash.hpp`, and it keeps compiling if `using namespace std` is removed from the header.
- **Lesson:** correct advice can arrive with an incorrect reason, and the reason is what gets reused later. A lookup rule only applies within the scope that defines it, so the scope has to be checked, not just the name.

## 2026-09-29: Unnecessary change justified by a guarantee that did not matter

- **Asked:** review `rehash` in `Scalar_Swiss_Map`.
- **What went wrong:** Claude recommended replacing `slots.resize(m_capacity)` with `slots.assign(m_capacity, pair<K, V>{})`, because a moved-from `std::vector` is only guaranteed to be "valid but unspecified", so `slots` might not be empty after `move`. The standard fact was correct, but it did not support the change. `resize(n)` always leaves the vector with exactly `n` elements, whatever it held before. Any leftover contents would sit in slots whose control bytes say empty, and those slots are never read. The advice added a change for a problem the design already made harmless.
- **How it was found:** asking whether `slots` needed specific contents at all, given that the control bytes decide which slots are read.
- **What was done:** kept `resize`. The underlying cost the question pointed at, every empty slot holding a constructed `pair<K, V>`, is recorded as a known limitation rather than fixed, since avoiding it needs raw storage and placement `new`.
- **Lesson:** a true rule can still be the wrong rule to apply. The relevant invariant was that slot contents only matter where the control byte says full, and checking the advice against that invariant would have ruled it out.

## 2026-09-29: Iteration tests that missed the bug they were written for

- **Asked:** write V3-specific tests for `Swiss_Map`'s boundary handling, including iteration when the last slot is full and when the only live entry is in the last slot.
- **What went wrong:** Claude's first version of `test_swiss_map.cpp` had three iteration tests for the end of the table: `IterateLastSlotFull`, `IterateOnlyLastSlotLive` and `IterateOnlyLastSlotLiveAfterErase`. None of them failed when the iterator's bound check `pos + i < capacity` was removed, and that check is what stops iteration from returning a mirrored control byte as a real slot. With the last slot full, the iterator stops there before it can reach a mirrored byte. With only slot 0 wrapped, the stray position is exactly `capacity`, which compares equal to `end()`. The bug only shows when the last real slots are empty and slot 1 or later at the front is full, and no test built that layout.
- **How it was found:** mutation testing by Claude before handing the tests over. Five single-line mutants of `swiss_map.hpp` were compiled against the new tests from copies in a scratch directory, leaving `src/` untouched. The mutant without the bound check was caught only by chance, in two int-key tests that were not about iteration.
- **What was done:** added `IterateStopsBeforeMirroredBytes`, which leaves live entries in slot 1 and slot `capacity - 3`, with slots 0 and `capacity - 1` erased. It catches that mutant for all three key types. All five mutants are now caught by at least one targeted test.
- **Lesson:** a test named after a boundary case is not evidence that the boundary's failure mode is covered. Working out what a specific bug would change, then checking that a test fails on it, is what shows coverage.

## 2026-09-29: An allocation test that measured nothing

- **Asked:** build the E5 memory benchmark, which counts heap bytes through a replaced global `operator new`.
- **What went wrong:** before writing the counter, Claude suspected that `std::string` allocations happen inside `libstdc++-6.dll`, where a replacement `operator new` in the executable cannot reach them. It wrote a scratch test that copied a 16-character string and checked the counter. The counter rose by 0 bytes, with dynamic and static libstdc++ alike, which looked like confirmation. It was not. Nothing read the copied string's contents, so GCC was allowed to remove the allocation entirely. The test measured the optimiser, not the allocator. Taken at face value, it would have led to linking `swiss_bench` with `-static-libstdc++` for a problem that does not exist.
- **How it was found:** the same test showed a string inside an `unordered_map` node being counted, which contradicted the first result. A second test stored four copies in a vector and read a character back. It counted 68 bytes, 4 × 17, with both link modes. In C++23 mode libstdc++ no longer compiles `std::string` into the DLL, so its allocations reach the replacement.
- **What was done:** no static linking. The E5 counter was checked against hand-computed sizes instead. For example, V1 with int keys at load 0.5 should use 2 slots × 8 bytes = 16 bytes per element, and it measured 16.0.
- **Lesson:** a benchmark whose result is never used can be optimised away, and that applies to scratch tests too. A zero is only evidence once something forces the work to happen.
