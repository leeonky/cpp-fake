# AI Coding Agent Guide for cpp-fake

Purpose: cpp-fake is a tiny C++ function / method hooking (patching) helper used inside unit tests to temporarily replace code of non-virtual functions (free, static, non-virtual member, overloaded) on x86. It is intentionally minimal: it only installs a JMP trampoline, leaving behavior verification to CppUTest (and optionally its mocking extensions).

## Big Picture
- Single production artifact: `main/mocker.h` (header-only). No other production sources – tests compile this directly.
- Core concept: RAII object `Mocker` patches machine code bytes of the target function prologue with a short/long/64-bit relative/indirect jump to the replacement, then restores original bytes on destruction.
- Jump strategy classes: `ShortJumper` (2 bytes, opcode 0xEB + 1-byte rel), `LongJumper` (5 bytes, opcode 0xE9 + 4-byte rel), `Long64Jumper` (14-byte absolute indirect for far targets on x86_64). Strategy chosen automatically by relative offset unless caller forces via `JumpPolicy`.
- Memory protection is toggled per page using `mprotect` via `PageWriteable` helper.

## Supported & Limits
- Platforms: x86 Linux/macOS (Windows VC++ conceptually, but unverified). Not ARM.
- Only non-virtual, non-private, non-constructor/destructor functions. No virtual dispatch patching.
- No chaining / nesting safeguards beyond lifetime restore: one `Mocker` per site expected.

## Key Files / Directories
- `main/mocker.h`: Entire implementation; modify cautiously; size limits (backup buffer 32 bytes) assume patched prologue <= chosen jumper size.
- `test/*.cc`: Usage examples exercising free, namespace, static, instance, overloaded, subclass and multi-inheritance scenarios.
- `makefile`: Integrates with vendored `cpputest` sources (in `cpputest/`) using CppUTest MakefileWorker; defines include paths and compiler flags (treats most warnings as errors, adds feature-detect for `-std=c++0x`).
- `build_test_run.sh`: End-to-end script: builds cpputest via autotools + builds & runs tests (`make all`).

## Build & Test Workflow (macOS/Linux)
1. Build framework + tests:
   ```bash
   ./build_test_run.sh
   ```
   - Inside script: `cpputest/configure && make -j4 tdd` then top-level `make all` (target binary = `cpp-fake`).
2. Run tests only (after previous build of cpputest):
   ```bash
   make clean && make all
   ./cpp-fake   # executes all tests (Colorized output enabled via -c)
   ```
3. Focus a single test group/case (CppUTest filter):
   ```bash
   ./cpp-fake -g InstanceMethod -n mock_non_virtual_overloading
   ```
4. Debugging: Set breakpoints in generated test binary (`cpp-fake`). Remember patched code bytes change while a `Mocker` is alive.

## Conventions & Patterns
- Usage pattern: Create a scoped `Mocker` object inside a `{}` block to confine patch lifetime; after scope exit original function restored.
  ```c++
  {
      Mocker m(&ClassWithStaticMethod::bool_, &return_true); // Auto policy
      CHECK_TRUE(ClassWithStaticMethod::bool_());
  }
  CHECK_FALSE(ClassWithStaticMethod::bool_());
  ```
- Overloaded member functions: Explicitly resolve pointer with signature.
  ```c++
  bool (ClassWithInstanceMethod::*mPtr)(uint32_t) = &ClassWithInstanceMethod::overloaded_method;
  Mocker m(mPtr, &overloaded_return_true);
  ```
- For const vs non-const: use the exact member pointer (`&Class::const_bool`).
- Namespaced / global functions: pass fully qualified symbol (`&nm::bool_`).
- For system / libc functions: works like free functions if symbol is directly linkable (e.g., `strerror`). Ensure symbol not inlined.
- Jump policy override: pass `Short`, `Long`, `Long64` to force; failure if offset exceeds capacity or policy too small.

## Safety / Edge Cases
- Backup buffer is fixed at 32 bytes; extending jumper implementations may require increasing if future patches copy more than their size (currently copies `size` only; fine).
- `Long64Jumper` code is compiled only under `__x86_64__`. On 32-bit, far jumps rely on `LongJumper`.
- Multiple simultaneous `Mocker` objects targeting same function cause last-created patch to overwrite previous; destruction order matters for restore (no ref counting). Avoid nesting mocks of same target.
- Thread-safety: Not synchronized; patching during concurrent execution could race. Tests assume single-threaded.

## When Modifying `mocker.h`
- Maintain ABI of `Mocker` template ctor and `JumpPolicy` enum – tests and README examples rely on them.
- Keep page protection logic minimal; macOS & Linux require RX -> RWX -> back to RX (ASLR assumed). Avoid leaving pages writable.
- If adding architectures (ARM), isolate with `#ifdef` and keep current x86 paths untouched.

## Extending Tests
- Add new usage scenarios under `test/` and they'll auto-build (files picked up via `TEST_SRC_DIRS`).
- Prefer small focused `TEST( Group, Name )` functions; mimic existing naming (`mock_*`).

## Typical Agent Tasks
- Implement new jump strategy or architecture variant.
- Add detection / graceful failure messages for unsupported scenarios.
- Expand README with new platform notes after verifying.
- Introduce unit test for nested mocks or error paths (e.g., forcing `Short` when out-of-range should throw).

Focus on these conventions instead of generic C++ guidance. Keep edits minimal and validate via `./build_test_run.sh`.

## Development Workflow (Red ➜ Green ➜ Refactor)
Each change should be in exactly one of these sequential phases. Always surface (paste/summarize) the latest test run results after invoking the test binary.

### 1. Add Test
Goal: Add or adjust test code (under `test/`) so that at least one test fails, clearly demonstrating current behavior does NOT meet the new requirement. Do NOT touch production code (`main/mocker.h`) in this phase. Once the test is written, run the suite immediately.

### 2. Add Feature
Goal: Modify production implementation (typically inside `mocker.h`) so all tests (new + existing) pass. After changes, rebuild and run tests; confirm green.

### 3. Refactor
Goal: Improve internal design per user guidance or self-review (remove duplication, simplify logic, align naming) without altering externally observable behavior. Run the full test suite after every meaningful refactor step to ensure it stays green.

Test Run Commands (show output):
```bash
make clean && make all && ./cpp-fake
# or end-to-end (rebuild cpputest if needed)
./build_test_run.sh
```

While in phases 2 or 3, if any test fails, revert to phase 2 (implementation) until green again before continuing refactors.
