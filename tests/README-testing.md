# JJAM testing

This guide is to describe how to add additional tests in this repository. Tests use GoogleTest for assertions, gMock for dependency mocking, and CTest to run suites.

## Run tests

From the repository root, with CMake 3.20+ and a C++17 compiler:

```sh
cmake -S . -B build-tests -DJJAM_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-tests --parallel 2
ctest --test-dir build-tests --output-on-failure --no-tests=error
```

The first configure downloads GoogleTest/gMock automatically. The test build
runs without Pi hardware. CI runs these commands on every PR and push to `main`.

## Add a test suite

Create a file under `tests/`. For example, `tests/sentence_splitter_test.cpp`:

```cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tts/sentence_splitter.h"

TEST(SentenceSplitterTest, SplitsTwoSentences) {
  EXPECT_THAT(tts::splitIntoSentences("Hello. Goodbye!"),
              testing::ElementsAre("Hello.", "Goodbye!"));
}
```

Add this to the bottom of `tests/CMakeLists.txt`:

```cmake
jjam_add_test(sentence_splitter_tests sentence_splitter_test.cpp
  "${PROJECT_SOURCE_DIR}/tts/src/sentence_splitter.cpp")
target_include_directories(sentence_splitter_tests PRIVATE
  "${PROJECT_SOURCE_DIR}/tts/include")
```

This registers each test with CTest. Do not add your own `main()`. Re-run the configure/build/test commands after adding files. No workflow changes are needed. To run a specific suite:

```sh
ctest --test-dir build-tests -R SentenceSplitterTest --output-on-failure
```

## Mock a dependency

Call your real module from tests, and replace its external dependencies with
callbacks or small interfaces. Keep reusable mock classes in `tests/mocks/`.

For example, a new module could expose this signature in `page_processor.h`:

```cpp
#include <functional>
bool processPage(const std::function<bool()> &recognize,
                 const std::function<void()> &speak);
```

If its intended behavior is to skip speech when recognition fails, test it with:

```cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "page_processor.h"

TEST(PageProcessorTest, SkipsSpeechWhenRecognitionFails) {
  testing::MockFunction<bool()> recognize;
  testing::MockFunction<void()> speak;
  EXPECT_CALL(recognize, Call()).WillOnce(testing::Return(false));
  EXPECT_CALL(speak, Call()).Times(0);

  EXPECT_FALSE(processPage(recognize.AsStdFunction(), speak.AsStdFunction()));
}
```

`WillOnce(Return(...))` supplies a result; `Times(0)` verifies that a dependency
is never called. `AsStdFunction()` connects the mock to the production callback.

Feel free to use the existing suites as examples:

- `hardware_loop_test.cpp` and `mocks/mock_loop_services.h`: callback adapters,
  failure cases, and ordered expectations with `testing::InSequence`.
- `display_test.cpp` and `mocks/waveshare.cpp`: test replacements for a C API,
  linked into the test executable in place of the real device backend.

Cover successful behavior, failures, and useful edge cases.
